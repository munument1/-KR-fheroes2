import ast
import json
import re
from collections import Counter
from pathlib import Path

SOURCE = Path('files/lang/tr.po')
TARGET = Path('files/lang/ko.po')
PAYLOAD = Path('/tmp/ko_strings.json')
EXPECTED_COUNT = 3240
EXPECTED_PLURALS = 30


def qvalue(text):
    return ast.literal_eval(text.strip())


def blank_entry():
    return {
        'msgctxt': '',
        'msgid': '',
        'msgid_plural': '',
        'msgstr': '',
        'msgstr_plural': {},
        'flags': [],
        'refs': [],
        'extracted_comments': [],
    }


def parse_po(path):
    entries = []
    state = {'entry': blank_entry(), 'active': None}

    def flush():
        entry = state['entry']
        if entry['msgid'] or entry['msgid_plural']:
            entries.append(entry)
        state['entry'] = blank_entry()
        state['active'] = None

    def append_active(value):
        kind, index = state['active']
        entry = state['entry']
        if kind == 'msgstr_plural':
            entry['msgstr_plural'][index] = entry['msgstr_plural'].get(index, '') + value
        else:
            entry[kind] += value

    for raw in path.read_text(encoding='utf-8-sig').splitlines():
        line = raw.rstrip('\n')
        entry = state['entry']
        if not line.strip():
            flush()
            continue
        if line.startswith('#:'):
            entry['refs'].extend(line[2:].strip().split())
            state['active'] = None
            continue
        if line.startswith('#,'):
            entry['flags'].extend(x.strip() for x in line[2:].split(',') if x.strip())
            state['active'] = None
            continue
        if line.startswith('#.'):
            entry['extracted_comments'].append(line[2:].strip())
            state['active'] = None
            continue
        if line.startswith('#'):
            state['active'] = None
            continue
        if line.startswith('msgctxt '):
            entry['msgctxt'] = qvalue(line[len('msgctxt '):])
            state['active'] = ('msgctxt', None)
            continue
        if line.startswith('msgid_plural '):
            entry['msgid_plural'] = qvalue(line[len('msgid_plural '):])
            state['active'] = ('msgid_plural', None)
            continue
        if line.startswith('msgid '):
            entry['msgid'] = qvalue(line[len('msgid '):])
            state['active'] = ('msgid', None)
            continue
        if line.startswith('msgstr['):
            close = line.index(']')
            index = int(line[len('msgstr['):close])
            entry['msgstr_plural'][index] = qvalue(line[close + 1:].strip())
            state['active'] = ('msgstr_plural', index)
            continue
        if line.startswith('msgstr '):
            entry['msgstr'] = qvalue(line[len('msgstr '):])
            state['active'] = ('msgstr', None)
            continue
        if line.startswith('"') and state['active'] is not None:
            append_active(qvalue(line))
            continue
        raise RuntimeError(f'Unsupported PO line: {line!r}')
    flush()
    return entries


def po_quote(value):
    value = value.replace('\\', '\\\\').replace('"', '\\"')
    value = value.replace('\t', '\\t').replace('\r', '\\r').replace('\n', '\\n')
    return '"' + value + '"'


def tokens(text):
    token_re = re.compile(r'%\{[^}]+\}|%(?:\d+\$)?[diuoxXfFeEgGaAcspn%]')
    return Counter(token_re.findall(text))


korean = json.loads(PAYLOAD.read_text(encoding='utf-8'))
assert isinstance(korean, list)
assert len(korean) == EXPECTED_COUNT, len(korean)
assert all(isinstance(value, str) and value for value in korean)

source_entries = parse_po(SOURCE)
assert len(source_entries) == EXPECTED_COUNT, len(source_entries)
assert sum(bool(e['msgid_plural']) for e in source_entries) == EXPECTED_PLURALS

for index, (entry, target) in enumerate(zip(source_entries, korean), 1):
    singular = tokens(entry['msgid'])
    translated = tokens(target)
    if entry['msgid_plural']:
        plural = tokens(entry['msgid_plural'])
        assert singular == plural, ('source plural token mismatch', index)
        assert singular == translated, ('Korean token mismatch', index)
    else:
        assert singular == translated, ('Korean token mismatch', index)

out = [
    '# Korean translation for fheroes2',
    '# Copyright (C) 2026 fheroes2 team <fhomm2@gmail.com>',
    '# This file is distributed under the same license as the fheroes2 package.',
    '#',
    'msgid ""',
    'msgstr ""',
    '"Project-Id-Version: fheroes2\\n"',
    '"Report-Msgid-Bugs-To: \\n"',
    '"POT-Creation-Date: 2026-06-10 08:09+0000\\n"',
    '"PO-Revision-Date: 2026-08-14 15:38+0900\\n"',
    '"Language-Team: Korean\\n"',
    '"Language: ko\\n"',
    '"MIME-Version: 1.0\\n"',
    '"Content-Type: text/plain; charset=UTF-8\\n"',
    '"Content-Transfer-Encoding: 8bit\\n"',
    '"Plural-Forms: nplurals=1; plural=0;\\n"',
    '',
]

for entry, target in zip(source_entries, korean):
    for comment in entry['extracted_comments']:
        out.append('#. ' + comment)
    if entry['refs']:
        out.append('#: ' + ' '.join(entry['refs']))
    flags = [flag for flag in entry['flags'] if flag != 'fuzzy']
    if flags:
        out.append('#, ' + ', '.join(flags))
    if entry['msgctxt']:
        out.append('msgctxt ' + po_quote(entry['msgctxt']))
    out.append('msgid ' + po_quote(entry['msgid']))
    if entry['msgid_plural']:
        out.append('msgid_plural ' + po_quote(entry['msgid_plural']))
        out.append('msgstr[0] ' + po_quote(target))
    else:
        out.append('msgstr ' + po_quote(target))
    out.append('')

TARGET.write_text('\n'.join(out), encoding='utf-8', newline='\n')

generated = parse_po(TARGET)
assert len(generated) == EXPECTED_COUNT
assert sum(bool(e['msgid_plural']) for e in generated) == EXPECTED_PLURALS
assert [e['msgid'] for e in generated] == [e['msgid'] for e in source_entries]
assert [e['msgid_plural'] for e in generated] == [e['msgid_plural'] for e in source_entries]
assert all('fuzzy' not in e['flags'] for e in generated)

text = TARGET.read_text(encoding='utf-8')
assert '"Language: ko\\n"' in text
assert '"Plural-Forms: nplurals=1; plural=0;\\n"' in text

print(f'entries={len(generated)}')
print(f'plural_entries={sum(bool(e["msgid_plural"]) for e in generated)}')
print('placeholder_mismatches=0')
