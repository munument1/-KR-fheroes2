/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fheroes2
{
    namespace utf8
    {
        constexpr uint32_t replacementCharacter = 0xFFFD;

        struct Character
        {
            uint32_t codePoint{ replacementCharacter };
            size_t byteCount{ 1 };
        };

        inline bool isContinuationByte( const uint8_t value )
        {
            return ( value & 0xC0 ) == 0x80;
        }

        inline Character decode( const std::string_view text, const size_t offset )
        {
            if ( offset >= text.size() ) {
                return { 0, 0 };
            }

            const auto byteAt = [&text]( const size_t index ) { return static_cast<uint8_t>( text[index] ); };
            const uint8_t first = byteAt( offset );

            if ( first < 0x80 ) {
                return { first, 1 };
            }

            if ( first >= 0xC2 && first <= 0xDF && offset + 1 < text.size() ) {
                const uint8_t second = byteAt( offset + 1 );
                if ( isContinuationByte( second ) ) {
                    return { static_cast<uint32_t>( ( first & 0x1F ) << 6 ) | ( second & 0x3F ), 2 };
                }
            }
            else if ( first >= 0xE0 && first <= 0xEF && offset + 2 < text.size() ) {
                const uint8_t second = byteAt( offset + 1 );
                const uint8_t third = byteAt( offset + 2 );
                const bool validContinuation = isContinuationByte( second ) && isContinuationByte( third );
                const bool validRange = ( first != 0xE0 || second >= 0xA0 ) && ( first != 0xED || second < 0xA0 );

                if ( validContinuation && validRange ) {
                    return { static_cast<uint32_t>( ( first & 0x0F ) << 12 ) | static_cast<uint32_t>( ( second & 0x3F ) << 6 ) | ( third & 0x3F ), 3 };
                }
            }
            else if ( first >= 0xF0 && first <= 0xF4 && offset + 3 < text.size() ) {
                const uint8_t second = byteAt( offset + 1 );
                const uint8_t third = byteAt( offset + 2 );
                const uint8_t fourth = byteAt( offset + 3 );
                const bool validContinuation = isContinuationByte( second ) && isContinuationByte( third ) && isContinuationByte( fourth );
                const bool validRange = ( first != 0xF0 || second >= 0x90 ) && ( first != 0xF4 || second < 0x90 );

                if ( validContinuation && validRange ) {
                    return { static_cast<uint32_t>( ( first & 0x07 ) << 18 ) | static_cast<uint32_t>( ( second & 0x3F ) << 12 )
                                 | static_cast<uint32_t>( ( third & 0x3F ) << 6 ) | ( fourth & 0x3F ),
                             4 };
                }
            }

            return { replacementCharacter, 1 };
        }

        inline size_t nextCharacterOffset( const std::string_view text, const size_t offset )
        {
            const Character character = decode( text, offset );
            if ( character.byteCount == 0 ) {
                return text.size();
            }

            return offset + character.byteCount;
        }

        inline size_t previousCharacterOffset( const std::string_view text, size_t offset )
        {
            if ( offset == 0 || text.empty() ) {
                return 0;
            }

            offset = ( offset > text.size() ) ? text.size() : offset;
            --offset;

            while ( offset > 0 && isContinuationByte( static_cast<uint8_t>( text[offset] ) ) ) {
                --offset;
            }

            return offset;
        }

        inline bool isCharacterBoundary( const std::string_view text, const size_t offset )
        {
            return offset == 0 || offset >= text.size() || !isContinuationByte( static_cast<uint8_t>( text[offset] ) );
        }
    }
}
