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
        // Visual-only prototype. These are small rasterized glyph subsets, not
        // embedded TTF files. Body glyphs: Galmuri11 11 px.
        // Decorative glyphs: Solmoe KimDaeGeon Medium 22 px.
        enum class FontStyle : uint8_t
        {
            body,
            decorative
        };

        struct Glyph
        {
            std::array<uint32_t, 24> rows;
            uint8_t width;
            uint8_t height;
            uint8_t advance;
        };

        static constexpr Glyph body_AC08{ { 0x00000000U, 0x0000011FU, 0x00000100U, 0x00000300U, 0x00000110U, 0x0000010FU, 0x000001FEU, 0x00000100U, 0x000001FEU, 0x00000002U, 0x000001FEU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph body_AE00{ { 0x00000000U, 0x000001FEU, 0x00000100U, 0x00000100U, 0x00000100U, 0x000003FFU, 0x00000000U, 0x00000100U, 0x000001FEU, 0x00000002U, 0x000001FEU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph body_AE30{ { 0x00000000U, 0x0000013FU, 0x00000120U, 0x00000120U, 0x00000120U, 0x00000110U, 0x00000110U, 0x00000108U, 0x00000104U, 0x00000103U, 0x00000100U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph body_AF34{ { 0x00000000U, 0x000001DEU, 0x00000110U, 0x00000110U, 0x00000020U, 0x000003FFU, 0x00000000U, 0x00000100U, 0x000001FEU, 0x00000002U, 0x000001FEU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph body_B9AC{ { 0x00000000U, 0x0000013FU, 0x00000120U, 0x00000120U, 0x00000120U, 0x0000013FU, 0x00000101U, 0x00000101U, 0x00000101U, 0x0000013FU, 0x00000100U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph body_BB34{ { 0x00000000U, 0x000001FEU, 0x00000102U, 0x00000102U, 0x000001FEU, 0x00000000U, 0x000003FFU, 0x00000020U, 0x00000020U, 0x00000020U, 0x00000020U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph body_BCF8{ { 0x00000000U, 0x00000102U, 0x000001FEU, 0x00000102U, 0x00000030U, 0x000003FFU, 0x00000000U, 0x00000002U, 0x00000002U, 0x00000002U, 0x000001FEU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 11, 13, 11 };
        static constexpr Glyph decor_AC74{ { 0x00000000U, 0x0000C000U, 0x00018000U, 0x00038000U, 0x00038600U, 0x000387F8U, 0x000387F0U, 0x0003C700U, 0x0003FF00U, 0x0003B980U, 0x000381C0U, 0x000380E0U, 0x00018070U, 0x0000403CU, 0x00003000U, 0x00000800U, 0x00000200U, 0x00038180U, 0x0003C180U, 0x0001FF80U, 0x00007E00U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };
        static constexpr Glyph decor_AE00{ { 0x00000000U, 0x00000000U, 0x00003800U, 0x0000FFC0U, 0x0000E380U, 0x00006000U, 0x00006000U, 0x00006000U, 0x00000000U, 0x0007C000U, 0x0007FFC2U, 0x00007FFEU, 0x0000003CU, 0x00007F00U, 0x00007FC0U, 0x00001E40U, 0x00007F80U, 0x00007180U, 0x00001E00U, 0x00007100U, 0x00003F00U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };
        static constexpr Glyph decor_AE40{ { 0x00000000U, 0x0000C000U, 0x0001C000U, 0x00018000U, 0x00018700U, 0x000187F8U, 0x00018770U, 0x00018300U, 0x00018380U, 0x000181C0U, 0x000180C0U, 0x0000C0E0U, 0x00000078U, 0x00000000U, 0x0001F000U, 0x0001CF00U, 0x0000C700U, 0x0000C600U, 0x0000C600U, 0x0000E600U, 0x0001E600U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };
        static constexpr Glyph decor_AF34{ { 0x00000000U, 0x00000000U, 0x0000F000U, 0x0000FDF8U, 0x0000C1F8U, 0x0000C180U, 0x0000C180U, 0x00004E80U, 0x00000C40U, 0x00038C00U, 0x0007FE00U, 0x0002FFFEU, 0x0000007CU, 0x00007C00U, 0x00007FC0U, 0x00003CC0U, 0x00007F00U, 0x00007380U, 0x00001E00U, 0x00007100U, 0x00003F00U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };
        static constexpr Glyph decor_B300{ { 0x00000000U, 0x0000C000U, 0x00038000U, 0x00038000U, 0x00031C00U, 0x00031800U, 0x000339C0U, 0x000339FEU, 0x0003387EU, 0x00033834U, 0x0003B810U, 0x0003F818U, 0x0003FE18U, 0x00033B98U, 0x000301F8U, 0x00030078U, 0x00030000U, 0x00030000U, 0x00010000U, 0x00010000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };
        static constexpr Glyph decor_C2DD{ { 0x00000000U, 0x0000C000U, 0x0001C000U, 0x00018000U, 0x000180C0U, 0x000180C0U, 0x000180C0U, 0x000180C0U, 0x00018060U, 0x000181F0U, 0x00018738U, 0x0000C61CU, 0x00002004U, 0x00007000U, 0x0001FC00U, 0x0001DF80U, 0x0001C380U, 0x0001C000U, 0x0000C000U, 0x0000C000U, 0x0000C000U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };
        static constexpr Glyph decor_C7A5{ { 0x00000000U, 0x00003800U, 0x00007000U, 0x00007000U, 0x000061C0U, 0x000061FCU, 0x000061D8U, 0x000060C0U, 0x000660E0U, 0x000761E0U, 0x0003E730U, 0x0000E718U, 0x0000220EU, 0x00001800U, 0x00000400U, 0x00003300U, 0x00006180U, 0x00006180U, 0x00006180U, 0x00003380U, 0x00001F00U, 0x00000000U, 0x00000000U, 0x00000000U }, 20, 24, 20 };

        inline const Glyph * getGlyph( const uint32_t codePoint, const FontStyle style )
        {
            if ( style == FontStyle::body ) {
                switch ( codePoint ) {
            case 0xAC08: return &body_AC08;
            case 0xAE00: return &body_AE00;
            case 0xAE30: return &body_AE30;
            case 0xAF34: return &body_AF34;
            case 0xB9AC: return &body_B9AC;
            case 0xBB34: return &body_BB34;
            case 0xBCF8: return &body_BCF8;
                default: return nullptr;
                }
            }

            switch ( codePoint ) {
            case 0xAC74: return &decor_AC74;
            case 0xAE00: return &decor_AE00;
            case 0xAE40: return &decor_AE40;
            case 0xAF34: return &decor_AF34;
            case 0xB300: return &decor_B300;
            case 0xC2DD: return &decor_C2DD;
            case 0xC7A5: return &decor_C7A5;
            default: return nullptr;
            }
        }

        inline void drawGlyph( Image & output, const int32_t x, const int32_t y, const Glyph & glyph, const uint8_t color )
        {
            for ( int32_t row = 0; row < glyph.height; ++row ) {
                for ( int32_t column = 0; column < glyph.width; ++column ) {
                    if ( ( glyph.rows[row] & ( 1U << column ) ) == 0 ) {
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

        inline void drawText( Image & output, const int32_t x, const int32_t y, const std::string_view text, const FontStyle style )
        {
            const uint8_t foreground = style == FontStyle::body ? GetColorId( 245, 245, 245 ) : GetColorId( 245, 235, 180 );
            const uint8_t shadow = GetColorId( 24, 24, 24 );
            const int32_t spaceAdvance = style == FontStyle::body ? 6 : 8;

            int32_t offsetX = x;
            size_t offset = 0;

            while ( offset < text.size() ) {
                const utf8::Character character = utf8::decode( text, offset );
                if ( character.byteCount == 0 ) {
                    break;
                }
                offset += character.byteCount;

                if ( character.codePoint == ' ' ) {
                    offsetX += spaceAdvance;
                    continue;
                }

                const Glyph * glyph = getGlyph( character.codePoint, style );
                if ( glyph == nullptr ) {
                    offsetX += style == FontStyle::body ? 11 : 20;
                    continue;
                }

                drawGlyph( output, offsetX + 1, y + 1, *glyph, shadow );
                drawGlyph( output, offsetX, y, *glyph, foreground );
                offsetX += glyph->advance;
            }
        }

        inline void drawMainMenuTestText( Image & output, const int32_t x, const int32_t y )
        {
            // Explicit UTF-8 byte sequences keep this independent of the compiler source-code page.
            constexpr std::string_view decorativeText = "\\xEA" "\\xB9" "\\x80" "\\xEB" "\\x8C" "\\x80" "\\xEA" "\\xB1" "\\xB4" " " "\\xEC" "\\x9E" "\\xA5" "\\xEC" "\\x8B" "\\x9D" " " "\\xEA" "\\xB8" "\\x80" "\\xEA" "\\xBC" "\\xB4";
            constexpr std::string_view bodyText = "\\xEA" "\\xB0" "\\x88" "\\xEB" "\\xAC" "\\xB4" "\\xEB" "\\xA6" "\\xAC" " " "\\xEA" "\\xB8" "\\xB0" "\\xEB" "\\xB3" "\\xB8" " " "\\xEA" "\\xB8" "\\x80" "\\xEA" "\\xBC" "\\xB4";

            drawText( output, x, y - 34, decorativeText, FontStyle::decorative );
            drawText( output, x, y, bodyText, FontStyle::body );
        }
    }
}
