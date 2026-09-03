/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "artifact.h"
#include "artifact_ultimate.h"
#include "audio.h"
#include "audio_manager.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "castle.h"
#include "dialog.h"
#include "dialog_system_options.h"
#include "direction.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h" // IWYU pragma: associated
#include "game_io.h"
#include "game_mode.h"
#include "game_over.h"
#include "heroes.h"
#include "image.h"
#include "interface_base.h"
#include "interface_buttons.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "kingdom.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "mus.h"
#include "players.h"
#include "puzzle.h"
#include "resource.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "spell_book.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_tool.h"
#include "view_world.h"
#include "world.h"

void Interface::AdventureMap::ShowPathOrStartMoveHero( Heroes * hero, const int32_t destinationIdx )
{
    if ( hero == nullptr || !Maps::isValidAbsIndex( destinationIdx ) ) {
        return;
    }

    assert( !hero->Modes( Heroes::ENABLEMOVE ) );

    const Route::Path & path = hero->GetPath();

    // Calculate and show the hero's path
    if ( path.GetDestinationIndex() != destinationIdx ) {
        hero->calculatePath( destinationIdx );

        DEBUG_LOG( DBG_GAME, DBG_TRACE, hero->GetName() << ", distance: " << world.getDistance( *hero, destinationIdx ) << ", route: " << path.String() )

        _gameArea.SetRedraw();
        _buttonsPanel.setRedraw();
    }
    // Start the hero's movement
    else if ( path.isValidForMovement() && hero->MayStillMove( false, true ) ) {
        _startHeroMove( *hero );
    }
}

void Interface::AdventureMap::MoveHeroFromArrowKeys( Heroes & hero, const int direction )
{
    const int32_t heroIndex = hero.GetIndex();

    if ( !Maps::isValidDirection( heroIndex, direction ) ) {
        return;
    }

    const int32_t dstIndex = Maps::GetDirectionIndex( heroIndex, direction );
    const Maps::Tile & tile = world.getTile( dstIndex );

    if ( !tile.isPassableFrom( Direction::CENTER, hero.isShipMaster(), false, hero.GetColor() ) ) {
        return;
    }

    ShowPathOrStartMoveHero( &hero, dstIndex );
}

void Interface::AdventureMap::_startHeroMove( Heroes & hero )
{
    SetFocus( &hero, true );
    RedrawFocus();

    hero.SetMove( true );

    // We pass this delay to start hero moving immediately and set all the variables needed to handle game events correctly
    // and to stop handling mouse click events until hero stops. Otherwise there could be a rare case
    // when double click is faster than this delay and the second click will also be handled which should not happen.
    Game::passAnimationDelay( Game::DelayType::CURRENT_HERO_DELAY );
}

void Interface::AdventureMap::EventSwitchFocusedHero( const int32_t tileIndex )
{
    Heroes * selectedHero = world.getTile( tileIndex ).getHero();
    if ( selectedHero == nullptr || selectedHero == GetFocusHeroes() || selectedHero->GetColor() != Settings::Get().GetPlayers().getCurrentColor() ) {
        return;
    }
    SetFocus( selectedHero, false );
    RedrawFocus();
}

fheroes2::GameMode Interface::AdventureMap::EventCheatCodeCheck( const fheroes2::Key key )
{
    static const std::array<std::string_view, 15> cheatCodes
        = { "911", "1313", "1911", "8675309", "101495", "101111", "899101", "844691", "844690", "32167", "1134", "1135", "1136", "1137", "1138" };

    const auto keyToDigit = []( const fheroes2::Key keyValue ) -> char {
        switch ( keyValue ) {
        case fheroes2::Key::KEY_0:
            return '0';
        case fheroes2::Key::KEY_1:
            return '1';
        case fheroes2::Key::KEY_2:
            return '2';
        case fheroes2::Key::KEY_3:
            return '3';
        case fheroes2::Key::KEY_4:
            return '4';
        case fheroes2::Key::KEY_5:
            return '5';
        case fheroes2::Key::KEY_6:
            return '6';
        case fheroes2::Key::KEY_7:
            return '7';
        case fheroes2::Key::KEY_8:
            return '8';
        case fheroes2::Key::KEY_9:
            return '9';
        default:
            return '\0';
        }
    };

    static std::string codeBuffer;

    const char digit = keyToDigit( key );
    if ( digit == '\0' ) {
        codeBuffer.clear();
        return fheroes2::GameMode::CANCEL;
    }

    codeBuffer += digit;

    const auto isKnownPrefix = []() {
        return std::any_of( cheatCodes.begin(), cheatCodes.end(), []( const std::string_view code ) {
            return code.size() >= codeBuffer.size() && code.compare( 0, codeBuffer.size(), codeBuffer.data(), codeBuffer.size() ) == 0;
        } );
    };

    if ( !isKnownPrefix() ) {
        codeBuffer.assign( 1, digit );
        if ( !isKnownPrefix() ) {
            codeBuffer.clear();
        }

        return fheroes2::GameMode::CANCEL;
    }

    const auto cheatCode = std::find( cheatCodes.begin(), cheatCodes.end(), codeBuffer );
    if ( cheatCode == cheatCodes.end() ) {
        return fheroes2::GameMode::CANCEL;
    }

    const std::string activatedCode{ *cheatCode };
    codeBuffer.clear();

    if ( activatedCode == "911" || activatedCode == "1313" ) {
        GameOver::Result & gameResult = GameOver::Result::Get();
        gameResult.setCheatResult( activatedCode == "911" ? GameOver::WINS_ALL : GameOver::LOSS_ALL );
        return gameResult.checkGameOver();
    }

    if ( activatedCode == "1911" ) {
        if ( !Settings::Get().isCampaignGameType() ) {
            return fheroes2::GameMode::CANCEL;
        }

        Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( saveData.getCampaignID() );
        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();

        for ( auto scenario = scenarios.rbegin(); scenario != scenarios.rend(); ++scenario ) {
            if ( campaignData.isLastScenario( scenario->getScenarioInfoId() ) ) {
                saveData.setCurrentScenarioInfo( scenario->getScenarioInfoId() );
                return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            }
        }

        return fheroes2::GameMode::CANCEL;
    }

    if ( activatedCode == "8675309" ) {
        world.ActionFor8675309CheatCode( Settings::Get().CurrentColor() );
        Interface::GameArea::updateMapFogDirections();
        setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );
        return fheroes2::GameMode::CANCEL;
    }

    if ( activatedCode == "101495" ) {
        Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );
        kingdom.PuzzleMaps().set();
        EventPuzzleMaps();
        return fheroes2::GameMode::CANCEL;
    }

    if ( activatedCode == "101111" || activatedCode == "899101" || activatedCode == "844691" || activatedCode == "844690" ) {
        int resourceType = Resource::UNKNOWN;
        uint32_t amount = 10;

        if ( activatedCode == "101111" ) {
            resourceType = Resource::GOLD;
            amount = 1000;
        }
        else if ( activatedCode == "899101" ) {
            resourceType = Resource::GEMS;
        }
        else if ( activatedCode == "844691" ) {
            resourceType = Resource::ORE;
        }
        else {
            resourceType = Resource::CRYSTAL;
        }

        world.GetKingdom( Settings::Get().CurrentColor() ).AddFundsResource( Funds( resourceType, amount ) );
        setRedraw( Interface::REDRAW_STATUS );
        return fheroes2::GameMode::CANCEL;
    }

    Heroes * hero = GetFocusHeroes();
    if ( hero == nullptr ) {
        return fheroes2::GameMode::CANCEL;
    }

    if ( activatedCode == "32167" ) {
        hero->GetArmy().JoinTroop( Monster::BLACK_DRAGON, 5, false );
        RedrawFocus();
        return fheroes2::GameMode::CANCEL;
    }

    Monster::MonsterType monsterType = Monster::UNKNOWN;
    if ( activatedCode == "1134" ) {
        monsterType = Monster::TITAN;
    }
    else if ( activatedCode == "1135" ) {
        monsterType = Monster::ARCHMAGE;
    }
    else if ( activatedCode == "1136" ) {
        monsterType = Monster::STEEL_GOLEM;
    }
    else if ( activatedCode == "1137" ) {
        monsterType = Monster::ROC;
    }
    else if ( activatedCode == "1138" ) {
        monsterType = Monster::HALFLING;
    }

    if ( monsterType != Monster::UNKNOWN ) {
        hero->GetArmy().JoinTroop( monsterType, 100, false );
        RedrawFocus();
    }

    return fheroes2::GameMode::CANCEL;
}

void Interface::AdventureMap::EventNextHero()
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const VecHeroes & myHeroes = myKingdom.GetHeroes();

    if ( myHeroes.empty() ) {
        return;
    }

    if ( GetFocusHeroes() ) {
        VecHeroes::const_iterator it = std::find( myHeroes.begin(), myHeroes.end(), GetFocusHeroes() );
        VecHeroes::const_iterator currentHero = it;

        do {
            ++it;

            if ( it == myHeroes.end() ) {
                it = myHeroes.begin();
            }

            if ( ( *it )->MayStillMove( true, false ) ) {
                SetFocus( *it, false );
                break;
            }
        } while ( it != currentHero );
    }
    else {
        for ( Heroes * hero : myHeroes ) {
            if ( hero->MayStillMove( true, false ) ) {
                SetFocus( hero, false );
                break;
            }
        }
    }

    RedrawFocus();
}

fheroes2::GameMode Interface::AdventureMap::EventHeroMovement()
{
    if ( Heroes * hero = GetFocusHeroes(); hero ) {
        if ( hero->GetPath().isValidForMovement() && hero->MayStillMove( false, true ) ) {
            _startHeroMove( *hero );
        }
        else if ( MP2::isRevisitAllowedForObject( hero->getObjectTypeUnderHero(), hero->isShipMaster() ) ) {
            return EventDefaultAction();
        }
        else if ( hero->GetPath().isValidForMovement() ) {
            fheroes2::showStandardTextMessage( _( "Hero Movement Points" ),
                                               _( "The hero's army cannot move anymore today. The movement points will be refilled tomorrow after you end your turn." ),
                                               Dialog::OK );
        }
    }

    return fheroes2::GameMode::CANCEL;
}

void Interface::AdventureMap::EventResetHeroPath()
{
    Heroes * hero = GetFocusHeroes();
    if ( hero == nullptr ) {
        return;
    }

    hero->GetPath().Reset();

    setRedraw( REDRAW_GAMEAREA | REDRAW_BUTTONS );
}

void Interface::AdventureMap::EventKingdomInfo() const
{
    Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    myKingdom.openOverviewDialog();

    _iconsPanel.setRedraw();
}

void Interface::AdventureMap::EventCastSpell()
{
    Heroes * hero = GetFocusHeroes();
    if ( hero == nullptr ) {
        return;
    }

    // Center on the hero before opening the spell book
    _gameArea.SetCenter( hero->GetCenter() );
    redraw( REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR );

    const Spell spell = hero->OpenSpellBook( SpellBook::Filter::ADVN, true, false, {} );
    if ( !spell.isValid() ) {
        return;
    }

    hero->ActionSpellCast( spell );

    // The spell will consume the hero's spell points (and perhaps also movement points) and can move the
    // hero to another location, so we may have to update the terrain music theme and environment sounds
    ResetFocus( GameFocus::HEROES, true );
    RedrawFocus();
}

fheroes2::GameMode Interface::AdventureMap::EventEndTurn() const
{
#ifndef NDEBUG
    const Heroes * focusedHero = GetFocusHeroes();
#endif
    assert( focusedHero == nullptr || !focusedHero->Modes( Heroes::ENABLEMOVE ) );

    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    if ( !myKingdom.HeroesMayStillMove()
         || Dialog::YES
                == fheroes2::showStandardTextMessage( _( "End Turn" ), _( "One or more heroes may still move, are you sure you want to end your turn?" ),
                                                      Dialog::YES | Dialog::NO ) ) {
        return fheroes2::GameMode::END_TURN;
    }

    return fheroes2::GameMode::CANCEL;
}

fheroes2::GameMode Interface::AdventureMap::EventAdventureDialog()
{
    switch ( Dialog::AdventureOptions( GameFocus::HEROES == GetFocusType() ) ) {
    case Dialog::WORLD:
        EventViewWorld();
        break;

    case Dialog::PUZZLE:
        EventPuzzleMaps();
        break;

    case Dialog::INFO:
        return EventScenarioInformation();

    case Dialog::DIG:
        return EventDigArtifact();

    default:
        break;
    }

    return fheroes2::GameMode::CANCEL;
}

void Interface::AdventureMap::EventViewWorld()
{
    ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::OnlyVisible, *this );
}

fheroes2::GameMode Interface::AdventureMap::EventFileDialog() const
{
    return Dialog::FileOptions();
}

void Interface::AdventureMap::EventSystemDialog() const
{
    fheroes2::showSystemOptionsDialog();
}

void Interface::AdventureMap::EventNextTown()
{
    Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    VecCastles & myCastles = myKingdom.GetCastles();

    if ( !myCastles.empty() ) {
        if ( GetFocusCastle() ) {
            VecCastles::const_iterator it = std::find( myCastles.begin(), myCastles.end(), GetFocusCastle() );
            ++it;
            if ( it == myCastles.end() )
                it = myCastles.begin();
            SetFocus( *it );
        }
        else
            ResetFocus( GameFocus::CASTLE, false );

        RedrawFocus();
    }
}

fheroes2::GameMode Interface::AdventureMap::EventNewGame() const
{
    return Dialog::YES == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to restart? (Your current game will be lost.)" ), Dialog::YES | Dialog::NO )
               ? fheroes2::GameMode::NEW_GAME
               : fheroes2::GameMode::CANCEL;
}

fheroes2::GameMode Interface::AdventureMap::EventSaveGame() const
{
    while ( true ) {
        const std::string filename = Dialog::SelectFileSave();
        if ( filename.empty() ) {
            return fheroes2::GameMode::CANCEL;
        }

        if ( System::IsFile( filename )
             && Dialog::NO == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to overwrite the save with this name?" ), Dialog::YES | Dialog::NO ) ) {
            continue;
        }

        if ( Game::Save( filename ) ) {
            fheroes2::showStandardTextMessage( "", _( "Game saved successfully." ), Dialog::OK );
        }
        else {
            fheroes2::showStandardTextMessage( "", _( "There was an issue during saving." ), Dialog::OK );
        }
        return fheroes2::GameMode::CANCEL;
    }
}

fheroes2::GameMode Interface::AdventureMap::EventLoadGame() const
{
    return Dialog::YES
                   == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to load a new game? (Your current game will be lost.)" ),
                                                         Dialog::YES | Dialog::NO )
               ? fheroes2::GameMode::LOAD_GAME
               : fheroes2::GameMode::CANCEL;
}

void Interface::AdventureMap::EventPuzzleMaps() const
{
    world.GetKingdom( Settings::Get().CurrentColor() ).PuzzleMaps().ShowMapsDialog();
}

fheroes2::GameMode Interface::AdventureMap::EventScenarioInformation()
{
    if ( Settings::Get().isCampaignGameType() ) {
        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::ImageRestorer saver( display, 0, 0, display.width(), display.height() );

        // We are opening campaign scenario info. It is a full screen image change. So do fade-out and set the fade-in.
        fheroes2::fadeOutDisplay();
        Game::setDisplayFadeIn();

        AudioManager::ResetAudio();

        const fheroes2::GameMode returnMode = Game::SelectCampaignScenario( fheroes2::GameMode::CANCEL, true );
        if ( returnMode == fheroes2::GameMode::CANCEL ) {
            // We are going back to the Adventure map with fade-in.
            Game::setDisplayFadeIn();

            saver.restore();

            Game::restoreSoundsForCurrentFocus();
        }
        else {
            saver.reset();
        }

        return returnMode;
    }

    Dialog::GameInfo();
    return fheroes2::GameMode::CANCEL;
}

void Interface::AdventureMap::EventSwitchHeroSleeping()
{
    Heroes * hero = GetFocusHeroes();

    if ( hero ) {
        hero->Modes( Heroes::SLEEPER ) ? hero->ResetModes( Heroes::SLEEPER ) : hero->SetModes( Heroes::SLEEPER );

        setRedraw( REDRAW_HEROES );
        _buttonsPanel.setRedraw();
    }
}

fheroes2::GameMode Interface::AdventureMap::EventDigArtifact()
{
    Heroes * hero = GetFocusHeroes();
    if ( hero == nullptr ) {
        // TODO: verify how this method can be called if a hero is not selected.
        return fheroes2::GameMode::CANCEL;
    }

    if ( hero->isShipMaster() ) {
        fheroes2::showStandardTextMessage( "", _( "Try looking on land!!!" ), Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    if ( hero->GetBagArtifacts().isFull() ) {
        fheroes2::showStandardTextMessage(
            "", _( "Searching for the Ultimate Artifact is fruitless. Your hero could not carry it even if he found it - all his artifact slots are full." ),
            Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    if ( hero->GetMaxMovePoints() > hero->GetMovePoints() ) {
        fheroes2::showStandardTextMessage( "", _( "Digging for artifacts requires a whole day, try again tomorrow." ), Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    // Original Editor allows to put an Ultimate Artifact on an invalid tile. So checking tile index solves this issue.
    const UltimateArtifact & ultimateArtifact = world.GetUltimateArtifact();
    if ( world.getTile( hero->GetIndex() ).isSuitableForUltimateArtifact() || ( ultimateArtifact.getPosition() == hero->GetIndex() && !ultimateArtifact.isFound() ) ) {
        AudioManager::PlaySound( M82::DIGSOUND );

        hero->ResetMovePoints();
        hero->GetPath().Reset();

        if ( world.DiggingForUltimateArtifact( hero->GetCenter() ) ) {
            const AudioManager::MusicRestorer musicRestorer;

            if ( Settings::Get().MusicMIDI() ) {
                AudioManager::PlaySound( M82::TREASURE );
            }
            else {
                AudioManager::PlayMusic( MUS::ULTIMATE_ARTIFACT, Music::PlaybackMode::PLAY_ONCE );
            }

            const Artifact & ultimate = ultimateArtifact.GetArtifact();

            if ( !hero->PickupArtifact( ultimate ) ) {
                assert( 0 );
            }

            std::string msg( _( "After spending many hours digging here, you have uncovered the %{artifact}." ) );
            StringReplace( msg, "%{artifact}", ultimate.GetName() );

            const fheroes2::ArtifactDialogElement artifactUI( ultimate.GetID() );
            fheroes2::showStandardTextMessage( _( "Congratulations!" ), std::move( msg ), Dialog::OK, { &artifactUI } );
        }
        else {
            fheroes2::showStandardTextMessage( "", _( "Nothing here. Where could it be?" ), Dialog::OK );
        }

        redraw( REDRAW_HEROES | REDRAW_BUTTONS );
        fheroes2::Display::instance().render();

        // check if the game is over due to conditions related to the ultimate artifact
        return GameOver::Result::Get().checkGameOver();
    }

    fheroes2::showStandardTextMessage( "", _( "Try searching on clear ground." ), Dialog::OK );

    return fheroes2::GameMode::CANCEL;
}

fheroes2::GameMode Interface::AdventureMap::EventDefaultAction()
{
    if ( Heroes * hero = GetFocusHeroes(); hero ) {
        if ( MP2::isRevisitAllowedForObject( hero->getObjectTypeUnderHero(), hero->isShipMaster() ) ) {
            hero->Action( hero->GetIndex() );

            // The action object can alter the status of the hero (e.g. Stables or Well) or
            // move it to another location (e.g. Stone Liths or Whirlpool)
            ResetFocus( GameFocus::HEROES, true );
            RedrawFocus();

            // If the hero has performed an action, we have to check the completion condition
            // of the scenario
            if ( hero->isAction() ) {
                hero->ResetAction();

                return GameOver::Result::Get().checkGameOver();
            }
        }
    }
    else if ( Castle * castle = GetFocusCastle(); castle ) {
        Game::OpenCastleDialog( *castle );
    }

    return fheroes2::GameMode::CANCEL;
}

void Interface::AdventureMap::EventOpenFocus() const
{
    if ( Heroes * hero = GetFocusHeroes(); hero ) {
        Game::OpenHeroesDialog( *hero, true, true );
    }
    else if ( Castle * castle = GetFocusCastle(); castle ) {
        Game::OpenCastleDialog( *castle );
    }
}

void Interface::AdventureMap::EventSwitchShowRadar() const
{
    Settings & conf = Settings::Get();

    if ( conf.isHideInterfaceEnabled() ) {
        if ( conf.ShowRadar() ) {
            conf.SetShowRadar( false );
            _gameArea.SetRedraw();
        }
        else {
            conf.SetShowRadar( true );
            _radar.SetRedraw( REDRAW_RADAR );
        }
    }
}

void Interface::AdventureMap::EventSwitchShowButtons() const
{
    Settings & conf = Settings::Get();

    if ( conf.isHideInterfaceEnabled() ) {
        if ( conf.ShowButtons() ) {
            conf.SetShowButtons( false );
            _gameArea.SetRedraw();
        }
        else {
            conf.SetShowButtons( true );
            _buttonsPanel.setRedraw();
        }
    }
}

void Interface::AdventureMap::EventSwitchShowStatus() const
{
    Settings & conf = Settings::Get();

    if ( conf.isHideInterfaceEnabled() ) {
        if ( conf.ShowStatus() ) {
            conf.SetShowStatus( false );
            _gameArea.SetRedraw();
        }
        else {
            conf.SetShowStatus( true );
            _statusPanel.setRedraw();
        }
    }
}

void Interface::AdventureMap::EventSwitchShowIcons() const
{
    Settings & conf = Settings::Get();

    if ( conf.isHideInterfaceEnabled() ) {
        if ( conf.ShowIcons() ) {
            conf.SetShowIcons( false );
            _gameArea.SetRedraw();
        }
        else {
            conf.SetShowIcons( true );
            _iconsPanel.setRedraw();
        }
    }
}

void Interface::AdventureMap::EventSwitchShowControlPanel() const
{
    Settings & conf = Settings::Get();

    if ( conf.isHideInterfaceEnabled() ) {
        conf.SetShowControlPanel( !conf.ShowControlPanel() );
        _gameArea.SetRedraw();
    }
}

void Interface::AdventureMap::EventKeyArrowPress( const int dir )
{
    if ( Heroes * hero = GetFocusHeroes(); hero ) {
        MoveHeroFromArrowKeys( *hero, dir );

        return;
    }

    if ( _gameArea.isDragScroll() ) {
        return;
    }

    if ( Settings::Get().ScrollSpeed() == SCROLL_SPEED_NONE ) {
        // Scrolling is disabled.
        return;
    }

    switch ( dir ) {
    case Direction::TOP_LEFT:
        _gameArea.SetScroll( SCROLL_TOP );
        _gameArea.SetScroll( SCROLL_LEFT );
        break;
    case Direction::TOP:
        _gameArea.SetScroll( SCROLL_TOP );
        break;
    case Direction::TOP_RIGHT:
        _gameArea.SetScroll( SCROLL_TOP );
        _gameArea.SetScroll( SCROLL_RIGHT );
        break;
    case Direction::RIGHT:
        _gameArea.SetScroll( SCROLL_RIGHT );
        break;
    case Direction::BOTTOM_RIGHT:
        _gameArea.SetScroll( SCROLL_BOTTOM );
        _gameArea.SetScroll( SCROLL_RIGHT );
        break;
    case Direction::BOTTOM:
        _gameArea.SetScroll( SCROLL_BOTTOM );
        break;
    case Direction::BOTTOM_LEFT:
        _gameArea.SetScroll( SCROLL_BOTTOM );
        _gameArea.SetScroll( SCROLL_LEFT );
        break;
    case Direction::LEFT:
        _gameArea.SetScroll( SCROLL_LEFT );
        break;
    default:
        break;
    }
}
