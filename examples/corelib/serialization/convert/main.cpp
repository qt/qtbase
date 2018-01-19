/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "converter.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <stdio.h>

static QVector<Converter *> *availableConverters;

Converter::Converter()
{
    if (!availableConverters)
        availableConverters = new QVector<Converter *>;
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
    for (Converter *conv : qAsConst(*availableConverters)) {
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
        for (Converter *conv : qAsConst(*availableConverters)) {
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
        for (Converter *conv : qAsConst(*availableConverters)) {
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
        for (Converter *conv : qAsConst(*availableConverters)) {
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
        for (Converter *conv : qAsConst(*availableConverters)) {
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
        for (Converter *conv : qAsConst(*availableConverters)) {
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
