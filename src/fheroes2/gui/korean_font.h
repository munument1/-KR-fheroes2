/***************************************************************************
 *   fheroes2 Korean UTF-8 prototype                                       *
 ***************************************************************************/

#pragma once

#include <cstdint>

namespace fheroes2
{
    class Sprite;
    struct FontType;

    namespace koreanFont
    {
        constexpr uint16_t hangulSyllableCount = 11172;
        constexpr uint8_t asciiDigitCount = 10;

        bool isLeadByte( uint8_t value );
        bool isMiddleByte( uint8_t value );
        bool isTrailByte( uint8_t value );

        uint16_t decodeSyllableIndex( uint8_t lead, uint8_t middle, uint8_t trail );

        int32_t getAdvance( const FontType & fontType );
        const Sprite & getAdvanceSprite( const FontType & fontType );
        const Sprite & getZeroAdvanceSprite();
        const Sprite & getGlyphSprite( uint16_t syllableIndex, const FontType & fontType );
        const Sprite & getDigitSprite( uint8_t digitIndex, const FontType & fontType );
    }
}
