# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import re

def _convert_pattern(pattern):
    # patterns from http://www.unicode.org/reports/tr35/#Date_Format_Patterns
    qt_regexps = {
        r"yyy{3,}" : "yyyy", # more that three digits hence convert to four-digit year
        r"L" : "M",          # stand-alone month names. not supported.
        r"g{1,}": "",        # modified julian day. not supported.
        r"S{1,}" : "",       # fractional seconds. not supported.
        r"A{1,}" : ""        # milliseconds in day. not supported.
    }
    qt_patterns = {
        "G" : "", "GG" : "", "GGG" : "", "GGGG" : "", "GGGGG" : "", # Era. not supported.
        "y" : "yyyy", # four-digit year without leading zeroes
        "Q" : "", "QQ" : "", "QQQ" : "", "QQQQ" : "", # quarter. not supported.
        "q" : "", "qq" : "", "qqq" : "", "qqqq" : "", # quarter. not supported.
        "MMMMM" : "MMM", # narrow month name.
        "LLLLL" : "MMM", # stand-alone narrow month name.
        "l" : "", # special symbol for chinese leap month. not supported.
        "w" : "", "W" : "", # week of year/month. not supported.
        "D" : "", "DD" : "", "DDD" : "", # day of year. not supported.
        "F" : "", # day of week in month. not supported.
        "E" : "ddd", "EE" : "ddd", "EEE" : "ddd", "EEEEE" : "ddd", "EEEE" : "dddd", # day of week
        "e" : "ddd", "ee" : "ddd", "eee" : "ddd", "eeeee" : "ddd", "eeee" : "dddd", # local day of week
        "c" : "ddd", "cc" : "ddd", "ccc" : "ddd", "ccccc" : "ddd", "cccc" : "dddd", # stand-alone local day of week
        "a" : "AP", # AM/PM
        "K" : "h", # Hour 0-11
        "k" : "H", # Hour 1-24
        "j" : "", # special reserved symbol.
        "z" : "t", "zz" : "t", "zzz" : "t", "zzzz" : "t", # timezone
        "Z" : "t", "ZZ" : "t", "ZZZ" : "t", "ZZZZ" : "t", # timezone
        "v" : "t", "vv" : "t", "vvv" : "t", "vvvv" : "t", # timezone
        "V" : "t", "VV" : "t", "VVV" : "t", "VVVV" : "t"  # timezone
    }
    if pattern in qt_patterns:
        return qt_patterns[pattern]
    for r,v in qt_regexps.items():
        pattern = re.sub(r, v, pattern)
    return pattern

def convert_date(input):
    result = ""
    patterns = "GyYuQqMLlwWdDFgEecahHKkjmsSAzZvV"
    last = ""
    inquote = 0
    chars_to_strip = " -"
    for c in input:
        if c == "'":
            inquote = inquote + 1
        if inquote % 2 == 0:
            if c in patterns:
                if not last:
                    last = c
                else:
                    if c in last:
                        last += c
                    else:
                        # pattern changed
                        converted = _convert_pattern(last)
                        result += converted
                        if not converted:
                            result = result.rstrip(chars_to_strip)
                        last = c
                continue
        if last:
            # pattern ended
            converted = _convert_pattern(last)
            result += converted
            if not converted:
                result = result.rstrip(chars_to_strip)
            last = ""
        result += c
    if last:
        converted = _convert_pattern(last)
        result += converted
        if not converted:
            result = result.rstrip(chars_to_strip)
    return result.lstrip(chars_to_strip)
