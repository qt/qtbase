#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from pro2cmake import Scope, SetOperation, merge_scopes, recursive_evaluate_scope

import pytest
import typing

ScopeList = typing.List[Scope]

def _map_to_operation(**kwargs):
    result = {}  # type: typing.Mapping[str, typing.List[SetOperation]]
    for (key, value) in kwargs.items():
        result[key] = [SetOperation([value])]
    return result


def _new_scope(*, parent_scope=None, condition='', **kwargs) -> Scope:
    return Scope(parent_scope=parent_scope,
                 qmake_file='file1', condition=condition, operations=_map_to_operation(**kwargs))


def _evaluate_scopes(scopes: ScopeList) -> ScopeList:
    for s in scopes:
        if not s.parent:
            recursive_evaluate_scope(s)
    return scopes


def _validate(input_scopes: ScopeList, output_scopes: ScopeList):
    merged_scopes = merge_scopes(input_scopes)
    assert merged_scopes == output_scopes


def test_evaluate_one_scope():
    scope = _new_scope(condition='QT_FEATURE_foo', test1='bar')

    input_scope = scope
    recursive_evaluate_scope(scope)
    assert scope == input_scope


def test_evaluate_child_scope():
    scope = _new_scope(condition='QT_FEATURE_foo', test1='bar')
    _new_scope(parent_scope=scope, condition='QT_FEATURE_bar', test2='bar')

    input_scope = scope
    recursive_evaluate_scope(scope)

    assert scope.total_condition == 'QT_FEATURE_foo'
    assert len(scope.children) == 1
    assert scope.get_string('test1') == 'bar'
    assert scope.get_string('test2', 'not found') == 'not found'

    child = scope.children[0]
    assert child.total_condition == 'QT_FEATURE_bar AND QT_FEATURE_foo'
    assert child.get_string('test1', 'not found') == 'not found'
    assert child.get_string('test2') == 'bar'


def test_evaluate_two_child_scopes():
    scope = _new_scope(condition='QT_FEATURE_foo', test1='bar')
    _new_scope(parent_scope=scope, condition='QT_FEATURE_bar', test2='bar')
    _new_scope(parent_scope=scope, condition='QT_FEATURE_buz', test3='buz')

    input_scope = scope
    recursive_evaluate_scope(scope)

    assert scope.total_condition == 'QT_FEATURE_foo'
    assert len(scope.children) == 2
    assert scope.get_string('test1') == 'bar'
    assert scope.get_string('test2', 'not found') == 'not found'
    assert scope.get_string('test3', 'not found') == 'not found'

    child1 = scope.children[0]
    assert child1.total_condition == 'QT_FEATURE_bar AND QT_FEATURE_foo'
    assert child1.get_string('test1', 'not found') == 'not found'
    assert child1.get_string('test2') == 'bar'
    assert child1.get_string('test3', 'not found') == 'not found'

    child2 = scope.children[1]
    assert child2.total_condition == 'QT_FEATURE_buz AND QT_FEATURE_foo'
    assert child2.get_string('test1', 'not found') == 'not found'
    assert child2.get_string('test2') == ''
    assert child2.get_string('test3', 'not found') == 'buz'


def test_evaluate_else_child_scopes():
    scope = _new_scope(condition='QT_FEATURE_foo', test1='bar')
    _new_scope(parent_scope=scope, condition='QT_FEATURE_bar', test2='bar')
    _new_scope(parent_scope=scope, condition='else', test3='buz')

    input_scope = scope
    recursive_evaluate_scope(scope)

    assert scope.total_condition == 'QT_FEATURE_foo'
    assert len(scope.children) == 2
    assert scope.get_string('test1') == 'bar'
    assert scope.get_string('test2', 'not found') == 'not found'
    assert scope.get_string('test3', 'not found') == 'not found'

    child1 = scope.children[0]
    assert child1.total_condition == 'QT_FEATURE_bar AND QT_FEATURE_foo'
    assert child1.get_string('test1', 'not found') == 'not found'
    assert child1.get_string('test2') == 'bar'
    assert child1.get_string('test3', 'not found') == 'not found'

    child2 = scope.children[1]
    assert child2.total_condition == 'QT_FEATURE_foo AND NOT QT_FEATURE_bar'
    assert child2.get_string('test1', 'not found') == 'not found'
    assert child2.get_string('test2') == ''
    assert child2.get_string('test3', 'not found') == 'buz'


def test_evaluate_invalid_else_child_scopes():
    scope = _new_scope(condition='QT_FEATURE_foo', test1='bar')
    _new_scope(parent_scope=scope, condition='else', test3='buz')
    _new_scope(parent_scope=scope, condition='QT_FEATURE_bar', test2='bar')

    input_scope = scope
    with pytest.raises(AssertionError):
        recursive_evaluate_scope(scope)


def test_merge_empty_scope_list():
    _validate([], [])


def test_merge_one_scope():
    scopes = [_new_scope(test='foo')]

    recursive_evaluate_scope(scopes[0])

    _validate(scopes, scopes)


def test_merge_one_on_scope():
    scopes = [_new_scope(condition='ON', test='foo')]

    recursive_evaluate_scope(scopes[0])

    _validate(scopes, scopes)


def test_merge_one_off_scope():
    scopes = [_new_scope(condition='OFF', test='foo')]

    recursive_evaluate_scope(scopes[0])

    _validate(scopes, [])


def test_merge_one_conditioned_scope():
    scopes = [_new_scope(condition='QT_FEATURE_foo', test='foo')]

    recursive_evaluate_scope(scopes[0])

    _validate(scopes, scopes)


def test_merge_two_scopes_with_same_condition():
    scopes = [_new_scope(condition='QT_FEATURE_bar', test='foo'),
              _new_scope(condition='QT_FEATURE_bar', test2='bar')]

    recursive_evaluate_scope(scopes[0])
    recursive_evaluate_scope(scopes[1])

    result = merge_scopes(scopes)

    assert len(result) == 1
    r0 = result[0]
    assert r0.total_condition == 'QT_FEATURE_bar'
    assert r0.get_string('test') == 'foo'
    assert r0.get_string('test2') == 'bar'


def test_merge_three_scopes_two_with_same_condition():
    scopes = [_new_scope(condition='QT_FEATURE_bar', test='foo'),
              _new_scope(condition='QT_FEATURE_baz', test1='buz'),
              _new_scope(condition='QT_FEATURE_bar', test2='bar')]

    recursive_evaluate_scope(scopes[0])
    recursive_evaluate_scope(scopes[1])
    recursive_evaluate_scope(scopes[2])

    result = merge_scopes(scopes)

    assert len(result) == 2
    r0 = result[0]
    assert r0.total_condition == 'QT_FEATURE_bar'
    assert r0.get_string('test') == 'foo'
    assert r0.get_string('test2') == 'bar'

    assert result[1] == scopes[1]


def test_merge_two_unrelated_on_off_scopes():
    scopes = [_new_scope(condition='ON', test='foo'),
              _new_scope(condition='OFF', test2='bar')]

    recursive_evaluate_scope(scopes[0])
    recursive_evaluate_scope(scopes[1])

    _validate(scopes, [scopes[0]])


def test_merge_two_unrelated_on_off_scopes():
    scopes = [_new_scope(condition='OFF', test='foo'),
              _new_scope(condition='ON', test2='bar')]

    recursive_evaluate_scope(scopes[0])
    recursive_evaluate_scope(scopes[1])

    _validate(scopes, [scopes[1]])


def test_merge_parent_child_scopes_with_different_conditions():
    scope = _new_scope(condition='FOO', test1='parent')
    scopes = [scope, _new_scope(parent_scope=scope, condition='bar', test2='child')]

    recursive_evaluate_scope(scope)

    _validate(scopes, scopes)


def test_merge_parent_child_scopes_with_same_conditions():
    scope = _new_scope(condition='FOO AND bar', test1='parent')
    scopes = [scope, _new_scope(parent_scope=scope, condition='FOO AND bar', test2='child')]

    recursive_evaluate_scope(scope)

    result = merge_scopes(scopes)

    assert len(result) == 1
    r0 = result[0]
    assert r0.parent == None
    assert r0.total_condition == 'FOO AND bar'
    assert r0.get_string('test1') == 'parent'
    assert r0.get_string('test2') == 'child'


def test_merge_parent_child_scopes_with_on_child_condition():
    scope = _new_scope(condition='FOO AND bar', test1='parent')
    scopes = [scope, _new_scope(parent_scope=scope, condition='ON', test2='child')]

    recursive_evaluate_scope(scope)

    result = merge_scopes(scopes)

    assert len(result) == 1
    r0 = result[0]
    assert r0.parent == None
    assert r0.total_condition == 'FOO AND bar'
    assert r0.get_string('test1') == 'parent'
    assert r0.get_string('test2') == 'child'


# Real world examples:

# qstandardpaths selection:

def test_qstandardpaths_scopes():
    # top level:
    scope1 = _new_scope(condition='ON', scope_id=1)

    # win32 {
    scope2 = _new_scope(parent_scope=scope1, condition='WIN32')
    #     !winrt {
    #         SOURCES += io/qstandardpaths_win.cpp
    scope3 = _new_scope(parent_scope=scope2, condition='NOT WINRT',
                        SOURCES='qsp_win.cpp')
    #     } else {
    #         SOURCES += io/qstandardpaths_winrt.cpp
    scope4 = _new_scope(parent_scope=scope2, condition='else',
                        SOURCES='qsp_winrt.cpp')
    #     }
    # else: unix {
    scope5 = _new_scope(parent_scope=scope1, condition='else')
    scope6 = _new_scope(parent_scope=scope5, condition='UNIX')
    #     mac {
    #         OBJECTIVE_SOURCES += io/qstandardpaths_mac.mm
    scope7 = _new_scope(parent_scope=scope6, condition='MACOS', SOURCES='qsp_mac.mm')
    #     } else:android {
    #         SOURCES += io/qstandardpaths_android.cpp
    scope8 = _new_scope(parent_scope=scope6, condition='else')
    scope9 = _new_scope(parent_scope=scope8, condition='ANDROID AND NOT UNKNOWN_PLATFORM', SOURCES='qsp_android.cpp')
    #     } else:haiku {
    #              SOURCES += io/qstandardpaths_haiku.cpp
    scope10 = _new_scope(parent_scope=scope8, condition='else')
    scope11 = _new_scope(parent_scope=scope10, condition='HAIKU', SOURCES='qsp_haiku.cpp')
    #     } else {
    #         SOURCES +=io/qstandardpaths_unix.cpp
    scope12 = _new_scope(parent_scope=scope10, condition='else', SOURCES='qsp_unix.cpp')
    #     }
    # }

    recursive_evaluate_scope(scope1)

    assert scope1.total_condition == 'ON'
    assert scope2.total_condition == 'WIN32'
    assert scope3.total_condition == 'WIN32 AND NOT WINRT'
    assert scope4.total_condition == 'WINRT'
    assert scope5.total_condition == 'UNIX'
    assert scope6.total_condition == 'UNIX'
    assert scope7.total_condition == 'MACOS'
    assert scope8.total_condition == 'UNIX AND NOT MACOS'
    assert scope9.total_condition == 'ANDROID AND NOT UNKNOWN_PLATFORM'
    assert scope10.total_condition == 'UNIX AND NOT MACOS AND (UNKNOWN_PLATFORM OR NOT ANDROID)'
    assert scope11.total_condition == 'HAIKU AND (UNKNOWN_PLATFORM OR NOT ANDROID)'
    assert scope12.total_condition == 'UNIX AND NOT HAIKU AND NOT MACOS AND (UNKNOWN_PLATFORM OR NOT ANDROID)'

def test_recursive_expansion():
    scope = _new_scope(A='Foo',B='$$A/Bar')
    assert scope.get_string('A') == 'Foo'
    assert scope.get_string('B') == '$$A/Bar'
    assert scope._expand_value('$$B/Source.cpp') == ['Foo/Bar/Source.cpp']
    assert scope._expand_value('$$B') == ['Foo/Bar']
