#!/usr/bin/env python3

import sys
import zlib
import struct


HEADER_SIZE = 512


def gen_image_header(input_bin, output_header):
    # read binary
    with open(input_bin, "rb") as f:
        data = f.read()

    size = len(data)
    crc32 = zlib.crc32(data) & 0xffffffff

    print(f"Input : {input_bin}")
    print(f"Size  : 0x{size:08X}")
    print(f"CRC32 : 0x{crc32:08X}")

    # header format:
    # offset 0x00 : uint32_t image_size
    # offset 0x04 : uint32_t image_crc32
    # offset 0x08 ~ 0x1FF : reserved (zero)

    header = bytearray(HEADER_SIZE)

    # little endian
    struct.pack_into("<II", header, 0, size, crc32)

    with open(output_header, "wb") as f:
        f.write(header)

    print(f"Output: {output_header}")
    print(f"Header size: {HEADER_SIZE} bytes")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.bin> <header.bin>")
        sys.exit(1)

    gen_image_header(sys.argv[1], sys.argv[2])
