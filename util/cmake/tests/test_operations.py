#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from pro2cmake import AddOperation, SetOperation, UniqueAddOperation, RemoveOperation

def test_add_operation():
    op = AddOperation(['bar', 'buz'])

    result = op.process(['foo', 'bar'], ['foo', 'bar'], lambda x: x)
    assert ['foo', 'bar', 'bar', 'buz'] == result


def test_uniqueadd_operation():
    op = UniqueAddOperation(['bar', 'buz'])

    result = op.process(['foo', 'bar'], ['foo', 'bar'], lambda x: x)
    assert ['foo', 'bar', 'buz'] == result


def test_set_operation():
    op = SetOperation(['bar', 'buz'])

    result = op.process(['foo', 'bar'], ['foo', 'bar'], lambda x: x)
    assert ['bar', 'buz'] == result


def test_remove_operation():
    op = RemoveOperation(['bar', 'buz'])

    result = op.process(['foo', 'bar'], ['foo', 'bar'], lambda x: x)
    assert ['foo', '-buz'] == result
