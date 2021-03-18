#!/bin/sh

#############################################################################
##
## Copyright (C) 2016 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

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
