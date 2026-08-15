#!/usr/bin/env python3

from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    file_path = Path(path)
    text = file_path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"Expected exactly one match in {path}, found {count}: {old!r}")
    file_path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


# Scenario selection: map names and descriptions are loaded dynamically from map files.
replace_once(
    "src/fheroes2/dialog/dialog_selectscenario.cpp",
    "const fheroes2::Text header( info->name, fheroes2::FontType::normalYellow(), info->getSupportedLanguage() );",
    "const fheroes2::Text header( _( info->name ), fheroes2::FontType::normalYellow(), info->getSupportedLanguage() );",
)
replace_once(
    "src/fheroes2/dialog/dialog_selectscenario.cpp",
    "fheroes2::Text mapNameText{ info.name, fheroes2::FontType::normalWhite(), info.getSupportedLanguage() };",
    "fheroes2::Text mapNameText{ _( info.name ), fheroes2::FontType::normalWhite(), info.getSupportedLanguage() };",
)
replace_once(
    "src/fheroes2/dialog/dialog_selectscenario.cpp",
    "fheroes2::Text descriptionText( info.description, fheroes2::FontType::normalWhite(), info.getSupportedLanguage() );",
    "fheroes2::Text descriptionText( _( info.description ), fheroes2::FontType::normalWhite(), info.getSupportedLanguage() );",
)
replace_once(
    "src/fheroes2/dialog/dialog_selectscenario.cpp",
    "fheroes2::Text mapName{ info.name,",
    "fheroes2::Text mapName{ _( info.name ),",
)

# Scenario setup title.
replace_once(
    "src/fheroes2/game/game_scenarioinfo.cpp",
    "fheroes2::Text text{ info.name, fheroes2::FontType::normalWhite(), info.getSupportedLanguage() };",
    "fheroes2::Text text{ _( info.name ), fheroes2::FontType::normalWhite(), info.getSupportedLanguage() };",
)

# Ground events and signs/bottles.
replace_once(
    "src/fheroes2/heroes/heroes_action.cpp",
    "const fheroes2::Text body{ sign->message.text, fheroes2::FontType::normalWhite(), sign->message.language };",
    "const fheroes2::Text body{ _( sign->message.text ), fheroes2::FontType::normalWhite(), sign->message.language };",
)
replace_once(
    "src/fheroes2/heroes/heroes_action.cpp",
    "const fheroes2::Text body( mapEvent->message, fheroes2::FontType::normalWhite(), Settings::Get().getCurrentMapInfo().getSupportedLanguage() );",
    "const fheroes2::Text body( _( mapEvent->message ), fheroes2::FontType::normalWhite(), Settings::Get().getCurrentMapInfo().getSupportedLanguage() );",
)

# Scheduled map/campaign events shown at the beginning of a turn.
replace_once(
    "src/fheroes2/game/game_startgame.cpp",
    "fheroes2::Text( event.title, fheroes2::FontType::normalYellow(), language )",
    "fheroes2::Text( _( event.title ), fheroes2::FontType::normalYellow(), language )",
)
replace_once(
    "src/fheroes2/game/game_startgame.cpp",
    "fheroes2::Text( event.message, fheroes2::FontType::normalWhite(), language )",
    "fheroes2::Text( _( event.message ), fheroes2::FontType::normalWhite(), language )",
)
replace_once(
    "src/fheroes2/game/game_startgame.cpp",
    "const fheroes2::Text header( event.title, fheroes2::FontType::normalYellow(), language );",
    "const fheroes2::Text header( _( event.title ), fheroes2::FontType::normalYellow(), language );",
)
replace_once(
    "src/fheroes2/game/game_startgame.cpp",
    "const fheroes2::Text body( event.message, fheroes2::FontType::normalWhite(), language );",
    "const fheroes2::Text body( _( event.message ), fheroes2::FontType::normalWhite(), language );",
)

# Custom tavern rumors from the current map.
replace_once(
    "src/fheroes2/castle/castle_tavern.cpp",
    "text->add( fheroes2::Text{ std::move( rumor.text ), fheroes2::FontType::normalWhite(), rumor.language } );",
    "text->add( fheroes2::Text{ _( rumor.text ), fheroes2::FontType::normalWhite(), rumor.language } );",
)

print("Patched dynamic map text display paths to use gettext.")
