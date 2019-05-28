/****************************************************************************
 **
 ** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QTextDocument>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Converts the Qt-supported subset of HTML to Markdown.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QGuiApplication::translate("main", "input"),
                                 QGuiApplication::translate("main", "input file"));
    parser.addPositionalArgument(QGuiApplication::translate("main", "output"),
                                 QGuiApplication::translate("main", "output file"));
    parser.process(app);
    if (parser.positionalArguments().count() != 2)
        parser.showHelp(1);

    QFile inFile(parser.positionalArguments().first());
    if (!inFile.open(QIODevice::ReadOnly)) {
        qFatal("failed to open %s for reading", parser.positionalArguments().first().toLocal8Bit().data());
        exit(2);
    }
    QFile outFile(parser.positionalArguments().at(1));
    if (!outFile.open(QIODevice::WriteOnly)) {
        qFatal("failed to open %s for writing", parser.positionalArguments().at(1).toLocal8Bit().data());
        exit(2);
    }
    QTextDocument doc;
    doc.setHtml(QString::fromUtf8(inFile.readAll()));
    outFile.write(doc.toMarkdown().toUtf8());
}
