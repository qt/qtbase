# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import itertools


class Option:
    def __init__(self, name, possible_options):
        self.name = name
        self.possible_options = possible_options


class OptionManager:
    def __init__(self, name):
        self.options = {}
        self.name = name

    def add_option(self, option: Option):
        self.options[option.name] = option

    def iterate(self):
        for x in itertools.product(*[
            [(name, x) for x in self.options[name]]
            for name in self.options.keys()
        ]):
            yield dict(x)


def function_describing_options():
    om = OptionManager("function options")

    om.add_option(Option("blocking", [True, False]))
    om.add_option(Option("filter", [True, False]))
    om.add_option(Option("map", [False, True]))
    om.add_option(Option("reduce", [False, True]))
    om.add_option(Option("inplace", [True, False]))
    om.add_option(Option("iterators", [False, True]))
    om.add_option(Option("initialvalue", [False, True]))
    om.add_option(Option("pool", [True, False]))

    return om


def skip_function_description(options):
    if options["reduce"] and options["inplace"]:
        return "we cannot do a reduction in-place"

    if options["initialvalue"] and not options["reduce"]:
        return "without reduction, we do not need an initial value"

    if not options["reduce"] and not options["map"] and not options["filter"]:
        return "no operation at all"

    # the following things are skipped because Qt does not support them
    if options["filter"] and options["map"]:
        return "Not supported by Qt: both map and filter operation"

    if not options["filter"] and not options["map"]:
        return "Not supported by Qt: no map and no filter operation"

    if options["inplace"] and options["iterators"] and options["filter"]:
        return "Not supported by Qt: filter operation in-place with iterators"

    return False


def qt_quirk_case(options):
    # whenever a function should return a QFuture<Sequence>,
    # it returns a QFuture<item> instead
    if options["inplace"] or options["reduce"] or options["blocking"]:
        return False

    return True


def need_separate_output_sequence(options):
    # do we need an output sequence?
    if not (options["inplace"] or options["reduce"]):
        # transforming a sequence into a sequence
        if options["iterators"] or options["map"]:
            return True

    return False


def testcase_describing_options():
    om = OptionManager("testcase options")

    om.add_option(Option("inputsequence", ["standard", "moveonly"]))
    om.add_option(Option("inputsequencepassing", ["lvalue", "rvalue"]))
    om.add_option(Option("inputitemtype", ["standard", "noconstruct", "moveonly", "moveonlynoconstruct"]))

    om.add_option(Option("outputsequence", ["standard", "moveonly"]))

    om.add_option(Option("maptype", ["same", "different"]))
    om.add_option(Option("mappeditemtype", ["standard", "noconstruct", "moveonly", "moveonlynoconstruct"]))

    om.add_option(Option("reductiontype", ["same", "different"]))

    om.add_option(Option("reductionitemtype", [
        "standard", "noconstruct", "moveonly", "moveonlynoconstruct"]))

    om.add_option(Option("filterfunction", ["functor", "function", "memberfunction", "lambda", "moveonlyfunctor"]))
    om.add_option(Option("filterfunctionpassing", ["lvalue", "rvalue"]))

    om.add_option(Option("mapfunction", ["functor", "function", "memberfunction", "lambda", "moveonlyfunctor"]))
    om.add_option(Option("mapfunctionpassing", ["lvalue", "rvalue"]))

    om.add_option(Option("reductionfunction", ["functor", "function", "lambda", "moveonlyfunctor"]))
    om.add_option(Option("reductionfunctionpassing", ["lvalue", "rvalue"]))

    om.add_option(Option("reductioninitialvaluepassing", ["lvalue", "rvalue"]))

    om.add_option(Option("reductionoptions", [
        "unspecified", "UnorderedReduce", "OrderedReduce", "SequentialReduce"]))

    return om


def disabled_testcase_describing_options(options):
    disabled_options = []

    if options["inplace"] or options["iterators"]:
        disabled_options.append("inputsequencepassing")

    if not need_separate_output_sequence(options):
        disabled_options.append("outputsequence")

    if not options["map"]:
        disabled_options.append("mappeditemtype")

    if options["map"] and options["inplace"]:
        disabled_options.append("mappeditemtype")

    if not options["filter"]:
        disabled_options.append("filterfunction")
        disabled_options.append("filterfunctionpassing")

    if not options["map"]:
        disabled_options.append("mapfunction")
        disabled_options.append("mapfunctionpassing")

    if not options["reduce"]:
        disabled_options.append("reductionfunction")
        disabled_options.append("reductionfunctionpassing")

    if not options["reduce"]:
        disabled_options.append("reductiontype")
        disabled_options.append("reductioninitialvaluepassing")
        disabled_options.append("reductionoptions")
        disabled_options.append("reductionitemtype")

    if not options["initialvalue"]:
        disabled_options.append("reductioninitialvaluepassing")

    if not options["map"]:
        disabled_options.append("maptype")
    else:
        if options["inplace"]:
            disabled_options.append("maptype")

    return disabled_options


def skip_testcase_description(options):
    if (
            "maptype" in options and
            options["maptype"] == "same" and
            "inputitemtype" in options and "mappeditemtype" in options and
            (options["inputitemtype"] != options["mappeditemtype"])
    ):
        return ("Not possible: map should map to same type, "
                "but mapped item type should differ from input item type.")

    if (
            "reductiontype" in options and
            options["reductiontype"] == "same"):
        # we have to check that this is possible
        if ("mappeditemtype" in options and "reductionitemtype" in options
                and (options["mappeditemtype"] != options["reductionitemtype"])
        ):
            return ("Not possible: should reduce in the same type, "
                    "but reduction item type should differ from mapped item type.")
        if ("mappeditemtype" not in options
                and (options["inputitemtype"] != options["reductionitemtype"])):
            return ("Not possible: should reduce in the same type, "
                    "but reduction item type should differ from input item type.")

    if (
            options["map"] and not options["inplace"]
            and options["mapfunction"] == "memberfunction"
    ):
        return "map with memberfunction only available for in-place map"

    return False
