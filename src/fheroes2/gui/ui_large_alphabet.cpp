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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

#include <zlib.h>

#include "image.h"
#include "ui_text.h"

#if __has_include( "ui_large_alphabet_generated.h" )
#include "ui_large_alphabet_generated.h"
#define FHEROES2_HAS_GENERATED_LARGE_ALPHABET 1
#else
#define FHEROES2_HAS_GENERATED_LARGE_ALPHABET 0
#endif

namespace
{
#if FHEROES2_HAS_GENERATED_LARGE_ALPHABET
    struct FontData
    {
        int32_t width;
        int32_t height;
        int32_t advance;
        size_t uncompressedSize;
        std::string_view compressedBase64;
    };

    std::vector<uint8_t> decodeBase64( const std::string_view input )
    {
        static const std::array<int8_t, 256> table = []() {
            std::array<int8_t, 256> output{};
            output.fill( -1 );
            constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            for ( size_t i = 0; i < alphabet.size(); ++i ) {
                output[static_cast<uint8_t>( alphabet[i] )] = static_cast<int8_t>( i );
            }
            return output;
        }();

        std::vector<uint8_t> output;
        output.reserve( input.size() * 3 / 4 );

        uint32_t accumulator = 0;
        int bits = 0;
        for ( const unsigned char c : input ) {
            if ( c == '=' ) {
                break;
            }

            const int8_t value = table[c];
            if ( value < 0 ) {
                continue;
            }

            accumulator = ( accumulator << 6 ) | static_cast<uint32_t>( value );
            bits += 6;
            if ( bits >= 8 ) {
                bits -= 8;
                output.push_back( static_cast<uint8_t>( ( accumulator >> bits ) & 0xFFU ) );
            }
        }

        return output;
    }

    std::vector<uint32_t> inflateRows( const FontData & data )
    {
        const std::vector<uint8_t> compressed = decodeBase64( data.compressedBase64 );
        std::vector<uint8_t> raw( data.uncompressedSize );
        uLongf outputSize = static_cast<uLongf>( raw.size() );

        const int result = uncompress( raw.data(), &outputSize, compressed.data(), static_cast<uLong>( compressed.size() ) );
        assert( result == Z_OK && outputSize == raw.size() );
        if ( result != Z_OK || outputSize != raw.size() ) {
            return {};
        }

        std::vector<uint32_t> rows;
        rows.reserve( raw.size() / 4 );
        for ( size_t offset = 0; offset + 3 < raw.size(); offset += 4 ) {
            rows.push_back( static_cast<uint32_t>( raw[offset] ) | ( static_cast<uint32_t>( raw[offset + 1] ) << 8 )
                            | ( static_cast<uint32_t>( raw[offset + 2] ) << 16 ) | ( static_cast<uint32_t>( raw[offset + 3] ) << 24 ) );
        }

        return rows;
    }

    const FontData & getFontData( const fheroes2::FontType & fontType )
    {
        using namespace fheroes2::largeAlphabetGenerated;

        static const FontData small{ smallWidth, smallHeight, smallAdvance, smallRawSize, smallBase64 };
        static const FontData normal{ normalWidth, normalHeight, normalAdvance, normalRawSize, normalBase64 };
        static const FontData large{ largeWidth, largeHeight, largeAdvance, largeRawSize, largeBase64 };

        switch ( fontType.size ) {
        case fheroes2::FontSize::SMALL:
            return small;
        case fheroes2::FontSize::LARGE:
            return large;
        case fheroes2::FontSize::NORMAL:
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return normal;
        default:
            assert( 0 );
            return normal;
        }
    }

    const std::vector<uint32_t> & getRows( const fheroes2::FontType & fontType )
    {
        using namespace fheroes2::largeAlphabetGenerated;

        static const std::vector<uint32_t> smallRows = inflateRows( FontData{ smallWidth, smallHeight, smallAdvance, smallRawSize, smallBase64 } );
        static const std::vector<uint32_t> normalRows = inflateRows( FontData{ normalWidth, normalHeight, normalAdvance, normalRawSize, normalBase64 } );
        static const std::vector<uint32_t> largeRows = inflateRows( FontData{ largeWidth, largeHeight, largeAdvance, largeRawSize, largeBase64 } );

        switch ( fontType.size ) {
        case fheroes2::FontSize::SMALL:
            return smallRows;
        case fheroes2::FontSize::LARGE:
            return largeRows;
        case fheroes2::FontSize::NORMAL:
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return normalRows;
        default:
            assert( 0 );
            return normalRows;
        }
    }

    size_t findGlyphIndex( const uint32_t codePoint )
    {
        using namespace fheroes2::largeAlphabetGenerated;

        const auto iter = std::lower_bound( codePoints.begin(), codePoints.end(), codePoint );
        if ( iter == codePoints.end() || *iter != codePoint ) {
            return glyphCount;
        }

        return static_cast<size_t>( std::distance( codePoints.begin(), iter ) );
    }
#else
    constexpr int32_t baseGlyphWidth = 11;
    constexpr int32_t baseGlyphHeight = 13;

    struct BitmapGlyph
    {
        uint32_t codePoint;
        std::array<uint32_t, baseGlyphHeight> rows;
    };

    // A small built-in bootstrap set keeps the branch buildable even when the
    // generated header is absent. CI and release builds generate the complete
    // large alphabet before compiling.
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
#endif

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
#if FHEROES2_HAS_GENERATED_LARGE_ALPHABET
        return findGlyphIndex( codePoint ) < largeAlphabetGenerated::glyphCount;
#else
        return findGlyph( codePoint ) != nullptr;
#endif
    }

    int32_t getAdvance( const FontType & fontType )
    {
#if FHEROES2_HAS_GENERATED_LARGE_ALPHABET
        return getFontData( fontType ).advance;
#else
        return baseGlyphWidth * getScale( fontType );
#endif
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
#if FHEROES2_HAS_GENERATED_LARGE_ALPHABET
        const size_t glyphIndex = findGlyphIndex( codePoint );
        if ( glyphIndex >= largeAlphabetGenerated::glyphCount ) {
            return getZeroAdvanceSprite();
        }
#else
        const BitmapGlyph * glyphData = findGlyph( codePoint );
        if ( glyphData == nullptr ) {
            return getZeroAdvanceSprite();
        }
#endif

        static std::map<uint64_t, Sprite> cache;
        const uint64_t key = makeCacheKey( codePoint, fontType );
        if ( const auto iter = cache.find( key ); iter != cache.end() ) {
            return iter->second;
        }

#if FHEROES2_HAS_GENERATED_LARGE_ALPHABET
        const FontData & data = getFontData( fontType );
        const std::vector<uint32_t> & rows = getRows( fontType );
        const size_t rowOffset = glyphIndex * static_cast<size_t>( data.height );
        if ( rows.size() < rowOffset + static_cast<size_t>( data.height ) ) {
            return getZeroAdvanceSprite();
        }

        Sprite glyph( data.width, data.height, -data.advance, 0 );
        glyph.reset();

        const uint8_t foreground = getForegroundColor( fontType );
        const uint8_t shadow = GetColorId( 35, 35, 35 );

        for ( int32_t y = 0; y + 1 < data.height; ++y ) {
            const uint32_t row = rows[rowOffset + static_cast<size_t>( y )];
            for ( int32_t x = 0; x + 1 < data.width; ++x ) {
                if ( ( row & ( 1U << x ) ) != 0 ) {
                    SetPixel( glyph, x + 1, y + 1, shadow );
                }
            }
        }

        for ( int32_t y = 0; y < data.height; ++y ) {
            const uint32_t row = rows[rowOffset + static_cast<size_t>( y )];
            for ( int32_t x = 0; x < data.width; ++x ) {
                if ( ( row & ( 1U << x ) ) != 0 ) {
                    SetPixel( glyph, x, y, foreground );
                }
            }
        }
#else
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
#endif

        auto [iter, inserted] = cache.emplace( key, std::move( glyph ) );
        assert( inserted );
        return iter->second;
    }
}
