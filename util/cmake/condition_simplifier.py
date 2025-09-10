#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import re
from sympy import simplify_logic, And, Or, Not, SympifyError  # type: ignore
from condition_simplifier_cache import simplify_condition_memoize


def _iterate_expr_tree(expr, op, matches):
    assert expr.func == op
    keepers = ()
    for arg in expr.args:
        if arg in matches:
            matches = tuple(x for x in matches if x != arg)
        elif arg == op:
            (matches, extra_keepers) = _iterate_expr_tree(arg, op, matches)
            keepers = (*keepers, *extra_keepers)
        else:
            keepers = (*keepers, arg)
    return matches, keepers


def _simplify_expressions(expr, op, matches, replacement):
    for arg in expr.args:
        expr = expr.subs(arg, _simplify_expressions(arg, op, matches, replacement))

    if expr.func == op:
        (to_match, keepers) = tuple(_iterate_expr_tree(expr, op, matches))
        if len(to_match) == 0:
            # build expression with keepers and replacement:
            if keepers:
                start = replacement
                current_expr = None
                last_expr = keepers[-1]
                for repl_arg in keepers[:-1]:
                    current_expr = op(start, repl_arg)
                    start = current_expr
                top_expr = op(start, last_expr)
            else:
                top_expr = replacement

            expr = expr.subs(expr, top_expr)

    return expr


def _simplify_flavors_in_condition(base: str, flavors, expr):
    """Simplify conditions based on the knowledge of which flavors
    belong to which OS."""
    base_expr = simplify_logic(base)
    false_expr = simplify_logic("false")
    for flavor in flavors:
        flavor_expr = simplify_logic(flavor)
        expr = _simplify_expressions(expr, And, (base_expr, flavor_expr), flavor_expr)
        expr = _simplify_expressions(expr, Or, (base_expr, flavor_expr), base_expr)
        expr = _simplify_expressions(expr, And, (Not(base_expr), flavor_expr), false_expr)
    return expr


def _simplify_os_families(expr, family_members, other_family_members):
    for family in family_members:
        for other in other_family_members:
            if other in family_members:
                continue  # skip those in the sub-family

            f_expr = simplify_logic(family)
            o_expr = simplify_logic(other)

            expr = _simplify_expressions(expr, And, (f_expr, Not(o_expr)), f_expr)
            expr = _simplify_expressions(expr, And, (Not(f_expr), o_expr), o_expr)
            expr = _simplify_expressions(expr, And, (f_expr, o_expr), simplify_logic("false"))
    return expr


def _recursive_simplify(expr):
    """Simplify the expression as much as possible based on
    domain knowledge."""
    input_expr = expr

    # Simplify even further, based on domain knowledge:
    # windowses = ('WIN32', 'WINRT')
    apples = ("MACOS", "UIKIT", "IOS", "TVOS", "WATCHOS")
    bsds = ("FREEBSD", "OPENBSD", "NETBSD")
    androids = ("ANDROID",)
    unixes = (
        "APPLE",
        *apples,
        "BSD",
        *bsds,
        "LINUX",
        *androids,
        "HAIKU",
        "INTEGRITY",
        "VXWORKS",
        "QNX",
        "WASM",
    )

    unix_expr = simplify_logic("UNIX")
    win_expr = simplify_logic("WIN32")
    false_expr = simplify_logic("false")
    true_expr = simplify_logic("true")

    expr = expr.subs(Not(unix_expr), win_expr)  # NOT UNIX -> WIN32
    expr = expr.subs(Not(win_expr), unix_expr)  # NOT WIN32 -> UNIX

    # UNIX [OR foo ]OR WIN32 -> ON [OR foo]
    expr = _simplify_expressions(expr, Or, (unix_expr, win_expr), true_expr)
    # UNIX  [AND foo ]AND WIN32 -> OFF [AND foo]
    expr = _simplify_expressions(expr, And, (unix_expr, win_expr), false_expr)

    expr = _simplify_flavors_in_condition("WIN32", ("WINRT",), expr)
    expr = _simplify_flavors_in_condition("APPLE", apples, expr)
    expr = _simplify_flavors_in_condition("BSD", bsds, expr)
    expr = _simplify_flavors_in_condition("UNIX", unixes, expr)

    # Simplify families of OSes against other families:
    expr = _simplify_os_families(expr, ("WIN32", "WINRT"), unixes)
    expr = _simplify_os_families(expr, androids, unixes)
    expr = _simplify_os_families(expr, ("BSD", *bsds), unixes)

    for family in ("HAIKU", "QNX", "INTEGRITY", "LINUX", "VXWORKS"):
        expr = _simplify_os_families(expr, (family,), unixes)

    # Now simplify further:
    expr = simplify_logic(expr)

    while expr != input_expr:
        input_expr = expr
        expr = _recursive_simplify(expr)

    return expr


@simplify_condition_memoize
def simplify_condition(condition: str) -> str:
    input_condition = condition.strip()

    # Map to sympy syntax:
    condition = " " + input_condition + " "
    condition = condition.replace("(", " ( ")
    condition = condition.replace(")", " ) ")

    tmp = ""
    while tmp != condition:
        tmp = condition

        condition = condition.replace(" NOT ", " ~ ")
        condition = condition.replace(" AND ", " & ")
        condition = condition.replace(" OR ", " | ")
        condition = condition.replace(" ON ", " true ")
        condition = condition.replace(" OFF ", " false ")
        # Replace dashes with a token
        condition = condition.replace("-", "_dash_")

    # SymPy chokes on expressions that contain two tokens one next to
    # the other delimited by a space, which are not an operation.
    # So a CMake condition like "TARGET Foo::Bar" fails the whole
    # expression simplifying process.
    # Turn these conditions into a single token so that SymPy can parse
    # the expression, and thus simplify it.
    # Do this by replacing and keeping a map of conditions to single
    # token symbols.
    # Support both target names without double colons, and with double
    # colons.
    pattern = re.compile(r"(TARGET [a-zA-Z]+(?:::[a-zA-Z]+)?)")
    target_symbol_mapping = {}
    all_target_conditions = re.findall(pattern, condition)
    for target_condition in all_target_conditions:
        # Replace spaces and colons with underscores.
        target_condition_symbol_name = re.sub("[ :]", "_", target_condition)
        target_symbol_mapping[target_condition_symbol_name] = target_condition
        condition = re.sub(target_condition, target_condition_symbol_name, condition)

    # Do similar token mapping for comparison operators.
    pattern = re.compile(r"([a-zA-Z_0-9]+ (?:STRLESS|STREQUAL|STRGREATER) [a-zA-Z_0-9]+)")
    comparison_symbol_mapping = {}
    all_comparisons = re.findall(pattern, condition)
    for comparison in all_comparisons:
        # Replace spaces and colons with underscores.
        comparison_symbol_name = re.sub("[ ]", "_", comparison)
        comparison_symbol_mapping[comparison_symbol_name] = comparison
        condition = re.sub(comparison, comparison_symbol_name, condition)

    try:
        # Generate and simplify condition using sympy:
        condition_expr = simplify_logic(condition)
        condition = str(_recursive_simplify(condition_expr))

        # Restore the target conditions.
        for symbol_name in target_symbol_mapping:
            condition = re.sub(symbol_name, target_symbol_mapping[symbol_name], condition)

        # Restore comparisons.
        for comparison in comparison_symbol_mapping:
            condition = re.sub(comparison, comparison_symbol_mapping[comparison], condition)

        # Map back to CMake syntax:
        condition = condition.replace("~", "NOT ")
        condition = condition.replace("&", "AND")
        condition = condition.replace("|", "OR")
        condition = condition.replace("True", "ON")
        condition = condition.replace("False", "OFF")
        condition = condition.replace("_dash_", "-")
    except (SympifyError, TypeError, AttributeError):
        # sympy did not like our input, so leave this condition alone:
        condition = input_condition

    return condition or "ON"
