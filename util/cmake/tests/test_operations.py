#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2022 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:COMM$
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## $QT_END_LICENSE$
##
##
##
##
##
##
##
##
##############################################################################

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
