#!/usr/bin/python3

import argparse
import matplotlib.pyplot as plt
import numpy as np
import os
import re


def ff(frequency):
    f1 = frequency // 1000000
    f2 = (frequency // 1000) % 1000
    f3 = frequency % 1000
    return "%3d.%03d.%03d Hz" % (f1, f2, f3)


def make_spectrogram(data, sample_rate):
    data = np.fft.fft(data)
    data = np.absolute(data**2.0) / np.float32(sample_rate)
    data = np.fft.fftshift(10.0 * np.log10(data), axes=(1,))
    return data


def get_file_info(file):
    dir = os.path.dirname(file)
    (name, extension) = os.path.splitext(os.path.basename(file))
    return (dir, name, extension[1:])


def read_raw_iq_data(input_file, fit_size):
    (dir, name, extension) = get_file_info(input_file)
    if extension == "cs8" or name.endswith("cs8"):
        data = np.memmap(input_file, dtype=np.int8, mode="r").astype(np.complex64) / 127.5
    else:
        data = np.memmap(input_file, dtype=np.complex64, mode="r")

    if data.size % fit_size != 0:
        data = data[: -(data.size % fit_size)]
    return data.reshape(-1, fit_size)


def read_spectrogram_data(input_file, fit_size):
    (dir, name, extension) = get_file_info(input_file)
    if extension == "s8" or name.endswith("s8"):
        data = np.memmap(input_file, dtype=np.int8, mode="r").astype(np.float32)
    elif extension == "u8" or name.endswith("u8"):
        data = np.memmap(input_file, dtype=np.uint8, mode="r").astype(np.float32)
    else:
        data = np.memmap(input_file, dtype=np.float32, mode="r")

    if data.size % fit_size != 0:
        data = data[: -(data.size % fit_size)]
    return data.reshape(-1, fit_size)


def spectrogram(input_file, fft, power):
    (dir, name, extension) = get_file_info(input_file)
    output_file = "%s/%s.jpg" % (dir, name)
    frequency = int(re.split(r"[._]", input_file)[3])
    sample_rate = int(re.split(r"[._]", input_file)[4])
    if power:
        data = read_spectrogram_data(input_file, fft)
    else:
        data = read_raw_iq_data(input_file, fft)
        data = make_spectrogram(data, sample_rate)
    print(
        "psd, file: %s.%s, frequency: %s, sample_rate: %d, fft: %d, length: %4d, duration: %5.2f"
        % (name, extension, ff(frequency), sample_rate, fft, data.shape[0], data.size / sample_rate)
    )
    norm = plt.Normalize(vmin=data.min(), vmax=data.max())
    image = plt.cm.jet(norm(data))
    plt.imsave(output_file, image)


def gqrx(input_file):
    (dir, name, extension) = get_file_info(input_file)
    if extension != "raw":
        data = read_raw_iq_data(input_file)
        print("gqrx, file: %s.%s, length: %d" % (name, extension, data.size))
        output_file = "%s/%s.raw" % (dir, name)
        with open(output_file, "wb") as f:
            np.save(f, data)


def main():
    parser = argparse.ArgumentParser(description="Raw IQ data converter")
    parser.add_argument("-f", "--fft", help="fft size", type=int, default=2048)
    parser.add_argument("--spectrogram", help="draw spectrogram", action="store_true")
    parser.add_argument("--gqrx", help="convert to gqrx format", action="store_true")
    parser.add_argument("--power", help="file contains ready power data", action="store_true")
    parser.add_argument("file", help="file to read", type=str, nargs="+")
    args = vars(parser.parse_args())

    if args["spectrogram"]:
        for file in args["file"]:
            spectrogram(file, args["fft"], args["power"])
    if args["gqrx"]:
        for file in args["file"]:
            gqrx(file)


if __name__ == "__main__":
    main()
