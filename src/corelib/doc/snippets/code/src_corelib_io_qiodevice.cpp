// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QProcess gzip;
gzip.start("gzip", QStringList() << "-c");
if (!gzip.waitForStarted())
    return false;

gzip.write("uncompressed data");

QByteArray compressed;
while (gzip.waitForReadyRead())
    compressed += gzip.readAll();
//! [0]


//! [1]
qint64 CustomDevice::bytesAvailable() const
{
    return buffer.size() + QIODevice::bytesAvailable();
}
//! [1]


//! [2]
QFile file("box.txt");
if (file.open(QFile::ReadOnly)) {
    char buf[1024];
    qint64 lineLength = file.readLine(buf, sizeof(buf));
    if (lineLength != -1) {
        // the line is available in buf
    }
}
//! [2]


//! [3]
bool CustomDevice::canReadLine() const
{
    return buffer.contains('\n') || QIODevice::canReadLine();
}
//! [3]


//! [4]
bool isExeFile(QFile *file)
{
    char buf[2];
    if (file->peek(buf, sizeof(buf)) == sizeof(buf))
        return (buf[0] == 'M' && buf[1] == 'Z');
    return false;
}
//! [4]


//! [5]
bool isExeFile(QFile *file)
{
    return file->peek(2) == "MZ";
}
//! [5]
