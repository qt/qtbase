# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
