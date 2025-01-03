#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#
# This script generates the QFontDatabase::WritingSystem enum.  It
# uses the Unicode 4.0 Scripts.txt data file as the source, with the
# following modifications: 
#
# * Inherited is removed
# * East Asian scripts (chapter 11) are renamed to: SimplifiedChinese,
#   TraditionalChinese, Japanese, Korean, Vietnamese
# * Additiona Modern scripts (chapter 12) are removed
# * Archaic scripts (chapter 13) are removed

grep -Ev "(^[[:space:]]*#|^$)" data/Scripts.txt \
          | awk '{print $3}' \
          | grep -Ev "(Inherited|Hangul|Ogham|Old_Italic|Runic|Gothic|Ugaritic|Linear_B|Cypriot|Katakana_Or_Hiragana|Ethiopic|Mongolian|Osmanya|Cherokee|Canadian_Aboriginal|Deseret|Shavian)" \
          | sed -e s,_,,g -e 's,^Common$,Any,' -e 's,^Hiragana$,SimplifiedChinese NEWLINE TraditionalChinese,' -e 's,^Katakana$,Japanese,' -e 's,^Bopomofo$,Korean,' -e 's,^Han$,Vietnamese,' -e 's,^#$,,' \
          | uniq > writingSystems
echo "" >> writingSystems
echo "Other" >> writingSystems
