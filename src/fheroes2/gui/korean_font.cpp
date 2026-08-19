/***************************************************************************
 *   fheroes2 Korean UTF-8 prototype                                       *
 ***************************************************************************/

#include "korean_font.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>
#include <vector>

#include <zlib.h>

#include "image.h"
#include "korean_font_generated.h"
#include "ui_text.h"

namespace
{
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
        using namespace fheroes2::koreanFontGenerated;
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
        using namespace fheroes2::koreanFontGenerated;
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

    uint32_t makeCacheKey( const uint16_t glyphIndex, const fheroes2::FontType & fontType )
    {
        return static_cast<uint32_t>( glyphIndex ) | ( static_cast<uint32_t>( fontType.size ) << 16 ) | ( static_cast<uint32_t>( fontType.color ) << 24 );
    }

    const fheroes2::Sprite & getRasterSprite( const uint16_t glyphIndex, const fheroes2::FontType & fontType )
    {
        static std::map<uint32_t, fheroes2::Sprite> cache;
        const uint32_t key = makeCacheKey( glyphIndex, fontType );
        if ( const auto iter = cache.find( key ); iter != cache.end() ) {
            return iter->second;
        }

        const FontData & data = getFontData( fontType );
        const std::vector<uint32_t> & rows = getRows( fontType );
        const size_t rowOffset = static_cast<size_t>( glyphIndex ) * static_cast<size_t>( data.height );
        if ( rows.size() < rowOffset + static_cast<size_t>( data.height ) ) {
            return fheroes2::koreanFont::getZeroAdvanceSprite();
        }

        fheroes2::Sprite glyph( data.width, data.height, -data.advance, 0 );
        glyph.reset();
        const uint8_t foreground = getForegroundColor( fontType );
        const uint8_t shadow = fheroes2::GetColorId( 35, 35, 35 );

        // Add a crisp 1-pixel drop shadow without changing glyph dimensions or advance.
        // Drawing the shadow first lets the foreground overwrite any overlapping pixels.
        for ( int32_t y = 0; y + 1 < data.height; ++y ) {
            const uint32_t row = rows[rowOffset + static_cast<size_t>( y )];
            for ( int32_t x = 0; x + 1 < data.width; ++x ) {
                if ( ( row & ( 1U << x ) ) != 0 ) {
                    fheroes2::SetPixel( glyph, x + 1, y + 1, shadow );
                }
            }
        }

        for ( int32_t y = 0; y < data.height; ++y ) {
            const uint32_t row = rows[rowOffset + static_cast<size_t>( y )];
            for ( int32_t x = 0; x < data.width; ++x ) {
                if ( ( row & ( 1U << x ) ) != 0 ) {
                    fheroes2::SetPixel( glyph, x, y, foreground );
                }
            }
        }

        auto [iter, inserted] = cache.emplace( key, std::move( glyph ) );
        assert( inserted );
        return iter->second;
    }
}

namespace fheroes2::koreanFont
{
    bool isLeadByte( const uint8_t value )
    {
        return value >= 0x80 && value <= 0x9F;
    }

    bool isMiddleByte( const uint8_t value )
    {
        return value >= 0xA0 && value <= 0xBF;
    }

    bool isTrailByte( const uint8_t value )
    {
        return value >= 0xC0;
    }

    uint16_t decodeSyllableIndex( const uint8_t lead, const uint8_t middle, const uint8_t trail )
    {
        if ( !isLeadByte( lead ) || !isMiddleByte( middle ) || !isTrailByte( trail ) ) {
            return hangulSyllableCount;
        }
        const uint32_t index = ( static_cast<uint32_t>( lead - 0x80 ) << 11 ) | ( static_cast<uint32_t>( middle - 0xA0 ) << 6 )
                               | static_cast<uint32_t>( trail - 0xC0 );
        return index < hangulSyllableCount ? static_cast<uint16_t>( index ) : hangulSyllableCount;
    }

    int32_t getAdvance( const FontType & fontType )
    {
        return getFontData( fontType ).advance;
    }

    const Sprite & getAdvanceSprite( const FontType & fontType )
    {
        const int32_t advance = getAdvance( fontType );
        const bool isButtonFont = fontType.size == FontSize::BUTTON_RELEASED || fontType.size == FontSize::BUTTON_PRESSED;
        const int32_t cacheKey = advance * 2 + ( isButtonFont ? 1 : 0 );

        static std::map<int32_t, Sprite> sprites;
        const int32_t spriteX = isButtonFont ? -1 : 0;
        const int32_t spriteWidth = isButtonFont ? advance + 1 : advance;
        auto [iter, inserted] = sprites.try_emplace( cacheKey, spriteWidth, 1, spriteX, 0 );
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

    const Sprite & getGlyphSprite( const uint16_t syllableIndex, const FontType & fontType )
    {
        if ( syllableIndex >= hangulSyllableCount ) {
            return getZeroAdvanceSprite();
        }

        return getRasterSprite( syllableIndex, fontType );
    }

    const Sprite & getDigitSprite( const uint8_t digitIndex, const FontType & fontType )
    {
        if ( digitIndex >= asciiDigitCount ) {
            return getZeroAdvanceSprite();
        }

        return getRasterSprite( static_cast<uint16_t>( hangulSyllableCount + digitIndex ), fontType );
    }
}
