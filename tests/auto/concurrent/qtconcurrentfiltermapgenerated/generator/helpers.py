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
