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

#include "game_video.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "audio.h"
#include "cursor.h"
#include "dir.h"
#include "game_delays.h"
#include "game_video_type.h"
#include "localevent.h"
#include "logging.h"
#include "math_tools.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    // Anim2 directory is used in Russian Buka version of the game.
    std::array<std::string, 4> videoDir = { "anim", "anim2", System::concatPath( "heroes2", "anim" ), "data" };

    void playAudio( const std::vector<std::vector<uint8_t>> & audioChannels )
    {
        for ( const std::vector<uint8_t> & audio : audioChannels ) {
            if ( !audio.empty() ) {
                Mixer::Play( audio.data(), static_cast<uint32_t>( audio.size() ), false );
            }
        }
    }

    // Internal video state structure during playback.
    struct VideoState final
    {
        Video::VideoControl control{ Video::VideoControl::PLAY_NONE };
        fheroes2::Rect area;
        int32_t delayBetweenFramesInMs{ 0 };
        int32_t nextFrameInMs{ 0 };
    };

    std::vector<Video::Subtitle> getSuccessionWarsIntroSubtitles()
    {
        const fheroes2::FontType subtitleFont = fheroes2::FontType::normalWhite();
        const fheroes2::Point subtitlePosition{ 320, 430 };
        constexpr int32_t subtitleWidth = 600;

        std::vector<Video::Subtitle> subtitles;
        subtitles.reserve( 13 );

        // Initial timings are aligned to the English narration in INTRO.SMK.
        // They are intentionally kept here as data so they can be tuned after
        // an in-game playback check without touching the video decoder.
        subtitles.emplace_back(
            fheroes2::Text( u8"\uBAA8\uB4E0 \uC77C\uC740 3\uB144 \uC804, \uB299\uC740 \uAD6D\uC655 \uC544\uC774\uC5B8\uD53C\uC2A4\uD2B8\uAC00 \uC138\uC0C1\uC744 \uB5A0\uB098\uBA74\uC11C \uC2DC\uC791\uB418\uC5C8\uB2E4.", subtitleFont ),
            6000, 7000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( u8"\uC655\uC5D0\uAC8C\uB294 \uB450 \uC544\uB4E4\uC774 \uC788\uC5C8\uB2E4.", subtitleFont ), 13500, 2500, subtitlePosition,
                                subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uB864\uB79C\uB4DC\uB294 \uC120\uB7C9\uD558\uACE0 \uCE5C\uC808\uD558\uBA70 \uBA85\uC608\uB85C\uC6B4 \uC778\uBB3C\uC774\uC5C8\uB2E4.", subtitleFont ), 16500, 3500,
            subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( u8"\uBC18\uBA74 \uC544\uCE58\uBC1C\uB4DC\uB294... \uADF8\uB9AC \uC120\uB7C9\uD558\uC9C0 \uC54A\uC558\uB2E4.", subtitleFont ), 20500, 3500,
                                subtitlePosition, subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uC804\uD1B5\uC5D0 \uB530\uB77C \uC655\uC704 \uACC4\uC2B9\uC790\uB294 \uC655\uC2E4 \uC608\uC5B8\uC790\uAC00 \uC815\uD588\uC9C0\uB9CC, \uADF8\uB294 \uBD88\uC758\uC758 \uBCF4\uD2B8 \uC0AC\uACE0\uB85C \uBAA9\uC228\uC744 \uC783\uC5C8\uB2E4.", subtitleFont ),
            24500, 8000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uB4A4\uB97C \uC774\uC740 \uC608\uC5B8\uC790\uB4E4\uC758 \uC6B4\uB3C4 \uB098\uC558\uB2E4\uACE4 \uD560 \uC218 \uC5C6\uC5C8\uB2E4. \uD504\uB808\uB354\uB9AD\uC740 \uCC3D\uBB38\uC5D0\uC11C \uB5A8\uC5B4\uC84C\uACE0,", subtitleFont ),
            33000, 6000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( u8"\uB85C\uBC84\uD2B8\uB294 \uC6A9\uC5D0\uAC8C \uBAA9\uC228\uC744 \uC783\uC5C8\uC73C\uBA70,", subtitleFont ), 39500, 2500, subtitlePosition,
                                subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( u8"\uC694\uD55C\uC740 \uC2DD\uC911\uB3C5\uC73C\uB85C \uC8FD\uC5C8\uB2E4.", subtitleFont ), 42500, 3000, subtitlePosition,
                                subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uC544\uCE58\uBC1C\uB4DC\uB294 \uC608\uC5B8\uC790\uB4E4\uC744 \uC8FD\uC778 \uBC94\uC778\uC774 \uB864\uB79C\uB4DC\uB77C \uC8FC\uC7A5\uD558\uBA70 \uADF8\uB97C \uBC18\uC5ED\uC790\uB85C \uC120\uD3EC\uD588\uB2E4.", subtitleFont ),
            46000, 6000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uBAA9\uC228\uC758 \uC704\uD611\uC744 \uB290\uB080 \uB864\uB79C\uB4DC\uB294 \uAD81\uC804\uC744 \uB5A0\uB098 \uC11C\uCABD\uC758 \uC790\uC2E0\uC758 \uC131\uC73C\uB85C \uB2EC\uC544\uB0AC\uB2E4.", subtitleFont ),
            52500, 6000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uB864\uB79C\uB4DC\uAC00 \uC0AC\uB77C\uC9C0\uC790 \uC544\uCE58\uBC1C\uB4DC\uB294 \uC0C8 \uC655\uC2E4 \uC608\uC5B8\uC790\uC758 \uACB0\uC815\uC5D0 \uC601\uD5A5\uB825\uC744 \uD589\uC0AC\uD560 \uC218 \uC788\uC5C8\uB2E4.", subtitleFont ),
            59000, 6000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back(
            fheroes2::Text( u8"\uC608\uC5B8\uC790\uB294 \uC544\uCE58\uBC1C\uB4DC\uB97C \uD6C4\uACC4\uC790\uB85C \uC120\uD0DD\uD588\uACE0, \uADF8\uB294 \uB2E4\uC74C \uB0A0 \uC2A4\uC2A4\uB85C \uC655\uC704\uC5D0 \uC62C\uB790\uB2E4.", subtitleFont ),
            65500, 6000, subtitlePosition, subtitleWidth );
        subtitles.emplace_back( fheroes2::Text( u8"\uADF8\uB807\uAC8C \uC655\uC704 \uACC4\uC2B9 \uC804\uC7C1\uC774 \uC2DC\uC791\uB418\uC5C8\uB2E4.", subtitleFont ), 72000, 4500,
                                subtitlePosition, subtitleWidth );

        return subtitles;
    }
}

namespace Video
{
    bool getVideoFilePath( const std::string & fileName, std::string & path )
    {
        for ( const std::string & rootDir : Settings::GetRootDirs() ) {
            for ( size_t dirIdx = 0; dirIdx < videoDir.size(); ++dirIdx ) {
                const std::string fullDirPath = System::concatPath( rootDir, videoDir[dirIdx] );

                if ( System::IsDirectory( fullDirPath ) ) {
                    ListFiles videoFiles;
                    videoFiles.FindFileInDir( fullDirPath, fileName );
                    if ( videoFiles.empty() ) {
                        continue;
                    }

                    std::swap( videoFiles.front(), path );

                    if ( dirIdx > 0 ) {
                        // Put the current directory at the first place to increase cache hit chance.
                        std::swap( videoDir[0], videoDir[dirIdx] );
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool ShowVideo( const std::vector<VideoInfo> & infos )
    {
        if ( infos.size() == 1 && infos.front().fileName == "INTRO.SMK" ) {
            return ShowVideo( infos, getSuccessionWarsIntroSubtitles(), false );
        }

        return ShowVideo( infos, {}, false );
    }

    bool ShowVideo( const std::vector<VideoInfo> & infos, const std::vector<Subtitle> & subtitles /* = {} */, const bool fadeColorsOnEnd /* = false */ )
    {
        if ( infos.empty() ) {
            // What it is expected from an empty video?
            return false;
        }

        // Stop any cycling animation.
        fheroes2::ScreenPaletteRestorer screenRestorer;

        // Do not allow users to close the application because of changed palette.
        const fheroes2::ApplicationClosureRestorer closureRestorer;

        std::vector<std::pair<VideoState, std::unique_ptr<SMKVideoSequence>>> sequences;
        // Minimal delay for playback in milliseconds.
        // The framerate of each video is reasonably low so let's put 1 FPS as a start point.
        int32_t minDelayInMs = 1000;

        fheroes2::Rect videoRoi;

        for ( const auto & info : infos ) {
            if ( info.fileName.empty() ) {
                // What do you expect from an empty entry?
                DEBUG_LOG( DBG_GAME, DBG_INFO, "An empty file entry has been provided." )
                return false;
            }

            if ( info.control == VideoControl::PLAY_NONE ) {
                // This entry has no action.
                DEBUG_LOG( DBG_GAME, DBG_INFO, info.fileName << " entry has no action." )
                return false;
            }

            std::string videoPath;
            if ( !getVideoFilePath( info.fileName, videoPath ) ) {
                // File doesn't exist, so no need to even try to load it.
                DEBUG_LOG( DBG_GAME, DBG_INFO, info.fileName << " video file does not exist." )
                return false;
            }
            auto video = std::make_unique<SMKVideoSequence>( videoPath );
            if ( video->frameCount() < 1 ) {
                // The file is corrupted.
                DEBUG_LOG( DBG_GAME, DBG_INFO, info.fileName << " video file has no frames." )
                return false;
            }
            const int32_t delay = static_cast<int32_t>( std::lround( video->microsecondsPerFrame() / 1000 ) );
            minDelayInMs = std::min( minDelayInMs, delay );

            const fheroes2::Rect frameRoi{ info.offset.x, info.offset.y, video->width(), video->height() };
            const VideoState state{ info.control, frameRoi, delay, delay };

            if ( videoRoi == fheroes2::Rect{} ) {
                videoRoi = frameRoi;
            }
            else {
                videoRoi = fheroes2::getBoundaryRect( videoRoi, frameRoi );
            }

            sequences.emplace_back( state, std::move( video ) );
        }

        // Center the video in the middle of the application.
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Point videoOffset{ ( display.width() - videoRoi.width ) / 2, ( display.height() - videoRoi.height ) / 2 };
        videoRoi = { videoOffset.x + videoRoi.x, videoOffset.y + videoRoi.y, videoRoi.width, videoRoi.height };

        for ( auto & [state, video] : sequences ) {
            state.area.x += videoOffset.x;
            state.area.y += videoOffset.y;
        }

        // TODO: if we need to play multiple videos with different framerate then we are going to slow down some of them
        //       due to invalid refresh time which is now set to the lowest value.
        //       We need to use a set of custom delays.

        // Hide mouse cursor.
        const CursorRestorer cursorRestorer( false );

        display.fill( 0 );
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        std::vector<uint8_t> currPalette;
        std::vector<uint8_t> prevPalette;

        // Render the first frame.
        for ( auto & [state, video] : sequences ) {
            video->resetFrame();

            if ( state.control & VideoControl::PLAY_VIDEO ) {
                video->getNextFrame( display, state.area.x, state.area.y, state.area.width, state.area.height, prevPalette );
            }
            else {
                video->skipFrame();
            }
            screenRestorer.changePalette( prevPalette.data() );

            state.nextFrameInMs -= minDelayInMs;
        }

        // Render subtitles on the first frame.
        for ( const Subtitle & subtitle : subtitles ) {
            if ( subtitle.needRender( 0 ) ) {
                subtitle.render( display, videoRoi );
            }
        }

        LocalEvent & le = LocalEvent::Get();

        Game::passCustomAnimationDelay( minDelayInMs );
        // Make sure that the first run is passed immediately.
        assert( !Game::isCustomDelayNeeded( minDelayInMs ) );

        // Play audio just before rendering the frame. This is important to minimize synchronization issues between audio and video.
        if ( Audio::isValid() ) {
            for ( const auto & [state, video] : sequences ) {
                if ( state.control & VideoControl::PLAY_AUDIO ) {
                    playAudio( video->getAudioChannels() );
                }
            }
        }

        bool endVideo = false;
        int32_t timePassed = 0;
        while ( le.HandleEvents( Game::isCustomDelayNeeded( minDelayInMs ) ) ) {
            if ( le.isAnyKeyPressed() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                Mixer::Stop();
                break;
            }

            if ( Game::validateCustomAnimationDelay( minDelayInMs ) ) {
                // Render the prepared frame.
                display.render( videoRoi );

                for ( auto & [state, video] : sequences ) {
                    if ( video->getCurrentFrameId() < video->frameCount() ) {
                        if ( video->getCurrentFrameId() + 1 == video->frameCount() ) {
                            // This is the last frame in the video sequence.
                            if ( state.control & VideoControl::PLAY_LOOP ) {
                                // Since the video is in a loop, we need to restart its video and audio.
                                video->resetFrame();

                                if ( Audio::isValid() && ( state.control & VideoControl::PLAY_AUDIO ) ) {
                                    playAudio( video->getAudioChannels() );
                                }
                            }
                            else {
                                // Play last frame as long as possible.
                                if ( minDelayInMs > state.nextFrameInMs ) {
                                    endVideo = true;
                                }
                            }
                        }

                        // Prepare the next frame for render.
                        if ( state.nextFrameInMs <= minDelayInMs ) {
                            if ( state.control & VideoControl::PLAY_VIDEO ) {
                                video->getNextFrame( display, state.area.x, state.area.y, state.area.width, state.area.height, currPalette );
                            }
                            else {
                                video->skipFrame();
                            }
                            state.nextFrameInMs = state.delayBetweenFramesInMs;
                        }
                        else {
                            if ( state.control & VideoControl::PLAY_VIDEO ) {
                                video->getCurrentFrame( display, state.area.x, state.area.y, state.area.width, state.area.height, currPalette );
                            }
                            state.nextFrameInMs -= minDelayInMs;
                        }

                        if ( prevPalette != currPalette ) {
                            screenRestorer.changePalette( currPalette.data() );
                            std::swap( currPalette, prevPalette );
                        }
                    }
                    else if ( !( state.control & VideoControl::PLAY_WAIT ) ) {
                        endVideo = true;
                    }
                }

                timePassed += minDelayInMs;
                // Render subtitles on the prepared frame.
                for ( const Subtitle & subtitle : subtitles ) {
                    if ( subtitle.needRender( timePassed ) ) {
                        subtitle.render( display, videoRoi );
                    }
                }
            }

            if ( endVideo ) {
                break;
            }
        }

        if ( fadeColorsOnEnd ) {
            // Do color fade for 1 second with 15 FPS.
            fheroes2::colorFade( currPalette, videoRoi, 1000, 15.0 );
        }
        else {
            display.fill( 0 );
        }
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        return true;
    }

    Subtitle::Subtitle( const fheroes2::TextBase & subtitleText, const uint32_t startTimeMS, const uint32_t durationMS,
                        const fheroes2::Point & position /* = { -1, -1 } */, const int32_t maxWidth /* = fheroes2::Display::DEFAULT_WIDTH */ )
        : _position( position )
        , _startTimeMS( startTimeMS )
    {
        assert( maxWidth > 0 );
        const int32_t textWidth = subtitleText.width( maxWidth );

        // We add extra 1 to have space for contour.
        _subtitleImage.resize( textWidth + 1, subtitleText.height( textWidth ) + 1 );

        // Draw text and remove all shadow data if it could not be properly applied to video palette.
        // We use the black color with id = 36 so no shadow will be applied to it.
        const uint8_t blackColor = 36;
        _subtitleImage.fill( blackColor );

        // At the left and bottom there is space for contour left by original font shadows, we leave 1 extra pixel from the right and top.
        subtitleText.draw( 0, 1, textWidth, _subtitleImage );
        fheroes2::ReplaceColorIdByTransformId( _subtitleImage, blackColor, 1 );
        // Add black contour to the text.
        fheroes2::Blit( fheroes2::CreateContour( _subtitleImage, blackColor ), _subtitleImage );

        // This is made to avoid overflow when calculating the end frame.
        _endTimeMS = _startTimeMS + std::min( durationMS, UINT32_MAX - _startTimeMS );

        // If position has negative value: position subtitles at the bottom center by using default screen size (it is currently equal to video size).
        if ( ( _position.x < 0 ) || ( _position.y < 0 ) ) {
            _position.x = ( fheroes2::Display::DEFAULT_WIDTH - _subtitleImage.width() ) / 2;
            _position.y = fheroes2::Display::DEFAULT_HEIGHT - _subtitleImage.height();
        }
        else {
            _position.x -= _subtitleImage.width() / 2;
        }
    }
}
