// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QDirListing>

using namespace Qt::StringLiterals;

[[maybe_unused]] static void func() {
{
//! [0]
using ItFlag = QDirListing::IteratorFlag;
for (const auto &dirEntry : QDirListing(u"/etc"_s, ItFlag::Recursive)) {
    qDebug() << dirEntry.filePath();
    // /etc/.
    // /etc/..
    // /etc/X11
    // /etc/X11/fs
    // ...
}
//! [0]
}

{
//! [1]
using ItFlag = QDirListing::IteratorFlag;
QDirListing dirList(u"/sys"_s, QStringList{u"scaling_cur_freq"_s},
                    QDir::NoFilter, ItFlag::Recursive);
for (const auto &dirEntry : dirList) {
    QFile f(dirEntry.filePath());
    f.open(QIODevice::ReadOnly);
    qDebug() << f.fileName() << f.readAll().trimmed().toDouble() / 1000 << "MHz";
}
//! [1]
}

{
//! [2]
QDirListing audioFileIt(u"/home/johndoe/"_s, {"*.mp3", "*.wav"}, QDir::Files);
//! [2]
}

{
//! [3]
using ItFlag = QDirListing::IteratorFlag;
for (const auto &dirEntry : QDirListing(u"/etc"_s, ItFlag::Recursive)) {
    // Faster
    if (dirEntry.fileName().endsWith(u".conf")) { /* ... */ }

    // This works, but might be potentially slower, since it has to construct a
    // QFileInfo, whereas (depending on the implemnetation) the fileName could
    // be known already
    if (dirEntry.fileInfo().fileName().endsWith(u".conf")) { /* ... */ }
}
//! [3]
}

{
//! [4]
using ItFlag = QDirListing::IteratorFlag;
for (const auto &dirEntry : QDirListing(u"/etc"_s, ItFlag::Recursive)) {
    // Both approaches are the same, because DirEntry will have to construct
    // a QFileInfo to get this info (for example, by calling system stat())

    if (dirEntry.size() >= 4'000 /* 4KB */) { /* ...*/ }
    if (dirEntry.fileInfo().size() >= 4'000 /* 4KB */) { /* ... */ }
}
//! [4]
}

}
