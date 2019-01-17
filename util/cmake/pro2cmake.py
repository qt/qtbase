#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2018 The Qt Company Ltd.
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

from argparse import ArgumentParser
import os.path
import re
import io
import typing

import pyparsing as pp

from helper import map_qt_library, map_qt_base_library, featureName, \
    substitute_platform, substitute_libs


def _parse_commandline():
    parser = ArgumentParser(description='Generate CMakeLists.txt files from .'
                            'pro files.')
    parser.add_argument('--debug', dest='debug', action='store_true',
                        help='Turn on all debug output')
    parser.add_argument('--debug-parser', dest='debug_parser',
                        action='store_true',
                        help='Print debug output from qmake parser.')
    parser.add_argument('--debug-parse-result', dest='debug_parse_result',
                        action='store_true',
                        help='Dump the qmake parser result.')
    parser.add_argument('--debug-parse-dictionary',
                        dest='debug_parse_dictionary', action='store_true',
                        help='Dump the qmake parser result as dictionary.')
    parser.add_argument('--debug-pro-structure', dest='debug_pro_structure',
                        action='store_true',
                        help='Dump the structure of the qmake .pro-file.')
    parser.add_argument('--debug-full-pro-structure',
                        dest='debug_full_pro_structure', action='store_true',
                        help='Dump the full structure of the qmake .pro-file '
                        '(with includes).')
    parser.add_argument('files', metavar='<.pro/.pri file>', type=str,
                        nargs='+', help='The .pro/.pri file to process')

    return parser.parse_args()


def spaces(indent: int) -> str:
    return '    ' * indent


def map_to_file(f: str, top_dir: str, current_dir: str,
                want_absolute_path: bool = False) -> typing.Optional[str]:
    if f == '$$NO_PCH_SOURCES':
        return None
    if f.startswith('$$PWD/') or f == '$$PWD':  # INCLUDEPATH += $$PWD
        return os.path.join(os.path.relpath(current_dir, top_dir), f[6:])
    if f.startswith('$$OUT_PWD/'):
        return "${CMAKE_CURRENT_BUILD_DIR}/" + f[10:]
    if f.startswith('$$QT_SOURCE_TREE'):
        return "${PROJECT_SOURCE_DIR}/" + f[17:]
    if f.startswith("./"):
        return os.path.join(current_dir, f)
    if want_absolute_path and not os.path.isabs(f):
        return os.path.join(current_dir, f)
    return f


def map_source_to_cmake(source: str, base_dir: str,
                        vpath: typing.List[str]) -> str:
    if not source or source == '$$NO_PCH_SOURCES':
        return ''
    if source.startswith('$$PWD/'):
        return source[6:]
    if source == '.':
        return "${CMAKE_CURRENT_SOURCE_DIR}"
    if source.startswith('$$QT_SOURCE_TREE/'):
        return "${PROJECT_SOURCE_DIR}/" + source[17:]

    if os.path.exists(os.path.join(base_dir, source)):
        return source

    for v in vpath:
        fullpath = os.path.join(v, source)
        if os.path.exists(fullpath):
            relpath = os.path.relpath(fullpath, base_dir)
            return relpath

    print('    XXXX: Source {}: Not found.'.format(source))
    return '{}-NOTFOUND'.format(source)


def map_source_to_fs(base_dir: str, file: str,
                     source: str) -> str:
    if source is None or source == '$$NO_PCH_SOURCES':
        return ''
    if source.startswith('$$PWD/'):
        return os.path.join(os.path.dirname(file), source[6:])
    if source.startswith('$$QT_SOURCE_TREE/'):
        return os.path.join('.', source[17:])
    if source.startswith('${PROJECT_SOURCE_DIR}/'):
        return os.path.join('.', source[22:])
    if source.startswith('${CMAKE_CURRENT_SOURCE_DIR}/'):
        return os.path.join(base_dir, source[28:])
    return os.path.join(base_dir, source)


class Operation:
    def __init__(self, value):
        if isinstance(value, list):
            self._value = value
        else:
            self._value = [str(value), ]

    def process(self, input):
        assert(False)

    def __repr__(self):
        assert(False)


class AddOperation(Operation):
    def process(self, input):
        return input + self._value

    def __repr__(self):
        return '+({})'.format(','.join(self._value))


class UniqueAddOperation(Operation):
    def process(self, input):
        result = input
        for v in self._value:
            if v not in result:
                result += [v, ]
        return result

    def __repr__(self):
        return '*({})'.format(self._value)


class SetOperation(Operation):
    def process(self, input):
        return self._value

    def __repr__(self):
        return '=({})'.format(self._value)


class RemoveOperation(Operation):
    def __init__(self, value):
        super().__init__(value)

    def process(self, input):
        input_set = set(input)
        result = []
        for v in self._value:
            if v in input_set:
                continue
            else:
                result += ['-{}'.format(v), ]
        return result

    def __repr__(self):
        return '-({})'.format(self._value)


class Scope:
    def __init__(self, parent_scope: typing.Optional['Scope'],
                 file: typing.Optional[str] = None, condition: str = '',
                 base_dir: str = '') -> None:
        if parent_scope:
            parent_scope._add_child(self)
        else:
            self._parent = None  # type: typing.Optional[Scope]

        self._basedir = base_dir
        if file:
            self._currentdir = os.path.dirname(file)
        if not self._currentdir:
            self._currentdir = '.'
        if not self._basedir:
            self._basedir = self._currentdir

        self._file = file
        self._condition = map_condition(condition)
        self._children = []  # type: typing.List[Scope]
        self._operations = {}  # type: typing.Dict[str, typing.List[Operation]]

    def merge(self, other: 'Scope') -> None:
        for c in other._children:
            self._add_child(c)

        for key in other._operations.keys():
            if key in self._operations:
                self._operations[key] += other._operations[key]
            else:
                self._operations[key] = other._operations[key]

    def basedir(self) -> str:
        return self._basedir

    def currentdir(self) -> str:
        return self._currentdir

    def diff(self, key: str,
             default: typing.Optional[typing.List[str]] = []) \
            -> typing.List[str]:
        mine = self.get(key, default)

        if self._parent:
            parent = self._parent.get(key, default)
            if (parent == mine):
                return []

            parent_set = set(parent)
            mine_set = set(mine)

            added = [x for x in mine if x not in parent_set]
            removed = [x for x in parent if x not in mine_set]

            return added + list('# {}'.format(x) for x in removed)
        return mine

    @staticmethod
    def FromDict(parent_scope: typing.Optional['Scope'],
                 file: str, statements, cond: str = '', base_dir: str = ''):
        scope = Scope(parent_scope, file, cond, base_dir)
        for statement in statements:
            if isinstance(statement, list):  # Handle skipped parts...
                assert not statement
                continue

            operation = statement.get('operation', None)
            if operation:
                key = statement.get('key', '')
                value = statement.get('value', [])
                assert key != ''

                if key in ('HEADERS', 'SOURCES', 'INCLUDEPATH') \
                        or key.endswith('_HEADERS') \
                        or key.endswith('_SOURCES'):
                    value = [map_to_file(v, scope.basedir(),
                                         scope.currentdir()) for v in value]

                if operation == '=':
                    scope._append_operation(key, SetOperation(value))
                elif operation == '-=':
                    scope._append_operation(key, RemoveOperation(value))
                elif operation == '+=':
                    scope._append_operation(key, AddOperation(value))
                elif operation == '*=':
                    scope._append_operation(key, UniqueAddOperation(value))
                else:
                    print('Unexpected operation "{}" in scope with '
                          'condition {}.'.format(operation, cond))
                    assert(False)

                continue

            condition = statement.get('condition', None)
            if condition:
                Scope.FromDict(scope, file,
                               statement.get('statements'), condition,
                               scope.basedir())

                else_statements = statement.get('else_statements')
                if else_statements:
                    Scope.FromDict(scope, file, else_statements,
                                   'NOT ' + condition, scope.basedir())
                continue

            loaded = statement.get('loaded')
            if loaded:
                scope._append_operation('_LOADED', UniqueAddOperation(loaded))
                continue

            option = statement.get('option', None)
            if option:
                scope._append_operation('_OPTION', UniqueAddOperation(option))
                continue

            included = statement.get('included', None)
            if included:
                scope._append_operation('_INCLUDED',
                                        UniqueAddOperation(
                                            map_to_file(included,
                                                        scope.basedir(),
                                                        scope.currentdir())))
                continue

        return scope

    def _append_operation(self, key: str, op: Operation) -> None:
        if key in self._operations:
            self._operations[key].append(op)
        else:
            self._operations[key] = [op, ]

    def file(self) -> str:
        return self._file or ''

    def cMakeListsFile(self) -> str:
        return os.path.join(self.basedir(), 'CMakeLists.txt')

    def condition(self) -> str:
        return self._condition

    def _add_child(self, scope: 'Scope') -> None:
        scope._parent = self
        self._children.append(scope)

    def children(self) -> typing.List['Scope']:
        return self._children

    def dump(self, *, indent: int = 0) -> None:
        ind = '    ' * indent
        if self._condition == '':
            print('{}Scope {} in "{}".'.format(ind, self._file, self._basedir))
        else:
            print('{}Scope {} in "{}" with condition: "{}".'
                  .format(ind, self._file, self._basedir, self._condition))
        print('{}Keys:'.format(ind))
        keys = self._operations.keys()
        if not keys:
            print('{}    -- NONE --'.format(ind))
        else:
            for k in sorted(keys):
                print('{}    {} = "{}"'.format(ind, k, self._operations.get(k, [])))
        print('{}Children:'.format(ind))
        if not self._children:
            print('{}    -- NONE --'.format(ind))
        else:
            for c in self._children:
                c.dump(indent=indent + 1)

    def get(self, key: str, default=None) -> typing.List[str]:
        assert key != '_INCLUDED'  # Special case things that may not recurse!
        result = []  # type: typing.List[str]

        if self._parent:
            result = self._parent.get(key, default)
        else:
            if default:
                if isinstance(default, list):
                    result = default
                else:
                    result = [str(default), ]

        for op in self._operations.get(key, []):
            result = op.process(result)
        return result

    def getString(self, key: str, default: str = '') -> str:
        v = self.get(key, default)
        if len(v) == 0:
            return default
        assert len(v) == 1
        return v[0]

    def getTemplate(self) -> str:
        return self.getString('TEMPLATE', 'app')

    def _rawTemplate(self) -> str:
        return self.getString('TEMPLATE')

    def getTarget(self) -> str:
        return self.getString('TARGET') \
            or os.path.splitext(os.path.basename(self.file()))[0]

    def getIncludes(self) -> typing.List[str]:
        result = []
        for op in self._operations.get('_INCLUDED', []):
            result = op.process(result)
        return result


class QmakeParser:
    def __init__(self, *, debug: bool = False) -> None:
        self._Grammar = self._generate_grammar(debug)

    def _generate_grammar(self, debug: bool):
        # Define grammar:
        pp.ParserElement.setDefaultWhitespaceChars(' \t')

        LC = pp.Suppress(pp.Literal('\\') + pp.LineEnd())
        EOL = pp.Suppress(pp.Optional(pp.pythonStyleComment()) + pp.LineEnd())

        Identifier = pp.Word(pp.alphas + '_', bodyChars=pp.alphanums+'_-./')
        Substitution \
            = pp.Combine(pp.Literal('$')
                         + (((pp.Literal('$') + Identifier
                            + pp.Optional(pp.nestedExpr()))
                            | (pp.Literal('(') + Identifier + pp.Literal(')'))
                            | (pp.Literal('{') + Identifier + pp.Literal('}'))
                            | (pp.Literal('$') + pp.Literal('{')
                                + Identifier + pp.Optional(pp.nestedExpr())
                                + pp.Literal('}'))
                            | (pp.Literal('$') + pp.Literal('[') + Identifier
                                + pp.Literal(']'))
                             )))
        # Do not match word ending in '\' since that breaks line
        # continuation:-/
        LiteralValuePart = pp.Word(pp.printables, excludeChars='$#{}()')
        SubstitutionValue \
            = pp.Combine(pp.OneOrMore(Substitution | LiteralValuePart
                                      | pp.Literal('$')))
        Value = (pp.QuotedString(quoteChar='"', escChar='\\')
                 | SubstitutionValue)

        Values = pp.ZeroOrMore(Value)('value')

        Op = pp.Literal('=') | pp.Literal('-=') | pp.Literal('+=') \
            | pp.Literal('*=')

        Operation = Identifier('key') + Op('operation') + Values('value')
        Load = pp.Keyword('load') + pp.Suppress('(') \
            + Identifier('loaded') + pp.Suppress(')')
        Include = pp.Keyword('include') + pp.Suppress('(') \
            + pp.CharsNotIn(':{=}#)\n')('included') + pp.Suppress(')')
        Option = pp.Keyword('option') + pp.Suppress('(') \
            + Identifier('option') + pp.Suppress(')')
        DefineTest = pp.Suppress(pp.Keyword('defineTest')
                                 + pp.Suppress('(') + Identifier
                                 + pp.Suppress(')')
                                 + pp.nestedExpr(opener='{', closer='}')
                                 + pp.LineEnd())  # ignore the whole thing...
        ForLoop = pp.Suppress(pp.Keyword('for') + pp.nestedExpr()
                              + pp.nestedExpr(opener='{', closer='}',
                                              ignoreExpr=None)
                              + pp.LineEnd())  # ignore the whole thing...
        FunctionCall = pp.Suppress(Identifier + pp.nestedExpr())

        Scope = pp.Forward()

        Statement = pp.Group(Load | Include | Option | DefineTest
                             | ForLoop | FunctionCall | Operation)
        StatementLine = Statement + EOL
        StatementGroup = pp.ZeroOrMore(StatementLine | Scope | EOL)

        Block = pp.Suppress('{') + pp.Optional(EOL) \
            + pp.ZeroOrMore(EOL | Statement + EOL | Scope) \
            + pp.Optional(Statement) + pp.Optional(EOL) \
            + pp.Suppress('}') + pp.Optional(EOL)

        Condition = pp.Optional(pp.White()) + pp.CharsNotIn(':{=}#\\\n')
        Condition.setParseAction(lambda x: ' '.join(x).strip())

        SingleLineScope = pp.Suppress(pp.Literal(':')) \
            + pp.Group(Scope | Block | StatementLine)('statements')
        MultiLineScope = Block('statements')

        SingleLineElse = pp.Suppress(pp.Literal(':')) \
            + pp.Group(Scope | StatementLine)('else_statements')
        MultiLineElse = pp.Group(Block)('else_statements')
        Else = pp.Suppress(pp.Keyword('else')) \
            + (SingleLineElse | MultiLineElse)
        Scope <<= pp.Group(Condition('condition')
                           + (SingleLineScope | MultiLineScope)
                           + pp.Optional(Else))

        if debug:
            for ename in 'EOL Identifier Substitution SubstitutionValue ' \
                         'LiteralValuePart Value Values SingleLineScope ' \
                         'MultiLineScope Scope SingleLineElse ' \
                         'MultiLineElse Else Condition Block ' \
                         'StatementGroup Statement Load Include Option ' \
                         'DefineTest ForLoop FunctionCall Operation'.split():
                expr = locals()[ename]
                expr.setName(ename)
                expr.setDebug()

        Grammar = StatementGroup('statements')
        Grammar.ignore(LC)

        return Grammar

    def parseFile(self, file: str):
        print('Parsing \"{}\"...'.format(file))
        try:
            result = self._Grammar.parseFile(file, parseAll=True)
        except pp.ParseException as pe:
            print(pe.line)
            print(' '*(pe.col-1) + '^')
            print(pe)
            raise pe
        return result


def parseProFile(file: str, *, debug=False):
    parser = QmakeParser(debug=debug)
    return parser.parseFile(file)


def map_condition(condition: str) -> str:
    condition = condition.replace('!', 'NOT ')
    condition = condition.replace('&&', ' AND ')
    condition = condition.replace('|', ' OR ')
    condition = condition.replace('==', ' STREQUAL ')

    cmake_condition = ''
    for part in condition.split():
        # some features contain e.g. linux, that should not be
        # turned upper case
        feature = re.match(r"(qtConfig|qtHaveModule)\(([a-zA-Z0-9_-]+)\)",
                           part)
        if feature:
            if (feature.group(1) == "qtHaveModule"):
                part = 'TARGET {}'.format(map_qt_base_library(
                                            feature.group(2)))
            else:
                part = 'QT_FEATURE_' + featureName(feature.group(2))
        else:
            part = substitute_platform(part)

        part = part.replace('true', 'ON')
        part = part.replace('false', 'OFF')
        cmake_condition += ' ' + part

    return cmake_condition.strip()


def handle_subdir(scope: Scope, cm_fh: typing.IO[str], *,
                  indent: int = 0) -> None:
    assert scope.getTemplate() == 'subdirs'
    ind = '    ' * indent
    for sd in scope.get('SUBDIRS', []):
        full_sd = os.path.join(scope.basedir(), sd)
        if os.path.isdir(full_sd):
            cm_fh.write('{}add_subdirectory({})\n'.format(ind, sd))
        elif os.path.isfile(full_sd):
            subdir_result = parseProFile(full_sd, debug=False)
            subdir_scope \
                = Scope.FromDict(scope, full_sd,
                                 subdir_result.asDict().get('statements'),
                                 '', scope.basedir())

            cmakeify_scope(subdir_scope, cm_fh, indent=indent + 1)
        elif sd.startswith('-'):
            cm_fh.write('{}### remove_subdirectory'
                        '("{}")\n'.format(ind, sd[1:]))
        else:
            print('    XXXX: SUBDIR {} in {}: '
                  'Not found.'.format(sd, scope.file()))

    for c in scope.children():
        cond = c.condition()
        if cond == 'else':
            cm_fh.write('\n{}else()\n'.format(ind))
        elif cond:
            cm_fh.write('\n{}if({})\n'.format(ind, cond))

        handle_subdir(c, cm_fh, indent=indent + 1)

        if cond:
            cm_fh.write('{}endif()\n'.format(ind))


def sort_sources(sources) -> typing.List[str]:
    to_sort = {}  # type: typing.Dict[str, typing.List[str]]
    for s in sources:
        if s is None:
            continue

        dir = os.path.dirname(s)
        base = os.path.splitext(os.path.basename(s))[0]
        if base.endswith('_p'):
            base = base[:-2]
        sort_name = os.path.join(dir, base)

        array = to_sort.get(sort_name, [])
        array.append(s)

        to_sort[sort_name] = array

    lines = []
    for k in sorted(to_sort.keys()):
        lines.append(' '.join(sorted(to_sort[k])))

    return lines


def write_header(cm_fh: typing.IO[str], name: str,
                 typename: str, *, indent: int = 0):
    cm_fh.write('{}###########################################'
                '##########################\n'.format(spaces(indent)))
    cm_fh.write('{}## {} {}:\n'.format(spaces(indent), name, typename))
    cm_fh.write('{}###########################################'
                '##########################\n\n'.format(spaces(indent)))


def write_scope_header(cm_fh: typing.IO[str], *, indent: int = 0):
    cm_fh.write('\n{}## Scopes:\n'.format(spaces(indent)))
    cm_fh.write('{}###########################################'
                '##########################\n'.format(spaces(indent)))


def write_sources_section(cm_fh: typing.IO[str], scope: Scope, *,
                          indent: int = 0, known_libraries=set()) -> None:
    ind = spaces(indent)

    plugin_type = scope.get('PLUGIN_TYPE')
    if plugin_type:
        cm_fh.write('{}    TYPE {}\n'.format(ind, plugin_type[0]))

    sources = scope.diff('SOURCES') + scope.diff('HEADERS') \
        + scope.diff('OBJECTIVE_SOURCES') + scope.diff('NO_PCH_SOURCES') \
        + scope.diff('FORMS')
    resources = scope.diff('RESOURCES')
    if resources:
        qrc_only = True
        for r in resources:
            if not r.endswith('.qrc'):
                qrc_only = False
                break

        if not qrc_only:
            print('     XXXX Ignoring non-QRC file resources.')
        else:
            sources += resources

    vpath = scope.get('VPATH')

    sources = [map_source_to_cmake(s, scope.basedir(), vpath) for s in sources]
    if sources:
        cm_fh.write('{}    SOURCES\n'.format(ind))
    for l in sort_sources(sources):
        cm_fh.write('{}        {}\n'.format(ind, l))

    defines = scope.diff('DEFINES')
    if defines:
        cm_fh.write('{}    DEFINES\n'.format(ind))
        for d in defines:
            d = d.replace('=\\\\\\"$$PWD/\\\\\\"',
                          '="${CMAKE_CURRENT_SOURCE_DIR}/"')
            cm_fh.write('{}        {}\n'.format(ind, d))
    includes = scope.diff('INCLUDEPATH')
    if includes:
        cm_fh.write('{}    INCLUDE_DIRECTORIES\n'.format(ind))
        for i in includes:
            cm_fh.write('{}        {}\n'.format(ind, i))

    dependencies = [map_qt_library(q) for q in scope.diff('QT')
                    if map_qt_library(q) not in known_libraries]
    dependencies += [map_qt_library(q) for q in scope.diff('QT_FOR_PRIVATE')
                     if map_qt_library(q) not in known_libraries]
    dependencies += scope.diff('QMAKE_USE_PRIVATE') \
        + scope.diff('LIBS_PRIVATE') + scope.diff('LIBS')
    if dependencies:
        cm_fh.write('{}    LIBRARIES\n'.format(ind))
        is_framework = False
        for d in dependencies:
            if d == '-framework':
                is_framework = True
                continue
            if is_framework:
                d = '${FW%s}' % d
            if d.startswith('-l'):
                d = d[2:]

            if d.startswith('-'):
                d = '# Remove: {}'.format(d[1:])
            else:
                d = substitute_libs(d)
            cm_fh.write('{}        {}\n'.format(ind, d))
            is_framework = False


def write_extend_target(cm_fh: typing.IO[str], target: str,
                        scope: Scope, parent_condition: str = '',
                        previous_conditon: str = '', *,
                        indent: int = 0) -> str:
    total_condition = scope.condition()
    if total_condition == 'else':
        assert previous_conditon, \
            "Else branch without previous condition in: %s" % scope.file()
        total_condition = 'NOT ({})'.format(previous_conditon)
    if parent_condition:
        total_condition = '({}) AND ({})'.format(parent_condition,
                                                 total_condition)

    extend_qt_io_string = io.StringIO()
    write_sources_section(extend_qt_io_string, scope)
    extend_qt_string = extend_qt_io_string.getvalue()

    extend_scope = '\n{}extend_target({} CONDITION {}\n' \
                   '{})\n'.format(spaces(indent), target, total_condition,
                                  extend_qt_string)

    if not extend_qt_string:
        # Comment out the generated extend_target call because there
        # no sources were found, but keep it commented for
        # informational purposes.
        extend_scope = ''.join(['#' + line for line in
                                extend_scope.splitlines(keepends=True)])
    cm_fh.write(extend_scope)

    children = scope.children()
    if children:
        prev_condition = ''
        for c in children:
            prev_condition = write_extend_target(cm_fh, target, c,
                                                 total_condition,
                                                 prev_condition)

    return total_condition


def write_main_part(cm_fh: typing.IO[str], name: str, typename: str,
                    cmake_function: str, scope: Scope, *,
                    extra_lines: typing.List[str] = [],
                    indent: int = 0,
                    **kwargs: typing.Any):
    write_header(cm_fh, name, typename, indent=indent)

    cm_fh.write('{}{}({}\n'.format(spaces(indent), cmake_function, name))
    for extra_line in extra_lines:
        cm_fh.write('{}    {}\n'.format(spaces(indent), extra_line))

    write_sources_section(cm_fh, scope, indent=indent, **kwargs)

    # Footer:
    cm_fh.write('{})\n'.format(spaces(indent)))

    # Scopes:
    if not scope.children():
        return

    write_scope_header(cm_fh, indent=indent)

    for c in scope.children():
        write_extend_target(cm_fh, name, c, '', indent=indent)


def write_module(cm_fh: typing.IO[str], scope: Scope, *,
                 indent: int = 0) -> None:
    module_name = scope.getTarget()
    assert module_name.startswith('Qt')

    extra = []
    if 'static' in scope.get('CONFIG'):
        extra.append('STATIC')
    if 'no_module_headers' in scope.get('CONFIG'):
        extra.append('NO_MODULE_HEADERS')

    write_main_part(cm_fh, module_name[2:], 'Module', 'add_qt_module', scope,
                    extra_lines=extra, indent=indent,
                    known_libraries={'Qt::Core', })

    if 'qt_tracepoints' in scope.get('CONFIG'):
        tracepoints = map_to_file(scope.getString('TRACEPOINT_PROVIDER'),
                                  scope.basedir(), scope.currentdir())
        cm_fh.write('\n\n{}qt_create_tracepoints({} {})\n'
                    .format(spaces(indent), module_name[2:], tracepoints))


def write_tool(cm_fh: typing.IO[str], scope: Scope, *,
               indent: int = 0) -> None:
    tool_name = scope.getTarget()

    write_main_part(cm_fh, tool_name, 'Tool', 'add_qt_tool', scope,
                    indent=indent, known_libraries={'Qt::Core', })


def write_test(cm_fh: typing.IO[str], scope: Scope, *,
               indent: int = 0) -> None:
    test_name = scope.getTarget()
    assert test_name

    write_main_part(cm_fh, test_name, 'Test', 'add_qt_test', scope,
                    indent=indent, known_libraries={'Qt::Core', 'Qt::Test', })


def write_binary(cm_fh: typing.IO[str], scope: Scope,
                 gui: bool = False, *, indent: int = 0) -> None:
    binary_name = scope.getTarget()
    assert binary_name

    extra = ['GUI', ] if gui else []
    write_main_part(cm_fh, binary_name, 'Binary', 'add_qt_executable', scope,
                    extra_lines=extra, indent=indent,
                    known_libraries={'Qt::Core', })


def write_plugin(cm_fh, scope, *, indent: int = 0):
    plugin_name = scope.getTarget()
    assert plugin_name

    write_main_part(cm_fh, plugin_name, 'Plugin', 'add_qt_plugin', scope,
                    indent=indent, known_libraries={'QtCore', })


def handle_app_or_lib(scope: Scope, cm_fh: typing.IO[str], *,
                      indent: int = 0) -> None:
    assert scope.getTemplate() in ('app', 'lib')

    is_lib = scope.getTemplate() == 'lib'
    is_plugin = any('qt_plugin' == s for s in scope.get('_LOADED', []))

    if is_lib or 'qt_module' in scope.get('_LOADED', []):
        write_module(cm_fh, scope, indent=indent)
    elif is_plugin:
        write_plugin(cm_fh, scope, indent=indent)
    elif 'qt_tool' in scope.get('_LOADED', []):
        write_tool(cm_fh, scope, indent=indent)
    else:
        if 'testcase' in scope.get('CONFIG') \
                or 'testlib' in scope.get('CONFIG'):
            write_test(cm_fh, scope, indent=indent)
        else:
            gui = 'console' not in scope.get('CONFIG')
            write_binary(cm_fh, scope, gui, indent=indent)

    docs = scope.getString("QMAKE_DOCS")
    if docs:
        cm_fh.write("\n{}add_qt_docs({})\n"
                    .format(spaces(indent),
                            map_to_file(docs, scope.basedir(),
                                        scope.currentdir())))


def cmakeify_scope(scope: Scope, cm_fh: typing.IO[str], *,
                   indent: int = 0) -> None:
    template = scope.getTemplate()
    if template == 'subdirs':
        handle_subdir(scope, cm_fh, indent=indent)
    elif template in ('app', 'lib'):
        handle_app_or_lib(scope, cm_fh, indent=indent)
    else:
        print('    XXXX: {}: Template type {} not yet supported.'
              .format(scope.file(), template))


def generate_cmakelists(scope: Scope) -> None:
    with open(scope.cMakeListsFile(), 'w') as cm_fh:
        assert scope.file()
        cm_fh.write('# Generated from {}.\n\n'
                    .format(os.path.basename(scope.file())))
        cmakeify_scope(scope, cm_fh)


def do_include(scope: Scope, *, debug: bool = False) -> None:
    for i in scope.getIncludes():
        dir = scope.basedir()
        include_file = i
        if not include_file:
            continue
        if not os.path.isfile(include_file):
            print('    XXXX: Failed to include {}.'.format(include_file))
            continue

        include_result = parseProFile(include_file, debug=debug)
        include_scope \
            = Scope.FromDict(scope, include_file,
                             include_result.asDict().get('statements'),
                             '', dir)

        do_include(include_scope)

        scope.merge(include_scope)

    for c in scope.children():
        do_include(c)


def main() -> None:
    args = _parse_commandline()

    debug_parsing = args.debug_parser or args.debug

    for file in args.files:
        parseresult = parseProFile(file, debug=debug_parsing)

        if args.debug_parse_result or args.debug:
            print('\n\n#### Parser result:')
            print(parseresult)
            print('\n#### End of parser result.\n')
        if args.debug_parse_dictionary or args.debug:
            print('\n\n####Parser result dictionary:')
            print(parseresult.asDict())
            print('\n#### End of parser result dictionary.\n')

        file_scope = Scope.FromDict(None, file,
                                    parseresult.asDict().get('statements'))

        if args.debug_pro_structure or args.debug:
            print('\n\n#### .pro/.pri file structure:')
            print(file_scope.dump())
            print('\n#### End of .pro/.pri file structure.\n')

        do_include(file_scope, debug=debug_parsing)

        if args.debug_full_pro_structure or args.debug:
            print('\n\n#### Full .pro/.pri file structure:')
            print(file_scope.dump())
            print('\n#### End of full .pro/.pri file structure.\n')

        generate_cmakelists(file_scope)


if __name__ == '__main__':
    main()
