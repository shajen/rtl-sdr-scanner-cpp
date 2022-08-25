import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import matplotlib.ticker as tick
import numpy as np
import asyncio
import asyncio_mqtt
import datetime
import argparse
import os
import math
import struct
import logging
import concurrent
import copy
import gc
import skimage.measure


def config_logger(verbose, dir):
    params = {}

    levels = [logging.ERROR, logging.WARNING, logging.INFO, logging.DEBUG]
    level = levels[min(len(levels) - 1, verbose)]

    params["format"] = "[%(asctime)s.%(msecs)03d][%(levelname)7s][%(name)6s] %(message)s"
    params["level"] = level
    params["datefmt"] = "%Y-%m-%d %H:%M:%S"

    if dir:
        now = datetime.datetime.now()
        os.makedirs(dir, exist_ok=True)
        filename = "%s/auto-sdr-waterfall %04d-%02d-%02d %02d:%02d:%02d.txt" % (dir, now.year, now.month, now.day, now.hour, now.minute, now.second)
        params["handlers"] = [logging.FileHandler(filename), logging.StreamHandler()]
    logging.basicConfig(**params)
    logging.getLogger("websockets.client").setLevel(logging.INFO)
    logging.getLogger("matplotlib.font_manager").setLevel(logging.INFO)
    logging.getLogger("matplotlib.colorbar").setLevel(logging.INFO)


def format_frequency(frequency, pos=None, separator="."):
    f1 = frequency / 1000000
    f2 = (frequency / 1000) % 1000
    f3 = frequency % 1000
    return "%d%s%03d%s%03d" % (f1, separator, f2, separator, f3)


def waterfall(graph, output_dir, x_max_size=2000, y_max_size=2000):
    x_org = graph["frequencies"]
    y_org = graph["labels"]
    z_org = np.array(graph["values"])
    x_merge_size = math.ceil((len(x_org) - 1) / x_max_size)
    y_merge_size = math.ceil(len(y_org) / y_max_size)
    x_size = len(x_org) // x_merge_size * x_merge_size
    y_size = len(y_org) // y_merge_size * y_merge_size
    x = x_org[:x_size][::x_merge_size]
    y = y_org[:y_size][::y_merge_size]
    z = skimage.measure.block_reduce(z_org[:y_size, :x_size], block_size=(y_merge_size, x_merge_size), func=np.mean)
    if x_size != len(x_org):
        x = x + x_org[-1:]
        z = np.concatenate((z, z_org[:y_size][::y_merge_size, -1:]), axis=1)
    start_datetime = graph["start_datetime"]

    start = x[0]
    stop = x[-1]

    logger = logging.getLogger("graph")
    logger.info("draw %s MHz - %s MHz, samples: %s, %s - %s" % (format_frequency(start), format_frequency(stop), z.shape, y[0], y[-1]))

    plt.rcParams.update({"font.size": 10})
    fig = plt.figure(figsize=(20, 10))
    ax = fig.add_subplot(1, 1, 1)
    ax.set_title("%s, %s MHz - %s MHz" % (start_datetime.strftime("%Y-%m-%d"), format_frequency(start), format_frequency(stop)))
    ax.invert_yaxis()

    ax.set_xlabel("Frequency [MHz]")
    ax.xaxis.set_major_formatter(tick.FuncFormatter(format_frequency))

    ax.set_ylabel("Time")
    ax.yaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))

    c = ax.pcolormesh(x, y, z, vmin=graph["min"], vmax=graph["max"])
    c = fig.colorbar(c, ax=ax)
    c.set_label("Power [dB]")

    output_dir = "%s/%s %s/%s" % (output_dir, format_frequency(start, None, "_"), format_frequency(stop, None, "_"), start_datetime.strftime("%Y-%m-%d"))
    os.makedirs(output_dir, exist_ok=True)
    fig.savefig("%s/%s.jpg" % (output_dir, start_datetime.strftime("%H:%M:%S")), dpi=100, bbox_inches="tight")
    fig.clear()
    plt.close(fig)
    logger.info("finish")


def seconds_from_midnight(dt):
    return (dt.hour * 60 + dt.minute) * 60 + dt.second


def new_graph(frequencies, dt, min=100.0, max=-100.0):
    return {
        "frequencies": frequencies,
        "labels": [],
        "values": [],
        "start_datetime": dt,
        "flush_datetime": dt,
        "min": min,
        "max": max,
    }


async def graph_task(queue, output_dir):
    loop = asyncio.get_running_loop()
    while True:
        data = await queue.get()
        with concurrent.futures.ProcessPoolExecutor(1) as executor:
            await loop.run_in_executor(executor, waterfall, data, output_dir)


async def mqtt_task(hostname, port, username, password, aggregate_seconds, flush_seconds, graphs, logger, queue):
    async with asyncio_mqtt.Client(hostname, port, username=username, password=password) as client:
        logger.info("connection succeeded")
        async with client.unfiltered_messages() as messages:
            await client.subscribe("sdr/spectrogram")
            async for message in messages:
                if message.topic == "sdr/spectrogram":
                    gc.collect()
                    (timestamp, begin_frequency, end_frequency, step_frequency, samples_count) = struct.unpack("<QLLLL", message.payload[:24])
                    dt = datetime.datetime.fromtimestamp(timestamp / 1000.0)
                    frequencies = list(range(begin_frequency, end_frequency + step_frequency, step_frequency))
                    data = np.array(struct.unpack("<%db" % samples_count, message.payload[24:])).astype(np.int8)
                    key = (frequencies[0], frequencies[-1])
                    logger.debug("received spectrogram datetime: %s", dt)

                    if not key in graphs:
                        graphs[key] = new_graph(frequencies, dt)

                    graphs[key]["labels"].append(dt)
                    graphs[key]["values"].append(data)
                    graphs[key]["min"] = min(graphs[key]["min"], data.min() // 10 * 10)
                    graphs[key]["max"] = max(graphs[key]["max"], data.max() // 10 * 10 + 10)

                    if seconds_from_midnight(graphs[key]["start_datetime"]) // aggregate_seconds != seconds_from_midnight(dt) // aggregate_seconds:
                        await queue.put(copy.deepcopy(graphs[key]))
                        g = new_graph(frequencies, dt, graphs[key]["min"], graphs[key]["max"])
                        g["labels"].extend(graphs[key]["labels"][-2:])
                        g["values"].extend(graphs[key]["values"][-2:])
                        graphs[key] = g
                    elif (
                        0 < flush_seconds
                        and seconds_from_midnight(graphs[key]["flush_datetime"]) // flush_seconds != seconds_from_midnight(dt) // flush_seconds
                    ):
                        await queue.put(copy.deepcopy(graphs[key]))
                        graphs[key]["flush_datetime"] = dt


async def run():
    parser = argparse.ArgumentParser(description="Draw waterfall from rtl-sdr-scanner-cpp")
    parser.add_argument("--hostname", help="mqtt hostname", type=str, default="test.mosquitto.org", metavar="server")
    parser.add_argument("--port", help="mqtt port", type=int, default=1883, metavar="port")
    parser.add_argument("--username", help="mqtt username", type=str, default="")
    parser.add_argument("--password", help="mqtt password", type=str, default="")
    parser.add_argument("-rs", "--reconnect_seconds", help="ws reconnect interval in seconds", type=int, default=10)
    parser.add_argument("-as", "--aggregate_seconds", help="aggregate n seconds in single plot", type=int, default=600)
    parser.add_argument("-fs", "--flush_seconds", help="flush plot ever n seconds", type=int, default=0)
    parser.add_argument("-od", "--output_dir", help="flush plot ever n seconds", type=str, default="sdr/waterfalls", metavar="dir")
    parser.add_argument("-ld", "--log_directory", help="store output log in directory", type=str, default="sdr/logs", metavar="dir")
    parser.add_argument("-v", "--verbose", action="count", default=0)
    args = vars(parser.parse_args())

    config_logger(args["verbose"], args["log_directory"])
    graphs = {}
    logger = logging.getLogger("mqtt_task")
    queue = asyncio.Queue()
    while True:
        try:
            await asyncio.gather(
                graph_task(queue, args["output_dir"]),
                mqtt_task(
                    args["hostname"], args["port"], args["username"], args["password"], args["aggregate_seconds"], args["flush_seconds"], graphs, logger, queue
                ),
            )
        except KeyboardInterrupt:
            break


def main():
    asyncio.run(run())


if __name__ == "__main__":
    main()
