#!/usr/bin/env python
# A simple Python script for fake packing the payload
import sys
import os
from struct import pack

def fakepack(src, dst):
    input_len = len(src)

    # Write the uncompressed payload size as little endian
    length_bytes = pack('<I', input_len)
    dst.write(length_bytes)

    src_offset = 0

    # Write a new block header for each 30-byte segment of the input
    while src_offset <= input_len - 1:
        dst.write(b'\x00\x00\x00\x00')

        if src_offset + 30 > input_len:
            dst.write(src[src_offset:input_len])
        else:
            dst.write(src[src_offset:src_offset + 30])

        src_offset += 30

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: fakepack.py <uncompressed file> <output file>")
        exit()

    try:
        with open(sys.argv[1], 'rb') as f:
            indata = f.read()
    except Exception as e:
        print("Failed to read:", e)
        exit()

    try:
        with open(sys.argv[2], 'wb') as out:
            fakepack(indata, out)
    except Exception as e:
        print("Failed to write:", e)
        exit()
