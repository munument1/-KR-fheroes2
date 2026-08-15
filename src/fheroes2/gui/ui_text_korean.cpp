/***************************************************************************
 *   fheroes2 Korean UTF-8 prototype                                       *
 ***************************************************************************/

// This translation unit wraps the stock byte-oriented text renderer. Korean
// translations are converted from UTF-8 to a compact three-byte Hangul code
// in the build workflow. We keep the existing layout engine untouched and
// intercept only font image lookups while Korean is active.

#include "ui_text.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <numeric>

#include "game_assets.h"
#include "icn.h"
#include "korean_font.h"
#include "settings.h"
#include "ui_language.h"

namespace
{
    struct KoreanSequenceState
    {
        uint8_t lead{ 0 };
        uint8_t middle{ 0 };
        uint8_t stage{ 0 };
    };

    thread_local KoreanSequenceState koreanSequenceState;

    bool isKoreanMode()
    {
        return Settings::Get().getGameLanguage() == "ko";
    }

    bool isKoreanFontIcn( const int icnId )
    {
        switch ( icnId ) {
        case ICN::SMALFONT:
        case ICN::GRAY_SMALL_FONT:
        case ICN::YELLOW_SMALLFONT:
        case ICN::FONT:
        case ICN::GRAY_FONT:
        case ICN::YELLOW_FONT:
        case ICN::GOLDEN_GRADIENT_FONT:
        case ICN::SILVER_GRADIENT_FONT:
        case ICN::WHITE_LARGE_FONT:
        case ICN::GOLDEN_GRADIENT_LARGE_FONT:
        case ICN::SILVER_GRADIENT_LARGE_FONT:
        case ICN::BUTTON_GOOD_FONT_RELEASED:
        case ICN::BUTTON_EVIL_FONT_RELEASED:
        case ICN::BUTTON_GOOD_FONT_PRESSED:
        case ICN::BUTTON_EVIL_FONT_PRESSED:
            return true;
        default:
            return false;
        }
    }

    fheroes2::FontType getKoreanFontType( const int icnId )
    {
        switch ( icnId ) {
        case ICN::SMALFONT:
            return { fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE };
        case ICN::GRAY_SMALL_FONT:
            return { fheroes2::FontSize::SMALL, fheroes2::FontColor::GRAY };
        case ICN::YELLOW_SMALLFONT:
            return { fheroes2::FontSize::SMALL, fheroes2::FontColor::YELLOW };
        case ICN::FONT:
            return { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE };
        case ICN::GRAY_FONT:
            return { fheroes2::FontSize::NORMAL, fheroes2::FontColor::GRAY };
        case ICN::YELLOW_FONT:
            return { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW };
        case ICN::GOLDEN_GRADIENT_FONT:
            return { fheroes2::FontSize::NORMAL, fheroes2::FontColor::GOLDEN_GRADIENT };
        case ICN::SILVER_GRADIENT_FONT:
            return { fheroes2::FontSize::NORMAL, fheroes2::FontColor::SILVER_GRADIENT };
        case ICN::WHITE_LARGE_FONT:
            return { fheroes2::FontSize::LARGE, fheroes2::FontColor::WHITE };
        case ICN::GOLDEN_GRADIENT_LARGE_FONT:
            return { fheroes2::FontSize::LARGE, fheroes2::FontColor::GOLDEN_GRADIENT };
        case ICN::SILVER_GRADIENT_LARGE_FONT:
            return { fheroes2::FontSize::LARGE, fheroes2::FontColor::SILVER_GRADIENT };
        case ICN::BUTTON_GOOD_FONT_RELEASED:
            return { fheroes2::FontSize::BUTTON_RELEASED, fheroes2::FontColor::WHITE };
        case ICN::BUTTON_EVIL_FONT_RELEASED:
            return { fheroes2::FontSize::BUTTON_RELEASED, fheroes2::FontColor::GRAY };
        case ICN::BUTTON_GOOD_FONT_PRESSED:
            return { fheroes2::FontSize::BUTTON_PRESSED, fheroes2::FontColor::WHITE };
        case ICN::BUTTON_EVIL_FONT_PRESSED:
            return { fheroes2::FontSize::BUTTON_PRESSED, fheroes2::FontColor::GRAY };
        default:
            assert( 0 );
            return fheroes2::FontType::normalWhite();
        }
    }
}

namespace Assets
{
    const fheroes2::Sprite & getKoreanAwareImage( const int icnId, const uint32_t index )
    {
        if ( !isKoreanMode() || !isKoreanFontIcn( icnId ) ) {
            koreanSequenceState = {};
            return getImage( icnId, index );
        }

        if ( index < 0x80 ) {
            koreanSequenceState = {};
            const fheroes2::Sprite & sprite = getImage( icnId, index );
            if ( !sprite.empty() ) {
                return sprite;
            }

            // Some user maps contain printable ASCII symbols for which the original
            // Heroes II font resource has no sprite. Korean map translations can keep
            // those symbols, so use a visible safe fallback instead of tripping the
            // stock renderer's !charSprite.empty() assertion.
            const fheroes2::Sprite & fallback = getImage( icnId, static_cast<uint32_t>( '?' ) );
            if ( !fallback.empty() ) {
                return fallback;
            }

            return fheroes2::koreanFont::getZeroAdvanceSprite();
        }

        const uint8_t value = static_cast<uint8_t>( index );
        const fheroes2::FontType fontType = getKoreanFontType( icnId );

        if ( fheroes2::koreanFont::isLeadByte( value ) ) {
            koreanSequenceState.lead = value;
            koreanSequenceState.middle = 0;
            koreanSequenceState.stage = 1;
            return fheroes2::koreanFont::getAdvanceSprite( fontType );
        }

        if ( fheroes2::koreanFont::isMiddleByte( value ) ) {
            if ( koreanSequenceState.stage == 1 ) {
                koreanSequenceState.middle = value;
                koreanSequenceState.stage = 2;
            }
            else {
                koreanSequenceState = {};
            }
            return fheroes2::koreanFont::getZeroAdvanceSprite();
        }

        if ( fheroes2::koreanFont::isTrailByte( value ) ) {
            if ( koreanSequenceState.stage == 2 ) {
                const uint16_t syllableIndex
                    = fheroes2::koreanFont::decodeSyllableIndex( koreanSequenceState.lead, koreanSequenceState.middle, value );
                koreanSequenceState = {};
                return fheroes2::koreanFont::getGlyphSprite( syllableIndex, fontType );
            }

            koreanSequenceState = {};
            return fheroes2::koreanFont::getZeroAdvanceSprite();
        }

        koreanSequenceState = {};
        const fheroes2::Sprite & sprite = getImage( icnId, index );
        if ( !sprite.empty() ) {
            return sprite;
        }

        const fheroes2::Sprite & fallback = getImage( icnId, static_cast<uint32_t>( '?' ) );
        if ( !fallback.empty() ) {
            return fallback;
        }

        return fheroes2::koreanFont::getZeroAdvanceSprite();
    }

    uint32_t getKoreanAwareImageCount( const int icnId )
    {
        if ( isKoreanMode() && ( icnId == ICN::SMALFONT || icnId == ICN::FONT || icnId == ICN::BUTTON_GOOD_FONT_RELEASED ) ) {
            return 256;
        }

        return getImageCount( icnId );
    }
}

#define getImage getKoreanAwareImage
#define getImageCount getKoreanAwareImageCount
#include "ui_text.cpp"
#undef getImageCount
#undef getImage
