// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shellhelpers.h"

#include <QtCore/qfile.h>
#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void forceStopBundle(const Hdc &hdc, const QString &bundleName)
{
    hdc.shell({ u"aa"_s, u"force-stop"_s, bundleName });
}

bool isProcessAlive(const Hdc &hdc, const QString &bundleName)
{
    return !hdc.shell({u"pidof"_s, bundleName}).trimmed().isEmpty();
}

q23::expected<QByteArray, QString> copyDeviceFile(
        const Hdc &hdc, const QString &devicePath, const QString &localPath)
{
    static const QRegularExpression transferFinishedPattern(u"FileTransfer finish, Size:(\\d+),"_s);

    const QString hdcAnswer = hdc.receiveFile(devicePath, localPath);
    const QRegularExpressionMatch sizeMatch = transferFinishedPattern.match(hdcAnswer);
    if (!sizeMatch.hasMatch())
        return q23::unexpected(u"hdc file recv answered \"%1\""_s.arg(hdcAnswer.trimmed()));
    QFile localCopy(localPath);
    if (!localCopy.open(QIODevice::ReadOnly))
        return q23::unexpected(localCopy.errorString());
    QByteArray contents = localCopy.readAll();
    const qint64 reportedSize = sizeMatch.captured(1).toLongLong();
    if (contents.size() != reportedSize) {
        return q23::unexpected(u"hdc reported %1 bytes, the local copy holds %2"_s
                                            .arg(reportedSize).arg(contents.size()));
    }
    return contents;
}

DeviceFileState queryDeviceFileState(const Hdc &hdc, const QString &devicePath)
{
    const QString presentMarker = u"HARMONYOSTESTRUNNER_FILE_PRESENT"_s;
    const QString absentMarker = u"HARMONYOSTESTRUNNER_FILE_ABSENT"_s;

    const QString output = hdc.shell({ u"test"_s, u"-e"_s, devicePath, u"&&"_s, u"echo"_s,
                                       presentMarker, u"||"_s, u"echo"_s, absentMarker }).trimmed();
    if (output == presentMarker)
        return DeviceFileState::Present;
    if (output == absentMarker)
        return DeviceFileState::Absent;
    return DeviceFileState::Unknown;
}

QString readDeviceFile(const Hdc &hdc, const QString &devicePath)
{
    return hdc.shell({ u"cat"_s, devicePath });
}

QT_END_NAMESPACE
