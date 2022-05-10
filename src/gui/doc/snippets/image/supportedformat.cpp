// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

void wrapper() {
//! [0]
    QImageWriter writer;
    writer.setFormat("png");
    if (writer.supportsOption(QImageIOHandler::Description))
        qDebug() << "Png supports embedded text";
//! [0]

} // wrapper
