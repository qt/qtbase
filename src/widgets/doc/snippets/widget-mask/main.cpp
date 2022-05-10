// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//! [0]
    QLabel topLevelLabel;
    QPixmap pixmap(":/images/tux.png");
    topLevelLabel.setPixmap(pixmap);
    topLevelLabel.setMask(pixmap.mask());
//! [0]
    topLevelLabel.show();
    return app.exec();
}
