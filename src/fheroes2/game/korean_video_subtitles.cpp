/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <vector>

#include "game_video.h"
#include "ui_text.h"

namespace
{
    std::vector<Video::Subtitle> getSuccessionWarsIntroSubtitles()
    {
        const fheroes2::FontType subtitleFont = fheroes2::FontType::normalWhite();
        const fheroes2::Point subtitlePosition{ 320, 430 };
        constexpr int32_t subtitleWidth = 600;

        std::vector<Video::Subtitle> subtitles;
        subtitles.reserve( 13 );

        // Initial timings are aligned to the English narration in INTRO.SMK.
        // They are intentionally kept here as data so they can be tuned after
        // an in-game playback check without touching the video decoder.
        subtitles.emplace_back( fheroes2::Text( "모든 일은 3년 전, 늙은 국왕 아이언피스트가 세상을 떠나면서 시작되었다.", subtitleFont ), 6000, 7000, subtitlePosition,
                                subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "왕에게는 두 아들이 있었다.", subtitleFont ), 13500, 2500, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "롤랜드는 선량하고 친절하며 명예로운 인물이었다.", subtitleFont ), 16500, 3500, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "반면 아치발드는... 그리 선량하지 않았다.", subtitleFont ), 20500, 3500, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "전통에 따라 왕위 계승자는 왕실 예언자가 정했지만, 그는 불의의 보트 사고로 목숨을 잃었다.", subtitleFont ), 24500, 8000,
                                subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "뒤를 이은 예언자들의 운도 나았다곤 할 수 없었다. 프레더릭은 창문에서 떨어졌고,", subtitleFont ), 33000, 6000,
                                subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "로버트는 용에게 목숨을 잃었으며,", subtitleFont ), 39500, 2500, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "요한은 식중독으로 죽었다.", subtitleFont ), 42500, 3000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "아치발드는 예언자들을 죽인 범인이 롤랜드라 주장하며 그를 반역자로 선포했다.", subtitleFont ), 46000, 6000,
                                subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "목숨의 위협을 느낀 롤랜드는 궁전을 떠나 서쪽의 자신의 성으로 달아났다.", subtitleFont ), 52500, 6000, subtitlePosition,
                                subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "롤랜드가 사라지자 아치발드는 새 왕실 예언자의 결정에 영향력을 행사할 수 있었다.", subtitleFont ), 59000, 6000,
                                subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "예언자는 아치발드를 후계자로 선택했고, 그는 다음 날 스스로 왕위에 올랐다.", subtitleFont ), 65500, 6000,
                                subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( "그렇게 왕위 계승 전쟁이 시작되었다.", subtitleFont ), 72000, 4500, subtitlePosition, subtitleWidth );

        return subtitles;
    }
}

namespace Video
{
    bool ShowVideo( const std::vector<VideoInfo> & infos )
    {
        if ( infos.size() == 1 && infos.front().fileName == "INTRO.SMK" ) {
            return ShowVideo( infos, getSuccessionWarsIntroSubtitles(), false );
        }

        return ShowVideo( infos, {}, false );
    }
}
