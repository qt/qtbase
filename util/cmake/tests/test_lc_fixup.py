#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from qmake_parser import fixup_linecontinuation


def test_no_change():
    input = "test \\\nline2\n  line3"
    output = "test line2\n  line3"
    result = fixup_linecontinuation(input)
    assert output == result


def test_fix():
    input = "test \\\t\nline2\\\n  line3\\ \nline4 \\ \t\nline5\\\n\n\n"
    output = "test line2   line3 line4 line5 \n\n"
    result = fixup_linecontinuation(input)
    assert output == result
