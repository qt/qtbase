#!/usr/bin/env python3

# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

# Scrapes the copyright notices from the header comments of the libjpeg-turbo
# sources in src/ and writes a merged list (one entry per holder, with all of
# that holder's year specifications combined) to COPYRIGHT.txt.
#
# Usage: generate_copyright.py [TARGET_DIR]
# TARGET_DIR defaults to this script's own directory; it must contain a
# src/ subdirectory with the imported libjpeg-turbo sources.

import re
import sys
from pathlib import Path

BASE_DIR = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent
SRC_DIR = BASE_DIR / "src"
OUTPUT_FILE = BASE_DIR / "COPYRIGHT.txt"

COPYRIGHT_LINE_RE = re.compile(
    r'^Copyright\s+(?:\(C\)\s*)?'
    r'((?:\d{4}(?:-\d{4})?\s*,\s*)*\d{4}(?:-\d{4})?)'
    r'\s*,?\s+(.+?)\.?\s*$'
)

YEARS_OPEN_RE = re.compile(r'[\d,]\s*$')
EMAIL_RE = re.compile(r'\s*<[^>]*>')
ABBREVIATION_RE = re.compile(r'\b(?:Inc|Ltd|Corp|Co)$')


def header_comment(text):
    m = re.search(r'/\*(.*?)\*/', text, re.DOTALL)
    return m.group(1) if m else ''


def comment_lines(comment):
    lines = []
    for raw in comment.split('\n'):
        line = raw.strip()
        if line.startswith('*'):
            line = line[1:].strip()
        lines.append(line)
    return lines


def copyright_statements(lines):
    # Joins wrapped copyright lines, e.g. a year list that continues onto the
    # next line before the holder's name.
    statements = []
    current = None
    for line in lines:
        if line.startswith('Copyright'):
            if current is not None:
                statements.append(current)
            current = line
        elif current is not None and YEARS_OPEN_RE.search(current):
            current = current + ' ' + line
        else:
            if current is not None:
                statements.append(current)
                current = None
    if current is not None:
        statements.append(current)
    return statements


def parse_years(years_text):
    years = set()
    for part in years_text.split(','):
        part = part.strip()
        if not part:
            continue
        if '-' in part:
            start, end = part.split('-')
            years.update(range(int(start), int(end) + 1))
        else:
            years.add(int(part))
    return years


def format_years(years):
    ordered = sorted(years)
    ranges = []
    start = prev = ordered[0]
    for y in ordered[1:]:
        if y == prev + 1:
            prev = y
            continue
        ranges.append((start, prev))
        start = prev = y
    ranges.append((start, prev))
    return ', '.join(str(a) if a == b else f"{a}-{b}" for a, b in ranges)


def main():
    holders = {}
    order = []

    for filename in sorted(SRC_DIR.glob('*.c')) + sorted(SRC_DIR.glob('*.h')):
        text = filename.read_text(encoding='utf-8', errors='replace')
        lines = comment_lines(header_comment(text))
        for statement in copyright_statements(lines):
            m = COPYRIGHT_LINE_RE.match(statement)
            if not m:
                print(f"warning: unparsed copyright line in {filename}: {statement!r}",
                      file=sys.stderr)
                continue
            years_text, holder = m.groups()
            holder = EMAIL_RE.sub('', holder)
            holder = re.sub(r'\s+', ' ', holder).strip()
            if ABBREVIATION_RE.search(holder):
                holder += '.'
            if holder not in holders:
                holders[holder] = set()
                order.append(holder)
            holders[holder].update(parse_years(years_text))

    with open(OUTPUT_FILE, 'w') as out:
        for holder in order:
            out.write(f"Copyright (C) {format_years(holders[holder])} {holder}\n")


if __name__ == '__main__':
    main()
