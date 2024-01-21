// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QDir>
#include <QFileInfo>

using namespace Qt::StringLiterals;

[[maybe_unused]] static void func()
{
{
//![newstuff]
    QFileInfo fi("c:/temp/foo");
    qDebug() << fi.absoluteFilePath(); // "C:/temp/foo"
//![newstuff]
}

{
//! [0]
#ifdef Q_OS_UNIX

QFileInfo info1("/home/bob/bin/untabify");
info1.isSymLink();          // returns true
info1.absoluteFilePath();   // returns "/home/bob/bin/untabify"
info1.size();               // returns 56201
info1.symLinkTarget();      // returns "/opt/pretty++/bin/untabify"

QFileInfo info2(info1.symLinkTarget());
info2.isSymLink();          // returns false
info2.absoluteFilePath();   // returns "/opt/pretty++/bin/untabify"
info2.size();               // returns 56201

#endif
//! [0]
}

//! [1]
#ifdef Q_OS_WIN

QFileInfo info1("C:\\Users\\Bob\\untabify.lnk");
info1.isSymLink();          // returns true
info1.absoluteFilePath();   // returns "C:/Users/Bob/untabify.lnk"
info1.size();               // returns 63942
info1.symLinkTarget();      // returns "C:/Pretty++/untabify"

QFileInfo info2(info1.symLinkTarget());
info2.isSymLink();          // returns false
info2.absoluteFilePath();   // returns "C:/Pretty++/untabify"
info2.size();               // returns 63942

#endif
//! [1]

{
//! [2]
QFileInfo info("/usr/bin/env");

QString path = info.absolutePath(); // path = /usr/bin
QString base = info.baseName(); // base = env

info.setFile("/etc/hosts");

path = info.absolutePath(); // path = /etc
base = info.baseName(); // base = hosts
//! [2]
}

{
//! [3]
QFileInfo fi("/tmp/archive.tar.gz");
QString name = fi.fileName();                // name = "archive.tar.gz"
//! [3]
}

{
//! [4]
QFileInfo fi("/Applications/Safari.app");
QString bundle = fi.bundleName();                // name = "Safari"
//! [4]
}

{
//! [5]
QFileInfo fi("/tmp/archive.tar.gz");
QString base = fi.baseName();  // base = "archive"
//! [5]
}

{
//! [6]
QFileInfo fi("/tmp/archive.tar.gz");
QString base = fi.completeBaseName();  // base = "archive.tar"
//! [6]
}

{
//! [7]
QFileInfo fi("/tmp/archive.tar.gz");
QString ext = fi.completeSuffix();  // ext = "tar.gz"
//! [7]
}

{
//! [8]
QFileInfo fi("/tmp/archive.tar.gz");
QString ext = fi.suffix();  // ext = "gz"
//! [8]
}

{
QString fileName = "foo";
//! [9]
QFileInfo info(fileName);
if (info.isSymLink())
    fileName = info.symLinkTarget();
//! [9]
}

{
//! [10]
QFileInfo fi("/tmp/archive.tar.gz");
if (fi.permission(QFile::WriteUser | QFile::ReadGroup))
    qWarning("I can change the file; my group can read the file");
if (fi.permission(QFile::WriteGroup | QFile::WriteOther))
    qWarning("The group or others can change the file");
//! [10]
}

{
//! [11]
// Given a current working directory of "/home/user/Documents/memos/"
QFileInfo info1(u"relativeFile"_s);
qDebug() << info1.absolutePath(); // "/home/user/Documents/memos/"
qDebug() << info1.baseName(); // "relativeFile"
qDebug() << info1.absoluteDir(); // QDir(u"/home/user/Documents/memos"_s)
qDebug() << info1.absoluteDir().path(); // "/home/user/Documents/memos"

// A QFileInfo on a dir
QFileInfo info2(u"/home/user/Documents/memos"_s);
qDebug() << info2.absolutePath(); // "/home/user/Documents"
qDebug() << info2.baseName(); // "memos"
qDebug() << info2.absoluteDir(); // QDir(u"/home/user/Documents"_s)
qDebug() << info2.absoluteDir().path(); // "/home/user/Documents"
//! [11]
}

}
