# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import textwrap
import time
from subprocess import Popen, PIPE

from function_signature import build_function_signature, build_function_name
from option_management import need_separate_output_sequence, qt_quirk_case


def InputSequenceItem(toptions):
    if toptions["inputitemtype"] == "standard":
        return "SequenceItem<tag_input>"
    if toptions["inputitemtype"] == "noconstruct":
        return "NoConstructSequenceItem<tag_input>"
    if toptions["inputitemtype"] == "moveonly":
        return "MoveOnlySequenceItem<tag_input>"
    if toptions["inputitemtype"] == "moveonlynoconstruct":
        return "MoveOnlyNoConstructSequenceItem<tag_input>"
    assert False


def InputSequence(toptions):
    item = InputSequenceItem(toptions)
    if toptions["inputsequence"] == "standard":
        return "std::vector<{}>".format(item)
    if toptions["inputsequence"] == "moveonly":
        return "MoveOnlyVector<{}>".format(item)
    assert False


def InputSequenceInitializationString(toptions):
    t = InputSequenceItem(toptions)
    # construct IILE
    mystring = ("[](){" + InputSequence(toptions) + " result;\n"
                + "\n".join("result.push_back({}({}, true));".format(t, i) for i in range(1, 7))
                + "\n return result; }()")
    return mystring


def OutputSequenceItem(toptions):
    if toptions["map"] and (toptions["inplace"] or toptions["maptype"] == "same"):
        return InputSequenceItem(toptions)

    if toptions["map"]:
        if toptions["mappeditemtype"] == "standard":
            return "SequenceItem<tag_mapped>"
        if toptions["mappeditemtype"] == "noconstruct":
            return "NoConstructSequenceItem<tag_mapped>"
        if toptions["mappeditemtype"] == "moveonly":
            return "MoveOnlySequenceItem<tag_mapped>"
        if toptions["mappeditemtype"] == "moveonlynoconstruct":
            return "MoveOnlyNoConstructSequenceItem<tag_mapped>"
        assert(False)
    else:
        return InputSequenceItem(toptions)


def ReducedItem(toptions):
    if toptions["reductiontype"] == "same":
        return OutputSequenceItem(toptions)
    else:
        if toptions["reductionitemtype"] == "standard":
            return "SequenceItem<tag_reduction>"
        if toptions["reductionitemtype"] == "noconstruct":
            return "NoConstructSequenceItem<tag_reduction>"
        if toptions["reductionitemtype"] == "moveonly":
            return "MoveOnlySequenceItem<tag_reduction>"
        if toptions["reductionitemtype"] == "moveonlynoconstruct":
            return "MoveOnlyNoConstructSequenceItem<tag_reduction>"
        assert(False)


def OutputSequence(toptions):
    item = OutputSequenceItem(toptions)
    # quirk of qt: not a QFuture<Sequence> is returned
    if qt_quirk_case(toptions):
        return "QList<{}>".format(item)

    needs_extra = need_separate_output_sequence(toptions)
    if not needs_extra:
        return InputSequence(toptions)


    if toptions["outputsequence"] == "standard":
        return "std::vector<{}>".format(item)
    if toptions["outputsequence"] == "moveonly":
        return "MoveOnlyVector<{}>".format(item)
    assert False


def resultData(toptions):
    result = [1, 2, 3, 4, 5, 6]
    if toptions["filter"]:
        result = filter(lambda x: x % 2 == 1, result)
    if toptions["map"]:
        result = map(lambda x: 2 * x, result)
    if toptions["reduce"]:
        result = sum(result)
    return result


def OutputSequenceInitializationString(toptions):
    t = OutputSequenceItem(toptions)
    # construct IILE
    mystring = ("[](){" + OutputSequence(toptions) + " result;\n"
                + "\n".join("result.push_back({}({}, true));".format(t, i) for i in resultData(toptions))
                + "\n return result; }()")
    return mystring


def OutputScalarInitializationString(toptions):
    result = resultData(toptions)
    assert isinstance(result, int)
    return ReducedItem(toptions) + "(" + str(result) + ", true)"


def FilterInitializationString(toptions):
    item = InputSequenceItem(toptions)
    if toptions["filterfunction"] == "function":
        return "myfilter<{}>".format(item)
    if toptions["filterfunction"] == "functor":
        return "MyFilter<{}>{{}}".format(item)
    if toptions["filterfunction"] == "memberfunction":
        return "&{}::isOdd".format(item)
    if toptions["filterfunction"] == "lambda":
        return "[](const {}& x){{ return myfilter<{}>(x); }}".format(item, item)
    if toptions["filterfunction"] == "moveonlyfunctor":
        return "MyMoveOnlyFilter<{}>{{}}".format(item)
    assert False


def MapInitializationString(toptions):
    oldtype = InputSequenceItem(toptions)
    newtype = OutputSequenceItem(toptions)
    if toptions["inplace"]:
        assert oldtype == newtype
        if toptions["mapfunction"] == "function":
            return "myInplaceMap<{}>".format(oldtype)
        if toptions["mapfunction"] == "functor":
            return "MyInplaceMap<{}>{{}}".format(oldtype)
        if toptions["mapfunction"] == "memberfunction":
            return "&{}::multiplyByTwo".format(oldtype)
        if toptions["mapfunction"] == "lambda":
            return "[]({}& x){{ return myInplaceMap<{}>(x); }}".format(oldtype, oldtype)
        if toptions["mapfunction"] == "moveonlyfunctor":
            return "MyMoveOnlyInplaceMap<{}>{{}}".format(oldtype)
        assert False
    else:
        if toptions["mapfunction"] == "function":
            return "myMap<{f},{t}>".format(f=oldtype, t=newtype)
        if toptions["mapfunction"] == "functor":
            return "MyMap<{f},{t}>{{}}".format(f=oldtype, t=newtype)
        if toptions["mapfunction"] == "memberfunction":
            return "&{}::multiplyByTwo".format(newtype)
        if toptions["mapfunction"] == "lambda":
            return "[](const {f}& x){{ return myMap<{f},{t}>(x); }}".format(f=oldtype, t=newtype)
        if toptions["mapfunction"] == "moveonlyfunctor":
            return "MyMoveOnlyMap<{f},{t}>{{}}".format(f=oldtype, t=newtype)
        assert False


def ReductionInitializationString(toptions):
    elementtype = OutputSequenceItem(toptions)
    sumtype = ReducedItem(toptions)

    if toptions["reductionfunction"] == "function":
        return "myReduce<{f},{t}>".format(f=elementtype, t=sumtype)
    if toptions["reductionfunction"] == "functor":
        return "MyReduce<{f},{t}>{{}}".format(f=elementtype, t=sumtype)
    if toptions["reductionfunction"] == "lambda":
        return "[]({t}& sum, const {f}& x){{ return myReduce<{f},{t}>(sum, x); }}".format(f=elementtype, t=sumtype)
    if toptions["reductionfunction"] == "moveonlyfunctor":
        return "MyMoveOnlyReduce<{f},{t}>{{}}".format(f=elementtype, t=sumtype)
    assert False


def ReductionInitialvalueInitializationString(options):
    return ReducedItem(options) + "(0, true)"


def function_template_args(options):
    args = []
    out_s = OutputSequence(options)
    in_s = InputSequence(options)
    if options["reduce"] and options["reductionfunction"] == "lambda":
        args.append(ReducedItem(options))
    elif out_s != in_s:
        if not qt_quirk_case(options):
            args.append(out_s)

    if len(args):
        return "<" + ", ".join(args) + ">"

    return ""


numcall = 0


def generate_testcase(function_options, testcase_options):
    options = {**function_options, **testcase_options}

    option_description = "\n".join("  {}={}".format(
        a, b) for a, b in testcase_options.items())
    option_description = textwrap.indent(option_description, " "*12)
    function_signature = textwrap.indent(build_function_signature(function_options), " "*12)
    testcase_name = "_".join("{}_{}".format(x, y) for x, y in options.items())
    global numcall
    numcall += 1
    testcase_name = "test" + str(numcall)
    function_name = build_function_name(options)

    arguments = []

    template_args = function_template_args(options)

    # care about the thread pool
    if options["pool"]:
        pool_initialization = """QThreadPool pool;
    pool.setMaxThreadCount(1);"""
        arguments.append("&pool")
    else:
        pool_initialization = ""

    # care about the input sequence
    input_sequence_initialization_string = InputSequenceInitializationString(options)

    if "inputsequencepassing" in options and options["inputsequencepassing"] == "lvalue" or options["inplace"]:
        input_sequence_initialization = "auto input_sequence = " + \
                                        input_sequence_initialization_string + ";"
        arguments.append("input_sequence")
    elif "inputsequencepassing" in options and options["inputsequencepassing"] == "rvalue":
        input_sequence_initialization = ""
        arguments.append(input_sequence_initialization_string)
    else:
        input_sequence_initialization = "auto input_sequence = " + \
                                        input_sequence_initialization_string + ";"
        arguments.append("input_sequence.begin()")
        arguments.append("input_sequence.end()")

    # care about the map:
    if options["map"]:
        map_initialization_string = MapInitializationString(options)
        if options["mapfunctionpassing"] == "lvalue":
            map_initialization = "auto map = " + map_initialization_string + ";"
            arguments.append("map")
        elif options["mapfunctionpassing"] == "rvalue":
            map_initialization = ""
            arguments.append(map_initialization_string)
        else:
            assert False
    else:
        map_initialization = ""

    # care about the filter
    if options["filter"]:
        filter_initialization_string = FilterInitializationString(options)
        if options["filterfunctionpassing"] == "lvalue":
            filter_initialization = "auto filter = " + filter_initialization_string + ";"
            arguments.append("filter")
        elif options["filterfunctionpassing"] == "rvalue":
            filter_initialization = ""
            arguments.append(filter_initialization_string)
        else:
            assert (False)
    else:
        filter_initialization = ""

    reduction_initialvalue_initialization = ""
    # care about reduction
    if options["reduce"]:
        reduction_initialization_expression = ReductionInitializationString(options)
        if options["reductionfunctionpassing"] == "lvalue":
            reduction_initialization = "auto reductor = " + reduction_initialization_expression + ";"
            arguments.append("reductor")
        elif options["reductionfunctionpassing"] == "rvalue":
            reduction_initialization = ""
            arguments.append(reduction_initialization_expression)
        else:
            assert (False)

        # initialvalue:
        if options["initialvalue"]:
            reduction_initialvalue_initialization_expression = ReductionInitialvalueInitializationString(options)
            if options["reductioninitialvaluepassing"] == "lvalue":
                reduction_initialvalue_initialization = "auto initialvalue = " + reduction_initialvalue_initialization_expression + ";"
                arguments.append("initialvalue")
            elif options["reductioninitialvaluepassing"] == "rvalue":
                reduction_initialvalue_initialization = ""
                arguments.append(reduction_initialvalue_initialization_expression)
            else:
                assert (False)

        if options["reductionoptions"] == "UnorderedReduce":
            arguments.append("QtConcurrent::UnorderedReduce")
        elif options["reductionoptions"] == "OrderedReduce":
            arguments.append("QtConcurrent::OrderedReduce")
        elif options["reductionoptions"] == "SequentialReduce":
            arguments.append("QtConcurrent::SequentialReduce")
        else:
            assert options["reductionoptions"] == "unspecified"
    else:
        reduction_initialization = ""

    # what is the expected result
    if options["filter"]:
        if not options["reduce"]:
            expected_result_expression = OutputSequenceInitializationString(options)
        else:
            expected_result_expression = OutputScalarInitializationString(options)
    elif options["map"]:
        if not options["reduce"]:
            expected_result_expression = OutputSequenceInitializationString(options)
        else:
            expected_result_expression = OutputScalarInitializationString(options)

    wait_result_expression = ""
    if options["inplace"]:
        if options["blocking"]:
            result_accepting = ""
            result_variable = "input_sequence"
        else:
            result_accepting = "auto future = "
            result_variable = "input_sequence"
            wait_result_expression = "future.waitForFinished();"
    elif options["blocking"]:
        result_accepting = "auto result = "
        result_variable = "result"
    else:
        if not options["reduce"]:
            result_accepting = "auto result = "
            result_variable = "result.results()"
        else:
            result_accepting = "auto result = "
            result_variable = "result.takeResult()"

    arguments_passing = ", ".join(arguments)
    final_string = f"""
        void tst_QtConcurrentFilterMapGenerated::{testcase_name}()
        {{
            /* test for
{function_signature}

            with
{option_description}
            */

            {pool_initialization}
            {input_sequence_initialization}
            {filter_initialization}
            {map_initialization}
            {reduction_initialization}
            {reduction_initialvalue_initialization}

            {result_accepting}QtConcurrent::{function_name}{template_args}({arguments_passing});

            auto expected_result = {expected_result_expression};
            {wait_result_expression}
            QCOMPARE({result_variable}, expected_result);
        }}
        """
    p = Popen(["clang-format"], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    final_string = p.communicate(final_string.encode())[0].decode()

    return (f"    void {testcase_name}();\n", final_string)
