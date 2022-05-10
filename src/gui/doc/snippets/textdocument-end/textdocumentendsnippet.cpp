// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <iostream>

namespace textdocumentendsnippet {
void wrapper()
{
    QString contentString("One\nTwp\nThree");
    QTextDocument *doc = new QTextDocument(contentString);

//! [0]
for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    std::cout << it.text().toStdString() << "\n";
//! [0]

} // wrapper
} //textdocumentendsnippet
