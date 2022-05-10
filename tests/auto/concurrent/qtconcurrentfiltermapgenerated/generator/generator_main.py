# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
