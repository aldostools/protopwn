# ProtoPwn

An exploit for the Protokernel PlayStation 2 systems that enables arbitrary code execution through a flaw in the OSDSYS Browser update code.

Supported PS2 models:
- SCPH-10000
- SCPH-15000
- DTL-H10000(S)

Note: newer PS2 models are not supported and __will never be__ supported by this exploit.

## Usage

1. Run `make`
2. Copy `BIEXEC-SYSTEM` to the root of your memory card
3. Copy the payload you want to run to:
    - _ProtoPwn_: `BOOT/BOOT.ELF`
    - _ProtoPwn UMCS_: `BIEXEC-SYSTEM/BOOT.ELF`

## How it works

ProtoPwn consists of a fairly simple two-stage payload and a packer tool:
1. [MBROWS replacement](src/)
    - Runs from 0x7a0000
    - Patches OSDSYS code to execute a custom function in the main thread
    - Applies the EELOAD [kernel patch](kpatch/README.md)
    - Deinitializes OSDSYS
    - Finds the target ELF on `mc0` or `mc1`
    - Executes the embedded ELF loader
2. [Embedded ELF loader](loader/)
    - Cleans up after OSDSYS
    - Loads the target ELF 
    - Resets the IOP
    - Executes the target ELF
3. Simple packer [script](utils/fakepack.py)
    - Does the bare minimum required to get OSDSYS to accept and "decompress" our payload
    - Actually increases the file size

The target ELF path can be modified at build-time by passing `BOOT_PATH` argument to `make`.  
`BOOT_PATH` must be relative to `mc0/mc1`, e.g. `BOOT/BOOT.ELF`

### Browser update

During initialization, protokernel OSDSYS checks for a Browser (MBROWS) update manifest in `mc1:/BIEXEC-SYSTEM/` and `mc0:/BIEXEC-SYSTEM/`.  
The manifest file is called `OSBROWS` and consists of just three lines:
```
101 — module version, must be higher than 100
PROTPWN — module filename, relative to `BIEXEC-SYSTEM`. Cannot exceed 7 characters
007a0000 — module load address
```

When this file exists, OSDSYS will parse it, load the specified file at `0x1000000` and decompress it to the specified load address. This module is then executed as the OSD Browser thread function.  
Similar to `__mbr`, this module must be headerless, with its entry point located at the beginning of the payload.

For some reason, unlike system updates, the Browser update is completely unencrypted, has no intergrity or validity checks and uses a fairly unsophisitcated compression scheme that can be easily bypassed.

Thus, it is possible to get code execution just by creating a custom payload that will run in the Browser thread.

### Module compression

OSD modules are always compressed.
The compression scheme is block-based and structured as follows:
```
<4-byte uncompressed payload length in little endian>
<4-byte block descriptor>
<30-byte block>
<4-byte block descriptor>
<30-byte block>
...
```
The block descriptor marks whether the byte in this block is compressed and contains the byte shift and byte mask used to unpack the bytes.  
The OSDSYS unpacker stops processing right after reaching the declared payload length, ignoring all trailing bytes.

To bypass the compression, the uncompressed payload can be written as-is, with the length header at the start of the payload and four null bytes preceding each 30-byte block.

See [osdpack](utils/osdpack/) code for more details.

# Credits
- [Julian Uy](https://github.com/uyjulian) for [reverse-engineering](https://gist.github.com/uyjulian/14388e84b008a6433aa805f5d0436c87) the OSDSYS decompression code
- [Matías Israelson](https://github.com/israpps) for giving me a reason to explore this further