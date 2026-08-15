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

#include "ui_text.h"
#include "utf8.h"

namespace fheroes2
{
    // A compatibility bridge between the existing byte-oriented text layout
    // code and a UTF-8 large-alphabet glyph provider. The first byte of a
    // multi-byte character consumes one fixed-width cell, intermediate bytes
    // consume zero width and the final byte draws the completed glyph back
    // into the already-reserved cell.
    //
    // This keeps legacy code-page handling independent and unchanged. It is a
    // transitional path, not a full Unicode shaping engine.
    class LargeAlphabetCharHandler final
    {
    public:
        explicit LargeAlphabetCharHandler( FontType fontType );

        bool isAvailable( uint8_t value ) const;

        const Sprite & getSprite( uint8_t value ) const;

        int32_t getWidth( uint8_t value ) const;

        int32_t getSpaceCharWidth() const
        {
            return _legacyHandler.getSpaceCharWidth();
        }

        void reset() const;

    private:
        FontCharHandler _legacyHandler;
        FontType _fontType;
        mutable utf8::StreamDecoder _spriteDecoder;
        mutable utf8::StreamDecoder _widthDecoder;
    };
}
