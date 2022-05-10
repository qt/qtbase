// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
// file is an open QFile object.
QDomDocument document;
if (document.setContent(&file)) {

    QDomElement documentElement = document.documentElement();
    QString text;
    QDomNode node = documentElement.firstChild();

    while (!node.isNull()) {
        if (node.isText())
            text += node.nodeValue();
        else if (node.hasChildNodes()) {
            // Examine the node's children and read any text found.
            ...
        }
        node = node.nextSibling();
    }
}
//! [0]
