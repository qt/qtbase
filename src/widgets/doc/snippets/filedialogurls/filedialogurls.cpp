// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QFileDialog>
#include <QtGui>

int loadFileDialog()
{
//![0]
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile("/Users/foo/Code/qt5")
         << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first());

    QFileDialog dialog;
    dialog.setSidebarUrls(urls);
    dialog.setFileMode(QFileDialog::AnyFile);
    if (dialog.exec()) {
        // ...
    }
//![0]
    return 1;
}
