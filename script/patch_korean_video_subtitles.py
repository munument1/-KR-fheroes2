#!/usr/bin/env python3
"""Prepare Korean video subtitles for the custom fheroes2 byte renderer.

The Korean renderer does not consume UTF-8 directly. Runtime Korean text is
normally converted by compile_korean_mo.py to a compact three-byte Hangul
encoding. Video subtitles that live in C++ have to use the same encoding.

This build-time patch converts the Succession Wars intro literals and adds
subtitles for the first Roland/Archibald crystal-ball briefings.
"""

from __future__ import annotations

import codecs
import re
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


def cpp_byte_literal(text: str) -> str:
    return '"' + "".join(f"\\x{value:02X}" for value in encode_text(text)) + '"'


def decode_cpp_utf8_literal(body: str) -> str:
    # The subtitle literals use ASCII plus C++ \\uXXXX escapes, so unicode_escape
    # gives us the original Korean Unicode string without depending on the host
    # source-file code page.
    return codecs.decode(body, "unicode_escape")


def encode_intro_literals(source: str) -> str:
    start_marker = "    std::vector<Video::Subtitle> getSuccessionWarsIntroSubtitles()"
    start = source.find(start_marker)
    if start < 0:
        raise SystemExit("Succession Wars intro subtitle function was not found.")

    end_marker = "        return subtitles;\n    }"
    end = source.find(end_marker, start)
    if end < 0:
        raise SystemExit("Succession Wars intro subtitle function end was not found.")
    end += len(end_marker)

    block = source[start:end]

    def replacement(match: re.Match[str]) -> str:
        return cpp_byte_literal(decode_cpp_utf8_literal(match.group(1)))

    converted, count = re.subn(r'u8"((?:\\.|[^"\\])*)"', replacement, block)
    if count != 13:
        raise SystemExit(f"Expected 13 intro subtitle literals, converted {count}.")

    return source[:start] + converted + source[end:]


def make_briefing_function(name: str, lines: list[tuple[str, int, int]]) -> str:
    entries = []
    for korean, start_ms, duration_ms in lines:
        entries.append(
            "        subtitles.emplace_back( fheroes2::Text( "
            + cpp_byte_literal(korean)
            + ", subtitleFont ), "
            + f"{start_ms}, {duration_ms}, subtitlePosition, subtitleWidth );"
        )

    return f'''    std::vector<Video::Subtitle> {name}()\n    {{\n        const fheroes2::FontType subtitleFont = fheroes2::FontType::normalWhite();\n        const fheroes2::Point subtitlePosition{{ 320, 390 }};\n        constexpr int32_t subtitleWidth = 600;\n\n        std::vector<Video::Subtitle> subtitles;\n        subtitles.reserve( {len(lines)} );\n{chr(10).join(entries)}\n\n        return subtitles;\n    }}\n\n'''


def add_first_briefing_subtitles(source: str) -> str:
    if "getRolandScenario1Subtitles" in source:
        return source

    roland_lines = [
        ("충성스러운 신하여. 그대의 충성 맹세를 감사와 안도의 마음으로 받아들이오.", 500, 7500),
        ("알다시피 사악한 동생 아치발드는 아버지의 성을 차지하고 왕위를 빼앗았소.", 8200, 8500),
        ("지금은 성 근처의 영주들만 그에게 충성을 맹세했지만, 이대로라면 먼 곳의 영주들까지 따를 것이오.", 17200, 10500),
        ("그래서 여름 궁전 근처의 남작들을 무력으로 복종시키라는 명을 내릴 수밖에 없소.", 28200, 9000),
        ("임무를 위해 금화와 마법의 부적을 준비했소. 늘 지니면 계속 연락할 수 있을 것이오.", 37700, 10000),
    ]

    archibald_lines = [
        ("나를 섬기기로 한 선택에 감사하네. 충성하는 봉신에게 나는 매우 관대한 군주가 될 수 있지.", 500, 9500),
        ("하지만 내게 충성을 맹세하지 않는 겁쟁이 영주들에게까지 그럴 필요는 없겠지.", 10500, 8500),
        ("왕은 나다! 롤랜드가 아니야! 누구도 내 앞에서 감히 복종을 거부할 수 없다!", 19500, 8500),
        ("가까운 영주들에게 본보기를 보일 수 있도록 금화를 마련해 두었다. 가서 그들을 짓밟아라!", 28500, 10000),
        ("그리고 내가 보낸 마법의 부적으로 결과를 보고하도록 해라.", 39000, 8000),
    ]

    addition = make_briefing_function("getRolandScenario1Subtitles", roland_lines)
    addition += make_briefing_function("getArchibaldScenario1Subtitles", archibald_lines)

    marker = "}\n\nnamespace Video\n{"
    if marker not in source:
        raise SystemExit("Video namespace insertion point was not found.")

    return source.replace(marker, addition + "}\n\nnamespace Video\n{", 1)


def hook_briefing_subtitles(source: str) -> str:
    # INTRO.SMK subtitles should only be active when the Korean language is selected.
    old = '        if ( infos.size() == 1 && infos.front().fileName == "INTRO.SMK" ) {'
    new = '        if ( Settings::Get().getGameLanguage() == "ko" && infos.size() == 1 && infos.front().fileName == "INTRO.SMK" ) {'
    if old in source:
        source = source.replace(old, new, 1)
    elif new not in source:
        raise SystemExit("INTRO.SMK subtitle hook was not found.")

    hook_marker = '''        if ( infos.empty() ) {\n            // What it is expected from an empty video?\n            return false;\n        }\n\n'''
    hook = '''        if ( subtitles.empty() && Settings::Get().getGameLanguage() == "ko" && infos.size() == 2 ) {\n            if ( infos.front().fileName == "GOOD01.SMK" ) {\n                return ShowVideo( infos, getRolandScenario1Subtitles(), fadeColorsOnEnd );\n            }\n            if ( infos.front().fileName == "EVIL01.SMK" ) {\n                return ShowVideo( infos, getArchibaldScenario1Subtitles(), fadeColorsOnEnd );\n            }\n        }\n\n'''

    if hook in source:
        return source
    if hook_marker not in source:
        raise SystemExit("ShowVideo subtitle hook insertion point was not found.")

    return source.replace(hook_marker, hook_marker + hook, 1)


def main() -> None:
    path = Path("src/fheroes2/game/game_video.cpp")
    source = path.read_text(encoding="utf-8")
    source = encode_intro_literals(source)
    source = add_first_briefing_subtitles(source)
    source = hook_briefing_subtitles(source)
    path.write_text(source, encoding="utf-8", newline="\n")
    print("Prepared Korean subtitles for INTRO.SMK, GOOD01.SMK and EVIL01.SMK.")


if __name__ == "__main__":
    main()
