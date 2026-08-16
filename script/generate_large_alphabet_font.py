#!/usr/bin/env python3
"""Generate compressed fixed-cell bitmap glyph data for the UTF-8 large-alphabet provider."""
from __future__ import annotations

import argparse
import base64
import struct
import zlib
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def parse_codepoint(value: str) -> int:
    text = value.strip().upper()
    if text.startswith("U+"):
        result = int(text[2:], 16)
    elif text.startswith("0X"):
        result = int(text, 16)
    else:
        base = 16 if any(ch in "ABCDEF" for ch in text) else 10
        result = int(text, base)

    if result < 0 or result > 0x10FFFF or 0xD800 <= result <= 0xDFFF:
        raise argparse.ArgumentTypeError(f"Invalid Unicode code point: {value}")

    return result


def parse_range(value: str) -> tuple[int, int]:
    try:
        start_text, end_text = value.split(":", 1)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("Unicode ranges must use START:END, for example AC00:D7A3.") from exc

    start = parse_codepoint(start_text)
    end = parse_codepoint(end_text)
    if start > end:
        raise argparse.ArgumentTypeError(f"Unicode range start exceeds end: {value}")

    return start, end


def collect_codepoints(po_path: Path | None, ranges: list[tuple[int, int]]) -> list[int]:
    codepoints: set[int] = set()

    for start, end in ranges:
        codepoints.update(range(start, end + 1))

    if po_path is not None:
        import polib

        po = polib.pofile(str(po_path), encoding="utf-8")
        for entry in po:
            translated: list[str] = []
            if entry.msgstr:
                translated.append(entry.msgstr)
            translated.extend(value for value in entry.msgstr_plural.values() if value)

            for text in translated:
                for ch in text:
                    codepoint = ord(ch)
                    # ASCII continues to use the stock fheroes2 bitmap fonts.
                    if codepoint >= 0x80 and ch not in "\r\n\t":
                        codepoints.add(codepoint)

    return sorted(codepoints)


def build_rows(font_path: Path, codepoints: list[int], point_size: int, width: int, height: int) -> bytes:
    font = ImageFont.truetype(str(font_path), point_size)
    output = bytearray()

    for codepoint in codepoints:
        ch = chr(codepoint)

        # Render directly to a 1-bit target so pixel fonts retain their exact grid.
        image = Image.new("1", (width, height), 0)
        draw = ImageDraw.Draw(image)
        bbox = font.getbbox(ch)
        x = -bbox[0]
        y = -bbox[1]
        draw.text((x, y), ch, font=font, fill=1, stroke_width=0)

        pixels = image.load()
        for row_y in range(height):
            bits = 0
            for x_pos in range(width):
                if pixels[x_pos, row_y]:
                    bits |= 1 << x_pos
            output.extend(struct.pack("<I", bits))

    return bytes(output)


def quoted_chunks(value: str, width: int = 120) -> str:
    return "\n".join(f'    "{value[i:i + width]}"' for i in range(0, len(value), width))


def codepoint_chunks(codepoints: list[int], per_line: int = 12) -> str:
    lines: list[str] = []
    for i in range(0, len(codepoints), per_line):
        values = ", ".join(f"0x{value:04X}U" for value in codepoints[i:i + per_line])
        lines.append(f"        {values},")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("font", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--po", type=Path, default=None, help="Also include all non-ASCII code points used by translated strings in this PO file.")
    parser.add_argument(
        "--range",
        dest="ranges",
        action="append",
        type=parse_range,
        default=[],
        metavar="START:END",
        help="Include an inclusive Unicode range. May be repeated. Hex values such as AC00:D7A3 and U+4E00:U+9FFF are accepted.",
    )
    parser.add_argument("--base-size", type=int, default=12, help="Font point size for small and normal glyphs.")
    parser.add_argument("--cell-width", type=int, default=12, help="Fixed cell width for small and normal glyphs (maximum 32).")
    parser.add_argument("--cell-height", type=int, default=12, help="Fixed cell height for small and normal glyphs.")
    parser.add_argument("--large-scale", type=int, default=2, help="Integer scale applied to the large glyph set.")
    args = parser.parse_args()

    if args.base_size <= 0 or args.cell_width <= 0 or args.cell_height <= 0 or args.large_scale <= 0:
        raise SystemExit("Font size, cell dimensions and large scale must be positive.")
    if args.cell_width > 32 or args.cell_width * args.large_scale > 32:
        raise SystemExit("Generated glyph rows use 32-bit masks, so cell width (including large scale) cannot exceed 32 pixels.")

    codepoints = collect_codepoints(args.po, args.ranges)
    if not codepoints:
        raise SystemExit("No large-alphabet code points were selected.")

    specs = [
        ("small", args.base_size, args.cell_width, args.cell_height),
        ("normal", args.base_size, args.cell_width, args.cell_height),
        (
            "large",
            args.base_size * args.large_scale,
            args.cell_width * args.large_scale,
            args.cell_height * args.large_scale,
        ),
    ]

    blocks: list[str] = []
    blocks.append(
        "// Generated by script/generate_large_alphabet_font.py. Do not edit.\n"
        "#pragma once\n\n"
        "#include <array>\n"
        "#include <cstddef>\n"
        "#include <cstdint>\n"
        "#include <string_view>\n\n"
        "namespace fheroes2::largeAlphabetGenerated\n"
        "{"
    )
    blocks.append(f"    inline constexpr std::size_t glyphCount = {len(codepoints)};")
    blocks.append("    inline constexpr std::array<uint32_t, glyphCount> codePoints = {\n" + codepoint_chunks(codepoints) + "\n    };")

    for name, point_size, width, height in specs:
        raw = build_rows(args.font, codepoints, point_size, width, height)
        encoded = base64.b64encode(zlib.compress(raw, 9)).decode("ascii")
        blocks.append(f"    inline constexpr int {name}Width = {width};")
        blocks.append(f"    inline constexpr int {name}Height = {height};")
        blocks.append(f"    inline constexpr int {name}Advance = {width};")
        blocks.append(f"    inline constexpr std::size_t {name}RawSize = {len(raw)};")
        blocks.append(f"    inline constexpr std::string_view {name}Base64 =\n{quoted_chunks(encoded)};")

    blocks.append("}\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(blocks), encoding="utf-8")

    print(f"Generated {len(codepoints)} glyphs into {args.output}")


if __name__ == "__main__":
    main()
