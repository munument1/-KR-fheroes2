from pathlib import Path
import re


def replace_once(path, old, new):
    p = Path(path)
    s = p.read_text()
    count = s.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one match, found {count}")
    p.write_text(s.replace(old, new, 1))


def regex_replace_once(path, pattern, replacement):
    p = Path(path)
    s = p.read_text()
    s2, count = re.subn(pattern, replacement, s, count=1, flags=re.S)
    if count != 1:
        raise SystemExit(f"{path}: regex replacement count={count}")
    p.write_text(s2)


replace_once(
    "src/fheroes2/game/game_interface.h",
    "        void EventCheatCodeCheck( fheroes2::Key key );",
    "        fheroes2::GameMode EventCheatCodeCheck( fheroes2::Key key );",
)

replace_once(
    "src/fheroes2/game/game_startgame.cpp",
    "                EventCheatCodeCheck( le.getPressedKeyValue() );",
    """                const fheroes2::GameMode cheatResult = EventCheatCodeCheck( le.getPressedKeyValue() );
                if ( cheatResult != fheroes2::GameMode::CANCEL ) {
                    res = cheatResult;
                }""",
)

p = Path("src/fheroes2/gui/interface_events.cpp")
s = p.read_text()
for old, new in [
    ("#include <algorithm>\n", "#include <algorithm>\n#include <array>\n"),
    ("#include <string>\n", "#include <string>\n#include <string_view>\n"),
    (
        '#include "audio_manager.h"\n',
        '#include "audio_manager.h"\n#include "campaign_data.h"\n#include "campaign_savedata.h"\n#include "campaign_scenariodata.h"\n',
    ),
    ('#include "puzzle.h"\n', '#include "puzzle.h"\n#include "resource.h"\n'),
]:
    if s.count(old) != 1:
        raise SystemExit(f"interface_events.cpp include pattern not found exactly once: {old!r}")
    s = s.replace(old, new, 1)
p.write_text(s)

cheat_handler = r'''fheroes2::GameMode Interface::AdventureMap::EventCheatCodeCheck( const fheroes2::Key key )
{
    static const std::array<std::string_view, 16> cheatCodes = { "911",       "1313",      "1911",   "8675309", "123456789", "101495",
                                                                  "101111",    "899101",    "844691", "844690",  "32167",     "1134",
                                                                  "1135",      "1136",      "1137",   "1138" };

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

    const auto isKnownPrefix = [&codeBuffer]() {
        return std::any_of( cheatCodes.begin(), cheatCodes.end(), [&codeBuffer]( const std::string_view code ) {
            return code.size() >= codeBuffer.size() && code.compare( 0, codeBuffer.size(), codeBuffer ) == 0;
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

    if ( activatedCode == "123456789" ) {
        hero->SetModes( Heroes::CHEAT_MAX_LUCK );
        RedrawFocus();
        return fheroes2::GameMode::CANCEL;
    }

    if ( activatedCode == "32167" ) {
        if ( hero->inCastle() == nullptr ) {
            hero->GetArmy().JoinTroop( Monster::BLACK_DRAGON, 5, false );
            RedrawFocus();
        }
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
'''

regex_replace_once(
    "src/fheroes2/gui/interface_events.cpp",
    r"void Interface::AdventureMap::EventCheatCodeCheck\( fheroes2::Key key \)\n\{.*?\n\}\n\n(?=void Interface::AdventureMap::EventNextHero\(\))",
    cheat_handler + "\n",
)

p = Path("src/fheroes2/game/game_over.h")
s = p.read_text().replace("Copyright (C) 2019 - 2025", "Copyright (C) 2019 - 2026", 1)
old = """        void ResetResult()
        {
            result = GameOver::COND_NONE;
        }"""
new = """        void ResetResult()
        {
            result = GameOver::COND_NONE;
            _cheatResult = GameOver::COND_NONE;
        }

        void setCheatResult( const uint32_t cheatResult )
        {
            _cheatResult = cheatResult & ( GameOver::WINS | GameOver::LOSS );
        }"""
if s.count(old) != 1:
    raise SystemExit("game_over.h: ResetResult pattern not found")
s = s.replace(old, new, 1)
old = "        uint32_t result{ 0 };"
if s.count(old) != 1:
    raise SystemExit("game_over.h: result field pattern not found")
s = s.replace(old, old + "\n        uint32_t _cheatResult{ GameOver::COND_NONE };", 1)
p.write_text(s)

p = Path("src/fheroes2/game/game_over.cpp")
s = p.read_text().replace("Copyright (C) 2019 - 2025", "Copyright (C) 2019 - 2026", 1)
old = """void GameOver::Result::Reset()
{
    _colors = Game::GetKingdomColors();
    result = GameOver::COND_NONE;
}"""
new = """void GameOver::Result::Reset()
{
    _colors = Game::GetKingdomColors();
    result = GameOver::COND_NONE;
    _cheatResult = GameOver::COND_NONE;
}"""
if s.count(old) != 1:
    raise SystemExit("game_over.cpp: Reset pattern not found")
s = s.replace(old, new, 1)

old_start = """fheroes2::GameMode GameOver::Result::checkGameOver()
{
    const Settings & conf = Settings::Get();
"""
forced_start = """fheroes2::GameMode GameOver::Result::checkGameOver()
{
    const Settings & conf = Settings::Get();

    if ( _cheatResult != GameOver::COND_NONE ) {
        result = _cheatResult;
        _cheatResult = GameOver::COND_NONE;

        if ( result & GameOver::LOSS ) {
            DialogLoss( result );

            AudioManager::ResetAudio();
            Video::ShowVideo( { { \"LOSE.SMK\", Video::VideoControl::PLAY_CUTSCENE_LOOP } } );

            return fheroes2::GameMode::MAIN_MENU;
        }

        if ( result & GameOver::WINS ) {
            DialogWins( result );

            if ( conf.isCampaignGameType() ) {
                return fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO;
            }

            AudioManager::ResetAudio();
            Video::ShowVideo( { { \"WIN.SMK\", Video::VideoControl::PLAY_CUTSCENE_WAIT } }, { standardGameResults() }, true );
            AudioManager::PlayMusicAsync( MUS::VICTORY, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );

            return fheroes2::GameMode::HIGHSCORES_STANDARD;
        }
    }
"""
if s.count(old_start) != 1:
    raise SystemExit("game_over.cpp: checkGameOver start not found")
s = s.replace(old_start, forced_start, 1)
p.write_text(s)

p = Path("src/fheroes2/heroes/heroes.h")
s = p.read_text()
old = """        // UNUSED = 0x00000010,
        // UNUSED = 0x00000020,"""
new = """        // Gives this hero maximum luck. Used by the original 123456789 cheat code.
        CHEAT_MAX_LUCK = 0x00000010,
        // UNUSED = 0x00000020,"""
if s.count(old) != 1:
    raise SystemExit("heroes.h: unused mode bit pattern not found")
p.write_text(s.replace(old, new, 1))

p = Path("src/fheroes2/heroes/heroes.cpp")
s = p.read_text()
old = """int Heroes::getLuckWithModifiers( std::string * text ) const
{
    int result = Luck::NORMAL;
"""
new = """int Heroes::getLuckWithModifiers( std::string * text ) const
{
    if ( Modes( CHEAT_MAX_LUCK ) ) {
        return Luck::IRISH;
    }

    int result = Luck::NORMAL;
"""
if s.count(old) != 1:
    raise SystemExit("heroes.cpp: luck function pattern not found")
p.write_text(s.replace(old, new, 1))

for path in [
    "src/fheroes2/game/game_interface.h",
    "src/fheroes2/game/game_startgame.cpp",
    "src/fheroes2/gui/interface_events.cpp",
    "src/fheroes2/game/game_over.h",
    "src/fheroes2/game/game_over.cpp",
    "src/fheroes2/heroes/heroes.h",
    "src/fheroes2/heroes/heroes.cpp",
]:
    p = Path(path)
    s = p.read_text()
    s = re.sub(r"Copyright \(C\) (\d{4}) - 202[0-5]", r"Copyright (C) \1 - 2026", s, count=1)
    p.write_text(s)
