#!/usr/bin/env python3

import argparse
import csv
from pathlib import Path

import polib


def main() -> None:
    parser = argparse.ArgumentParser(description="Merge translated embedded map text into the Korean PO catalog.")
    parser.add_argument("input_po", type=Path)
    parser.add_argument("map_csv", type=Path)
    parser.add_argument("output_po", type=Path)
    args = parser.parse_args()

    po = polib.pofile(str(args.input_po), encoding="utf-8")

    # Temporary test-build corrections that are not yet committed to the working branch catalog.
    sphinx = po.find("sphinx|'\n\nYour answer?")
    if sphinx is not None and sphinx.msgstr == "\n\n당신의 답은?":
        sphinx.msgstr = "'\n\n당신의 답은?"

    korean_name = po.find("Korean")
    if korean_name is None:
        po.append(polib.POEntry(msgid="Korean", msgstr="한국어"))
    elif not korean_name.msgstr:
        korean_name.msgstr = "한국어"

    existing = {entry.msgid: entry for entry in po if entry.msgid}
    added = 0
    overlaps = 0

    with args.map_csv.open("r", encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        required = {"source_text", "korean"}
        if not required.issubset(reader.fieldnames or []):
            raise SystemExit(f"Map CSV is missing required columns: {sorted(required)}")

        rows = list(reader)

    if len(rows) != 4254:
        raise SystemExit(f"Expected 4254 unique map strings, found {len(rows)}")

    for row in rows:
        msgid = row["source_text"]
        msgstr = row["korean"]
        if not msgid:
            raise SystemExit("Map CSV contains an empty source_text value.")
        if not msgstr:
            raise SystemExit(f"Map CSV contains an empty Korean translation for: {msgid!r}")

        entry = existing.get(msgid)
        if entry is not None:
            overlaps += 1
            if entry.msgid_plural:
                # A dynamic map string cannot safely replace a plural catalog entry.
                continue
            if entry.msgstr and entry.msgstr != msgstr:
                raise SystemExit(
                    "Conflicting Korean translation for existing msgid:\n"
                    f"  msgid: {msgid!r}\n"
                    f"  PO:    {entry.msgstr!r}\n"
                    f"  CSV:   {msgstr!r}"
                )
            if not entry.msgstr:
                entry.msgstr = msgstr
            continue

        entry = polib.POEntry(msgid=msgid, msgstr=msgstr, comment="Embedded map text")
        po.append(entry)
        existing[msgid] = entry
        added += 1

    args.output_po.parent.mkdir(parents=True, exist_ok=True)
    po.save(str(args.output_po), newline="\n")

    print(f"Map CSV rows: {len(rows)}")
    print(f"Existing catalog overlaps: {overlaps}")
    print(f"New map entries added: {added}")
    if overlaps != 110 or added != 4144:
        raise SystemExit(f"Unexpected merge totals: overlaps={overlaps}, added={added}")


if __name__ == "__main__":
    main()
