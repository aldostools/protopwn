package main

import (
	"encoding/binary"
	"fmt"
)

type blockState struct {
	desc  uint32
	n     uint32
	shift uint32
	mask  uint32
}

// Based on reverse-engineered code by Julian Uy
// https://gist.github.com/uyjulian/14388e84b008a6433aa805f5d0436c87
func safeRead(src []byte, srcOffset int) byte {
	if srcOffset >= len(src) || srcOffset < 0 {
		return 0
	}
	return src[srcOffset]
}

func safeWrite(dst []byte, dstOffset int, val byte) {
	if dstOffset >= len(dst) || dstOffset < 0 {
		return
	}
	dst[dstOffset] = val
}

func decompress(src []byte) (dst []byte) {
	// Get the uncompressed payload length
	uncompressedLength := binary.LittleEndian.Uint32(src[0:4])

	if uncompressedLength > uint32(len(src)*16) {
		fmt.Println("Invalid output length", uncompressedLength)
		return nil
	}
	dst = make([]byte, uncompressedLength)

	// Setup offsets
	cycle := 0
	dstOffset := 0
	srcOffset := 4

	block := blockState{}
	for dstOffset <= int(uncompressedLength) {
		if cycle == 0 {
			// Initialize new block data and reset cycle count
			cycle = 30
			// Reset the block descriptor
			block.desc = 0
			for i := 0; i < 4; i++ {
				// Assemble block descriptor from four bytes
				block.desc <<= 8
				block.desc |= uint32(safeRead(src, srcOffset))
				srcOffset++
			}
			block.n = block.desc & 3
			block.shift = 14 - block.n
			block.mask = 0x3FFF >> block.n

			fmt.Printf("Got new block @ %08x; desc: %08x, n: %02x, shift: %02x, mask: %04x\n", srcOffset-4,
				block.desc, block.n, block.shift, block.mask,
			)
		}
		if block.desc&(1<<(cycle+1)) == 0 {
			if srcOffset < int(uncompressedLength) {
				fmt.Printf("\tWriting uncompressed data for byte %d @ %08x to %08x: %02x\n", 30-cycle, srcOffset, dstOffset, src[srcOffset])
			}
			// If this cycle has a flag set in the block descriptor, pass it as-is
			safeWrite(dst, dstOffset, safeRead(src, srcOffset))
			dstOffset++
			srcOffset++
		} else {
			fmt.Printf("\tUnwrapping data for byte %d @ %08x: %02x %02x\n", 30-cycle, srcOffset, src[srcOffset], src[srcOffset+1])
			// Else, unwrap the data
			h := uint32(safeRead(src, srcOffset)) << 8
			srcOffset++
			h |= uint32(safeRead(src, srcOffset))
			srcOffset++
			copyOffset := dstOffset - (int((h & block.mask) + 1))
			m := 2 + (h >> block.shift)
			fmt.Printf("\t\th is %08x, m is %08x, copy offset is %08x\n", h, m, copyOffset)
			for i := 0; i < int(m)+1; i++ {
				fmt.Printf("\t\tWriting compressed data @ %08x to %08x: %02x\n", copyOffset, dstOffset, dst[copyOffset])
				safeWrite(dst, dstOffset, safeRead(dst, copyOffset))
				dstOffset++
				copyOffset++
			}
		}
		cycle--
	}
	fmt.Println()

	return dst
}
