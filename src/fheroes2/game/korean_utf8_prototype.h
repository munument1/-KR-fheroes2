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

#include <array>
#include <cstdint>
#include <string_view>

#include "image.h"
#include "utf8.h"

namespace fheroes2
{
    namespace koreanUtf8Prototype
    {
        using Glyph = std::array<uint16_t, 12>;

        inline const Glyph * getGlyph( const uint32_t codePoint )
        {
            // Temporary 12x12 monochrome prototype glyphs. They cover only the phrase
            // used by drawMainMenuTestText(). Full Korean support will replace these
            // with a real Unicode glyph provider.
            static constexpr Glyph han{ 0x3C8, 0x008, 0xFF8, 0x008, 0x3CE, 0x268, 0x3C8, 0x008, 0x200, 0x200, 0x3FC, 0x000 };
            static constexpr Glyph guk{ 0x3F8, 0x008, 0x008, 0x008, 0xFFE, 0x040, 0x040, 0x3F8, 0x008, 0x008, 0x008, 0x000 };
            static constexpr Glyph eo{ 0x3C4, 0x644, 0x464, 0x43C, 0x424, 0x464, 0x644, 0x384, 0x004, 0x004, 0x004, 0x000 };
            static constexpr Glyph te{ 0x794, 0x414, 0x414, 0x7F4, 0x414, 0x414, 0x414, 0x7D4, 0x014, 0x014, 0x014, 0x000 };
            static constexpr Glyph seu{ 0x040, 0x040, 0x0E0, 0x1B0, 0x318, 0x604, 0x000, 0x000, 0xFFE, 0x000, 0x000, 0x000 };
            static constexpr Glyph teu{ 0x3F8, 0x200, 0x3F8, 0x200, 0x200, 0x3FC, 0x000, 0x000, 0xFFE, 0x000, 0x000, 0x000 };
            static constexpr Glyph replacement{ 0x3F0, 0x408, 0x008, 0x010, 0x020, 0x040, 0x040, 0x000, 0x040, 0x000, 0x000, 0x000 };

            switch ( codePoint ) {
            case 0xD55C:
                return &han;
            case 0xAD6D:
                return &guk;
            case 0xC5B4:
                return &eo;
            case 0xD14C:
                return &te;
            case 0xC2A4:
                return &seu;
            case 0xD2B8:
                return &teu;
            case utf8::replacementCharacter:
                return &replacement;
            default:
                return nullptr;
            }
        }

        inline void drawGlyph( Image & output, const int32_t x, const int32_t y, const Glyph & glyph, const uint8_t color )
        {
            for ( int32_t row = 0; row < static_cast<int32_t>( glyph.size() ); ++row ) {
                for ( int32_t column = 0; column < 12; ++column ) {
                    if ( ( glyph[row] & ( 1U << ( 11 - column ) ) ) == 0 ) {
                        continue;
                    }

                    const int32_t pixelX = x + column;
                    const int32_t pixelY = y + row;
                    if ( pixelX >= 0 && pixelX < output.width() && pixelY >= 0 && pixelY < output.height() ) {
                        SetPixel( output, pixelX, pixelY, color );
                    }
                }
            }
        }

        inline void drawText( Image & output, const int32_t x, const int32_t y, const std::string_view text )
        {
            const uint8_t foreground = GetColorId( 245, 245, 245 );
            const uint8_t shadow = GetColorId( 24, 24, 24 );

            int32_t offsetX = x;
            size_t offset = 0;

            while ( offset < text.size() ) {
                const utf8::Character character = utf8::decode( text, offset );
                if ( character.byteCount == 0 ) {
                    break;
                }
                offset += character.byteCount;

                if ( character.codePoint == ' ' ) {
                    offsetX += 7;
                    continue;
                }

                const Glyph * glyph = getGlyph( character.codePoint );
                if ( glyph == nullptr ) {
                    glyph = getGlyph( utf8::replacementCharacter );
                }

                drawGlyph( output, offsetX + 1, y + 1, *glyph, shadow );
                drawGlyph( output, offsetX, y, *glyph, foreground );
                offsetX += 13;
            }
        }

        inline void drawMainMenuTestText( Image & output, const int32_t x, const int32_t y )
        {
            // Explicit UTF-8 byte sequences keep this prototype independent of the compiler's source-code page.
            constexpr std::string_view testText = "\xED\x95\x9C" "\xEA\xB5\xAD" "\xEC\x96\xB4" " " "\xED\x85\x8C" "\xEC\x8A\xA4" "\xED\x8A\xB8";
            drawText( output, x, y, testText );
        }
    }
}
