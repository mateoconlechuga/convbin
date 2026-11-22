#!/usr/bin/env python3
"""Check ELF segments selected for TI-OS's fixed Flash App data copy."""

from pathlib import Path
import struct
import subprocess
import sys
import tempfile


APP_DATA_VADDR = 0xD1187E
APP_METADATA_OFFSET = 0x14E


def make_elf(path, segments):
    ident = b"\x7fELF\x01\x01\x01" + b"\0" * 9
    program_offset = 52
    data_offset = program_offset + len(segments) * 32
    program_headers = []
    data = bytearray()

    for paddr, vaddr, contents, memsz in segments:
        offset = data_offset + len(data)
        program_headers.append(struct.pack(
            "<8I", 1, offset, vaddr, paddr,
            len(contents), memsz, 6, 1))
        data.extend(contents)

    section_offset = data_offset + len(data)
    header = struct.pack(
        "<16sHHIIIIIHHHHHH",
        ident, 2, 220, 1, 0, program_offset, section_offset, 0,
        52, 32, len(segments), 40, 1, 0)
    path.write_bytes(header + b"".join(program_headers) + data + b"\0" * 40)


def convert(convbin, directory, name, segments, succeeds=True):
    source = directory / (name + ".obj")
    output = directory / (name + ".8ek")
    make_elf(source, segments)
    result = subprocess.run([
        convbin, "--iformat", "elf", "--input", str(source),
        "--oformat", "8ek", "--output", str(output), "--name", "TEST"
    ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if succeeds and result.returncode != 0:
        raise AssertionError(result.stdout)
    if not succeeds and result.returncode == 0:
        raise AssertionError("conversion unexpectedly succeeded")
    return output.read_bytes() if succeeds else result.stdout


def read_u24(data, offset):
    return int.from_bytes(data[offset:offset + 3], "little")


def copy_fields(data):
    return (
        read_u24(data, APP_METADATA_OFFSET + 0x15),
        read_u24(data, APP_METADATA_OFFSET + 0x18),
    )


def main():
    convbin = Path(sys.argv[1])
    if not convbin.exists() and convbin.with_suffix(".exe").exists():
        convbin = convbin.with_suffix(".exe")
    convbin = str(convbin)
    code = (0, 0, b"CODE", 4)

    with tempfile.TemporaryDirectory(prefix="convbin-app-data-") as temporary:
        directory = Path(temporary)

        unrelated = convert(convbin, directory, "unrelated", [
            code,
            (0x10, 0xD052C6, b"DATA", 8),
        ])
        assert copy_fields(unrelated) == (0, 0)

        contiguous = convert(convbin, directory, "contiguous", [
            code,
            (0x10, APP_DATA_VADDR, b"DATA", 4),
            (0x14, APP_DATA_VADDR + 4, b"MORE", 8),
        ])
        assert copy_fields(contiguous) == (0x2A + 0x10, 12)
        source = APP_METADATA_OFFSET + 0x2A + 0x10
        assert contiguous[source:source + 12] == b"DATAMORE" + b"\0" * 4

        mismatched = convert(convbin, directory, "mismatched", [
            code,
            (0x10, APP_DATA_VADDR, b"DATA", 4),
            (0x14, APP_DATA_VADDR + 5, b"MORE", 4),
        ])
        assert copy_fields(mismatched) == (0x2A + 0x10, 4)

        oversized = convert(convbin, directory, "oversized", [
            code,
            (0x10, APP_DATA_VADDR, b"D", 0x8001),
        ], succeeds=False)
        assert "exceeds its 32 KiB destination" in oversized

    return 0


if __name__ == "__main__":
    sys.exit(main())
