// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XMLWRITER_H
#define XMLWRITER_H

#include <QDomDocument>
#include <QTextBlock>

class QTextDocument;

class XmlWriter
{
public:
    XmlWriter(QTextDocument *document) : textDocument(document) {}
    QDomDocument *toXml();

private:
    void readFragment(const QTextBlock &currentBlock, QDomElement blockElement,
                      QDomDocument *document);
    void processBlock(const QTextBlock &currentBlock);
    void processFragment(const QTextFragment &currentFragment);

    QDomDocument *document;
    QTextDocument *textDocument;
};

#endif
