/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QStringList>
#include <QVector>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <QFontMetrics>
#include <QImage>
#include <QXmlStreamReader>

static const int fontPixelSize = 25;
static const QLatin1String fontFamily("Series 60 Sans");

struct testDataSet
{
    QString language;
    QString name;
    QString input;
    QString inputOriginal;
    QString output;
    QString outputOriginal;
    QVector<uint> outputGlyphIDs;
    QString outputGlyphIDsOriginal;
};

QString charHexCsv2String(const QString &csv)
{
    QString result;
    foreach (const QString &charString, csv.split(QLatin1Char(','), QString::SkipEmptyParts)) {
        bool isOk;
        const uint charUInt = charString.toUInt(&isOk, 16);
        Q_ASSERT(isOk);
        const int size = charUInt >= SHRT_MAX ? 2:1;
        result.append(QString::fromUtf16((const ushort*)&charUInt, size));
    }
    return result;
}

QList<testDataSet> testDataSetList()
{
    QList<testDataSet> result;
    QFile file("glyphshaping_data.xml");
    const bool success = file.open(QIODevice::ReadOnly);
    Q_ASSERT(success);

    const QLatin1String language("language");
    const QLatin1String test("test");
    const QLatin1String inputUtf16("inpututf16");
    const QLatin1String outputUtf16("outpututf16");
    const QLatin1String outputGlyphIDs("outputglyphids");
    const QLatin1String name("name");

    QString languageName;

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        switch (token) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == language) {
                Q_ASSERT(reader.attributes().hasAttribute(name));
                languageName = reader.attributes().value(name).toString();
            } else if (reader.name() == test) {
                if (!reader.attributes().hasAttribute(outputUtf16)
                    && !reader.attributes().hasAttribute(outputGlyphIDs))
                    continue;
                Q_ASSERT(!languageName.isEmpty());
                Q_ASSERT(reader.attributes().hasAttribute(name));
                Q_ASSERT(reader.attributes().hasAttribute(inputUtf16));
                testDataSet set;
                set.language = languageName;
                set.name = reader.attributes().value(name).toString();
                set.inputOriginal = reader.attributes().value(inputUtf16).toString();
                set.input = charHexCsv2String(set.inputOriginal);
                set.outputOriginal = reader.attributes().value(outputUtf16).toString();
                set.output = charHexCsv2String(set.outputOriginal);
                set.outputGlyphIDsOriginal = reader.attributes().value(outputGlyphIDs).toString();
                result.append(set);
            }
            break;
        default:
            break;
        }
    }
    return result;
}

QImage renderedText(const QString &text, const QFont &font)
{
    const QFontMetrics metrics(font);
    const QRect boundingRect = metrics.boundingRect(text);
    QImage result(boundingRect.size(), QImage::Format_ARGB32);
    result.fill(0);

    QPainter p(&result);
    p.setFont(font);
    p.drawText(boundingRect.translated(-boundingRect.topLeft()), text);

    return result;
}

QString dumpImageHtml(const QString &text, const QString &pathName)
{
    if (text.isEmpty())
        return QLatin1String("<td/>");
    QFont font(fontFamily);
    font.setPixelSize(fontPixelSize);
    const QImage textImage = renderedText(text, font);
    const QString imageFileName =
            (pathName + QDir::separator() + QLatin1String("%1.png"))
            .arg(textImage.cacheKey());
    const bool success = textImage.save(imageFileName);
    Q_ASSERT(success);
    return
        QString::fromLatin1("<td title=\"%2\"><img src=\"%1\" alt=\"%2\" width=\"%3\" height=\"%4\"/></td>")
        .arg(QDir::cleanPath(imageFileName)).arg(text).arg(textImage.width()).arg(textImage.height());
}

QString dlItem(const QString &dt, const QString &dd)
{
    if (!dd.trimmed().isEmpty())
        return QString::fromLatin1("\t\t\t\t\t\t<dt>%1</dt><dd>%2</dd>\n").arg(dt).arg(dd);
    return QString();
}

bool dumpHtml(const QString &pathName)
{
    QFile htmlPage(pathName + QDir::separator() + QLatin1String("index.html"));
    if (!htmlPage.open(QFile::WriteOnly))
        return false;

    QString platformName = QString::fromLatin1(
#if defined(Q_OS_WIN)
            "Win32"
#elif defined(Q_WS_X11)
            "X11"
#else
            ""
#endif
    );

    QString result = QString::fromLatin1(
            "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
            " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
            "\t<head>\n"
            "\t\t<title>Qt on %1 glyph shaping (%2)</title>\n"
            "\t\t<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />\n"
            "\t\t<style type=\"text/css\" media=\"screen\">\n"
            "\t\t\ttable { font-family: Arial; background-color: #ccccff; font-size: 12pt; }\n"
            "\t\t\ttd { font-family:\"%2\"; background-color: #eeeeee; font-size: %3px; }\n"
            "\t\t\tth { font-weight:normal; }\n"
            "\t\t\tdl { font-family: Arial; font-size: 8pt; margin: 3px; }\n"
            "\t\t\tdt { font-weight: bold; float: left; }\n"
            "\t\t\ttr:hover { background-color: #ddddff; }\n"
            "\t\t\ttd:hover { background-color: #ddddff; }\n"
            "\t\t</style>\n"
            "\t</head>\n"
            "\t<body>\n"
            "\t\t<h1>Qt on %1 glyph shaping (%2)</h1>\n"
            "\t\t<dl>\n"
            "\t\t\t<dt>I</dt><dd>Input Utf-16 to shaper</dd>\n"
            "\t\t\t<dt>O-Utf</dt><dd>expected output Utf-16</dd>\n"
            "\t\t\t<dt>O-ID</dt><dd>expected output Glyph IDs for \"Series 60 Sans\"</dd>\n"
            "\t\t</dl>\n"
            "\t\t<table>\n"
            ).arg(platformName).arg(fontFamily).arg(fontPixelSize);

    QString languageName;
    foreach (const testDataSet &dataSet, testDataSetList()) {
        if (languageName != dataSet.language) {
            result.append(QString::fromLatin1(
                    "\t\t\t<tr>\n"
                    "\t\t\t\t<th rowspan=\"2\"><h2>%1</h2></th>\n"
                    "\t\t\t\t<th colspan=\"2\">Qt/%2</th>\n"
                    "\t\t\t\t<th rowspan=\"2\">Glyphs</th>\n"
                    "\t\t\t\t<th colspan=\"2\">Browser</th>\n"
                    "\t\t\t</tr>\n"
                    "\t\t\t<tr>\n"
                    "\t\t\t\t<th>In</th>\n"
                    "\t\t\t\t<th>Out</th>\n"
                    "\t\t\t\t<th>In</th>\n"
                    "\t\t\t\t<th>Out</th>\n"
                    "\t\t\t</tr>\n"
                    ).arg(dataSet.language).arg(platformName));
            languageName = dataSet.language;
        }
        QString glyphsData;
        if (!dataSet.inputOriginal.isEmpty())
            glyphsData.append(dlItem(QLatin1String("I"), dataSet.inputOriginal));
        if (!dataSet.outputOriginal.isEmpty())
            glyphsData.append(dlItem(QLatin1String("O-Utf"), dataSet.outputOriginal));
        if (!dataSet.outputGlyphIDsOriginal.isEmpty())
            glyphsData.append(dlItem(QLatin1String("O-ID"), dataSet.outputGlyphIDsOriginal));
        if (!glyphsData.isEmpty()) {
            glyphsData.prepend(QLatin1String("\t\t\t\t\t<dl>\n"));
            glyphsData.append(QLatin1String("\t\t\t\t\t</dl>\n"));
        }
        result.append(QString::fromLatin1(
                "\t\t\t<tr>\n"
                "\t\t\t\t<th>%1</th>\n"
                "\t\t\t\t%2\n"
                "\t\t\t\t%3\n"
                "\t\t\t\t<td>\n"
                "%4"
                "\t\t\t\t</td>\n"
                "\t\t\t\t<td>%5</td>\n"
                "\t\t\t\t<td>%6</td>\n"
                "\t\t\t</tr>\n"
                ).arg(dataSet.name)
                .arg(dumpImageHtml(dataSet.input, pathName))
                .arg(dumpImageHtml(dataSet.output, pathName))
                .arg(glyphsData)
                .arg(dataSet.input)
                .arg(dataSet.output)
        );
    }

    result.append(QString::fromLatin1(
            "\t\t</table>\n"
            "\t</body>\n"
            "</html>")
    );

    htmlPage.write(result.toUtf8());

    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    return dumpHtml(QLatin1String(".")) ? 0 : 1;
}
