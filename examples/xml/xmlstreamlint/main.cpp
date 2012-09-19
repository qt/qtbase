/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>

/*
 This class exists for the sole purpose of creating a translation context.
*/
class XmlStreamLint
{
public:
    Q_DECLARE_TR_FUNCTIONS(XmlStreamLint)
};

int main(int argc, char *argv[])
{
    enum ExitCode
    {
        Success,
        ParseFailure,
        ArgumentError,
        WriteError,
        FileFailure
    };

    QCoreApplication app(argc, argv);

    QTextStream errorStream(stderr);

    if (argc != 2)
    {
        errorStream << XmlStreamLint::tr(
                       "Usage: xmlstreamlint <path to XML file>\n");
        return ArgumentError;
    }

    QString inputFilePath(QCoreApplication::arguments().at(1));
    QFile inputFile(inputFilePath);

    if (!QFile::exists(inputFilePath))
    {
        errorStream << XmlStreamLint::tr(
                       "File %1 does not exist.\n").arg(inputFilePath);
        return FileFailure;

    } else if (!inputFile.open(QIODevice::ReadOnly)) {
        errorStream << XmlStreamLint::tr(
                       "Failed to open file %1.\n").arg(inputFilePath);
        return FileFailure;
    }

    QFile outputFile;
    if (!outputFile.open(stdout, QIODevice::WriteOnly))
    {
        errorStream << XmlStreamLint::tr("Failed to open stdout.");
        return WriteError;
    }

//! [0]
    QXmlStreamReader reader(&inputFile);
    QXmlStreamWriter writer(&outputFile);
//! [0]

//! [1]
    while (!reader.atEnd())
    {
        reader.readNext();

        if (reader.error())
        {
            errorStream << XmlStreamLint::tr(
                           "Error: %1 in file %2 at line %3, column %4.\n").arg(
                               reader.errorString(), inputFilePath,
                               QString::number(reader.lineNumber()),
                               QString::number(reader.columnNumber()));
            return ParseFailure;
//! [1]

//! [2]
        } else
            writer.writeCurrentToken(reader);
    }
//! [2]

    return Success;
}
