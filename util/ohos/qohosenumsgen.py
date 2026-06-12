#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import annotations

import argparse
import difflib
import functools
import os
import re
import sys
from dataclasses import dataclass

# REUSE-IgnoreStart
CPP_HEADER_PREAMBLE_TEMPLATE = '''\
// Copyright (C) %d The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSENUMS_H
#define QOHOSENUMS_H

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <array>
#include <info/application_target_sdk_version.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {'''
# REUSE-IgnoreEnd


def cpp_namespace_path(full_type_name: str) -> list[str]:
    return ['enums'] + full_type_name.lstrip('@').split('.')[:-1]


@dataclass(frozen=True)
class OhosEnum:
    namespace_path: list[str]
    type_name: str
    cpp_qualified_name: str
    full_type_name: str
    enumerators: list[str]

    @staticmethod
    def build_from_full_type_name(full_type_name: str, enumerators: list[str]) -> OhosEnum:
        namespace_path = cpp_namespace_path(full_type_name)
        type_name = full_type_name.split('.')[-1]
        cpp_qualified_name = '::'.join(namespace_path + [type_name])
        return OhosEnum(
            namespace_path, type_name, cpp_qualified_name, full_type_name,
            sorted(enumerators))


@dataclass(frozen=True)
class CommonArgumentDefaults:
    enums_header: str
    api_ver: int
    copyright_year: int


def read_text_file(path: str) -> str:
    with open(path, encoding='utf-8') as opened_file:
        return opened_file.read()


def write_text_file(path: str, text: str) -> None:
    with open(path, 'w', encoding='utf-8') as opened_file:
        opened_file.write(text)


def find_ets_type_directories(sdk_directory: str) -> list[str]:
    return sorted(
        directory
        for directory, _, _ in os.walk(sdk_directory)
        if os.path.basename(os.path.dirname(directory)) == 'ets'
        and os.path.basename(directory) in ('api', 'kits'))


def index_ets_files_by_module(sdk_directory: str) -> dict[str, str]:
    ets_file_by_module: dict[str, str] = {}
    for directory in find_ets_type_directories(sdk_directory):
        for file_name in sorted(os.listdir(directory)):
            if not file_name.endswith('.d.ts'):
                continue
            module_specifier = file_name[:-len('.d.ts')]
            ets_file = os.path.join(directory, file_name)
            already_indexed_file = ets_file_by_module.get(module_specifier)
            if already_indexed_file is not None:
                raise Exception(
                    'duplicate ETS module specifier %s: %s and %s'
                    % (module_specifier, already_indexed_file, ets_file))
            ets_file_by_module[module_specifier] = ets_file
    if not ets_file_by_module:
        raise Exception('no ETS module files found under ' + sdk_directory)
    return ets_file_by_module


@functools.cache
def read_ets_file_without_comments(ets_file: str) -> str:
    string_or_comment_pattern = re.compile(
        r'\'(?:\\.|[^\'\\\n])*\'|"(?:\\.|[^"\\\n])*"|/\*.*?\*/|//[^\n]*', re.S)

    def remove_match_unless_string_literal(match: re.Match) -> str:
        return match.group() if match.group().startswith(("'", '"')) else ' '

    return string_or_comment_pattern.sub(
        remove_match_unless_string_literal, read_text_file(ets_file))


@functools.cache
def read_ets_file_with_since_markers(ets_file: str) -> str:
    string_or_comment_pattern = re.compile(
        r'\'(?:\\.|[^\'\\\n])*\'|"(?:\\.|[^"\\\n])*"|/\*.*?\*/|//[^\n]*', re.S)

    def replace_match(match: re.Match) -> str:
        token = match.group()
        if token.startswith(("'", '"')):
            return "''"
        since_values = [int(value) for value in re.findall(r'@since\s+(\d+)', token)]
        if not since_values:
            return ' '
        return ' \x00SINCE=%d\x00 ' % min(since_values)

    return string_or_comment_pattern.sub(replace_match, read_text_file(ets_file))


def try_min_marked_since(text: str) -> int | None:
    since_values = [int(value) for value in re.findall(r'\x00SINCE=(\d+)\x00', text)]
    return min(since_values) if since_values else None


def parse_ets_reexported_module_by_alias(ets_file: str) -> dict[str, str]:
    text = read_ets_file_without_comments(ets_file)
    module_by_alias: dict[str, str] = {}
    for clause, module_specifier in re.findall(
            r'\b(?:import|export)\s+([^;\'"]*?)\s+from\s+[\'"]([^\'"]+)[\'"]', text):
        for item in re.split(r'[{},]', clause):
            words = item.split()
            if words and words[-1].isidentifier():
                module_by_alias[words[-1]] = module_specifier
    return module_by_alias


def extract_balanced_brace_content(text: str, opening_brace_index: int) -> str:
    depth = 0
    for index in range(opening_brace_index, len(text)):
        if text[index] == '{':
            depth += 1
        elif text[index] == '}':
            depth -= 1
            if depth == 0:
                return text[opening_brace_index + 1:index]
    return text[opening_brace_index + 1:]


def try_parse_ets_enum(
        ets_file: str,
        enum_type_name: str) -> tuple[int | None, list[tuple[str, int | None]]] | None:
    text = read_ets_file_with_since_markers(ets_file)
    enum_pattern = re.compile(
        r'((?:\x00SINCE=\d+\x00\s*)*)'
        r'(?:(?:export|declare|const)\s+)*'
        r'\benum\s+' + re.escape(enum_type_name) + r'\s*\{')
    enum_matches = list(enum_pattern.finditer(text))
    if not enum_matches:
        return None
    if len(enum_matches) > 1:
        raise Exception(
            '%d declarations of enum %s in %s'
            % (len(enum_matches), enum_type_name, ets_file))
    enum_match = enum_matches[0]
    enum_since = try_min_marked_since(enum_match.group(1))
    enum_body = extract_balanced_brace_content(text, enum_match.end() - 1)
    enumerators: list[tuple[str, int | None]] = []
    for member in enum_body.split(','):
        member_since = try_min_marked_since(member)
        member_code = re.sub(r'\x00SINCE=\d+\x00', '', member)
        if not member_code.strip():
            continue
        member_match = re.match(r'\s*([A-Za-z_]\w*)', member_code)
        if member_match is None:
            raise Exception(
                'unparsable member %r of enum %s in %s'
                % (member_code.strip(), enum_type_name, ets_file))
        enumerators.append((member_match.group(1), member_since))
    return enum_since, enumerators


def find_longest_ets_module_prefix(
        ets_file_by_module: dict[str, str],
        name_components: list[str]) -> tuple[str | None, list[str]]:
    for length in range(len(name_components), 0, -1):
        module_specifier = '.'.join(name_components[:length])
        if module_specifier in ets_file_by_module:
            return module_specifier, name_components[length:]
    return None, name_components


def resolve_ets_enumerator_names(
        ets_file_by_module: dict[str, str], full_type_name: str, api_ver: int) -> list[str]:
    module_specifier, remaining = find_longest_ets_module_prefix(
        ets_file_by_module, full_type_name.split('.'))
    if module_specifier is None:
        raise Exception('no ETS module file for any name prefix')

    ets_file = ets_file_by_module[module_specifier]
    while len(remaining) > 1:
        reexported_module = parse_ets_reexported_module_by_alias(ets_file).get(remaining[0])
        if reexported_module not in ets_file_by_module:
            break
        ets_file = ets_file_by_module[reexported_module]
        remaining = remaining[1:]

    if not remaining:
        raise Exception('name is a module specifier without an enum type component')
    if len(remaining) > 1:
        raise Exception(
            '%s is not a re-export alias in %s' % (remaining[0], ets_file))
    enum_type_name = remaining[0]
    parsed_enum = try_parse_ets_enum(ets_file, enum_type_name)
    if parsed_enum is None:
        raise Exception('enum %s not found in %s' % (enum_type_name, ets_file))
    enum_since, enumerators = parsed_enum
    if enum_since is not None and enum_since > api_ver:
        raise Exception(
            'enum %s requires API %d but target API is %d'
            % (enum_type_name, enum_since, api_ver))
    return [
        name for name, member_since in enumerators
        if member_since is None or member_since <= api_ver]


def read_full_type_names_from_enums_header(enums_header_path: str) -> list[str]:
    return re.findall(r'fullTypeName\s*=\s*"([^"]+)"', read_text_file(enums_header_path))


def read_min_supported_api_version(qohosjsmain_path: str, variable_name: str) -> int:
    match = re.search(
        r'\b' + re.escape(variable_name) + r'\s*=\s*(\d+)', read_text_file(qohosjsmain_path))
    if match is None:
        raise Exception(
            'no numeric %s found in %s' % (variable_name, qohosjsmain_path))
    return int(match.group(1))


def read_copyright_year(enums_header_path: str) -> int:
    match = re.search(r'Copyright \(C\) (\d+)', read_text_file(enums_header_path))
    if match is None:
        raise Exception('no copyright year found in ' + enums_header_path)
    return int(match.group(1))


def render_cpp_enum_definitions(ohos_enums: list[OhosEnum]) -> str:
    lines: list[str] = []
    open_namespaces: list[str] = []

    def close_namespaces_to(depth: int) -> None:
        while len(open_namespaces) > depth:
            open_namespaces.pop()
            lines.extend(['}', ''])

    for ohos_enum in ohos_enums:
        shared_depth = 0
        for open_namespace, required_namespace in zip(
                open_namespaces, ohos_enum.namespace_path):
            if open_namespace != required_namespace:
                break
            shared_depth += 1
        close_namespaces_to(shared_depth)
        for namespace in ohos_enum.namespace_path[shared_depth:]:
            lines.extend(['namespace %s {' % namespace, ''])
            open_namespaces.append(namespace)
        lines.append('enum class %s {' % ohos_enum.type_name)
        lines.extend('    %s,' % enumerator for enumerator in ohos_enum.enumerators)
        lines.extend(['};', ''])
    close_namespaces_to(0)
    while lines and lines[-1] == '':
        lines.pop()
    return '\n'.join(lines)


def render_cpp_enum_metadata(ohos_enum: OhosEnum) -> str:
    lines = [
        'template<>',
        'struct OhosEnumMeta<%s>' % ohos_enum.cpp_qualified_name,
        '{',
        '    using Enum = %s;' % ohos_enum.cpp_qualified_name,
        '    static constexpr const char *fullTypeName = "%s";' % ohos_enum.full_type_name,
        '    static constexpr std::array<std::pair<Enum, const char *>, %d> '
        'enumeratorsNames = {{' % len(ohos_enum.enumerators),
    ]
    lines += [
        '        {Enum::%s, "%s"},' % (enumerator, enumerator)
        for enumerator in ohos_enum.enumerators]
    lines += ['    }};', '};']
    return '\n'.join(lines)


def render_cpp_metatype_declaration(ohos_enum: OhosEnum) -> str:
    return 'Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::%s));' % ohos_enum.cpp_qualified_name


def render_cpp_header(ohos_enums: list[OhosEnum], year: int) -> str:
    return '\n'.join([
        CPP_HEADER_PREAMBLE_TEMPLATE % year, '',
        render_cpp_enum_definitions(ohos_enums), '',
        'template<typename Enum>', 'struct OhosEnumMeta;', '',
        '\n\n'.join(render_cpp_enum_metadata(ohos_enum) for ohos_enum in ohos_enums), '',
        '}', '',
        'QT_END_NAMESPACE', '',
        '\n'.join(render_cpp_metatype_declaration(ohos_enum) for ohos_enum in ohos_enums), '',
        '#endif',
    ])


def resolve_ohos_enums(
        ets_file_by_module: dict[str, str], full_type_names: list[str],
        api_ver: int) -> list[OhosEnum]:
    ohos_enums: list[OhosEnum] = []
    unresolved: list[str] = []
    for full_type_name in full_type_names:
        try:
            enumerators = resolve_ets_enumerator_names(ets_file_by_module, full_type_name, api_ver)
        except Exception as exception:
            unresolved.append('%s: %s' % (full_type_name, exception))
            continue
        ohos_enums.append(OhosEnum.build_from_full_type_name(full_type_name, enumerators))
    if unresolved:
        raise Exception(
            '%d enum(s) could not be resolved, first failure: %s'
            % (len(unresolved), unresolved[0]))
    return ohos_enums


def generate_enums_header_text(
        ets_file_by_module: dict[str, str], full_type_names: list[str],
        api_ver: int, year: int) -> str:
    ohos_enums = sorted(
        resolve_ohos_enums(ets_file_by_module, full_type_names, api_ver),
        key=lambda ohos_enum: ohos_enum.namespace_path + [ohos_enum.type_name])
    return render_cpp_header(ohos_enums, year) + '\n'


def command_update_enums_header(arguments: argparse.Namespace) -> None:
    ets_file_by_module = index_ets_files_by_module(arguments.sdk_dir)
    full_type_names = read_full_type_names_from_enums_header(arguments.enums_header)
    write_text_file(
        arguments.enums_header,
        generate_enums_header_text(
            ets_file_by_module, full_type_names, arguments.api_ver, arguments.copyright_year))


def command_print_enums_diff(arguments: argparse.Namespace) -> None:
    ets_file_by_module = index_ets_files_by_module(arguments.sdk_dir)
    full_type_names = read_full_type_names_from_enums_header(arguments.enums_header)
    current_text = read_text_file(arguments.enums_header)
    generated_text = generate_enums_header_text(
        ets_file_by_module, full_type_names, arguments.api_ver, arguments.copyright_year)
    if generated_text != current_text:
        sys.stdout.write(
            ''.join(
                difflib.unified_diff(
                    current_text.splitlines(keepends=True),
                    generated_text.splitlines(keepends=True),
                    fromfile=arguments.enums_header + ' (current)',
                    tofile=arguments.enums_header + ' (generated)')))
        sys.exit(1)


def command_add_enums_to_header(arguments: argparse.Namespace) -> None:
    ets_file_by_module = index_ets_files_by_module(arguments.sdk_dir)
    full_type_names = read_full_type_names_from_enums_header(arguments.enums_header)
    enums_header_text = generate_enums_header_text(
        ets_file_by_module, full_type_names, arguments.api_ver, arguments.copyright_year)
    if enums_header_text != read_text_file(arguments.enums_header):
        raise Exception(
            'refusing to add: %s is out of sync with the SDK; run "update" first'
            % arguments.enums_header)
    for new_full_type_name in arguments.full_type_names:
        if new_full_type_name in full_type_names:
            raise Exception('already present: ' + new_full_type_name)
        full_type_names.append(new_full_type_name)
    write_text_file(
        arguments.enums_header,
        generate_enums_header_text(
            ets_file_by_module, full_type_names, arguments.api_ver, arguments.copyright_year))


def add_common_arguments(
        command_parser: argparse.ArgumentParser, defaults: CommonArgumentDefaults) -> None:
    command_parser.add_argument(
        '--enums-header', default=defaults.enums_header,
        help='qohosenums.h to read/update (default: %(default)s)')
    command_parser.add_argument(
        '--sdk-dir', required=True,
        help='SDK directory to search for .d.ts enum declarations')
    command_parser.add_argument(
        '--api-ver', default=defaults.api_ver, type=int,
        help='target API version (default: %(default)s); enums and enumerators '
             'introduced in a later API are considered non-existent')
    command_parser.add_argument(
        '--copyright-year', default=defaults.copyright_year, type=int,
        help='copyright year for the generated header (default: %(default)s)')


def parse_arguments(defaults: CommonArgumentDefaults) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Generate qohosenums.h from the OHOS/HMS SDK enum declarations.')
    subcommands = parser.add_subparsers(dest='mode', required=True)

    update_command = subcommands.add_parser(
        'update', help='rewrite the header with freshly generated content')
    add_common_arguments(update_command, defaults)
    update_command.set_defaults(handler=command_update_enums_header)

    diff_command = subcommands.add_parser(
        'diff', help='print the diff between the header and freshly generated content')
    add_common_arguments(diff_command, defaults)
    diff_command.set_defaults(handler=command_print_enums_diff)

    add_command = subcommands.add_parser(
        'add', help='add enums by full type name (the header must already be up to date)')
    add_common_arguments(add_command, defaults)
    add_command.add_argument(
        'full_type_names', nargs='+', metavar='FULL_TYPE_NAME',
        help='OHOS enum full name, e.g. @ohos.window.WindowEventType')
    add_command.set_defaults(handler=command_add_enums_to_header)

    return parser.parse_args()


def main() -> None:
    qpa_plugin_dir = os.path.normpath(
        os.path.join(
            os.path.dirname(__file__),
            '..', '..', 'src', 'plugins', 'platforms', 'ohos'))
    default_enums_header = os.path.join(qpa_plugin_dir, 'qohosenums.h')
    defaults = CommonArgumentDefaults(
        enums_header=default_enums_header,
        api_ver=read_min_supported_api_version(
            os.path.join(qpa_plugin_dir, 'qohosjsmain.cpp'),
            'minSupportedOhosSdkApiVersion'),
        copyright_year=read_copyright_year(default_enums_header))
    arguments = parse_arguments(defaults)
    if not os.path.isdir(arguments.sdk_dir):
        raise Exception('SDK directory not found: ' + arguments.sdk_dir)
    arguments.handler(arguments)


if __name__ == '__main__':
    main()
