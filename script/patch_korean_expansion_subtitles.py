#!/usr/bin/env python3

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2026                                                    #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

'''Add Korean subtitles for the Price of Loyalty expansion campaigns.

The base subtitle patch owns the renderer and the Succession Wars subtitle
set. This patch only appends expansion narration to the existing Korean video
subtitle dispatcher. Timings are a first-pass alignment to the narration and
ANIM.zip speech windows and can be refined independently after playback QA.
'''

from pathlib import Path

from patch_korean_video_subtitles import function_name_for_video, make_briefing_function


EXPANSION_CAMPAIGN_SUBTITLES: dict[str, list[tuple[str, int, int]]] = {
    # The Price of Loyalty
    "MIXPOL1.SMK": [
        ("북부 변경의 지휘관이자 황제의 오랜 벗이었던 크래거 자작이 반란을 일으켰다.", 500, 7000),
        ("그에게는 제국을 무너뜨릴 병력이 없지만, 우리를 파멸시킬 힘을 지닌 전설의 유물을 찾고 있다.", 7800, 7500),
        ("황제께서는 반역자를 추적해 그의 세력을 꺾고, 그 유물을 제국의 손에 넣으라고 명하셨다.", 15600, 7800),
        ("크래거가 먼저 유물을 손에 넣게 해서는 안 된다.", 23700, 5600),
    ],
    "MIXPOL2.SMK": [
        ("북부 지방의 반란은 진압되었지만 크래거는 다크스케일 산맥으로 달아났다.", 500, 7000),
        ("그가 노리는 전설의 유물은 세 조각으로 나뉘어 있으며, 지금은 첫 번째 조각을 찾고 있다.", 7800, 7600),
        ("그보다 먼저 조각을 찾아야 한다. 첫 조각을 잃으면 나머지 조각의 행방도 알아낼 수 없게 된다.", 15700, 7800),
        ("산맥으로 진군해 크래거의 추종자들을 제거하고 유물을 확보하라.", 23800, 6200),
    ],
    "MIXPOL3.SMK": [
        ("첫 번째 유물 조각을 확보하는 데 성공했다.", 500, 5800),
        ("하지만 크래거는 서쪽 글레이브 섬으로 달아났고, 그곳에는 두 번째 조각이 숨겨져 있다.", 6600, 7200),
        ("그를 따라잡으려면 다크스케일 산맥을 통과해야 한다.", 14100, 6400),
        ("위험하지만 빠른 심연의 길을 택할 수도 있고, 더 멀지만 엘프들의 숲을 통과할 수도 있다.", 20800, 8200),
        ("어느 길로 추격할지는 그대가 결정하라.", 29300, 11500),
    ],
    "MIXPOL4.SMK": [
        ("엘프 군주 일타니스가 제국을 지지하며 목재와 정예 엘프 병력을 지원하겠다고 약속했다.", 500, 7500),
        ("그러나 크래거는 이미 글레이브 섬에서 두 번째 유물 조각을 손에 넣고 북쪽으로 달아났다.", 8300, 7500),
        ("마지막 조각은 눈 덮인 북방에 있다. 그곳으로 가는 길은 거대한 야만인 왕국이 막고 있다.", 16100, 7600),
        ("통로를 장악하고 야만인들을 굴복시켜 크래거보다 먼저 북쪽으로 진군하라.", 24000, 7000),
    ],
    "MIXPOL5.SMK": [
        ("크래거는 두 번째 조각을 가진 채 마지막 유물이 있는 얼어붙은 북쪽으로 향했다.", 500, 7000),
        ("우리도 지체할 수 없다. 마지막 조각을 그가 먼저 손에 넣으면 제국 전체가 위험해진다.", 7800, 7200),
        ("정찰병들은 끝없는 설원 한가운데 기이할 정도로 푸른 땅이 펼쳐져 있다고 보고했다.", 15300, 7200),
        ("그곳을 수색해 안두란의 투구를 찾아라.", 22800, 6200),
    ],
    "MIXPOL6.SMK": [
        ("궁정 대마법사가 크래거의 정신을 들여다보다 충격적인 사실을 알아냈다.", 500, 7000),
        ("이 반란은 크래거 혼자 꾸민 것이 아니었다. 네크로맨서 비밀 결사가 뒤에서 그를 이끌고 있었다.", 7800, 7600),
        ("우리는 반역자를 직접 추적해 끝장을 낼 수도 있고, 모든 사건의 근원인 네크로맨서들을 먼저 칠 수도 있다.", 15700, 8200),
        ("제국의 적을 어떤 순서로 제거할지는 그대가 결정하라.", 24200, 7800),
    ],
    "MIXPOL7.SMK": [
        ("크래거는 네크로맨서들의 마법에 정신을 지배당하고 있었다.", 500, 6500),
        ("안두란의 투구가 그 지배를 깨뜨렸고, 이제 그는 자신이 저지른 일을 똑똑히 기억하고 있다.", 7300, 7200),
        ("황제께서 어느 정도 자비를 베푸실지는 모르지만, 유물의 세 조각은 모두 제국의 손으로 돌아왔다.", 14800, 7600),
        ("이제 진정한 적인 네크로맨서 결사를 완전히 파괴할 차례다.", 22700, 7600),
    ],
    "MIXPOL8.SMK": [
        ("제국은 황제의 지혜와 충성스러운 영웅들의 힘으로 다시 안정을 되찾았다.", 500, 6800),
        ("크래거는 마법에 조종당했다 해도 황제에게 무기를 들었기에 그 죄에 대한 벌을 피할 수 없었다.", 7600, 7200),
        ("그대의 충성과 공로에는 크래거의 모든 영지와 작위가 보상으로 내려졌다.", 15100, 7000),
        ("그것이 바로 충성의 대가였다.", 22400, 7000),
    ],

    # Descendants
    "MIXDES9.SMK": [
        ("아득한 옛날, 이 왕국이 세워지기 전에는 수많은 부족이 이 땅을 차지하려 서로 싸우고 있었다.", 500, 7600),
        ("야만인 자르코나스는 훗날 긴 왕가의 시조가 될 인물이었고, 그에게는 하나의 꿈이 있었다.", 8400, 7600),
        ("모든 부족을 정복하고 하나의 왕국으로 통일하는 것이었다.", 16300, 7600),
    ],
    "MIXDES10.SMK": [
        ("몇 세대가 흐르는 동안 자르코나스가 세운 왕국은 자유롭게 성장하고 번영했다.", 500, 6500),
        ("그러나 동쪽에서 하론데일 왕국이 강성해지면서 두 왕국은 국경을 두고 치열하게 맞서기 시작했다.", 7300, 7200),
        ("그 오랜 경쟁은 결국 전쟁으로 번지게 되었다.", 14800, 5200),
    ],
    "MIXDES11.SMK": [
        ("에타니아의 손자 자르코나스 3세는 임종을 앞두고 쇠약해진 왕국의 앞날을 걱정했다.", 500, 7200),
        ("끊임없는 전쟁으로 왕국은 큰 대가를 치렀고, 다시 번영하려면 먼 친족의 도움이 필요했다.", 8000, 7200),
        ("소문에 따르면 방황하던 아들 조셉과 괴짜 삼촌 이반이 아직 살아 있다고 한다.", 15500, 7200),
        ("왕국을 위해 누구를 찾아 도움을 청할 것인지 결정해야 한다.", 23000, 8200),
    ],
    "MIXDES12.SMK": [
        ("부족 통일 이후 200년째, 왕가가 가장 힘겨운 시기를 보내던 때 남쪽 국경에서 정찰병이 달려왔다.", 500, 7600),
        ("야만인 무리가 여러 전초기지를 점령하고 한 마을을 불태웠다는 보고였다.", 8400, 7000),
        ("당시 여왕 겔드리아는 빼앗긴 도시를 되찾고 국경을 지키기 위해 군대를 남쪽으로 보냈다.", 15700, 8000),
    ],
    "MIXDES13.SMK": [
        ("전쟁과 혼란으로 자르코나스 왕국이 흔들리던 중 치명적인 타격이 닥쳤다.", 500, 7000),
        ("하론데일의 첩자가 배신하면서 아이보리 게이츠 성이 적의 손에 넘어간 것이다.", 7800, 7000),
        ("당시의 통치자 에타니아 겔드리아 2세는 어려운 선택을 해야 했다.", 15100, 7000),
        ("엘프들에게 도움을 청할 것인가, 아니면 곧바로 성을 탈환할 것인가.", 22400, 8500),
    ],
    "MIXDES14.SMK": [
        ("자르코나스 6세는 이제 선조들이 시작한 싸움을 자신의 손으로 끝내기로 결심했다.", 500, 7000),
        ("아이보리 게이츠를 되찾은 뒤 왕가는 다시 강해졌고, 하론데일은 다른 전선에서도 밀리고 있었다.", 7800, 7600),
        ("지금이 오랜 숙적을 무너뜨릴 기회다. 모든 병력을 모아 하론데일의 심장부로 진군하라.", 15700, 7600),
        ("이 전투로 수백 년에 걸친 국경 전쟁을 끝내야 한다.", 23600, 6400),
    ],
    "MIXDES15.SMK": [
        ("하론데일 왕국은 마침내 우리의 지배 아래 들어왔다.", 500, 6000),
        ("그들의 통치자들은 먼 땅으로 달아나거나 전쟁에서 목숨을 잃었다.", 6800, 6500),
        ("자르코나스 6세는 성으로 돌아가 남은 생을 평화롭게 보냈고, 왕가는 오랜 숙원을 이루었다.", 13600, 7200),
    ],

    # Wizard's Isle
    "MIXWIZ16.SMK": [
        ("최근 서쪽 바다에서, 아무것도 없던 곳에 안개로 뒤덮인 군도가 솟아났다는 이야기가 들려온다.", 500, 7200),
        ("그곳은 전설의 장막의 섬이며, 마법의 샘이 숨겨져 있다고 한다.", 8000, 7000),
        ("천 년마다 바다 위로 나타나며 그 힘을 차지한 자는 다음 천 년의 마법을 지배한다고 전해진다.", 15300, 7600),
        ("경쟁자들도 이미 섬의 출현을 알고 정복을 준비하고 있을 것이다. 먼저 움직여라.", 23200, 8000),
    ],
    "MIXWIZ17.SMK": [
        ("섬의 주민들에게서 고대 도서관의 위치를 알아냈다.", 500, 6500),
        ("그곳에는 거의 끝이 없는 마법 지식이 쌓여 있으며, 무엇보다 마법의 샘에 관한 기록이 있을 것이다.", 7300, 7600),
        ("그 지식을 얻는다면 마법의 샘을 찾아 그 힘을 자신의 의지에 묶을 수 있다.", 15200, 7200),
        ("도서관이 있는 크로노스 도시를 점령하라.", 22700, 6500),
    ],
    "MIXWIZ18.SMK": [
        ("크로노스의 도서관을 손에 넣어 마법의 샘이 있는 장소를 알아냈다.", 500, 7000),
        ("하지만 불행히도 경쟁자 한 명이 이미 그곳을 차지했다.", 7800, 6500),
        ("그가 샘의 힘을 완전히 지배하기까지는 시간이 걸리겠지만, 성공한다면 누구도 막을 수 없다.", 14600, 7600),
        ("도서관에서는 주변의 모든 마법을 무효화하는 강력한 유물, 무효화의 구체에 관한 기록도 발견했다.", 22500, 8200),
        ("곧장 마법의 샘을 공격할지, 먼저 그 유물을 찾아 힘을 꺾을지는 그대가 선택하라.", 31000, 12500),
    ],
    "MIXWIZ19.SMK": [
        ("무효화의 구체를 손에 넣었다. 이제 승리는 거의 확실하다.", 500, 6500),
        ("하지만 시간이 얼마 남지 않았다. 경쟁자가 마법의 샘의 힘을 완전히 묶기 전에 움직여야 한다.", 7300, 7200),
        ("서둘러 마법의 성을 점령하고 샘을 차지하라.", 14800, 7500),
    ],
    "MIXWIZ20.SMK": [
        ("모든 경쟁자를 쓰러뜨린 끝에 마법의 샘은 마침내 그대의 것이 되었다.", 500, 6800),
        ("이제 새로운 마법의 시대를 만들고 시간 자체의 비밀을 풀 힘을 손에 넣었다.", 7600, 7000),
        ("그 힘을 현명하게 사용한다면 이 세계는 물론 어느 세계에서도 가장 위대한 마법사가 될 것이다.", 14900, 7800),
    ],

    # Voyage Home
    "MIXVOY21.SMK": [
        ("알베론 경의 명으로 외딴 섬들을 향해 항해하던 중 갑작스럽고 사나운 폭풍이 배를 난파시켰다.", 500, 7200),
        ("나와 소수의 선원만이 이름 모를 섬의 해안까지 간신히 살아서 도착했다.", 8000, 6800),
        ("이제 본토의 고향으로 돌아갈 수 있을 만큼 큰 배를 만들어야 한다.", 15100, 7800),
    ],
    "MIXVOY22.SMK": [
        ("항해는 순조롭다. 이대로라면 보름 안에 고향에 도착할 수 있을 것이다.", 500, 6500),
        ("그런데 우연히 악명 높은 해적 군도를 발견했다. 이들은 오랫동안 알베론 경의 해안을 약탈해 왔다.", 7300, 7200),
        ("여행을 계속하기 전에 해적들의 우두머리 마르틴을 찾아 제거하는 것이 나의 의무다.", 14800, 7200),
    ],
    "MIXVOY23.SMK": [
        ("어젯밤 짙은 안개를 틈타 마침내 고향에 도착했고, 해변에서 사촌을 만났다.", 500, 7000),
        ("내가 없는 동안 내전이 일어났으며, 반란군의 지도자는 다름 아닌 네크로맨서인 내 누이 드라코니아라고 했다.", 7800, 7800),
        ("오랫동안 나를 명예롭게 대해 준 주군 알베론에게 충성을 지켜야 할까?", 15900, 7000),
        ("아니면 더 오래된 피의 인연을 따라 누이의 대의에 함께해야 할까? 이제 선택해야 한다.", 23200, 9800),
    ],
    "MIXVOY24.SMK": [
        ("나는 주군 알베론에게 충성을 지키기로 했고, 함께 반란군을 물리쳤다.", 500, 6500),
        ("누이는 패배해 사슬에 묶인 채 성채의 감옥으로 끌려갔다.", 7300, 6500),
        ("내 선택에는 후회가 없지만, 이 싸움이 결국 가족의 피를 흘리게 했다는 사실만은 오래도록 마음에 남을 것이다.", 14100, 7800),
    ],
    "MIXVOY25.SMK": [
        ("옛 주군 알베론의 성이 불타는 모습이 멀리 보인다.", 500, 6000),
        ("누이와 나는 승리했지만, 내가 그에게 맞서 달려들던 순간 보았던 배신당한 눈빛을 결코 잊지 못할 것이다.", 6800, 7600),
        ("피의 인연을 선택한 대가는 승리 뒤에도 내 기억 속에 남았다.", 14700, 7000),
    ],
}


def add_expansion_subtitles(source: str) -> str:
    sentinel = "getKoreanMIXPOL1Subtitles"
    if sentinel in source:
        return source

    dispatcher_marker = "    std::vector<Video::Subtitle> getKoreanCampaignSubtitles( const std::vector<Video::VideoInfo> & infos )\n"
    dispatcher_pos = source.find(dispatcher_marker)
    if dispatcher_pos < 0:
        raise SystemExit("Korean campaign subtitle dispatcher was not found. Run the base video subtitle patch first.")

    functions = []
    dispatch_cases = []
    for file_name, lines in EXPANSION_CAMPAIGN_SUBTITLES.items():
        function_name = function_name_for_video(file_name)
        functions.append(make_briefing_function(function_name, lines))
        dispatch_cases.append(
            f'''            if ( info.fileName == "{file_name}" ) {{\n                return {function_name}();\n            }}'''
        )

    source = source[:dispatcher_pos] + "".join(functions) + source[dispatcher_pos:]

    dispatcher_pos = source.find(dispatcher_marker)
    return_marker = "\n        return {};\n    }\n\n"
    return_pos = source.find(return_marker, dispatcher_pos)
    if return_pos < 0:
        raise SystemExit("Korean campaign subtitle dispatcher return statement was not found.")

    cases = "\n" + "\n".join(dispatch_cases)
    return source[:return_pos] + cases + source[return_pos:]


def main() -> None:
    path = Path("src/fheroes2/game/game_video.cpp")
    source = path.read_text(encoding="utf-8")
    source = add_expansion_subtitles(source)
    path.write_text(source, encoding="utf-8", newline="\n")
    print(f"Added {len(EXPANSION_CAMPAIGN_SUBTITLES)} Korean expansion campaign subtitle tracks.")


if __name__ == "__main__":
    main()
