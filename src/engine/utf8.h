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

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fheroes2::utf8
{
    constexpr uint32_t replacementCharacter = 0xFFFDU;

    struct Character
    {
        uint32_t codePoint{ replacementCharacter };
        size_t byteCount{ 1 };
        bool valid{ false };
    };

    constexpr bool isContinuationByte( const uint8_t value )
    {
        return ( value & 0xC0U ) == 0x80U;
    }

    constexpr Character decode( const std::string_view text, const size_t offset )
    {
        if ( offset >= text.size() ) {
            return { 0, 0, false };
        }

        const auto byteAt = [&text]( const size_t index ) { return static_cast<uint8_t>( text[index] ); };
        const uint8_t first = byteAt( offset );

        if ( first < 0x80U ) {
            return { first, 1, true };
        }

        if ( first >= 0xC2U && first <= 0xDFU && offset + 1 < text.size() ) {
            const uint8_t second = byteAt( offset + 1 );
            if ( isContinuationByte( second ) ) {
                return { static_cast<uint32_t>( ( first & 0x1FU ) << 6 ) | ( second & 0x3FU ), 2, true };
            }
        }
        else if ( first >= 0xE0U && first <= 0xEFU && offset + 2 < text.size() ) {
            const uint8_t second = byteAt( offset + 1 );
            const uint8_t third = byteAt( offset + 2 );
            const bool validContinuation = isContinuationByte( second ) && isContinuationByte( third );
            const bool validRange = ( first != 0xE0U || second >= 0xA0U ) && ( first != 0xEDU || second < 0xA0U );

            if ( validContinuation && validRange ) {
                return { static_cast<uint32_t>( ( first & 0x0FU ) << 12 ) | static_cast<uint32_t>( ( second & 0x3FU ) << 6 ) | ( third & 0x3FU ), 3,
                         true };
            }
        }
        else if ( first >= 0xF0U && first <= 0xF4U && offset + 3 < text.size() ) {
            const uint8_t second = byteAt( offset + 1 );
            const uint8_t third = byteAt( offset + 2 );
            const uint8_t fourth = byteAt( offset + 3 );
            const bool validContinuation = isContinuationByte( second ) && isContinuationByte( third ) && isContinuationByte( fourth );
            const bool validRange = ( first != 0xF0U || second >= 0x90U ) && ( first != 0xF4U || second < 0x90U );

            if ( validContinuation && validRange ) {
                return { static_cast<uint32_t>( ( first & 0x07U ) << 18 ) | static_cast<uint32_t>( ( second & 0x3FU ) << 12 )
                             | static_cast<uint32_t>( ( third & 0x3FU ) << 6 ) | ( fourth & 0x3FU ),
                         4, true };
            }
        }

        return { replacementCharacter, 1, false };
    }

    constexpr size_t nextCharacterOffset( const std::string_view text, const size_t offset )
    {
        const Character character = decode( text, offset );
        return character.byteCount == 0 ? text.size() : offset + character.byteCount;
    }

    constexpr size_t previousCharacterOffset( const std::string_view text, size_t offset )
    {
        if ( offset == 0 || text.empty() ) {
            return 0;
        }

        offset = offset > text.size() ? text.size() : offset;
        --offset;

        while ( offset > 0 && isContinuationByte( static_cast<uint8_t>( text[offset] ) ) ) {
            --offset;
        }

        return offset;
    }

    constexpr bool isCharacterBoundary( const std::string_view text, const size_t offset )
    {
        return offset == 0 || offset >= text.size() || !isContinuationByte( static_cast<uint8_t>( text[offset] ) );
    }

    struct StreamResult
    {
        uint32_t codePoint{ replacementCharacter };
        uint8_t byteCount{ 1 };
        bool started{ false };
        bool complete{ true };
        bool valid{ false };
    };

    class StreamDecoder final
    {
    public:
        constexpr void reset()
        {
            _codePoint = 0;
            _minimumCodePoint = 0;
            _expectedByteCount = 0;
            _receivedByteCount = 0;
        }

        constexpr StreamResult consume( const uint8_t value )
        {
            if ( _expectedByteCount == 0 ) {
                if ( value < 0x80U ) {
                    return { value, 1, true, true, true };
                }

                if ( value >= 0xC2U && value <= 0xDFU ) {
                    _codePoint = value & 0x1FU;
                    _minimumCodePoint = 0x80U;
                    _expectedByteCount = 2;
                }
                else if ( value >= 0xE0U && value <= 0xEFU ) {
                    _codePoint = value & 0x0FU;
                    _minimumCodePoint = 0x800U;
                    _expectedByteCount = 3;
                }
                else if ( value >= 0xF0U && value <= 0xF4U ) {
                    _codePoint = value & 0x07U;
                    _minimumCodePoint = 0x10000U;
                    _expectedByteCount = 4;
                }
                else {
                    return { replacementCharacter, 1, true, true, false };
                }

                _receivedByteCount = 1;
                return { 0, _expectedByteCount, true, false, true };
            }

            if ( !isContinuationByte( value ) ) {
                reset();
                return { replacementCharacter, 1, false, true, false };
            }

            _codePoint = ( _codePoint << 6 ) | ( value & 0x3FU );
            ++_receivedByteCount;

            if ( _receivedByteCount < _expectedByteCount ) {
                return { 0, _expectedByteCount, false, false, true };
            }

            const uint32_t codePoint = _codePoint;
            const uint8_t byteCount = _expectedByteCount;
            const uint32_t minimumCodePoint = _minimumCodePoint;
            reset();

            const bool valid = codePoint >= minimumCodePoint && codePoint <= 0x10FFFFU && !( codePoint >= 0xD800U && codePoint <= 0xDFFFU );
            return { valid ? codePoint : replacementCharacter, byteCount, false, true, valid };
        }

        constexpr bool hasPendingSequence() const
        {
            return _expectedByteCount != 0;
        }

    private:
        uint32_t _codePoint{ 0 };
        uint32_t _minimumCodePoint{ 0 };
        uint8_t _expectedByteCount{ 0 };
        uint8_t _receivedByteCount{ 0 };
    };
}
