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

static QList<const Converter *> *availableConverters;

Converter::Converter()
{
    if (!availableConverters)
        availableConverters = new QList<const Converter *>;
    availableConverters->append(this);
}

Converter::~Converter()
{
    availableConverters->removeAll(this);
}

static const Converter *prepareConverter(QString format, Converter::Direction direction,
                                         QFile *stream)
{
    const bool out = direction == Converter::Direction::Out;
    const QIODevice::OpenMode mode = out
        ? QIODevice::WriteOnly | QIODevice::Truncate
        : QIODevice::ReadOnly;
    const char *dirn = out ? "output" : "input";

    if (stream->fileName().isEmpty())
        stream->open(out ? stdout : stdin, mode);
    else
        stream->open(mode);

    if (!stream->isOpen()) {
        fprintf(stderr, "Could not open \"%s\" for %s: %s\n",
                qPrintable(stream->fileName()), dirn, qPrintable(stream->errorString()));
    } else if (format == "auto"_L1) {
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->directions().testFlag(direction) && conv->probeFile(stream))
                return conv;
        }
        if (out) // Failure to identify output format can be remedied by loadFile().
            return nullptr;

        // Input format, however, we must know before we can call that:
        fprintf(stderr, "Could not determine input format. Specify it with the -I option.\n");
    } else {
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->name() == format) {
                if (!conv->directions().testFlag(direction)) {
                    fprintf(stderr, "File format \"%s\" cannot be used for %s\n",
                            qPrintable(format), dirn);
                    continue; // on the off chance there's another with the same name
                }
                return conv;
            }
        }
        fprintf(stderr, "Unknown %s file format \"%s\"\n", dirn, qPrintable(format));
    }
    exit(EXIT_FAILURE);
    Q_UNREACHABLE_RETURN(nullptr);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList inputFormats;
    QStringList outputFormats;
    for (const Converter *conv : std::as_const(*availableConverters)) {
        auto direction = conv->directions();
        QString name = conv->name();
        if (direction.testFlag(Converter::Direction::In))
            inputFormats << name;
        if (direction.testFlag(Converter::Direction::Out))
            outputFormats << name;
    }
    inputFormats.sort();
    outputFormats.sort();
    inputFormats.prepend("auto"_L1);
    outputFormats.prepend("auto"_L1);

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt file format conversion tool"_L1);
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
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->name() == format) {
                const char *help = conv->optionsHelp();
                if (help) {
                    printf("The following options are available for format '%s':\n\n%s",
                           qPrintable(format), help);
                } else {
                    printf("Format '%s' supports no options.\n", qPrintable(format));
                }
                return EXIT_SUCCESS;
            }
        }

        fprintf(stderr, "Unknown file format '%s'\n", qPrintable(format));
        return EXIT_FAILURE;
    }

    QStringList files = parser.positionalArguments();
    QFile input(files.value(0));
    QFile output(files.value(1));
    const Converter *inconv = prepareConverter(parser.value(inputFormatOption),
                                               Converter::Direction::In, &input);
    const Converter *outconv = prepareConverter(parser.value(outputFormatOption),
                                                Converter::Direction::Out, &output);

    // now finally perform the conversion
    QVariant data = inconv->loadFile(&input, outconv);
    Q_ASSERT_X(outconv, "Converter Tool",
               "Internal error: converter format did not provide default");
    outconv->saveFile(&output, data, parser.values(optionOption));
    return EXIT_SUCCESS;
}
