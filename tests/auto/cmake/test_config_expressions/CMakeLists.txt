# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_config_expressions)

find_package(Qt6 REQUIRED)
include(QtFeature)

function(assert expected_value)
    set(expression ${ARGN})
    qt_evaluate_config_expression(result ${expression})
    if(NOT "${result}" STREQUAL "${expected_value}")
        message(FATAL_ERROR
            "Unexpected value '${result}' when evaluating expression '${expression}'.")
    endif()
endfunction()

function(assert_T)
    assert(ON ${ARGN})
endfunction()

function(assert_F)
    assert(OFF ${ARGN})
endfunction()

function(expect_failure expected_value)
    set(expression ${ARGN})
    qt_evaluate_config_expression(result ${expression})
    if("${result}" STREQUAL "${expected_value}")
        message(FATAL_ERROR
            "Expression '${expression}' unexpectedly evaluated to the correct value '${result}'.")
    endif()
endfunction()

function(expect_failure_T)
    expect_failure(ON ${ARGN})
endfunction()

function(expect_failure_F)
    expect_failure(OFF ${ARGN})
endfunction()

assert_T(ON)
assert_T(TRUE)
assert_F(OFF)
assert_F(FALSE)

assert_T(NOT FALSE)
assert_F(NOT TRUE)

assert_T(TRUE OR TRUE)
assert_T(TRUE OR FALSE)
assert_T(FALSE OR TRUE)
assert_F(FALSE OR FALSE)

assert_T(TRUE AND TRUE)
assert_F(TRUE AND FALSE)
assert_F(FALSE AND TRUE)
assert_F(FALSE AND FALSE)

# string comparison
set(str1 "foo")
set(str2 "bar")
assert_T(str1 STREQUAL "foo")
assert_T(str1 STREQUAL 'foo')
expect_failure_T(str1 STREQUAL str1)    # variable on rhs not supported
expect_failure_T("foo" STREQUAL "foo")  # literal on lhs not supported
expect_failure_T('foo' STREQUAL 'foo')  # literal on lhs not supported

# comparison with empty string
set(empty_string "")
assert_T(empty_string STREQUAL '')
assert_F(empty_string STREQUAL "x")

# string matching
set(processor ppc64)
assert_T(${processor} MATCHES "(ppc|ppc64)$")
assert_F(${processor} MATCHES "i[3-6]86$")

# logical operators and parentheses
set(A TRUE)
set(B FALSE)
set(C TRUE)
assert_F(B OR B)
assert_F(A AND B)
assert_T(A AND (B OR C))
assert_T((A AND B) OR C)
assert_T((A AND B) OR (NOT B AND C))
assert_F(NOT (B OR C))
assert_T(NOT (A AND B))
assert_F(NOT (B OR C))

# target check
set(lib1_cpp "${CMAKE_CURRENT_BINARY_DIR}/lib1.cpp")
file(WRITE "${lib1_cpp}" "int foo() { return 42; }")
add_library(lib1 STATIC EXCLUDE_FROM_ALL "${lib1_cpp}")
assert_T(TARGET lib1)
assert_F(TARGET does_not_exist)

# IN_LIST
set(primary_colors red yellow blue)
assert_T(yellow IN_LIST primary_colors)
assert_F(mauve IN_LIST primary_colors)

# more complex string check from a real world feature
set(INPUT_xcb "")
set(INPUT_xkbcommon no)
assert_F(NOT INPUT_xcb STREQUAL '' AND ( NOT INPUT_xcb STREQUAL 'no' )
    AND INPUT_xkbcommon STREQUAL 'no')
set(INPUT_xcb no)
set(INPUT_xkbcommon no)
assert_F(NOT INPUT_xcb STREQUAL '' AND ( NOT INPUT_xcb STREQUAL 'no' )
    AND INPUT_xkbcommon STREQUAL 'no')
set(INPUT_xcb yes)
set(INPUT_xkbcommon no)
assert_T(NOT INPUT_xcb STREQUAL '' AND ( NOT INPUT_xcb STREQUAL 'no' )
    AND INPUT_xkbcommon STREQUAL 'no')
set(INPUT_xcb no)
set(INPUT_xkbcommon yes)
assert_F(NOT INPUT_xcb STREQUAL '' AND ( NOT INPUT_xcb STREQUAL 'no' )
    AND INPUT_xkbcommon STREQUAL 'no')

set(FEATURE_shared ON)
set(FEATURE_debug ON)
set(FEATURE_debug_and_release OFF)
set(FEATURE_force_debug_info OFF)
set(TEST_separate_debug_info ON)
assert_T(
    ( FEATURE_shared )
    AND ( FEATURE_debug OR FEATURE_debug_and_release OR FEATURE_force_debug_info )
    AND ( MSVC OR APPLE OR TEST_separate_debug_info )
)
