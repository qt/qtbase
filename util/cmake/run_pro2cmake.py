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
import sys
import typing

script_path = os.path.dirname(os.path.abspath(__file__))
base_path = os.path.dirname(script_path)
pro2cmake = os.path.join(script_path, 'pro2cmake.py')

if len(sys.argv) > 1:
    base_path = os.path.abspath(sys.argv[1])

failed_files = []

all_files = glob.glob(os.path.join(base_path, '**/*.pro'), recursive=True)
files_count = len(all_files)

with concurrent.futures.ThreadPoolExecutor(initializer=os.nice, initargs=(10,)) as pool:

    def _process_a_file(data: typing.Tuple[str, int, int]) -> typing.Tuple[int, str, str]:
        filename, index, total = data
        result = subprocess.run((pro2cmake, os.path.basename(filename)),
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

if failed_files:
    print('The following files were not successfully '
          'converted ({} of {}):'.format(len(failed_files), files_count))
    for f in failed_files:
        print('    "{}"'.format(f))

