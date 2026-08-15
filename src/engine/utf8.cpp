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

#include "utf8.h"

namespace
{
    constexpr std::string_view hangulHan{ "\xED\x95\x9C", 3 };
    constexpr std::string_view cjkMiddle{ "\xE4\xB8\xAD", 3 };
    constexpr std::string_view grinningFace{ "\xF0\x9F\x98\x80", 4 };
    constexpr std::string_view invalidSurrogate{ "\xED\xA0\x80", 3 };

    static_assert( fheroes2::utf8::decode( "A", 0 ).codePoint == 0x41U );
    static_assert( fheroes2::utf8::decode( hangulHan, 0 ).codePoint == 0xD55CU );
    static_assert( fheroes2::utf8::decode( cjkMiddle, 0 ).codePoint == 0x4E2DU );
    static_assert( fheroes2::utf8::decode( grinningFace, 0 ).codePoint == 0x1F600U );
    static_assert( !fheroes2::utf8::decode( invalidSurrogate, 0 ).valid );

    constexpr bool verifyStreamingDecoder()
    {
        fheroes2::utf8::StreamDecoder decoder;

        const fheroes2::utf8::StreamResult first = decoder.consume( 0xEDU );
        if ( !first.started || first.complete || first.byteCount != 3 || !first.valid ) {
            return false;
        }

        const fheroes2::utf8::StreamResult second = decoder.consume( 0x95U );
        if ( second.started || second.complete || second.byteCount != 3 || !second.valid ) {
            return false;
        }

        const fheroes2::utf8::StreamResult third = decoder.consume( 0x9CU );
        return !third.started && third.complete && third.valid && third.byteCount == 3 && third.codePoint == 0xD55CU && !decoder.hasPendingSequence();
    }

    constexpr bool verifyInvalidStreamingSequence()
    {
        fheroes2::utf8::StreamDecoder decoder;
        decoder.consume( 0xF0U );
        decoder.consume( 0x80U );
        decoder.consume( 0x80U );
        const fheroes2::utf8::StreamResult result = decoder.consume( 0x80U );

        return result.complete && !result.valid && result.codePoint == fheroes2::utf8::replacementCharacter && !decoder.hasPendingSequence();
    }

    static_assert( verifyStreamingDecoder() );
    static_assert( verifyInvalidStreamingSequence() );
}
