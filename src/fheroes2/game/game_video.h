/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2026                                             *
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

#include <cstdint>
#include <string>
#include <vector>

#include "game_video_type.h"
#include "image.h"
#include "math_base.h"
#include "screen.h"

namespace fheroes2
{
    class TextBase;
}

namespace Video
{
    struct VideoInfo final
    {
        std::string fileName;
        VideoControl control{ VideoControl::PLAY_NONE };
        fheroes2::Point offset{ 0, 0 };
    };

    class Subtitle
    {
    public:
        // Generate the image from subtitles text and store it in Subtitle class.
        // The 'position' represents the top-center of subtitles image. All subtitles are center aligned.
        Subtitle( const fheroes2::TextBase & subtitleText, const uint32_t startTimeMS, const uint32_t durationMS, const fheroes2::Point & position = { -1, -1 },
                  const int32_t maxWidth = fheroes2::Display::DEFAULT_WIDTH );

        // Check if subtitles need to be rendered at the current time (in milliseconds).
        bool needRender( const uint32_t currentTimeMS ) const
        {
            return ( ( currentTimeMS >= _startTimeMS ) && ( currentTimeMS < _endTimeMS ) );
        }

        // Render subtitles image to the output image.
        void render( fheroes2::Image & output, const fheroes2::Rect & frameRoi ) const
        {
            fheroes2::Blit( _subtitleImage, 0, 0, output, frameRoi.x + _position.x, frameRoi.y + _position.y, _subtitleImage.width(), _subtitleImage.height() );
        }

        // SMK videos replace the global 256-color palette while playing. Render Korean
        // subtitles through the active video palette so their foreground remains visually
        // white and their one-pixel drop shadow remains visually black on every frame.
        void render( fheroes2::Image & output, const fheroes2::Rect & frameRoi, const std::vector<uint8_t> & palette ) const
        {
            if ( palette.size() != 256 * 3 ) {
                render( output, frameRoi );
                return;
            }

            const auto findClosestColor = [&palette]( const uint8_t red, const uint8_t green, const uint8_t blue ) {
                uint8_t closestColor = 0;
                uint32_t closestDistance = 3U * 255U * 255U + 1U;

                for ( uint32_t colorId = 0; colorId < 256; ++colorId ) {
                    const size_t offset = colorId * 3;
                    const int32_t redDelta = static_cast<int32_t>( palette[offset] ) - red;
                    const int32_t greenDelta = static_cast<int32_t>( palette[offset + 1] ) - green;
                    const int32_t blueDelta = static_cast<int32_t>( palette[offset + 2] ) - blue;
                    const uint32_t distance = static_cast<uint32_t>( redDelta * redDelta + greenDelta * greenDelta + blueDelta * blueDelta );

                    if ( distance < closestDistance ) {
                        closestDistance = distance;
                        closestColor = static_cast<uint8_t>( colorId );
                    }
                }

                return closestColor;
            };

            const uint8_t whiteColor = findClosestColor( 255, 255, 255 );
            const uint8_t blackColor = findClosestColor( 0, 0, 0 );
            const uint8_t sourceShadowColor = fheroes2::GetColorId( 35, 35, 35 );

            fheroes2::Image subtitleImage( _subtitleImage.width(), _subtitleImage.height() );
            subtitleImage.reset();

            const uint8_t * sourcePixels = _subtitleImage.image();
            const uint8_t * sourceTransform = _subtitleImage.transform();
            uint8_t * outputPixels = subtitleImage.image();
            uint8_t * outputTransform = subtitleImage.transform();
            const int32_t width = _subtitleImage.width();
            const int32_t height = _subtitleImage.height();

            // Draw the crisp one-pixel shadow first. Existing font shadows and transform
            // effects are deliberately ignored so every video uses exactly the same style.
            for ( int32_t y = 0; y < height; ++y ) {
                for ( int32_t x = 0; x < width; ++x ) {
                    const int32_t sourceOffset = x + y * width;
                    if ( sourceTransform[sourceOffset] != 0 || sourcePixels[sourceOffset] == sourceShadowColor ) {
                        continue;
                    }

                    if ( x + 1 < width && y + 1 < height ) {
                        const int32_t shadowOffset = x + 1 + ( y + 1 ) * width;
                        outputPixels[shadowOffset] = blackColor;
                        outputTransform[shadowOffset] = 0;
                    }
                }
            }

            // Draw a solid white foreground over the shadow.
            for ( int32_t y = 0; y < height; ++y ) {
                for ( int32_t x = 0; x < width; ++x ) {
                    const int32_t sourceOffset = x + y * width;
                    if ( sourceTransform[sourceOffset] != 0 || sourcePixels[sourceOffset] == sourceShadowColor ) {
                        continue;
                    }

                    outputPixels[sourceOffset] = whiteColor;
                    outputTransform[sourceOffset] = 0;
                }
            }

            fheroes2::Blit( subtitleImage, 0, 0, output, frameRoi.x + _position.x, frameRoi.y + _position.y, subtitleImage.width(), subtitleImage.height() );
        }

    private:
        fheroes2::Point _position{ -1, -1 };
        fheroes2::Image _subtitleImage;
        const uint32_t _startTimeMS{ 0 };
        uint32_t _endTimeMS{ 0 };
    };

    // Returns true if the file exists.
    bool getVideoFilePath( const std::string & fileName, std::string & path );

    // Korean builds use this overload to attach subtitles to selected videos.
    // Other one-argument calls are forwarded unchanged to the regular player.
    bool ShowVideo( const std::vector<VideoInfo> & infos );

    // Returns false if some videos are not present, or they are corrupted.
    bool ShowVideo( const std::vector<VideoInfo> & infos, const std::vector<Subtitle> & subtitles, const bool fadeColorsOnEnd = false );
}