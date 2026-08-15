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

#include "ui_large_alphabet.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <utility>

#include "image.h"
#include "ui_text.h"

namespace
{
    constexpr int32_t baseGlyphWidth = 11;
    constexpr int32_t baseGlyphHeight = 13;

    struct BitmapGlyph
    {
        uint32_t codePoint;
        std::array<uint32_t, baseGlyphHeight> rows;
    };

    // A deliberately small bootstrap set. The provider API is generic; the
    // complete language-specific glyph data will be generated separately once
    // the UTF-8 text path is proven to work without changing legacy code pages.
    constexpr std::array<BitmapGlyph, 7> prototypeGlyphs = { {
        { 0xAC08U, { 0x000U, 0x11FU, 0x100U, 0x300U, 0x110U, 0x10FU, 0x1FEU, 0x100U, 0x1FEU, 0x002U, 0x1FEU, 0x000U, 0x000U } },
        { 0xAE00U, { 0x000U, 0x1FEU, 0x100U, 0x100U, 0x100U, 0x3FFU, 0x000U, 0x100U, 0x1FEU, 0x002U, 0x1FEU, 0x000U, 0x000U } },
        { 0xAE30U, { 0x000U, 0x13FU, 0x120U, 0x120U, 0x120U, 0x110U, 0x110U, 0x108U, 0x104U, 0x103U, 0x100U, 0x000U, 0x000U } },
        { 0xAF34U, { 0x000U, 0x1DEU, 0x110U, 0x110U, 0x020U, 0x3FFU, 0x000U, 0x100U, 0x1FEU, 0x002U, 0x1FEU, 0x000U, 0x000U } },
        { 0xB9ACU, { 0x000U, 0x13FU, 0x120U, 0x120U, 0x120U, 0x13FU, 0x101U, 0x101U, 0x101U, 0x13FU, 0x100U, 0x000U, 0x000U } },
        { 0xBB34U, { 0x000U, 0x1FEU, 0x102U, 0x102U, 0x1FEU, 0x000U, 0x3FFU, 0x020U, 0x020U, 0x020U, 0x020U, 0x000U, 0x000U } },
        { 0xBCF8U, { 0x000U, 0x102U, 0x1FEU, 0x102U, 0x030U, 0x3FFU, 0x000U, 0x002U, 0x002U, 0x002U, 0x1FEU, 0x000U, 0x000U } },
    } };

    const BitmapGlyph * findGlyph( const uint32_t codePoint )
    {
        for ( const BitmapGlyph & glyph : prototypeGlyphs ) {
            if ( glyph.codePoint == codePoint ) {
                return &glyph;
            }
        }

        return nullptr;
    }

    int32_t getScale( const fheroes2::FontType & fontType )
    {
        return fontType.size == fheroes2::FontSize::LARGE ? 2 : 1;
    }

    uint8_t getForegroundColor( const fheroes2::FontType & fontType )
    {
        switch ( fontType.color ) {
        case fheroes2::FontColor::WHITE:
            return fontType.size == fheroes2::FontSize::BUTTON_PRESSED ? fheroes2::GetColorId( 215, 215, 215 ) : fheroes2::GetColorId( 245, 245, 245 );
        case fheroes2::FontColor::GRAY:
            return fontType.size == fheroes2::FontSize::BUTTON_PRESSED ? fheroes2::GetColorId( 125, 125, 125 ) : fheroes2::GetColorId( 170, 170, 170 );
        case fheroes2::FontColor::YELLOW:
            return fheroes2::GetColorId( 245, 220, 80 );
        case fheroes2::FontColor::GOLDEN_GRADIENT:
            return fheroes2::GetColorId( 245, 214, 128 );
        case fheroes2::FontColor::SILVER_GRADIENT:
            return fheroes2::GetColorId( 210, 215, 225 );
        default:
            assert( 0 );
            return fheroes2::GetColorId( 245, 245, 245 );
        }
    }

    uint64_t makeCacheKey( const uint32_t codePoint, const fheroes2::FontType & fontType )
    {
        return static_cast<uint64_t>( codePoint ) | ( static_cast<uint64_t>( fontType.size ) << 32 ) | ( static_cast<uint64_t>( fontType.color ) << 40 );
    }
}

namespace fheroes2::largeAlphabet
{
    bool isGlyphAvailable( const uint32_t codePoint )
    {
        return findGlyph( codePoint ) != nullptr;
    }

    int32_t getAdvance( const FontType & fontType )
    {
        return baseGlyphWidth * getScale( fontType );
    }

    const Sprite & getAdvanceSprite( const FontType & fontType )
    {
        static std::map<int32_t, Sprite> cache;

        const int32_t advance = getAdvance( fontType );
        const bool isButtonFont = fontType.size == FontSize::BUTTON_RELEASED || fontType.size == FontSize::BUTTON_PRESSED;
        const int32_t key = advance * 2 + ( isButtonFont ? 1 : 0 );
        const int32_t spriteX = isButtonFont ? -1 : 0;
        const int32_t spriteWidth = isButtonFont ? advance + 1 : advance;

        auto [iter, inserted] = cache.try_emplace( key, spriteWidth, 1, spriteX, 0 );
        if ( inserted ) {
            iter->second.reset();
        }

        return iter->second;
    }

    const Sprite & getZeroAdvanceSprite()
    {
        static const Sprite sprite = []() {
            Sprite output( 1, 1, -1, 0 );
            output.reset();
            return output;
        }();

        return sprite;
    }

    const Sprite & getGlyphSprite( const uint32_t codePoint, const FontType & fontType )
    {
        const BitmapGlyph * glyphData = findGlyph( codePoint );
        if ( glyphData == nullptr ) {
            return getZeroAdvanceSprite();
        }

        static std::map<uint64_t, Sprite> cache;
        const uint64_t key = makeCacheKey( codePoint, fontType );
        if ( const auto iter = cache.find( key ); iter != cache.end() ) {
            return iter->second;
        }

        const int32_t scale = getScale( fontType );
        const int32_t advance = getAdvance( fontType );
        const int32_t width = baseGlyphWidth * scale;
        const int32_t height = baseGlyphHeight * scale;

        Sprite glyph( width, height, -advance, 0 );
        glyph.reset();

        const uint8_t foreground = getForegroundColor( fontType );
        const uint8_t shadow = GetColorId( 35, 35, 35 );

        for ( int32_t y = 0; y < baseGlyphHeight; ++y ) {
            const uint32_t row = glyphData->rows[static_cast<size_t>( y )];
            for ( int32_t x = 0; x < baseGlyphWidth; ++x ) {
                if ( ( row & ( 1U << x ) ) == 0 ) {
                    continue;
                }

                for ( int32_t scaleY = 0; scaleY < scale; ++scaleY ) {
                    for ( int32_t scaleX = 0; scaleX < scale; ++scaleX ) {
                        const int32_t pixelX = x * scale + scaleX;
                        const int32_t pixelY = y * scale + scaleY;

                        if ( pixelX + 1 < width && pixelY + 1 < height ) {
                            SetPixel( glyph, pixelX + 1, pixelY + 1, shadow );
                        }
                    }
                }
            }
        }

        for ( int32_t y = 0; y < baseGlyphHeight; ++y ) {
            const uint32_t row = glyphData->rows[static_cast<size_t>( y )];
            for ( int32_t x = 0; x < baseGlyphWidth; ++x ) {
                if ( ( row & ( 1U << x ) ) == 0 ) {
                    continue;
                }

                for ( int32_t scaleY = 0; scaleY < scale; ++scaleY ) {
                    for ( int32_t scaleX = 0; scaleX < scale; ++scaleX ) {
                        SetPixel( glyph, x * scale + scaleX, y * scale + scaleY, foreground );
                    }
                }
            }
        }

        auto [iter, inserted] = cache.emplace( key, std::move( glyph ) );
        assert( inserted );
        return iter->second;
    }
}
