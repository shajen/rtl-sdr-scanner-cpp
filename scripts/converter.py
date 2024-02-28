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


def make_spectrogram(data, sample_rate, fft):
    data = np.fft.fft(data.reshape(-1, fft))
    data = np.absolute(data**2.0) / np.float32(sample_rate)
    data = np.fft.fftshift(10.0 * np.log10(data), axes=(1,))
    return data


def get_file_info(file):
    dir = os.path.dirname(file)
    (name, extension) = os.path.splitext(os.path.basename(file))
    return (dir, name, extension[1:])


def read_file(input_file):
    (dir, name, extension) = get_file_info(input_file)
    if extension == "cs8" or name.endswith("cs8"):
        return np.memmap(input_file, dtype=np.int8, mode="r").astype(np.complex64) / 127.5
    else:
        return np.memmap(input_file, dtype=np.complex64, mode="r")


def psd(input_file, fft):
    (dir, name, extension) = get_file_info(input_file)
    output_file = "%s/%s.jpg" % (dir, name)
    frequency = int(re.split(r"[._]", input_file)[3])
    sample_rate = int(re.split(r"[._]", input_file)[4])
    data = read_file(input_file)
    if data.size % fft != 0:
        data = data[: -(data.size % fft)]
    data = make_spectrogram(data, sample_rate, fft)
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
        data = read_file(input_file)
        print("gqrx, file: %s.%s, length: %d" % (name, extension, data.size))
        output_file = "%s/%s.raw" % (dir, name)
        with open(output_file, "wb") as f:
            np.save(f, data)


def main():
    parser = argparse.ArgumentParser(description="Raw IQ data converter")
    parser.add_argument("-f", "--fft", help="fft size", type=int, default=2048)
    parser.add_argument("--psd", help="draw psd of raw iq file", action="store_true")
    parser.add_argument("--gqrx", help="convert to gqrx format", action="store_true")
    parser.add_argument("file", help="file to read", type=str, nargs="+")
    args = vars(parser.parse_args())

    if args["psd"]:
        for file in args["file"]:
            psd(file, args["fft"])
    if args["gqrx"]:
        for file in args["file"]:
            gqrx(file)


if __name__ == "__main__":
    main()
