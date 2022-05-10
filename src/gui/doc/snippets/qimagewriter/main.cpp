// Copyright (C) 2018 Samuel Gaist <samuel.gaist@idiap.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtGui>

void wrapper() {
//! [0]
QString imagePath(QStringLiteral("path/image.jpeg"));
QImage image(64, 64, QImage::Format_RGB32);
image.fill(Qt::red);
{
    QImageWriter writer(imagePath);
    writer.write(image);
}

QFile::rename(imagePath,
              QStringLiteral("path/other_image.jpeg"));
//! [0]

} // wrapper
