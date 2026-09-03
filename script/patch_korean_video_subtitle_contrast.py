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

'''Force Korean SMK subtitles to a stable white + black-shadow appearance.

SMK playback replaces the engine palette on every video frame. A subtitle
sprite created with the normal game palette can therefore inherit unrelated
video colors. This patch keeps a clean text mask and renders that mask through
the active SMK palette each frame.
'''

from __future__ import annotations

import re
from pathlib import Path


def patch_subtitle_mask(source: str) -> str:
    old = '''        // Keep the approved large Korean glyphs and their built-in one-pixel
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

    new = '''        // Korean video subtitles are recolored against the active SMK palette
        // at render time. Keep only the source text mask here and reconstruct a
        // crisp one-pixel black shadow in Subtitle::render().
        _subtitleImage.resize( textWidth + 1, subtitleText.height( textWidth ) + 1 );

        if ( isKoreanSubtitle ) {
            _subtitleImage.reset();
            subtitleText.draw( 0, 0, textWidth, _subtitleImage );
        }
        else {
            const uint8_t blackColor = 36;
            _subtitleImage.fill( blackColor );
            subtitleText.draw( 0, 1, textWidth, _subtitleImage );
            fheroes2::ReplaceColorIdByTransformId( _subtitleImage, blackColor, 1 );
            fheroes2::Blit( fheroes2::CreateContour( _subtitleImage, blackColor ), _subtitleImage );
        }
'''

    if source.count(old) != 1:
        raise SystemExit('Expected exactly one Korean subtitle constructor block.')

    return source.replace(old, new, 1)


def patch_render_calls(source: str) -> str:
    pattern = re.compile(r'(?m)^(\s*)subtitle\.render\( display, videoRoi \);$')

    def replacement(match: re.Match[str]) -> str:
        indent = match.group(1)
        return (
            f'{indent}if ( Settings::Get().getGameLanguage() == "ko" ) {{\n'
            f'{indent}    subtitle.render( display, videoRoi, prevPalette );\n'
            f'{indent}}}\n'
            f'{indent}else {{\n'
            f'{indent}    subtitle.render( display, videoRoi );\n'
            f'{indent}}}'
        )

    source, count = pattern.subn(replacement, source)
    if count != 2:
        raise SystemExit(f'Expected exactly two subtitle render calls, found {count}.')

    return source


def main() -> None:
    path = Path('src/fheroes2/game/game_video.cpp')
    source = path.read_text(encoding='utf-8')
    source = patch_subtitle_mask(source)
    source = patch_render_calls(source)
    path.write_text(source, encoding='utf-8', newline='\n')
    print('Patched Korean video subtitles to use active-palette white with a black 1px shadow.')


if __name__ == '__main__':
    main()
