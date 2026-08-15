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

#include "ui_large_alphabet_text.h"

#include "ui_large_alphabet.h"

namespace
{
    constexpr uint8_t asciiLimit = 0x80U;
    constexpr uint8_t fallbackCharacter = '?';
}

namespace fheroes2
{
    LargeAlphabetCharHandler::LargeAlphabetCharHandler( FontType fontType )
        : _legacyHandler( fontType )
        , _fontType( fontType )
    {
        // Do nothing.
    }

    bool LargeAlphabetCharHandler::isAvailable( const uint8_t value ) const
    {
        return value < asciiLimit ? _legacyHandler.isAvailable( value ) : true;
    }

    const Sprite & LargeAlphabetCharHandler::getSprite( const uint8_t value ) const
    {
        if ( !_spriteDecoder.hasPendingSequence() && value < asciiLimit ) {
            return _legacyHandler.getSprite( value );
        }

        const utf8::StreamResult result = _spriteDecoder.consume( value );

        if ( result.started && !result.complete ) {
            return largeAlphabet::getAdvanceSprite( _fontType );
        }

        if ( !result.complete ) {
            return largeAlphabet::getZeroAdvanceSprite();
        }

        if ( result.valid && largeAlphabet::isGlyphAvailable( result.codePoint ) ) {
            return largeAlphabet::getGlyphSprite( result.codePoint, _fontType );
        }

        if ( result.started ) {
            return _legacyHandler.getSprite( fallbackCharacter );
        }

        // A malformed continuation byte belongs to a sequence whose lead byte
        // has already consumed its cell. Keep the cell width stable.
        return largeAlphabet::getZeroAdvanceSprite();
    }

    int32_t LargeAlphabetCharHandler::getWidth( const uint8_t value ) const
    {
        if ( !_widthDecoder.hasPendingSequence() && value < asciiLimit ) {
            return _legacyHandler.getWidth( value );
        }

        const utf8::StreamResult result = _widthDecoder.consume( value );

        if ( result.started && !result.complete ) {
            return largeAlphabet::getAdvance( _fontType );
        }

        if ( !result.complete ) {
            return 0;
        }

        if ( result.started && !result.valid ) {
            return _legacyHandler.getWidth( fallbackCharacter );
        }

        // Valid UTF-8 sequences have already consumed their full advance on
        // the lead byte. The final byte only completes the glyph lookup.
        return 0;
    }

    void LargeAlphabetCharHandler::reset() const
    {
        _spriteDecoder.reset();
        _widthDecoder.reset();
    }
}
