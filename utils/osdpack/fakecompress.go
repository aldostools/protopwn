package main

import (
	"encoding/binary"
	"io"
)

func fakecompress(input []byte, dst io.Writer) error {
	// Write the length of the input as the first 4 bytes of the output
	lengthBytes := make([]byte, 4)
	binary.LittleEndian.PutUint32(lengthBytes, uint32(len(input)))

	if _, err := dst.Write(lengthBytes); err != nil {
		return err
	}

	srcOffset := 0

	for srcOffset <= len(input)-1 {
		// Write a null block descriptor for each 30 bytes of the input
		_, err := dst.Write([]byte{0, 0, 0, 0})
		if err != nil {
			return err
		}

		if srcOffset+30 > len(input) {
			if _, err = dst.Write(input[srcOffset:]); err != nil {
				return err
			}
		} else {
			if _, err = dst.Write(input[srcOffset : srcOffset+30]); err != nil {
				return err
			}
		}

		srcOffset += 30
	}
	return nil
}
