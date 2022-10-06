// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "converter.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <stdio.h>

static QList<Converter *> *availableConverters;

Converter::Converter()
{
    if (!availableConverters)
        availableConverters = new QList<Converter *>;
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
    for (Converter *conv : std::as_const(*availableConverters)) {
        auto direction = conv->directions();
        QString name = conv->name();
        if (direction & Converter::In)
            inputFormats << name;
        if (direction & Converter::Out)
            outputFormats << name;
    }
    inputFormats.sort();
    outputFormats.sort();
    inputFormats.prepend("auto");
    outputFormats.prepend("auto");

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt file format conversion tool"));
    parser.addHelpOption();

    QCommandLineOption inputFormatOption(QStringList{"I", "input-format"});
    inputFormatOption.setDescription(QLatin1String("Select the input format for the input file. Available formats: ") +
                                     inputFormats.join(", "));
    inputFormatOption.setValueName("format");
    inputFormatOption.setDefaultValue(inputFormats.constFirst());
    parser.addOption(inputFormatOption);

    QCommandLineOption outputFormatOption(QStringList{"O", "output-format"});
    outputFormatOption.setDescription(QLatin1String("Select the output format for the output file. Available formats: ") +
                                     outputFormats.join(", "));
    outputFormatOption.setValueName("format");
    outputFormatOption.setDefaultValue(outputFormats.constFirst());
    parser.addOption(outputFormatOption);

    QCommandLineOption optionOption(QStringList{"o", "option"});
    optionOption.setDescription(QStringLiteral("Format-specific options. Use --format-options to find out what options are available."));
    optionOption.setValueName("options...");
    optionOption.setDefaultValues({});
    parser.addOption(optionOption);

    QCommandLineOption formatOptionsOption("format-options");
    formatOptionsOption.setDescription(QStringLiteral("Prints the list of valid options for --option for the converter format <format>."));
    formatOptionsOption.setValueName("format");
    parser.addOption(formatOptionsOption);

    parser.addPositionalArgument(QStringLiteral("[source]"),
                                 QStringLiteral("File to read from (stdin if none)"));
    parser.addPositionalArgument(QStringLiteral("[destination]"),
                                 QStringLiteral("File to write to (stdout if none)"));

    parser.process(app);

    if (parser.isSet(formatOptionsOption)) {
        QString format = parser.value(formatOptionsOption);
        for (Converter *conv : std::as_const(*availableConverters)) {
            if (conv->name() == format) {
                const char *help = conv->optionsHelp();
                if (help)
                    printf("The following options are available for format '%s':\n\n%s", qPrintable(format), help);
                else
                    printf("Format '%s' supports no options.\n", qPrintable(format));
                return EXIT_SUCCESS;
            }
        }

        fprintf(stderr, "Unknown file format '%s'\n", qPrintable(format));
        return EXIT_FAILURE;
    }

    Converter *inconv = nullptr;
    QString format = parser.value(inputFormatOption);
    if (format != "auto") {
        for (Converter *conv : std::as_const(*availableConverters)) {
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

    Converter *outconv = nullptr;
    format = parser.value(outputFormatOption);
    if (format != "auto") {
        for (Converter *conv : std::as_const(*availableConverters)) {
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
        for (Converter *conv : std::as_const(*availableConverters)) {
            if (conv->directions() & Converter::In && conv->probeFile(&input)) {
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
        for (Converter *conv : std::as_const(*availableConverters)) {
            if (conv->directions() & Converter::Out && conv->probeFile(&output)) {
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
