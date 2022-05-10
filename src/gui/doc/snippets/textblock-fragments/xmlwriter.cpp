// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "xmlwriter.h"

#include <QTextDocument>

QDomDocument *XmlWriter::toXml()
{
    QDomImplementation implementation;
    QDomDocumentType docType = implementation.createDocumentType(
        "scribe-document", "scribe", "qt.nokia.com/scribe");

    document = new QDomDocument(docType);

    // ### This processing instruction is required to ensure that any kind
    // of encoding is given when the document is written.
    QDomProcessingInstruction process = document->createProcessingInstruction(
        "xml", "version=\"1.0\" encoding=\"utf-8\"");
    document->appendChild(process);

    QDomElement documentElement = document->createElement("document");
    document->appendChild(documentElement);

//! [0]
    QTextBlock currentBlock = textDocument->begin();

    while (currentBlock.isValid()) {
//! [0]
        QDomElement blockElement = document->createElement("block");
        document->appendChild(blockElement);

        readFragment(currentBlock, blockElement, document);

//! [1]
        processBlock(currentBlock);
//! [1]

//! [2]
        currentBlock = currentBlock.next();
    }
//! [2]

    return document;
}

void XmlWriter::readFragment(const QTextBlock &currentBlock,
                             QDomElement blockElement,
                             QDomDocument *document)
{
//! [3] //! [4]
    QTextBlock::iterator it;
    for (it = currentBlock.begin(); !(it.atEnd()); ++it) {
        QTextFragment currentFragment = it.fragment();
        if (currentFragment.isValid())
//! [3] //! [5]
            processFragment(currentFragment);
//! [4] //! [5]

        if (currentFragment.isValid()) {
            QDomElement fragmentElement = document->createElement("fragment");
            blockElement.appendChild(fragmentElement);

            fragmentElement.setAttribute("length", currentFragment.length());
            QDomText fragmentText = document->createTextNode(currentFragment.text());

            fragmentElement.appendChild(fragmentText);
        }
//! [6] //! [7]
    }
//! [7] //! [6]
}

void XmlWriter::processBlock(const QTextBlock &currentBlock)
{
   // Dummy use of specified parameter currentBlock
   QTextBlock localBlock;
   localBlock = currentBlock;

}

void XmlWriter::processFragment(const QTextFragment &currentFragment)
{
   // Dummy use of specified parameter currentFragment
   QTextFragment localFragment;
   localFragment = currentFragment;
}
