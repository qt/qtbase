// Copyright (C) 2018 Intel Corporation.
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

    const Converter *inconv = nullptr;
    QString format = parser.value(inputFormatOption);
    if (format != "auto"_L1) {
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->name() == format) {
                inconv = conv;
                break;
            }
        }

        if (!inconv) {
            fprintf(stderr, "Unknown file format \"%s\"\n", qPrintable(format));
            return EXIT_FAILURE;
        }
    }

    const Converter *outconv = nullptr;
    format = parser.value(outputFormatOption);
    if (format != "auto"_L1) {
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->name() == format) {
                outconv = conv;
                break;
            }
        }

        if (!outconv) {
            fprintf(stderr, "Unknown file format \"%s\"\n", qPrintable(format));
            return EXIT_FAILURE;
        }
    }

    QStringList files = parser.positionalArguments();
    QFile input(files.value(0));
    QFile output(files.value(1));

    if (input.fileName().isEmpty())
        input.open(stdin, QIODevice::ReadOnly);
    else
        input.open(QIODevice::ReadOnly);
    if (!input.isOpen()) {
        fprintf(stderr, "Could not open \"%s\" for reading: %s\n",
                qPrintable(input.fileName()), qPrintable(input.errorString()));
        return EXIT_FAILURE;
    }

    if (output.fileName().isEmpty())
        output.open(stdout, QIODevice::WriteOnly | QIODevice::Truncate);
    else
        output.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!output.isOpen()) {
        fprintf(stderr, "Could not open \"%s\" for writing: %s\n",
                qPrintable(output.fileName()), qPrintable(output.errorString()));
        return EXIT_FAILURE;
    }

    if (!inconv) {
        // probe the input to find a file format
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->directions().testFlag(Converter::Direction::In)
                && conv->probeFile(&input)) {
                inconv = conv;
                break;
            }
        }

        if (!inconv) {
            fprintf(stderr, "Could not determine input format. pass -I option.\n");
            return EXIT_FAILURE;
        }
    }

    if (!outconv) {
        // probe the output to find a file format
        for (const Converter *conv : std::as_const(*availableConverters)) {
            if (conv->directions().testFlag(Converter::Direction::Out)
                && conv->probeFile(&output)) {
                outconv = conv;
                break;
            }
        }
    }

    // now finally perform the conversion
    QVariant data = inconv->loadFile(&input, outconv);
    Q_ASSERT_X(outconv, "Converter Tool",
               "Internal error: converter format did not provide default");
    outconv->saveFile(&output, data, parser.values(optionOption));
    return EXIT_SUCCESS;
}
