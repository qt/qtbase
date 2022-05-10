// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QFile>
#include <QXmlStreamWriter>
#include <stdio.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QFile output;
    output.open(stdout, QIODevice::WriteOnly);
//! [write output]
//! [start stream]
    QXmlStreamWriter stream(&output);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
//! [start stream]
    stream.writeDTD("<!DOCTYPE xbel>");
    stream.writeStartElement("xbel");
    stream.writeAttribute("version", "1.0");
    stream.writeStartElement("folder");
    stream.writeAttribute("folded", "no");
//! [write element]
    stream.writeStartElement("bookmark");
    stream.writeAttribute("href", "http://qt-project.org/");
    stream.writeTextElement("title", "Qt Project");
    stream.writeEndElement(); // bookmark
//! [write element]
    stream.writeEndElement(); // folder
    stream.writeEndElement(); // xbel
//! [finish stream]
    stream.writeEndDocument();
//! [finish stream]
//! [write output]
    output.close();
    return 0;
}
