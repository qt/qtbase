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

#include <QtCore>

void parseHtmlFile(QTextStream &out, const QString &fileName) {
    QFile file(fileName);

    out << "Analysis of HTML file: " << fileName << endl;

    if (!file.open(QIODevice::ReadOnly)) {
        out << "  Couldn't open the file." << endl << endl << endl;
        return;
    }

//! [0]
    QXmlStreamReader reader(&file);
//! [0]

//! [1]
    int paragraphCount = 0;
    QStringList links;
    QString title;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "title")
                title = reader.readElementText();
            else if(reader.name() == "a")
                links.append(reader.attributes().value("href").toString());
            else if(reader.name() == "p")
                ++paragraphCount;
        }
    }
//! [1]

//! [2]
    if (reader.hasError()) {
        out << "  The HTML file isn't well-formed: " << reader.errorString()
            << endl << endl << endl;
        return;
    }
//! [2]

    out << "  Title: \"" << title << "\"" << endl
        << "  Number of paragraphs: " << paragraphCount << endl
        << "  Number of links: " << links.size() << endl
        << "  Showing first few links:" << endl;

    while(links.size() > 5)
        links.removeLast();

    foreach(QString link, links)
        out << "    " << link << endl;
    out << endl << endl;
}

int main(int argc, char **argv)
{
    // initialize QtCore application
    QCoreApplication app(argc, argv);

    // get a list of all html files in the current directory
    QStringList filter;
    filter << "*.htm";
    filter << "*.html";

    QStringList htmlFiles = QDir(":/").entryList(filter, QDir::Files);

    QTextStream out(stdout);

    if (htmlFiles.isEmpty()) {
        out << "No html files available.";
        return 1;
    }

    // parse each html file and write the result to file/stream
    foreach(QString file, htmlFiles)
        parseHtmlFile(out, ":/" + file);

    return 0;
}
