# UTF-8 large alphabet prototype

This branch is a Korean integration and proof build on top of a language-agnostic UTF-8 text path.

The reusable engine pieces are:

- `src/engine/utf8.*`: UTF-8 decoding and byte-boundary helpers.
- `CodePage::UTF8`: opt-in encoding marker while legacy single-byte code pages remain unchanged.
- `FontCharHandler`: UTF-8 streaming decode integrated into the existing text layout/rendering path.
- `ui_large_alphabet.*`: code-point-to-sprite provider for fixed-cell, left-to-right large alphabets.
- `script/generate_large_alphabet_font.py`: language-agnostic generator that accepts arbitrary Unicode ranges and/or a PO repertoire.

The Korean proof layer is intentionally separate in concept:

- `SupportedLanguage::Korean` and locale registration.
- `files/lang/ko.po` and UTF-8 `ko.mo` generation.
- The Galmuri test font download and the `AC00:D7A3` repertoire selection in the prototype workflow.

The renderer work does not attempt full Unicode shaping, bidirectional layout, grapheme-cluster editing, or fallback font selection. The current target is UTF-8 encoded, left-to-right, glyph-per-code-point text while keeping all existing single-byte translations on their current code-page path.
