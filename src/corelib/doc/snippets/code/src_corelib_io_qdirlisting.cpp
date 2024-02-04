// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QDirIterator it("/etc", QDirIterator::Subdirectories);
while (it.hasNext()) {
    QString dir = it.next();
    qDebug() << dir;
    // /etc/.
    // /etc/..
    // /etc/X11
    // /etc/X11/fs
    // ...
}
//! [0]

//! [1]
QDirIterator it("/sys", QStringList() << "scaling_cur_freq", QDir::NoFilter, QDirIterator::Subdirectories);
while (it.hasNext()) {
    QFile f(it.next());
    f.open(QIODevice::ReadOnly);
    qDebug() << f.fileName() << f.readAll().trimmed().toDouble() / 1000 << "MHz";
}
//! [1]

//! [2]
QDirIterator audioFileIt(audioPath, {"*.mp3", "*.wav"}, QDir::Files);
//! [2]
