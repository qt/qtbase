#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2019 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

import pyparsing as pp  # type: ignore
import json
import re
from helper import _set_up_py_parsing_nicer_debug_output

_set_up_py_parsing_nicer_debug_output(pp)


class QMakeSpecificJSONParser:
    def __init__(self, *, debug: bool = False) -> None:
        self.debug = debug
        self.grammar = self.create_py_parsing_grammar()

    def create_py_parsing_grammar(self):
        # Keep around all whitespace.
        pp.ParserElement.setDefaultWhitespaceChars("")

        def add_element(name: str, value: pp.ParserElement):
            nonlocal self
            if self.debug:
                value.setName(name)
                value.setDebug()
            return value

        # Our grammar is pretty simple. We want to remove all newlines
        # inside quoted strings, to make the quoted strings JSON
        # compliant. So our grammar should skip to the first quote while
        # keeping everything before it as-is, process the quoted string
        # skip to the next quote, and repeat that until the end of the
        # file.

        EOF = add_element("EOF", pp.StringEnd())
        SkipToQuote = add_element("SkipToQuote", pp.SkipTo('"'))
        SkipToEOF = add_element("SkipToEOF", pp.SkipTo(EOF))

        def remove_newlines_and_whitespace_in_quoted_string(tokens):
            first_string = tokens[0]
            replaced_string = re.sub(r"\n[ ]*", " ", first_string)
            return replaced_string

        QuotedString = add_element(
            "QuotedString", pp.QuotedString(quoteChar='"', multiline=True, unquoteResults=False)
        )
        QuotedString.setParseAction(remove_newlines_and_whitespace_in_quoted_string)

        QuotedTerm = add_element("QuotedTerm", pp.Optional(SkipToQuote) + QuotedString)
        Grammar = add_element("Grammar", pp.OneOrMore(QuotedTerm) + SkipToEOF)

        return Grammar

    def parse_file_using_py_parsing(self, file: str):
        print(f'Pre processing "{file}" using py parsing to remove incorrect newlines.')
        try:
            with open(file, "r") as file_fd:
                contents = file_fd.read()

            parser_result = self.grammar.parseString(contents, parseAll=True)
            token_list = parser_result.asList()
            joined_string = "".join(token_list)

            return joined_string
        except pp.ParseException as pe:
            print(pe.line)
            print(" " * (pe.col - 1) + "^")
            print(pe)
            raise pe

    def parse(self, file: str):
        pre_processed_string = self.parse_file_using_py_parsing(file)
        print(f'Parsing "{file}" using json.loads().')
        json_parsed = json.loads(pre_processed_string)
        return json_parsed
