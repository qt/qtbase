#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from condition_simplifier import simplify_condition


def validate_simplify(input: str, expected: str) -> None:
    output = simplify_condition(input)
    assert output == expected


def validate_simplify_unchanged(input: str) -> None:
    validate_simplify(input, input)


def test_simplify_on():
    validate_simplify_unchanged('ON')


def test_simplify_off():
    validate_simplify_unchanged('OFF')


def test_simplify_not_on():
    validate_simplify('NOT ON', 'OFF')


def test_simplify_not_off():
    validate_simplify('NOT OFF', 'ON')


def test_simplify_isEmpty():
    validate_simplify_unchanged('isEmpty(foo)')


def test_simplify_not_isEmpty():
    validate_simplify_unchanged('NOT isEmpty(foo)')


def test_simplify_simple_and():
    validate_simplify_unchanged('QT_FEATURE_bar AND QT_FEATURE_foo')


def test_simplify_simple_or():
    validate_simplify_unchanged('QT_FEATURE_bar OR QT_FEATURE_foo')


def test_simplify_simple_not():
    validate_simplify_unchanged('NOT QT_FEATURE_foo')


def test_simplify_simple_and_reorder():
    validate_simplify('QT_FEATURE_foo AND QT_FEATURE_bar', 'QT_FEATURE_bar AND QT_FEATURE_foo')


def test_simplify_simple_or_reorder():
    validate_simplify('QT_FEATURE_foo OR QT_FEATURE_bar', 'QT_FEATURE_bar OR QT_FEATURE_foo')


def test_simplify_unix_or_win32():
    validate_simplify('WIN32 OR UNIX', 'ON')


def test_simplify_unix_or_win32_or_foobar_or_barfoo():
    validate_simplify('WIN32 OR UNIX OR foobar OR barfoo', 'ON')


def test_simplify_not_not_bar():
    validate_simplify('  NOT NOT bar   ', 'bar')


def test_simplify_not_unix():
    validate_simplify('NOT UNIX', 'WIN32')


def test_simplify_not_win32():
    validate_simplify('NOT WIN32', 'UNIX')


def test_simplify_unix_and_win32():
    validate_simplify('WIN32 AND UNIX', 'OFF')


def test_simplify_unix_or_win32():
    validate_simplify('WIN32 OR UNIX', 'ON')


def test_simplify_unix_and_win32_or_foobar_or_barfoo():
    validate_simplify('WIN32 AND foobar AND UNIX AND barfoo', 'OFF')


def test_simplify_watchos_and_win32():
    validate_simplify('WATCHOS AND WIN32', 'OFF')


def test_simplify_win32_and_watchos():
    validate_simplify('WIN32 AND WATCHOS', 'OFF')


def test_simplify_apple_and_appleosx():
    validate_simplify('APPLE AND MACOS', 'MACOS')


def test_simplify_apple_or_appleosx():
    validate_simplify('APPLE OR MACOS', 'APPLE')


def test_simplify_apple_or_appleosx_level1():
    validate_simplify('foobar AND (APPLE OR MACOS )', 'APPLE AND foobar')


def test_simplify_apple_or_appleosx_level1_double():
    validate_simplify('foobar AND (APPLE OR MACOS )', 'APPLE AND foobar')


def test_simplify_apple_or_appleosx_level1_double_with_extra_spaces():
    validate_simplify('foobar AND (APPLE OR MACOS ) '
                      'AND ( MACOS    OR APPLE    )', 'APPLE AND foobar')


def test_simplify_apple_or_appleosx_level2():
    validate_simplify('foobar AND ( ( APPLE OR WATCHOS ) '
                      'OR MACOS ) AND ( MACOS    OR APPLE    ) '
                      'AND ( (WIN32 OR WINRT) OR UNIX) ', 'APPLE AND foobar')


def test_simplify_not_apple_and_appleosx():
    validate_simplify('NOT APPLE AND MACOS', 'OFF')


def test_simplify_unix_and_bar_or_win32():
    validate_simplify('WIN32 AND bar AND UNIX', 'OFF')


def test_simplify_unix_or_bar_or_win32():
    validate_simplify('WIN32 OR bar OR UNIX', 'ON')


def test_simplify_complex_true():
    validate_simplify('WIN32     OR (    APPLE  OR  UNIX)', 'ON')


def test_simplify_apple_unix_freebsd():
    validate_simplify('(    APPLE  OR  ( UNIX OR FREEBSD ))', 'UNIX')


def test_simplify_apple_unix_freebsd_foobar():
    validate_simplify('(    APPLE  OR  ( UNIX OR FREEBSD ) OR foobar)',
                      'UNIX OR foobar')


def test_simplify_complex_false():
    validate_simplify('WIN32 AND foobar    AND (    '
                      'APPLE  OR  ( UNIX OR FREEBSD ))',
                      'OFF')


def test_simplify_android_not_apple():
    validate_simplify('ANDROID AND NOT MACOS', 'ANDROID')
