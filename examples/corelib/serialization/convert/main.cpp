// Copyright (C) 2018 Intel Corporation.
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "converter.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <stdio.h>

using namespace Qt::StringLiterals;

static const Converter *prepareConverter(QString format, Converter::Direction direction,
                                         QFile *stream)
{
    const bool out = direction == Converter::Direction::Out;
    const QIODevice::OpenMode mode = out
        ? QIODevice::WriteOnly | QIODevice::Truncate
        : QIODevice::ReadOnly;
    const char *dirn = out ? "output" : "input";

    bool isOpen;

    if (stream->fileName().isEmpty())
        isOpen = stream->open(out ? stdout : stdin, mode);
    else
        isOpen = stream->open(mode);

    if (!isOpen) {
        qFatal("Could not open \"%s\" for %s: %s",
               qPrintable(stream->fileName()), dirn, qPrintable(stream->errorString()));
    } else if (format == "auto"_L1) {
        for (const Converter *conv : Converter::allConverters()) {
            if (conv->directions().testFlag(direction) && conv->probeFile(stream))
                return conv;
        }
        if (out) // Failure to identify output format can be remedied by loadFile().
            return nullptr;

        // Input format, however, we must know before we can call that:
        qFatal("Could not determine input format. Specify it with the -I option.");
    } else {
        for (const Converter *conv : Converter::allConverters()) {
            if (conv->name() == format) {
                if (!conv->directions().testFlag(direction)) {
                    qWarning("File format \"%s\" cannot be used for %s",
                             qPrintable(format), dirn);
                    continue; // on the off chance there's another with the same name
                }
                return conv;
            }
        }
        qFatal("Unknown %s file format \"%s\"", dirn, qPrintable(format));
    }
    Q_UNREACHABLE_RETURN(nullptr);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//! [0]
    QStringList inputFormats;
    QStringList outputFormats;
    for (const Converter *conv : Converter::allConverters()) {
        auto direction = conv->directions();
        QString name = conv->name();
        if (direction.testFlag(Converter::Direction::In))
            inputFormats << name;
        if (direction.testFlag(Converter::Direction::Out))
            outputFormats << name;
    }
//! [0]
    inputFormats.sort();
    outputFormats.sort();
    inputFormats.prepend("auto"_L1);
    outputFormats.prepend("auto"_L1);

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt serialization format conversion tool"_L1);
    parser.addHelpOption();

    QCommandLineOption inputFormatOption(QStringList{ "I"_L1, "input-format"_L1 });
    inputFormatOption.setDescription(
            "Select the input format for the input file. Available formats: "_L1
            + inputFormats.join(", "_L1));
    inputFormatOption.setValueName("format"_L1);
    inputFormatOption.setDefaultValue(inputFormats.constFirst());
    parser.addOption(inputFormatOption);

    QCommandLineOption outputFormatOption(QStringList{ "O"_L1, "output-format"_L1 });
    outputFormatOption.setDescription(
            "Select the output format for the output file. Available formats: "_L1
            + outputFormats.join(", "_L1));
    outputFormatOption.setValueName("format"_L1);
    outputFormatOption.setDefaultValue(outputFormats.constFirst());
    parser.addOption(outputFormatOption);

    QCommandLineOption optionOption(QStringList{ "o"_L1, "option"_L1 });
    optionOption.setDescription(
        "Format-specific options. Use --format-options to find out what options are available."_L1);
    optionOption.setValueName("options..."_L1);
    optionOption.setDefaultValues({});
    parser.addOption(optionOption);

    QCommandLineOption formatOptionsOption("format-options"_L1);
    formatOptionsOption.setDescription(
        "Prints the list of valid options for --option for the converter format <format>."_L1);
    formatOptionsOption.setValueName("format"_L1);
    parser.addOption(formatOptionsOption);

    parser.addPositionalArgument("[source]"_L1, "File to read from (stdin if none)"_L1);
    parser.addPositionalArgument("[destination]"_L1, "File to write to (stdout if none)"_L1);

    parser.process(app);

    if (parser.isSet(formatOptionsOption)) {
        QString format = parser.value(formatOptionsOption);
//! [1]
        for (const Converter *conv : Converter::allConverters()) {
            if (conv->name() == format) {
                const char *help = conv->optionsHelp();
                if (help) {
                    qInfo("The following options are available for format '%s':\n\n%s",
                          qPrintable(format), help);
                } else {
                    qInfo("Format '%s' supports no options.", qPrintable(format));
                }
                return EXIT_SUCCESS;
            }
        }
//! [1]

        qFatal("Unknown file format '%s'", qPrintable(format));
    }

//! [2]
    QStringList files = parser.positionalArguments();
    QFile input(files.value(0));
    QFile output(files.value(1));
    const Converter *inconv = prepareConverter(parser.value(inputFormatOption),
                                               Converter::Direction::In, &input);
    const Converter *outconv = prepareConverter(parser.value(outputFormatOption),
                                                Converter::Direction::Out, &output);

    // Now finally perform the conversion:
    QVariant data = inconv->loadFile(&input, outconv);
    Q_ASSERT_X(outconv, "Serialization Converter",
               "Internal error: converter format did not provide default");
    outconv->saveFile(&output, data, parser.values(optionOption));
    return EXIT_SUCCESS;
//! [2]
}
