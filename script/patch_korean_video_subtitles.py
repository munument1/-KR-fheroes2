#!/usr/bin/env python3
'''Prepare Korean video subtitles for the custom fheroes2 byte renderer.

The rendering rules in this script are the approved Korean subtitle baseline:
large Korean font, no stock video contour/background, built-in glyph shadow,
bottom-center anchor at y=430, and multiline text growing upward.
'''

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


# Timings for GOOD01V/EVIL01V are the already play-tested values and must not
# be changed casually. The remaining Succession Wars campaign timings are
# aligned to the actual SMK durations and speech/pause boundaries from ANIM.zip.
BASE_CAMPAIGN_SUBTITLES: dict[str, list[tuple[str, int, int]]] = {
    "GOOD01V.SMK": [
        ("충성스러운 신하여. 그대의 충성 맹세를 감사와 안도의 마음으로 받아들이오.", 500, 7500),
        ("알다시피 사악한 동생 아치발드는 아버지의 성을 차지하고 왕위를 빼앗았소.", 8200, 8500),
        ("지금은 성 근처의 영주들만 그에게 충성을 맹세했지만, 이대로라면 먼 곳의 영주들까지 따를 것이오.", 17200, 10500),
        ("그래서 여름 궁전 근처의 남작들을 무력으로 복종시키라는 명을 내릴 수밖에 없소.", 28200, 9000),
        ("임무를 위해 금화와 마법의 부적을 준비했소. 늘 지니면 계속 연락할 수 있을 것이오.", 37700, 10000),
    ],
    "GOOD02W.SMK": [
        ("잘 해냈소! 첫 작전은 완벽한 성공이오.", 500, 5500),
        ("하지만 축하할 시간은 없소. 주변 귀족들이 우리의 승리에 반발해 방어를 굳혔소.", 6200, 7600),
        ("그들의 반란을 진압하시오. 지난 임무에서 모은 병력은 이 지역을 지키는 데 필요하오.", 14100, 8200),
        ("아치발드와의 전쟁 자금을 마련하려면 그들의 세금을 확보해야 하오.", 22600, 8000),
        ("남은 금화를 모두 주겠소. 군대를 일으켜 그들의 항복을 받아내시오.", 30900, 8200),
    ],
    "GOOD03QW.SMK": [
        ("축하하오! 훌륭한 지휘였소. 이 비옥한 지역 덕분에 전쟁에 필요한 금화를 마련했소.", 500, 8000),
        ("이제 자원을 확보해야 하오. 아치발드가 남부의 카라토르 광산 지대로 병력을 보내고 있소.", 8800, 8200),
        ("동시에 드워프 왕 로클린에게 구원 요청이 왔소. 아치발드의 워록들이 드워프 마을을 몰살했소.", 17400, 9000),
        ("금을 찾지 못한 그들은 다른 드워프 영지까지 공격하고 있소.", 26800, 7000),
        ("장군, 그대의 의견을 듣고 싶소. 카라토르를 즉시 공격하겠소?", 34200, 8500),
        ("아니면 드워프를 구하러 가겠소?", 43100, 8300),
    ],
    "GOOD04W.SMK": [
        ("드워프 왕 로클린이 그대의 도움에 영원히 감사한다는 전갈을 보내왔소.", 500, 6300),
        ("그는 그대가 나를 섬기는 한 드워프들의 전폭적인 지원을 약속했소.", 7100, 6000),
        ("이미 병력을 카라토르 산맥으로 보냈소. 이제 그대의 명령만 기다리고 있소.", 13400, 6100),
    ],
    "GOOD05V.SMK": [
        ("카라토르 광산을 장악한 덕분에 우리의 입지는 크게 강해졌소.", 500, 8000),
        ("관망하던 영주들도 우리 편에 서기 시작했고, 이 계곡의 두 영주도 충성을 맹세했소.", 8800, 8200),
        ("남은 두 영주의 요새는 아치발드 성으로 이어지는 보급로를 지배하는 핵심 계곡에 있소.", 17400, 9500),
        ("현지에서 군대를 일으켜 그들을 공격하시오. 승리하면 대륙의 자원을 장악하고 공세로 전환할 수 있소.", 27200, 9000),
        ("장군! 아치발드 왕이다. 그대의 활약이 인상적이어서 내 편에 설 기회를 한 번 주겠다.", 36900, 7000),
        ("내 동생은 꿈도 못 꿀 부와 권력을 주지. 내가 반란군을 짓밟으면 그대에게 남는 건 교수대뿐이다.", 44200, 7800),
        ("부와 권력을 택하겠나, 교수대를 택하겠나? 자, 대답해라.", 52300, 7200),
    ],
    "GOOD06AV.SMK": [
        ("예언자에게 들었소. 그대가 아치발드의 배신 제안을 거절했다지. 현명한 선택이오.", 500, 7500),
        ("전쟁이 끝날 때까지 충성을 지킨다면 그 공을 반드시 보상하겠소.", 8300, 8500),
        ("노라스톤의 소서리스들이 도움을 청했소. 나를 공개 지지한 탓에 아치발드가 대군을 보냈소.", 17100, 9000),
        ("우리의 신의를 지키려면 그들을 구해야 하오. 할튼 경을 보내 돕게 하겠소. 다시 한번 고맙소.", 26400, 9500),
    ],
    "GOOD06BV.SMK": [
        ("옳은 결정을 내렸소! 끝까지 내게 충성한다면 전쟁이 끝난 뒤 반드시 보답하겠소.", 500, 6500),
        ("노라스톤의 소서리스 길드에서 원군을 요청해 왔소.", 7300, 7600),
        ("나를 공개 지지한 탓에 아치발드가 본보기로 삼으려 대군을 보냈소. 반드시 지켜야 하오.", 15200, 7800),
        ("우리의 외교적 신의를 위해서요. 할튼 경을 보내 그들을 규합하도록 돕겠소. 고맙소.", 23300, 7700),
    ],
    "GOOD07QW.SMK": [
        ("소서리스들이 도움에 감사하며, 그대가 나를 섬기는 동안 최고의 마법사들을 지원하겠다고 했소.", 500, 6500),
        ("장군, 이제 아치발드에 대한 최종 공격을 준비할 때가 다가오고 있소.", 7400, 6200),
        ("참모들의 의견이 갈렸소. 절반은 아치발드에게 충성하는 영주들을 칠 병력을 모으라 하고,", 13900, 6200),
        ("나머지는 이동 관문을 지나 전설의 궁극의 왕관을 찾으라고 하오.", 20400, 6300),
        ("그대라면 어느 쪽을 택하겠소?", 27000, 4000),
    ],
    "GOOD09W.SMK": [
        ("그대의 군대가 아치발드의 방어선을 공격할 준비를 마친 것이 보이는군.", 500, 7000),
        ("하지만 서부 영지에 대한 필사적인 반격 때문에 지금은 원군을 보낼 수 없소.", 7800, 7000),
        ("그대가 모은 병력과 마법이 충분하기를 바랄 뿐이오. 할트 경이 도울 것이오.", 15100, 6800),
        ("그리고 반드시 코를라곤 장군을 생포하시오. 그의 지혜를 빼앗으면 아치발드는 훨씬 약해질 것이오.", 22200, 6500),
    ],
    "GOOD10W.SMK": [
        ("마침내 전쟁이 끝을 향해 가고 있소. 아치발드는 항복을 거부하고 언데드 군대를 백성들에게 풀어놓았소.", 500, 7200),
        ("코를라곤을 상대로 승리한 병력은 백성을 지키는 데 돌릴 수밖에 없소.", 8000, 7200),
        ("할트 경에게 방어를 맡기고, 그대에게는 약해진 아치발드의 군대를 공격하게 하겠소.", 15500, 7000),
        ("최근 몰수한 아치발드 봉신들의 영지를 마음껏 이용하시오. 현명하게 쓰시오.", 22800, 7200),
        ("내 동생을 생포해 이 내전을 끝냅시다!", 30300, 7800),
    ],
    "LIBRARYW.SMK": [
        ("아치발드 형제여. 왕국과 나에게 저지른 죄에 대해, 그대가 내게 베풀지 않았을 자비를 베풀겠소.", 500, 7200),
        ("그대를 석상으로 만들어 서쪽 탑에 가두겠소. 훗날 후손들이 가엾게 여겨 다시 살려낼 때까지 말이오.", 8000, 7200),
        ("물론 그런 날이 온다면 말이지. 어쨌든 그대가 다시 왕관을 볼 일은 없을 것이오.", 15500, 6800),
    ],
    "EVIL01V.SMK": [
        ("나를 섬기기로 한 선택에 감사하네. 충성하는 봉신에게 나는 매우 관대한 군주가 될 수 있지.", 500, 9500),
        ("하지만 내게 충성을 맹세하지 않는 겁쟁이 영주들에게까지 그럴 필요는 없겠지.", 10500, 8500),
        ("왕은 나다! 롤랜드가 아니야! 누구도 내 앞에서 감히 복종을 거부할 수 없다!", 19500, 8500),
        ("가까운 영주들에게 본보기를 보일 수 있도록 금화를 마련해 두었다. 가서 그들을 짓밟아라!", 28500, 10000),
        ("그리고 내가 보낸 마법의 부적으로 결과를 보고하도록 해라.", 39000, 8000),
    ],
    "EVIL02W.SMK": [
        ("훌륭하군, 장군. 제대로 본보기를 보여준 것 같지 않나?", 500, 7000),
        ("가까운 영주들은 충성을 맹세하겠지만, 먼 곳의 영주들은 아직 망설이고 있다. 두 번째 본보기가 필요해.", 7800, 7000),
        ("북쪽의 얼어붙은 크라쇼 지방에는 아버지가 정복하지 않은 야만인 부족들이 여럿 있지.", 15100, 7000),
        ("그들은 뭉치면 강하지만 다행히 한 번도 통합된 적이 없다. 네가 가서 정복하고 하나로 묶어라.", 22400, 6500),
        ("내 동생의 반란을 진압할 훌륭한 보병이 되어 줄 것이다.", 29200, 5800),
    ],
    "EVIL03QW.SMK": [
        ("결정할 일이 생겼다. 롤랜드가 드워프 왕과 동맹을 맺었다는 소식이 들어왔어.", 500, 6500),
        ("그 동맹이 우리의 입지에 얼마나 위험한지는 너도 알겠지.", 7300, 6200),
        ("동시에 네크로맨서 길드에서 지원을 요청했다. 왕실군이 바쁜 틈을 타 위저드 길드가 그들을 공격했다는군.", 13800, 6200),
        ("장군, 선택해라. 드워프 동맹을 깨뜨릴 것인가,", 20300, 6200),
        ("아니면 네크로맨서 길드를 구할 것인가?", 26800, 5400),
    ],
    "EVIL05AV.SMK": [
        ("축하한다. 적인 위저드들은 약해졌고, 우리 편인 네크로맨서들은 강해졌다.", 500, 7000),
        ("네크로맨서 길드가 영원한 감사를 전해 왔다. 네가 나를 섬기는 동안 최고의 술사 한 명도 언제든 보내주겠다는군.", 7800, 7000),
        ("이제 로렌데일 계곡의 남작들 문제다. 둘이 내 반역자 동생을 공개적으로 지지했다. 용납할 수 없는 죄지.", 15100, 8000),
        ("충성스러운 두 영주의 성을 이용해 맨손에서 군대를 일으켜라. 실패하지 마라.", 23400, 10500),
    ],
    "EVIL05BV.SMK": [
        ("잘했다, 장군. 드워프 왕이 몹시 화가 나서 내게 증오가 가득한 편지를 보내왔더군.", 500, 7000),
        ("'살인자이자 사악한 찬탈자 아치발드에게. 영원한 지옥불에 떨어져 타 버리길 빈다!'", 7800, 7500),
        ("'그리고 영원히 드워프의 재앙이라 부를 네 추악한 장군도 같은 운명을 맞기를!'", 15600, 7000),
        ("하! 참으로 감동적이군. 오우거 왕에게서도 전갈이 왔다.", 22900, 6800),
        ("결과가 마음에 들었다며 우리와 영구 동맹을 맺고 병력까지 보내겠다는군.", 30000, 7800),
        ("이제 로렌데일 계곡의 남작들 문제다. 둘이 내 반역자 동생을 공개적으로 지지했다. 용납할 수 없어.", 38100, 7500),
        ("충성스러운 두 영주의 성을 이용해 처음부터 군대를 일으켜라. 실패하면 매우 불쾌할 것이다.", 45900, 8200),
    ],
    "RBETRAYV.SMK": [
        ("장군, 정통 국왕 롤랜드요. 그대에게 제안이 있소. 내 편에 서서 싸우시오.", 500, 5700),
        ("그대도 사악한 내 동생을 섬기는 것이 잘못이라는 걸 알 것이오. 부당한 찬탈자를 섬기며 양심이 괴롭지 않소?", 6500, 6000),
        ("대가가 무엇이오, 돈? 돈이라면 나도 줄 수 있소. 나는 부뿐 아니라 명예와 의무, 목적을 약속하오.", 12800, 5600),
        ("내게 와서 정의를 위해 싸우시오. 자, 그대의 대답은?", 18700, 5700),
    ],
    "EVIL06AW.SMK": [
        ("내 네크로맨서가 너와 내 동생의 대화를 엿들었다. 현명한 선택을 했군.", 500, 6800),
        ("롤랜드에게 승산은 없어. 그쪽에 붙었다면 네 목도 놈과 함께 처형대에 올랐을 거다.", 7600, 7000),
        ("계속 잘 섬기면 동생의 여름 궁전이 있는 영지를 백작령으로 내리겠다.", 14900, 7000),
        ("이제 일이다. 내 밭을 가는 농민들이 롤랜드의 선동을 받아 반란을 일으켰다.", 22200, 7000),
        ("반란을 진압하고 주동자들을 잡아라. 코를라곤 경을 보내 돕게 하겠다.", 29500, 3000),
    ],
    "EVIL06BW.SMK": [
        ("승자의 편에 온 걸 환영한다. 나는 함께 일하기 쉬운 사람이지. 성공에는 상을, 실패에는 벌을 준다.", 500, 5800),
        ("끝까지 충성하고 우리가 이 전쟁에서 이기면 너에게 백작령을 내리겠다.", 6600, 5800),
        ("이제 일이다. 내 밭을 가는 농민들이 롤랜드의 선동을 받아 반란을 일으켰다.", 12700, 5800),
        ("반란을 진압하고 주동자들을 잡아라. 코를라곤 경을 보내 돕게 하겠다.", 18800, 6000),
    ],
    "EVIL07W.SMK": [
        ("농민들을 벌한 솜씨가 인상적이더군. 모범적인 처리였다.", 500, 6500),
        ("다음은 롤랜드 성 근처의 지방 영주들을 정복할 차례다.", 7300, 6400),
        ("평범한 방법으로도 이길 수 있지만, 내 수석 워록은 드래곤 왕의 도움이 훨씬 효과적이라고 주장한다.", 14000, 6200),
        ("문제는 드래곤 왕이 자발적으로 돕지 않는다는 거지. 장군, 네 의견은?", 20500, 6200),
        ("즉시 롤랜드의 봉신들을 칠까, 아니면 먼저 드래곤들을 정복할까?", 27000, 4600),
    ],
    "EVIL08.SMK": [
        ("잘했다. 이제 드래곤 왕은 우리에게 협조하고 있고, 내가 목줄을 단단히 쥐고 있지.", 500, 5000),
        ("그들은 우리 군대에 병력을 보낼 것이고, 야생에서 만나는 드래곤들도 협조할 것이다.", 5800, 5000),
        ("이제 롤랜드의 봉신들에 대한 공격을 계속해라.", 11100, 4700),
    ],
    "EVIL09W.SMK": [
        ("지방 영주들을 너무 빨리 쓸어버려서 쉬워 보일 지경이군. 다음엔 상대에게 기회라도 주며 천천히 해라!", 500, 6000),
        ("이제 롤랜드에 대한 최종 공격을 준비할 때다.", 6800, 6500),
        ("참모들의 의견이 갈렸다. 하나는 롤랜드를 꺾을 군대를 모으자는 것이고,", 13600, 6300),
        ("다른 하나는 궁극의 유물을 찾자는 것이다. 현장의 지휘관은 너다. 어느 쪽을 택하겠나?", 20200, 6200),
    ],
    "EVIL11W.SMK": [
        ("우리의 가장 위대한 순간이 왔다! 롤랜드의 군대는 자기 성 주변으로 물러났다.", 500, 5700),
        ("그 바보 롤랜드는 여름 궁전에서 우리의 마지막 공격을 기다리며 떨고 있지.", 6500, 5700),
        ("최후의 전투를 위해 군대를 모았으니 가라! 반란을 짓밟고 내 동생을 사슬에 묶어 데려와라!", 12500, 5500),
    ],
    "PRISON.SMK": [
        ("그래, 롤랜드. 우리 작은 승부는 내가 이긴 모양이군.", 1400, 6500),
        ("걱정 마라. 목숨은 살려주고, 서쪽 탑의 군주로 임명해 주기로 했다.", 8200, 7000),
        ("아주 위대한 제국의 지배자가 되는 거야. 십 분이면 구석구석의 모든 틈새를 다 알게 되겠지.", 15500, 7000),
        ("중요한 쥐와 거미 손님들을 접대하지 않는 날이면, 언젠가 그 화려한 궁정을 방문해 주마.", 22800, 8500),
    ],
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

    # Keep the approved large subtitle face without the stock contour.
    # y=430 is interpreted as a bottom-center anchor for Korean subtitles.
    block = block.replace("fheroes2::FontType::normalWhite()", "fheroes2::FontType::largeWhite()", 1)
    block = block.replace("constexpr int32_t subtitleWidth = 600;", "constexpr int32_t subtitleWidth = 520;", 1)

    # Keep the corrected opening timings from the play-tested baseline.
    timing_replacements = {
        "            6000, 7000, subtitlePosition, subtitleWidth );": "            300, 8500, subtitlePosition, subtitleWidth );",
        "subtitleFont ), 13500, 2500, subtitlePosition,": "subtitleFont ), 9000, 2300, subtitlePosition,",
        "            16500, 3500, subtitlePosition, subtitleWidth );": "            11300, 4900, subtitlePosition, subtitleWidth );",
        "                                20500, 3500, subtitlePosition, subtitleWidth );": "                                16300, 4200, subtitlePosition, subtitleWidth );",
        "            24500, 8000, subtitlePosition, subtitleWidth );": "            20800, 11500, subtitlePosition, subtitleWidth );",
    }
    for old, new in timing_replacements.items():
        if old not in block:
            raise SystemExit(f"Intro timing pattern was not found: {old!r}")
        block = block.replace(old, new, 1)

    def replacement(match: re.Match[str]) -> str:
        return cpp_byte_literal(decode_cpp_utf8_literal(match.group(1)))

    converted, count = re.subn(r'u8"((?:\\.|[^"\\])*)"', replacement, block)
    if count != 13:
        raise SystemExit(f"Expected 13 intro subtitle literals, converted {count}.")

    return source[:start] + converted + source[end:]


def function_name_for_video(file_name: str) -> str:
    stem = re.sub(r"[^A-Za-z0-9]", "", Path(file_name).stem)
    return f"getKorean{stem}Subtitles"


def make_briefing_function(name: str, lines: list[tuple[str, int, int]]) -> str:
    entries = []
    for korean, start_ms, duration_ms in lines:
        entries.append(
            "        subtitles.emplace_back( fheroes2::Text( "
            + cpp_byte_literal(korean)
            + ", subtitleFont ), "
            + f"{start_ms}, {duration_ms}, subtitlePosition, subtitleWidth );"
        )

    return f'''    std::vector<Video::Subtitle> {name}()
    {{
        const fheroes2::FontType subtitleFont = fheroes2::FontType::largeWhite();
        const fheroes2::Point subtitlePosition{{ 320, 430 }};
        constexpr int32_t subtitleWidth = 520;

        std::vector<Video::Subtitle> subtitles;
        subtitles.reserve( {len(lines)} );
{chr(10).join(entries)}

        return subtitles;
    }}

'''


def add_campaign_subtitle_functions(source: str) -> str:
    dispatcher_name = "getKoreanCampaignSubtitles"
    if dispatcher_name in source:
        return source

    functions = []
    dispatch_cases = []
    for file_name, lines in BASE_CAMPAIGN_SUBTITLES.items():
        function_name = function_name_for_video(file_name)
        functions.append(make_briefing_function(function_name, lines))
        dispatch_cases.append(
            f'''            if ( info.fileName == "{file_name}" ) {{
                return {function_name}();
            }}'''
        )

    dispatcher = f'''    std::vector<Video::Subtitle> {dispatcher_name}( const std::vector<Video::VideoInfo> & infos )
    {{
        for ( const Video::VideoInfo & info : infos ) {{
{chr(10).join(dispatch_cases)}
        }}

        return {{}};
    }}

'''

    marker = "}\n\nnamespace Video\n{"
    if marker not in source:
        raise SystemExit("Video namespace insertion point was not found.")

    return source.replace(marker, "".join(functions) + dispatcher + "}\n\nnamespace Video\n{", 1)


def hook_korean_subtitles(source: str) -> str:
    old = '        if ( infos.size() == 1 && infos.front().fileName == "INTRO.SMK" ) {'
    new = '        if ( Settings::Get().getGameLanguage() == "ko" && infos.size() == 1 && infos.front().fileName == "INTRO.SMK" ) {'
    if old in source:
        source = source.replace(old, new, 1)
    elif new not in source:
        raise SystemExit("INTRO.SMK subtitle hook was not found.")

    hook_marker = '''        if ( infos.empty() ) {
            // What it is expected from an empty video?
            return false;
        }

'''
    hook = '''        if ( subtitles.empty() && Settings::Get().getGameLanguage() == "ko" ) {
            std::vector<Subtitle> koreanSubtitles = getKoreanCampaignSubtitles( infos );
            if ( !koreanSubtitles.empty() ) {
                return ShowVideo( infos, koreanSubtitles, fadeColorsOnEnd );
            }
        }

'''

    if hook in source:
        return source
    if hook_marker not in source:
        raise SystemExit("ShowVideo subtitle hook insertion point was not found.")

    return source.replace(hook_marker, hook_marker + hook, 1)


def patch_korean_subtitle_rendering(source: str) -> str:
    old = '''        assert( maxWidth > 0 );
        const int32_t textWidth = subtitleText.width( maxWidth );

        // We add extra 1 to have space for contour.
        _subtitleImage.resize( textWidth + 1, subtitleText.height( textWidth ) + 1 );

        // Draw text and remove all shadow data if it could not be properly applied to video palette.
        // We use the black color with id = 36 so no shadow will be applied to it.
        const uint8_t blackColor = 36;
        _subtitleImage.fill( blackColor );

        // At the left and bottom there is space for contour left by original font shadows, we leave 1 extra pixel from the right and top.
        subtitleText.draw( 0, 1, textWidth, _subtitleImage );
        fheroes2::ReplaceColorIdByTransformId( _subtitleImage, blackColor, 1 );
        // Add black contour to the text.
        fheroes2::Blit( fheroes2::CreateContour( _subtitleImage, blackColor ), _subtitleImage );
'''

    new = '''        assert( maxWidth > 0 );
        const int32_t textWidth = subtitleText.width( maxWidth );
        const bool isKoreanSubtitle = Settings::Get().getGameLanguage() == "ko";

        // Keep the approved large Korean glyphs and their built-in one-pixel
        // shadow, but skip the stock video contour for Korean subtitles.
        _subtitleImage.resize( textWidth + 1, subtitleText.height( textWidth ) + 1 );

        const uint8_t blackColor = 36;
        _subtitleImage.fill( blackColor );
        subtitleText.draw( 0, 1, textWidth, _subtitleImage );
        fheroes2::ReplaceColorIdByTransformId( _subtitleImage, blackColor, 1 );

        if ( !isKoreanSubtitle ) {
            fheroes2::Blit( fheroes2::CreateContour( _subtitleImage, blackColor ), _subtitleImage );
        }
'''

    if old not in source:
        raise SystemExit("Subtitle constructor rendering block was not found.")
    source = source.replace(old, new, 1)

    old_positioning = '''        if ( ( _position.x < 0 ) || ( _position.y < 0 ) ) {
            _position.x = ( fheroes2::Display::DEFAULT_WIDTH - _subtitleImage.width() ) / 2;
            _position.y = fheroes2::Display::DEFAULT_HEIGHT - _subtitleImage.height();
        }
        else {
            _position.x -= _subtitleImage.width() / 2;
        }
'''

    new_positioning = '''        if ( ( _position.x < 0 ) || ( _position.y < 0 ) ) {
            _position.x = ( fheroes2::Display::DEFAULT_WIDTH - _subtitleImage.width() ) / 2;
            _position.y = fheroes2::Display::DEFAULT_HEIGHT - _subtitleImage.height();
        }
        else {
            _position.x -= _subtitleImage.width() / 2;
            if ( isKoreanSubtitle ) {
                // Korean subtitle coordinates use a bottom-center anchor. This
                // keeps the bottom edge fixed and makes 2/3-line subtitles grow
                // upward instead of falling outside the 640x480 video frame.
                _position.y -= _subtitleImage.height();
            }
        }
'''

    if old_positioning not in source:
        raise SystemExit("Subtitle positioning block was not found.")
    return source.replace(old_positioning, new_positioning, 1)


def main() -> None:
    path = Path("src/fheroes2/game/game_video.cpp")
    source = path.read_text(encoding="utf-8")
    source = encode_intro_literals(source)
    source = add_campaign_subtitle_functions(source)
    source = hook_korean_subtitles(source)
    source = patch_korean_subtitle_rendering(source)
    path.write_text(source, encoding="utf-8", newline="\n")
    print(
        f"Prepared approved Korean video rendering and {len(BASE_CAMPAIGN_SUBTITLES)} Succession Wars campaign subtitle tracks."
    )


if __name__ == "__main__":
    main()
