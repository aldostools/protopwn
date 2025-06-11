package main

import (
	"fmt"
	"os"
)

func main() {
	if len(os.Args) < 4 {
		goto fail
	}

	switch os.Args[1] {
	case "unpack":
		indata, err := os.ReadFile(os.Args[2])
		if err != nil {
			fmt.Println("Failed to read the input file:", err)
			return
		}

		outdata := decompress(indata)
		if outdata == nil {
			fmt.Println("Failed to decompress")
			return
		}

		if err = os.WriteFile(os.Args[3], outdata, 0644); err != nil {
			fmt.Println("Failed to write to the input file:", err)
		}

	case "pack":
		indata, err := os.ReadFile(os.Args[2])
		if err != nil {
			fmt.Println("Failed to read the input file:", err)
			return
		}

		outFile, err := os.Create(os.Args[3])
		if err != nil {
			fmt.Println("Failed to create the output file:", err)
			return
		}
		defer outFile.Close()

		if err := fakecompress(indata, outFile); err != nil {
			fmt.Println("Failed to compress:", err)
		}
	default:
		goto fail
	}

	return

fail:
	fmt.Println("Usage: osdpack <pack/unpack> <input file> <output file>")
	return
}
