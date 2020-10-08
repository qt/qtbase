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

import pandas as pd
from option_management import function_describing_options
from function_signature import build_function_signature


def generate_excel_file_of_functions(filename):
    olist = []
    for options in function_describing_options():
        # filter out unneccesary cases:
        if options["reduce"] and options["inplace"]:
            # we cannot do a reduction in-place
            options["comment"] = "reduce-inplace:nonsense"
            options["signature"] = ""

        elif options["initialvalue"] and not options["reduce"]:
            options["comment"] = "initial-noreduce:nonsense"
            options["signature"] = ""

        elif not options["reduce"] and not options["map"] and not options["filter"]:
            # no operation at all
            options["comment"] = "noop"
            options["signature"] = ""

        else:
            options["comment"] = ""
            if options["map"] and options["filter"]:
                options["implemented"] = "no:filter+map"
            elif not options["map"] and not options["filter"]:
                options["implemented"] = "no:nofilter+nomap"
            elif options["inplace"] and options["iterators"] and options["filter"]:
                options["implemented"] = "no:inplace+iterator+filter"
            else:
                options["implemented"] = "yes"

            options["signature"] = build_function_signature(options)

        olist.append(options)

    df = pd.DataFrame(olist)
    df.to_excel(filename)


generate_excel_file_of_functions("functions.xls")
