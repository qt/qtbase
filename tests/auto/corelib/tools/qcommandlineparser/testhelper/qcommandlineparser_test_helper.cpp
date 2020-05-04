/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineParser>

#include <stdio.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    // Test for QCoreApplication::arguments()
    const QStringList incomingArgs = QCoreApplication::arguments();
    for (int i = 0; i < argc; ++i) {
        if (incomingArgs.at(i) != QLatin1String(argv[i]))
            qDebug() << "ERROR: arguments[" << i << "] was" << incomingArgs.at(i) << "expected" << argv[i];
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("parsingMode", "The parsing mode to test.");
    parser.addPositionalArgument("command", "The command to execute.");
    parser.addOption(QCommandLineOption("load", "Load file from URL.", "url"));
    parser.addOption(QCommandLineOption(QStringList() << "o" << "output", "Set output file.", "file"));
    parser.addOption(QCommandLineOption("D", "Define macro.", "key=value"));
    parser.addOption(QCommandLineOption("long-option"));

    // An option with a longer description, to test wrapping
    QCommandLineOption noImplicitIncludesOption(QStringList() << QStringLiteral("n") << QStringLiteral("no-implicit-includes"));
    noImplicitIncludesOption.setDescription(QStringLiteral("Disable magic generation of implicit #include-directives."));
    parser.addOption(noImplicitIncludesOption);

    QCommandLineOption newlineOption(QStringList() << QStringLiteral("newline"));
    newlineOption.setDescription(QString::fromLatin1("This is an option with a rather long\n"
                "description using explicit newline characters "
                "(but testing automatic wrapping too). In addition, "
                "here, we test breaking after a comma. Testing -option. "
                "Long URL: http://qt-project.org/wiki/How_to_create_a_library_with_Qt_and_use_it_in_an_application "
                "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    parser.addOption(newlineOption);

    // A hidden option
    QCommandLineOption hiddenOption(QStringList() << QStringLiteral("hidden"));
    hiddenOption.setDescription(QStringLiteral("THIS SHOULD NEVER APPEAR"));
    hiddenOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(hiddenOption);
#if QT_DEPRECATED_SINCE(5, 8)
    QCommandLineOption hiddenOption2(QStringList() << QStringLiteral("hidden2"));
    hiddenOption2.setDescription(QStringLiteral("NEITHER SHOULD THIS"));
    hiddenOption2.setHidden(true);
    parser.addOption(hiddenOption2);
#endif

    // This program supports different options depending on the "command" (first argument).
    // Call parse() to find out the positional arguments.
    parser.parse(QCoreApplication::arguments());

    QStringList args = parser.positionalArguments();
    if (args.isEmpty())
        parser.showHelp(1);
    parser.setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode(args.takeFirst().toInt()));
    const QString command = args.isEmpty() ? QString() : args.first();
    if (command == "resize") {
        parser.clearPositionalArguments();
        parser.addPositionalArgument("resize", "Resize the object to a new size.", "resize [resize_options]");
        parser.addOption(QCommandLineOption("size", "New size.", "size"));
        parser.process(app);
        const QString size = parser.value("size");
        printf("Resizing %s to %s and saving to %s\n", qPrintable(parser.value("load")), qPrintable(size), qPrintable(parser.value("o")));
    } else if (command == "long") {
        // A very long option (QTBUG-79926)
        QCommandLineOption longOption(QStringList{QStringLiteral("looooooooooooong-option"), QStringLiteral("looooong-opt-alias")});
        longOption.setDescription(QStringLiteral("Short description"));
        longOption.setValueName(QStringLiteral("looooooooooooong-value-name"));
        parser.addOption(longOption);
        parser.process(app);
    } else {
        // Call process again, to handle unknown options this time.
        parser.process(app);
    }

    printf("Positional arguments: %s\n", qPrintable(parser.positionalArguments().join(",")));
    printf("Macros: %s\n", qPrintable(parser.values("D").join(",")));

    return 0;
}

