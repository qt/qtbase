// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>

void parseHtmlFile(QTextStream &out, const QString &fileName)
{
    QFile file(fileName);

    out << "Analysis of HTML file: " << fileName << Qt::endl;

    if (!file.open(QIODevice::ReadOnly)) {
        out << "  Couldn't open the file." << Qt::endl << Qt::endl << Qt::endl;
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
            if (reader.name() == QLatin1String("title"))
                title = reader.readElementText();
            else if (reader.name() == QLatin1String("a"))
                links.append(reader.attributes().value(QLatin1String("href")).toString());
            else if (reader.name() == QLatin1String("p"))
                ++paragraphCount;
        }
    }
//! [1]

//! [2]
    if (reader.hasError()) {
        out << "  The HTML file isn't well-formed: " << reader.errorString()
            << Qt::endl << Qt::endl << Qt::endl;
        return;
    }
//! [2]

    out << "  Title: \"" << title << '"' << Qt::endl
        << "  Number of paragraphs: " << paragraphCount << Qt::endl
        << "  Number of links: " << links.size() << Qt::endl
        << "  Showing first few links:" << Qt::endl;

    while (links.size() > 5)
        links.removeLast();

    for (const QString &link : std::as_const(links))
        out << "    " << link << Qt::endl;
    out << Qt::endl << Qt::endl;
}

int main(int argc, char **argv)
{
    // initialize Qt Core application
    QCoreApplication app(argc, argv);

    // get a list of all html files in the current directory
    const QStringList filter = { QStringLiteral("*.htm"),
                                 QStringLiteral("*.html") };

    const QStringList htmlFiles = QDir(QStringLiteral(":/")).entryList(filter, QDir::Files);

    QTextStream out(stdout);

    if (htmlFiles.isEmpty()) {
        out << "No html files available.";
        return 1;
    }

    // parse each html file and write the result to file/stream
    for (const QString &file : htmlFiles)
        parseHtmlFile(out, QStringLiteral(":/") + file);

    return 0;
}
