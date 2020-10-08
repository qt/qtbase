#############################################################################
#
# Copyright (C) 2020 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of the test suite of the Qt Toolkit.
#
# $QT_BEGIN_LICENSE:GPL-EXCEPT$
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
# $QT_END_LICENSE$
#
#############################################################################

from option_management import function_describing_options, skip_function_description, testcase_describing_options
from generate_testcase import generate_testcase
from helpers import insert_testcases_into_file
filename = "../tst_qtconcurrentfiltermapgenerated.cpp"

testcases = []
counter = 0
for fo in function_describing_options():
    if skip_function_description(fo):
        continue

    if not (
        fo["blocking"]
        and fo["filter"]
        # and not fo["map"]
        and fo["reduce"]
        and not fo["inplace"]
        and not fo["iterators"]
        and not fo["initialvalue"]
        and not fo["pool"]
    ):
        continue

    for to in testcase_describing_options(fo):
        print("generate test")
        testcases.append(generate_testcase(fo, to))
        counter += 1

print(counter)
insert_testcases_into_file(filename, testcases)
