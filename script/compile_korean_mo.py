#!/usr/bin/env python3
"""Convert a validated UTF-8 Korean GNU MO into fheroes2 prototype byte encoding.

Hangul syllables are encoded as three disjoint bytes:
  0x80..0x9F : lead
  0xA0..0xBF : middle
  0xC0..0xFF : trail

The first byte carries the cell advance in the renderer; the last two bytes
complete the glyph index. ASCII remains unchanged.
"""

from __future__ import annotations

import argparse
import struct
import unicodedata
from pathlib import Path

HANGUL_FIRST = 0xAC00
HANGUL_LAST = 0xD7A3

PUNCTUATION_FALLBACK = {
    "\u00a0": " ",
    "\u00b7": ".",
    "\u00d7": "x",
    "\u2010": "-",
    "\u2011": "-",
    "\u2012": "-",
    "\u2013": "-",
    "\u2014": "-",
    "\u2015": "-",
    "\u2018": "'",
    "\u2019": "'",
    "\u201c": '"',
    "\u201d": '"',
    "\u2026": "...",
    "\u2212": "-",
}


def encode_text(text: str) -> bytes:
    out = bytearray()
    for ch in unicodedata.normalize("NFC", text):
        cp = ord(ch)
        if cp < 0x80:
            out.append(cp)
            continue

        if HANGUL_FIRST <= cp <= HANGUL_LAST:
            index = cp - HANGUL_FIRST
            out.append(0x80 + ((index >> 11) & 0x1F))
            out.append(0xA0 + ((index >> 6) & 0x1F))
            out.append(0xC0 + (index & 0x3F))
            continue

        replacement = PUNCTUATION_FALLBACK.get(ch, "?")
        out.extend(replacement.encode("ascii"))

    return bytes(out)


def unpack_mo(data: bytes) -> tuple[str, list[bytes], list[bytes]]:
    if len(data) < 28:
        raise ValueError("MO file is too small")

    if data[:4] == b"\xde\x12\x04\x95":
        endian = "<"
    elif data[:4] == b"\x95\x04\x12\xde":
        endian = ">"
    else:
        raise ValueError("invalid MO magic")

    _magic, revision, count, original_offset, translation_offset, _hash_size, _hash_offset = struct.unpack_from(endian + "7I", data, 0)
    if revision >> 16 != 0:
        raise ValueError(f"unsupported MO major revision: {revision >> 16}")

    originals: list[bytes] = []
    translations: list[bytes] = []

    for i in range(count):
        length, offset = struct.unpack_from(endian + "2I", data, original_offset + i * 8)
        originals.append(data[offset:offset + length])

        length, offset = struct.unpack_from(endian + "2I", data, translation_offset + i * 8)
        translations.append(data[offset:offset + length])

    return endian, originals, translations


def transform_translation(original: bytes, translated: bytes) -> bytes:
    if not original:
        header = translated.decode("utf-8", errors="strict")
        header = header.replace("charset=UTF-8", "charset=FH2-KOREAN")
        return header.encode("ascii", errors="replace")

    forms = translated.split(b"\0")
    encoded_forms = [encode_text(form.decode("utf-8", errors="strict")) for form in forms]

    # The prototype engine does not know Korean plural rules yet. Keep two
    # identical forms so a future locale registration can safely pick either.
    if b"\0" in original and len(encoded_forms) == 1:
        encoded_forms.append(encoded_forms[0])

    return b"\0".join(encoded_forms)


def build_mo(originals: list[bytes], translations: list[bytes]) -> bytes:
    if len(originals) != len(translations):
        raise ValueError("original/translation table size mismatch")

    count = len(originals)
    header_size = 28
    original_table_offset = header_size
    translation_table_offset = original_table_offset + count * 8
    data_offset = translation_table_offset + count * 8

    original_table: list[tuple[int, int]] = []
    translation_table: list[tuple[int, int]] = []
    original_blob = bytearray()
    translation_blob = bytearray()

    cursor = data_offset
    for value in originals:
        original_table.append((len(value), cursor))
        original_blob.extend(value)
        original_blob.append(0)
        cursor += len(value) + 1

    for value in translations:
        translation_table.append((len(value), cursor))
        translation_blob.extend(value)
        translation_blob.append(0)
        cursor += len(value) + 1

    out = bytearray()
    out.extend(struct.pack("<7I", 0x950412DE, 0, count, original_table_offset, translation_table_offset, 0, 0))

    for length, offset in original_table:
        out.extend(struct.pack("<2I", length, offset))
    for length, offset in translation_table:
        out.extend(struct.pack("<2I", length, offset))

    out.extend(original_blob)
    out.extend(translation_blob)
    return bytes(out)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_mo", type=Path)
    parser.add_argument("output_mo", type=Path)
    args = parser.parse_args()

    data = args.input_mo.read_bytes()
    _endian, originals, translations = unpack_mo(data)
    transformed = [transform_translation(o, t) for o, t in zip(originals, translations)]

    args.output_mo.parent.mkdir(parents=True, exist_ok=True)
    args.output_mo.write_bytes(build_mo(originals, transformed))


if __name__ == "__main__":
    main()
