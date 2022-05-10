#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
from pro2cmake import map_condition
from qmake_parser import QmakeParser
from condition_simplifier import simplify_condition


_tests_path = os.path.dirname(os.path.abspath(__file__))


def validate_op(key, op, value, to_validate):
    assert key == to_validate['key']
    assert op == to_validate['operation']['value']
    assert value == to_validate.get('value', None)


def validate_single_op(key, op, value, to_validate):
    assert len(to_validate) == 1
    validate_op(key, op, value, to_validate[0])


def evaluate_condition(to_validate):
    assert 'condition' in to_validate
    assert 'statements' in to_validate

    return (to_validate['condition'],
            to_validate['statements'],
            to_validate.get('else_statements', {}))


def validate_default_else_test(file_name):
    result = parse_file(file_name)
    assert len(result) == 1

    (cond, if_branch, else_branch) = evaluate_condition(result[0])
    assert cond == 'qtConfig(timezone)'
    validate_single_op('A', '=', ['1'], if_branch)

    assert len(else_branch) == 1
    (cond2, if2_branch, else2_branch) = evaluate_condition(else_branch[0])
    assert cond2 == 'win32'
    validate_single_op('B', '=', ['2'], if2_branch)
    validate_single_op('C', '=', ['3'], else2_branch)


def parse_file(file):
    p = QmakeParser(debug=True)
    result, _ = p.parseFile(file)

    print('\n\n#### Parser result:')
    print(result)
    print('\n#### End of parser result.\n')

    print('\n\n####Parser result dictionary:')
    print(result.asDict())
    print('\n#### End of parser result dictionary.\n')

    result_dictionary = result.asDict()

    assert len(result_dictionary) == 1

    return result_dictionary['statements']


def test_else():
    result = parse_file(_tests_path + '/data/else.pro')
    assert len(result) == 1

    (cond, if_branch, else_branch) = evaluate_condition(result[0])

    assert cond == 'linux'
    validate_single_op('SOURCES', '+=', ['a.cpp'], if_branch)
    validate_single_op('SOURCES', '+=', ['b.cpp'], else_branch)


def test_else2():
    result = parse_file(_tests_path + '/data/else2.pro')
    assert len(result) == 1

    (cond, if_branch, else_branch) = evaluate_condition(result[0])
    assert cond == 'osx'
    validate_single_op('A', '=', ['1'], if_branch)

    assert len(else_branch) == 1
    (cond2, if2_branch, else2_branch) = evaluate_condition(else_branch[0])
    assert cond2 == 'win32'
    validate_single_op('B', '=', ['2'], if2_branch)

    validate_single_op('C', '=', ['3'], else2_branch)


def test_else3():
    validate_default_else_test(_tests_path + '/data/else3.pro')


def test_else4():
    validate_default_else_test(_tests_path + '/data/else4.pro')


def test_else5():
    validate_default_else_test(_tests_path + '/data/else5.pro')


def test_else6():
    validate_default_else_test(_tests_path + '/data/else6.pro')


def test_else7():
    result = parse_file(_tests_path + '/data/else7.pro')
    assert len(result) == 1


def test_else8():
    validate_default_else_test(_tests_path + '/data/else8.pro')


def test_multiline_assign():
    result = parse_file(_tests_path + '/data/multiline_assign.pro')
    assert len(result) == 2
    validate_op('A', '=', ['42', '43', '44'], result[0])
    validate_op('B', '=', ['23'], result[1])


def test_include():
    result = parse_file(_tests_path + '/data/include.pro')
    assert len(result) == 3
    validate_op('A', '=', ['42'], result[0])
    include = result[1]
    assert len(include) == 1
    assert 'included' in include
    assert include['included'].get('value', '') == 'foo'
    validate_op('B', '=', ['23'], result[2])


def test_load():
    result = parse_file(_tests_path + '/data/load.pro')
    assert len(result) == 3
    validate_op('A', '=', ['42'], result[0])
    load = result[1]
    assert len(load) == 1
    assert load.get('loaded', '') == 'foo'
    validate_op('B', '=', ['23'], result[2])


def test_definetest():
    result = parse_file(_tests_path + '/data/definetest.pro')
    assert len(result) == 1
    assert result[0] == []


def test_for():
    result = parse_file(_tests_path + '/data/for.pro')
    assert len(result) == 2
    validate_op('SOURCES', '=', ['main.cpp'], result[0])
    assert result[1] == []


def test_single_line_for():
    result = parse_file(_tests_path + '/data/single_line_for.pro')
    assert len(result) == 1
    assert result[0] == []


def test_unset():
    result = parse_file(_tests_path + '/data/unset.pro')
    assert len(result) == 1
    assert result[0] == []


def test_quoted():
    result = parse_file(_tests_path + '/data/quoted.pro')
    assert len(result) == 1


def test_complex_values():
    result = parse_file(_tests_path + '/data/complex_values.pro')
    assert len(result) == 1


def test_function_if():
    result = parse_file(_tests_path + '/data/function_if.pro')
    assert len(result) == 1


def test_realworld_standardpaths():
    result = parse_file(_tests_path + '/data/standardpaths.pro')

    (cond, if_branch, else_branch) = evaluate_condition(result[0])
    assert cond == 'win32'
    assert len(if_branch) == 1
    assert len(else_branch) == 1

    # win32:
    (cond1, if_branch1, else_branch1) = evaluate_condition(if_branch[0])
    assert cond1 == '!winrt'
    assert len(if_branch1) == 1
    validate_op('SOURCES', '+=', ['io/qstandardpaths_win.cpp'], if_branch1[0])
    assert len(else_branch1) == 1
    validate_op('SOURCES', '+=', ['io/qstandardpaths_winrt.cpp'], else_branch1[0])

    # unix:
    (cond2, if_branch2, else_branch2) = evaluate_condition(else_branch[0])
    assert cond2 == 'unix'
    assert len(if_branch2) == 1
    assert len(else_branch2) == 0

    # mac / else:
    (cond3, if_branch3, else_branch3) = evaluate_condition(if_branch2[0])
    assert cond3 == 'mac'
    assert len(if_branch3) == 1
    validate_op('OBJECTIVE_SOURCES', '+=', ['io/qstandardpaths_mac.mm'], if_branch3[0])
    assert len(else_branch3) == 1

    # android / else:
    (cond4, if_branch4, else_branch4) = evaluate_condition(else_branch3[0])
    assert cond4 == 'android'
    assert len(if_branch4) == 1
    validate_op('SOURCES', '+=', ['io/qstandardpaths_android.cpp'], if_branch4[0])
    assert len(else_branch4) == 1

    # haiku / else:
    (cond5, if_branch5, else_branch5) = evaluate_condition(else_branch4[0])
    assert cond5 == 'haiku'
    assert len(if_branch5) == 1
    validate_op('SOURCES', '+=', ['io/qstandardpaths_haiku.cpp'], if_branch5[0])
    assert len(else_branch5) == 1
    validate_op('SOURCES', '+=', ['io/qstandardpaths_unix.cpp'], else_branch5[0])


def test_realworld_comment_scope():
    result = parse_file(_tests_path + '/data/comment_scope.pro')
    assert len(result) == 2
    (cond, if_branch, else_branch) = evaluate_condition(result[0])
    assert cond == 'freebsd|openbsd'
    assert len(if_branch) == 1
    validate_op('QMAKE_LFLAGS_NOUNDEF', '=', [], if_branch[0])

    assert 'included' in result[1]
    assert result[1]['included'].get('value', '') == 'animation/animation.pri'


def test_realworld_contains_scope():
    result = parse_file(_tests_path + '/data/contains_scope.pro')
    assert len(result) == 2


def test_realworld_complex_assign():
    result = parse_file(_tests_path + '/data/complex_assign.pro')
    assert len(result) == 1
    validate_op('qmake-clean.commands', '+=', '( cd qmake && $(MAKE) clean ":-(==)-:" \'(Foo)\' )'.split(),
                result[0])


def test_realworld_complex_condition():
    result = parse_file(_tests_path + '/data/complex_condition.pro')
    assert len(result) == 1
    (cond, if_branch, else_branch) = evaluate_condition(result[0])
    assert cond == '!system("dbus-send --session --type=signal / ' \
                   'local.AutotestCheck.Hello >$$QMAKE_SYSTEM_NULL_DEVICE ' \
                   '2>&1")'
    assert len(if_branch) == 1
    validate_op('SOURCES', '=', ['dbus.cpp'], if_branch[0])

    assert len(else_branch) == 0


def test_realworld_sql():
    result = parse_file(_tests_path + '/data/sql.pro')
    assert len(result) == 2
    validate_op('TEMPLATE', '=', ['subdirs'], result[0])
    validate_op('SUBDIRS', '=', ['kernel'], result[1])


def test_realworld_qtconfig():
    result = parse_file(_tests_path + '/data/escaped_value.pro')
    assert len(result) == 1
    validate_op('MODULE_AUX_INCLUDES', '=', ['\\$\\$QT_MODULE_INCLUDE_BASE/QtANGLE'], result[0])


def test_realworld_lc():
    result = parse_file(_tests_path + '/data/lc.pro')
    assert len(result) == 3


def test_realworld_lc_with_comment_in_between():
    result = parse_file(_tests_path + '/data/lc_with_comment.pro')

    my_var = result[1]['value'][0]
    assert my_var == 'foo'

    my_var = result[2]['value'][0]
    assert my_var == 'foo2'

    my_var = result[3]['value'][0]
    assert my_var == 'foo3'

    my_var = result[4]['value'][0]
    assert my_var == 'foo4'

    my_var = result[5]['value'][0]
    assert my_var == 'foo5'

    sub_dirs = result[0]['value']
    assert sub_dirs[0] == 'tga'
    assert sub_dirs[1] == 'wbmp'
    assert len(result) == 6


def test_condition_without_scope():
    result = parse_file(_tests_path + '/data/condition_without_scope.pro')
    assert len(result) == 1


def test_multi_condition_divided_by_lc():
    result = parse_file(_tests_path + '/data/multi_condition_divided_by_lc.pro')
    assert len(result) == 1


def test_nested_function_calls():
    result = parse_file(_tests_path + '/data/nested_function_calls.pro')
    assert len(result) == 1

def test_value_function():
    result = parse_file(_tests_path + '/data/value_function.pro')
    target = result[0]['value'][0]
    assert target == 'Dummy'
    value = result[1]['value']
    assert value[0] == '$$TARGET'


def test_condition_operator_precedence():
    result = parse_file(_tests_path + '/data/condition_operator_precedence.pro')

    def validate_simplify(input_str: str, expected: str) -> None:
        output = simplify_condition(map_condition(input_str))
        assert output == expected

    validate_simplify(result[0]["condition"], "a1 OR a2")
    validate_simplify(result[1]["condition"], "b3 AND (b1 OR b2)")
    validate_simplify(result[2]["condition"], "c4 OR (c1 AND c3) OR (c2 AND c3)")
