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

import glob
import os
import subprocess
import concurrent.futures
import typing
import argparse
from argparse import ArgumentParser


def parse_command_line():
    parser = ArgumentParser(description='Run pro2cmake on all .pro files recursively in given path.')
    parser.add_argument('--only-existing', dest='only_existing', action='store_true',
                        help='Run pro2cmake only on .pro files that already have a CMakeLists.txt.')
    parser.add_argument('--only-qtbase-main-modules', dest='only_qtbase_main_modules', action='store_true',
                        help='Run pro2cmake only on the main modules in qtbase.')
    parser.add_argument('--is-example', dest='is_example', action='store_true',
                        help='Run pro2cmake with --is-example flag.')
    parser.add_argument('path', metavar='<path>', type=str,
                        help='The path where to look for .pro files.')

    return parser.parse_args()


def find_all_pro_files(base_path: str, args: argparse.Namespace):

    def sorter(pro_file: str) -> str:
        """ Sorter that tries to prioritize main pro files in a directory. """
        pro_file_without_suffix = pro_file.rsplit('/', 1)[-1][:-4]
        dir_name = os.path.dirname(pro_file)
        if dir_name.endswith('/' + pro_file_without_suffix):
            return dir_name
        return dir_name + "/__" + pro_file

    all_files = []
    previous_dir_name: str = None

    print('Finding .pro files.')
    glob_result = glob.glob(os.path.join(base_path, '**/*.pro'), recursive=True)

    def cmake_lists_exists_filter(path):
        path_dir_name = os.path.dirname(path)
        if os.path.exists(os.path.join(path_dir_name, 'CMakeLists.txt')):
            return True
        return False

    def qtbase_main_modules_filter(path):
        main_modules = [
            'corelib',
            'network',
            'gui',
            'widgets',
            'testlib',
            'printsupport',
            'opengl',
            'sql',
            'dbus',
            'concurrent',
            'xml',
        ]
        path_suffixes = ['src/{}/{}.pro'.format(m, m, '.pro') for m in main_modules]

        for path_suffix in path_suffixes:
            if path.endswith(path_suffix):
                return True
        return False

    filter_result = glob_result
    filter_func = None
    if args.only_existing:
        filter_func = cmake_lists_exists_filter
    elif args.only_qtbase_main_modules:
        filter_func = qtbase_main_modules_filter

    if filter_func:
        print('Filtering.')
        filter_result = [p for p in filter_result if filter_func(p)]

    for pro_file in sorted(filter_result, key=sorter):
        dir_name = os.path.dirname(pro_file)
        if dir_name == previous_dir_name:
            print("Skipping:", pro_file)
        else:
            all_files.append(pro_file)
            previous_dir_name = dir_name
    return all_files


def run(all_files: typing.List[str], pro2cmake: str, args: argparse.Namespace) -> typing.List[str]:
    failed_files = []
    files_count = len(all_files)
    workers = (os.cpu_count() or 1)

    if args.only_qtbase_main_modules:
        # qtbase main modules take longer than usual to process.
        workers = 2

    with concurrent.futures.ThreadPoolExecutor(max_workers=workers, initializer=os.nice, initargs=(10,)) as pool:
        print('Firing up thread pool executor.')

        def _process_a_file(data: typing.Tuple[str, int, int]) -> typing.Tuple[int, str, str]:
            filename, index, total = data
            pro2cmake_args = [pro2cmake]
            if args.is_example:
                pro2cmake_args.append('--is-example')
            pro2cmake_args.append(os.path.basename(filename))
            result = subprocess.run(pro2cmake_args,
                                    cwd=os.path.dirname(filename),
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = 'Converted[{}/{}]: {}\n'.format(index, total, filename)
            return result.returncode, filename, stdout + result.stdout.decode()

        for return_code, filename, stdout in pool.map(_process_a_file,
                                                      zip(all_files,
                                                          range(1, files_count + 1),
                                                          (files_count for _ in all_files))):
            if return_code:
                failed_files.append(filename)
            print(stdout)

    return failed_files


def main() -> None:
    args = parse_command_line()

    script_path = os.path.dirname(os.path.abspath(__file__))
    pro2cmake = os.path.join(script_path, 'pro2cmake.py')
    base_path = args.path

    all_files = find_all_pro_files(base_path, args)
    files_count = len(all_files)
    failed_files = run(all_files, pro2cmake, args)
    if len(all_files) == 0:
        print('No files found.')

    if failed_files:
        print('The following files were not successfully '
              'converted ({} of {}):'.format(len(failed_files), files_count))
        for f in failed_files:
            print('    "{}"'.format(f))


if __name__ == '__main__':
    main()
