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

#pragma once

#include <cstdint>

namespace fheroes2
{
    struct FontType;
    class Sprite;

    namespace largeAlphabet
    {
        // This prototype provider intentionally uses fixed-cell glyphs. It is
        // meant for large left-to-right alphabets such as Hangul and CJK while
        // keeping the existing single-byte code-page path untouched.
        bool isGlyphAvailable( uint32_t codePoint );

        int32_t getAdvance( const FontType & fontType );

        // The advance sprite is transparent and consumes one complete glyph cell.
        const Sprite & getAdvanceSprite( const FontType & fontType );

        // The glyph sprite is positioned one cell to the left so a completed
        // UTF-8 sequence can draw into the cell already consumed by its lead byte.
        const Sprite & getGlyphSprite( uint32_t codePoint, const FontType & fontType );

        const Sprite & getZeroAdvanceSprite();
    }
}
