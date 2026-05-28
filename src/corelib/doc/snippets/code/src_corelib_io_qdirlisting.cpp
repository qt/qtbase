// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QDirListing>
#include <QFile>

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
using F = QDirListing::IteratorFlag;
QDirListing dirList(u"/sys"_s, QStringList{u"scaling_cur_freq"_s}, F::FilesOnly | F::Recursive);
for (const auto &dirEntry : dirList) {
    QFile f(dirEntry.filePath());
    if (f.open(QIODevice::ReadOnly))
        qDebug() << f.fileName() << f.readAll().trimmed().toDouble() / 1000 << "MHz";
}
//! [1]
}

{
//! [2]
QDirListing audioFileIt(u"/home/johndoe/"_s, QStringList{u"*.mp3"_s, u"*.wav"_s},
                        QDirListing::IteratorFlag::FilesOnly);
//! [2]
}

{
//! [3]
using ItFlag = QDirListing::IteratorFlag;
for (const auto &dirEntry : QDirListing(u"/etc"_s, ItFlag::Recursive)) {
    // Faster
    if (dirEntry.fileName().endsWith(u".conf")) { /* ... */ }

    // This works, but might be potentially slower, since it has to construct a
    // QFileInfo, whereas (depending on the implementation) the fileName could
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

{
//! [5]
using F = QDirListing::IteratorFlag;
const auto flags = F::FilesOnly | F::Recursive;
for (const auto &dirEntry : QDirListing(u"/etc"_s, flags)) {
    // ...
}
//! [5]
}

{
//! [6]
using F = QDirListing::IteratorFlag;
const auto flags = F::FilesOnly | F::Recursive | F::ResolveSymlinks;
for (const auto &dirEntry : QDirListing(u"/etc"_s, flags)) {
    // ...
}
}
//! [6]

{
//! [7]
    using F = QDirListing::IteratorFlag;
    const auto flags = F::FilesOnly | F::Recursive | F::ResolveSymlinks;
    for (const auto &dirEntry : QDirListing(u"/usr"_s, flags)) {
        // Faster than using name filters, filter ".txt" and ".html" files
        // using QString API
        const QString fileName = dirEntry.fileName();
        if (fileName.endsWith(".txt"_L1) || fileName.endsWith(".html"_L1)) {
            // ...
        }
    }
}
//! [7]

}
