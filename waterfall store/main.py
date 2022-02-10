import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import matplotlib.ticker as tick
import numpy as np
import asyncio
import websockets
import json
import datetime
import argparse
import os
import time
import logging
import concurrent
import copy


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
    logger = logging.getLogger("websockets.client")
    logger.setLevel(logging.INFO)


def format_frequency(frequency, pos=None, separator="."):
    f1 = frequency / 1000000
    f2 = (frequency / 1000) % 1000
    f3 = frequency % 1000
    return "%d%s%03d%s%03d" % (f1, separator, f2, separator, f3)


def waterfall(graph, output_dir):
    x = graph["frequencies"]
    y = graph["labels"]
    z = np.array(graph["values"])
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
    fig.savefig("%s/%s.png" % (output_dir, start_datetime.strftime("%H:%M:%S")), dpi=100, bbox_inches="tight")
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


async def graph_task(queue, output_dir, threads):
    executor = concurrent.futures.ProcessPoolExecutor(threads)
    while True:
        executor.submit(waterfall, await queue.get(), output_dir)


async def ws_task(url, aggregate_seconds, flush_seconds, graphs, logger, queue):
    async with websockets.connect(url) as websocket:
        logger.info("connection succeeded")
        await websocket.send("Hello world!")
        while True:
            message = json.loads(await websocket.recv())
            if message["type"] == "spectrogram":
                frequencies = message["frequencies"]
                key = (frequencies[0], frequencies[-1])
                dt = datetime.datetime.fromtimestamp(message["timestamp_ms"] / 1000.0)
                data = np.array(message["powers"], dtype=np.float16)
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
                elif 0 < flush_seconds and seconds_from_midnight(graphs[key]["flush_datetime"]) // flush_seconds != seconds_from_midnight(dt) // flush_seconds:
                    await queue.put(copy.deepcopy(graphs[key]))
                    graphs[key]["flush_datetime"] = dt


async def run():
    parser = argparse.ArgumentParser(description="Draw waterfall from rtl-sdr-scanner-cpp")
    parser.add_argument("-s", "--server", help="ws server", type=str, default="localhost", metavar="server")
    parser.add_argument("-p", "--port", help="ws server", type=int, default=9999, metavar="port")
    parser.add_argument("-rs", "--reconnect_seconds", help="ws reconnect interval in seconds", type=int, default=10)
    parser.add_argument("-as", "--aggregate_seconds", help="aggregate n seconds in single plot", type=int, default=600)
    parser.add_argument("-fs", "--flush_seconds", help="flush plot ever n seconds", type=int, default=0)
    parser.add_argument("-od", "--output_dir", help="flush plot ever n seconds", type=str, default="sdr/waterfalls", metavar="dir")
    parser.add_argument("-ld", "--log_directory", help="store output log in directory", type=str, default="sdr/logs", metavar="dir")
    parser.add_argument("-t", "--threads", help="graphs generator threads", type=int, default=1)
    parser.add_argument("-v", "--verbose", action="count", default=0)
    args = vars(parser.parse_args())

    config_logger(args["verbose"], args["log_directory"])
    try:
        graphs = {}
        logger = logging.getLogger("ws")
        queue = asyncio.Queue()
        while True:
            try:
                await asyncio.gather(
                    graph_task(queue, args["output_dir"], args["threads"]),
                    ws_task("ws://%s:%d/" % (args["server"], args["port"]), args["aggregate_seconds"], args["flush_seconds"], graphs, logger, queue),
                )
            except websockets.exceptions.ConnectionClosedError:
                logger.warning("connection closed")
                time.sleep(args["reconnect_seconds"])
            except ConnectionRefusedError:
                logger.warning("connection refused")
                time.sleep(args["reconnect_seconds"])
            except asyncio.TimeoutError:
                logger.warning("connection timeout")
                time.sleep(args["reconnect_seconds"])
            except IOError as exception:
                if exception.errno == 101:
                    logger.warning("network is unreachable")
                    time.sleep(args["reconnect_seconds"])
                elif exception.errno == 113:
                    logger.warning("connect call failed")
                    time.sleep(args["reconnect_seconds"])
                else:
                    raise
    except KeyboardInterrupt:
        pass
    except Exception as e:
        logger.exception("exception: %s" % e)
        time.sleep(args["reconnect_seconds"])

def main():
    asyncio.run(run())


if __name__ == "__main__":
    main()
