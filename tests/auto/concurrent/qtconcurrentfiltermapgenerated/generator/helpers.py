# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


def insert_testcases_into_file(filename, testcases):
    # assume testcases is an array of tuples of (declaration, definition)
    with open(filename) as f:
        inputlines = f.readlines()
    outputlines = []
    skipping = False
    for line in inputlines:
        if not skipping:
            outputlines.append(line)
        else:
            if "END_GENERATED" in line:
                outputlines.append(line)
                skipping = False

        if "START_GENERATED_SLOTS" in line:
            # put in testcases
            outputlines += [t[0] for t in testcases]
            skipping = True

        if "START_GENERATED_IMPLEMENTATIONS" in line:
            # put in testcases
            outputlines += [t[1] for t in testcases]
            skipping = True

    if outputlines != inputlines:
        with open(filename, "w") as f:
            f.writelines(outputlines)
