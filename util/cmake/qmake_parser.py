#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import collections
import os
import re
from itertools import chain
from typing import Tuple

import pyparsing as pp  # type: ignore

from helper import _set_up_py_parsing_nicer_debug_output

_set_up_py_parsing_nicer_debug_output(pp)


def fixup_linecontinuation(contents: str) -> str:
    # Remove all line continuations, aka a backslash followed by
    # a newline character with an arbitrary amount of whitespace
    # between the backslash and the newline.
    # This greatly simplifies the qmake parsing grammar.
    contents = re.sub(r"([^\t ])\\[ \t]*\n", "\\1 ", contents)
    contents = re.sub(r"\\[ \t]*\n", "", contents)
    return contents


def fixup_comments(contents: str) -> str:
    # Get rid of completely commented out lines.
    # So any line which starts with a '#' char and ends with a new line
    # will be replaced by a single new line.
    # The # may be preceded by any number of spaces or tabs.
    #
    # This is needed because qmake syntax is weird. In a multi line
    # assignment (separated by backslashes and newlines aka
    # # \\\n ), if any of the lines are completely commented out, in
    # principle the assignment should fail.
    #
    # It should fail because you would have a new line separating
    # the previous value from the next value, and the next value would
    # not be interpreted as a value, but as a new token / operation.
    # qmake is lenient though, and accepts that, so we need to take
    # care of it as well, as if the commented line didn't exist in the
    # first place.

    contents = re.sub(r"(^|\n)[ \t]*#[^\n]*?\n", "\n", contents, re.DOTALL)
    return contents


def flatten_list(input_list):
    """Flattens an irregular nested list into a simple list."""
    for el in input_list:
        if isinstance(el, collections.abc.Iterable) and not isinstance(el, (str, bytes)):
            yield from flatten_list(el)
        else:
            yield el


def handle_function_value(group: pp.ParseResults):
    function_name = group[0]
    function_args = group[1]
    if function_name == "qtLibraryTarget":
        if len(function_args) > 1:
            raise RuntimeError(
                "Don't know what to with more than one function argument "
                "for $$qtLibraryTarget()."
            )
        return str(function_args[0])

    if function_name == "quote":
        # Do nothing, just return a string result
        return str(group)

    if function_name == "files":
        return str(function_args[0])

    if function_name == "basename":
        if len(function_args) != 1:
            print(f"XXXX basename with more than one argument")
        if function_args[0] == "_PRO_FILE_PWD_":
            return os.path.basename(os.getcwd())
        print(f"XXXX basename with value other than _PRO_FILE_PWD_")
        return os.path.basename(str(function_args[0]))

    if isinstance(function_args, pp.ParseResults):
        function_args = list(flatten_list(function_args.asList()))

    # For other functions, return the whole expression as a string.
    return f"$${function_name}({' '.join(function_args)})"


class QmakeParser:
    def __init__(self, *, debug: bool = False) -> None:
        self.debug = debug
        self._Grammar = self._generate_grammar()

    def _generate_grammar(self):
        # Define grammar:
        pp.ParserElement.setDefaultWhitespaceChars(" \t")

        def add_element(name: str, value: pp.ParserElement):
            nonlocal self
            if self.debug:
                value.setName(name)
                value.setDebug()
            return value

        EOL = add_element("EOL", pp.Suppress(pp.LineEnd()))
        Else = add_element("Else", pp.Keyword("else"))
        Identifier = add_element(
            "Identifier", pp.Word(f"{pp.alphas}_", bodyChars=pp.alphanums + "_-./")
        )
        BracedValue = add_element(
            "BracedValue",
            pp.nestedExpr(
                ignoreExpr=pp.quotedString
                | pp.QuotedString(
                    quoteChar="$(", endQuoteChar=")", escQuote="\\", unquoteResults=False
                )
            ).setParseAction(lambda s, l, t: ["(", *t[0], ")"]),
        )

        Substitution = add_element(
            "Substitution",
            pp.Combine(
                pp.Literal("$")
                + (
                    (
                        (pp.Literal("$") + Identifier + pp.Optional(pp.nestedExpr()))
                        | (pp.Literal("(") + Identifier + pp.Literal(")"))
                        | (pp.Literal("{") + Identifier + pp.Literal("}"))
                        | (
                            pp.Literal("$")
                            + pp.Literal("{")
                            + Identifier
                            + pp.Optional(pp.nestedExpr())
                            + pp.Literal("}")
                        )
                        | (pp.Literal("$") + pp.Literal("[") + Identifier + pp.Literal("]"))
                    )
                )
            ),
        )
        LiteralValuePart = add_element(
            "LiteralValuePart", pp.Word(pp.printables, excludeChars="$#{}()")
        )
        SubstitutionValue = add_element(
            "SubstitutionValue",
            pp.Combine(pp.OneOrMore(Substitution | LiteralValuePart | pp.Literal("$"))),
        )
        FunctionValue = add_element(
            "FunctionValue",
            pp.Group(
                pp.Suppress(pp.Literal("$") + pp.Literal("$"))
                + Identifier
                + pp.nestedExpr()  # .setParseAction(lambda s, l, t: ['(', *t[0], ')'])
            ).setParseAction(lambda s, l, t: handle_function_value(*t)),
        )
        Value = add_element(
            "Value",
            pp.NotAny(Else | pp.Literal("}") | EOL)
            + (
                pp.QuotedString(quoteChar='"', escChar="\\")
                | FunctionValue
                | SubstitutionValue
                | BracedValue
            ),
        )

        Values = add_element("Values", pp.ZeroOrMore(Value)("value"))

        Op = add_element(
            "OP",
            pp.Literal("=")
            | pp.Literal("-=")
            | pp.Literal("+=")
            | pp.Literal("*=")
            | pp.Literal("~="),
        )

        Key = add_element("Key", Identifier)

        Operation = add_element(
            "Operation", Key("key") + pp.locatedExpr(Op)("operation") + Values("value")
        )
        CallArgs = add_element("CallArgs", pp.nestedExpr())

        def parse_call_args(results):
            out = ""
            for item in chain(*results):
                if isinstance(item, str):
                    out += item
                else:
                    out += "(" + parse_call_args(item) + ")"
            return out

        CallArgs.setParseAction(parse_call_args)

        Load = add_element("Load", pp.Keyword("load") + CallArgs("loaded"))
        Include = add_element(
            "Include", pp.Keyword("include") + pp.locatedExpr(CallArgs)("included")
        )
        Option = add_element("Option", pp.Keyword("option") + CallArgs("option"))
        RequiresCondition = add_element("RequiresCondition", pp.originalTextFor(pp.nestedExpr()))

        def parse_requires_condition(s, l_unused, t):
            # The following expression unwraps the condition via the additional info
            # set by originalTextFor.
            condition_without_parentheses = s[t._original_start + 1 : t._original_end - 1]

            # And this replaces the colons with '&&' similar how it's done for 'Condition'.
            condition_without_parentheses = (
                condition_without_parentheses.strip().replace(":", " && ").strip(" && ")
            )
            return condition_without_parentheses

        RequiresCondition.setParseAction(parse_requires_condition)
        Requires = add_element(
            "Requires", pp.Keyword("requires") + RequiresCondition("project_required_condition")
        )

        FunctionArgumentsAsString = add_element(
            "FunctionArgumentsAsString", pp.originalTextFor(pp.nestedExpr())
        )
        QtNoMakeTools = add_element(
            "QtNoMakeTools",
            pp.Keyword("qtNomakeTools") + FunctionArgumentsAsString("qt_no_make_tools_arguments"),
        )

        # ignore the whole thing...
        DefineTestDefinition = add_element(
            "DefineTestDefinition",
            pp.Suppress(
                pp.Keyword("defineTest")
                + CallArgs
                + pp.nestedExpr(opener="{", closer="}", ignoreExpr=pp.LineEnd())
            ),
        )

        # ignore the whole thing...
        ForLoop = add_element(
            "ForLoop",
            pp.Suppress(
                pp.Keyword("for")
                + CallArgs
                + pp.nestedExpr(opener="{", closer="}", ignoreExpr=pp.LineEnd())
            ),
        )

        # ignore the whole thing...
        ForLoopSingleLine = add_element(
            "ForLoopSingleLine",
            pp.Suppress(pp.Keyword("for") + CallArgs + pp.Literal(":") + pp.SkipTo(EOL)),
        )

        # ignore the whole thing...
        FunctionCall = add_element("FunctionCall", pp.Suppress(Identifier + pp.nestedExpr()))

        Scope = add_element("Scope", pp.Forward())

        Statement = add_element(
            "Statement",
            pp.Group(
                Load
                | Include
                | Option
                | Requires
                | QtNoMakeTools
                | ForLoop
                | ForLoopSingleLine
                | DefineTestDefinition
                | FunctionCall
                | Operation
            ),
        )
        StatementLine = add_element("StatementLine", Statement + (EOL | pp.FollowedBy("}")))
        StatementGroup = add_element(
            "StatementGroup", pp.ZeroOrMore(StatementLine | Scope | pp.Suppress(EOL))
        )

        Block = add_element(
            "Block",
            pp.Suppress("{")
            + pp.Optional(EOL)
            + StatementGroup
            + pp.Optional(EOL)
            + pp.Suppress("}")
            + pp.Optional(EOL),
        )

        ConditionEnd = add_element(
            "ConditionEnd",
            pp.FollowedBy(
                (pp.Optional(pp.White()) + (pp.Literal(":") | pp.Literal("{") | pp.Literal("|")))
            ),
        )

        ConditionPart1 = add_element(
            "ConditionPart1", (pp.Optional("!") + Identifier + pp.Optional(BracedValue))
        )
        ConditionPart2 = add_element("ConditionPart2", pp.CharsNotIn("#{}|:=\\\n"))
        ConditionPart = add_element(
            "ConditionPart", (ConditionPart1 ^ ConditionPart2) + ConditionEnd
        )

        ConditionOp = add_element("ConditionOp", pp.Literal("|") ^ pp.Literal(":"))
        ConditionWhiteSpace = add_element(
            "ConditionWhiteSpace", pp.Suppress(pp.Optional(pp.White(" ")))
        )

        # Unfortunately qmake condition operators have no precedence,
        # and are simply evaluated left to right. To emulate that, wrap
        # each condition sub-expression in parentheses.
        # So c1|c2:c3 is evaluated by qmake as (c1|c2):c3.
        # The following variable keeps count on how many parentheses
        # should be added to the beginning of the condition. Each
        # condition sub-expression always gets an ")", and in the
        # end the whole condition gets many "(". Note that instead
        # inserting the actual parentheses, we insert special markers
        # which get replaced in the end.
        condition_parts_count = 0
        # Whitespace in the markers is important. Assumes the markers
        # never appear in .pro files.
        l_paren_marker = "_(_ "
        r_paren_marker = " _)_"

        def handle_condition_part(condition_part_parse_result: pp.ParseResults) -> str:
            condition_part_list = [*condition_part_parse_result]
            nonlocal condition_parts_count
            condition_parts_count += 1
            condition_part_joined = "".join(condition_part_list)
            # Add ending parenthesis marker. The counterpart is added
            # in handle_condition.
            return f"{condition_part_joined}{r_paren_marker}"

        ConditionPart.setParseAction(handle_condition_part)
        ConditionRepeated = add_element(
            "ConditionRepeated", pp.ZeroOrMore(ConditionOp + ConditionWhiteSpace + ConditionPart)
        )

        def handle_condition(condition_parse_results: pp.ParseResults) -> str:
            nonlocal condition_parts_count
            prepended_parentheses = l_paren_marker * condition_parts_count
            result = prepended_parentheses + " ".join(condition_parse_results).strip().replace(
                ":", " && "
            ).strip(" && ")
            # If there are only 2 condition sub-expressions, there is no
            # need for parentheses.
            if condition_parts_count < 3:
                result = result.replace(l_paren_marker, "")
                result = result.replace(r_paren_marker, "")
                result = result.strip(" ")
            else:
                result = result.replace(l_paren_marker, "( ")
                result = result.replace(r_paren_marker, " )")
                # Strip parentheses and spaces around the final
                # condition.
                result = result[1:-1]
                result = result.strip(" ")
            # Reset the parenthesis count for the next condition.
            condition_parts_count = 0
            return result

        Condition = add_element("Condition", pp.Combine(ConditionPart + ConditionRepeated))
        Condition.setParseAction(handle_condition)

        # Weird thing like write_file(a)|error() where error() is the alternative condition
        # which happens to be a function call. In this case there is no scope, but our code expects
        # a scope with a list of statements, so create a fake empty statement.
        ConditionEndingInFunctionCall = add_element(
            "ConditionEndingInFunctionCall",
            pp.Suppress(ConditionOp)
            + FunctionCall
            + pp.Empty().setParseAction(lambda x: [[]]).setResultsName("statements"),
        )

        SingleLineScope = add_element(
            "SingleLineScope",
            pp.Suppress(pp.Literal(":")) + pp.Group(Block | (Statement + EOL))("statements"),
        )
        MultiLineScope = add_element("MultiLineScope", Block("statements"))

        SingleLineElse = add_element(
            "SingleLineElse",
            pp.Suppress(pp.Literal(":")) + (Scope | Block | (Statement + pp.Optional(EOL))),
        )
        MultiLineElse = add_element("MultiLineElse", Block)
        ElseBranch = add_element("ElseBranch", pp.Suppress(Else) + (SingleLineElse | MultiLineElse))

        # Scope is already add_element'ed in the forward declaration above.
        Scope <<= pp.Group(
            Condition("condition")
            + (SingleLineScope | MultiLineScope | ConditionEndingInFunctionCall)
            + pp.Optional(ElseBranch)("else_statements")
        )

        Grammar = StatementGroup("statements")
        Grammar.ignore(pp.pythonStyleComment())

        return Grammar

    def parseFile(self, file: str) -> Tuple[pp.ParseResults, str]:
        print(f'Parsing "{file}"...')
        try:
            with open(file, "r") as file_fd:
                contents = file_fd.read()

            # old_contents = contents
            contents = fixup_comments(contents)
            contents = fixup_linecontinuation(contents)
            result = self._Grammar.parseString(contents, parseAll=True)
        except pp.ParseException as pe:
            print(pe.line)
            print(f"{' ' * (pe.col-1)}^")
            print(pe)
            raise pe
        return result, contents


def parseProFile(file: str, *, debug=False) -> Tuple[pp.ParseResults, str]:
    parser = QmakeParser(debug=debug)
    return parser.parseFile(file)
