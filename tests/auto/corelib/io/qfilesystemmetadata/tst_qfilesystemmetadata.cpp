// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/QDateTime>
#include <QtCore/private/qfilesystemmetadata_p.h>

class tst_QFileSystemMetaData : public QObject
{
    Q_OBJECT

private slots:
    void timeSinceEpoch();
};

#if defined(QT_BUILD_INTERNAL) && defined(QT_SHARED)
#ifdef Q_OS_WIN
static FILETIME epochToFileTime(long seconds)
{
    const qint64 sec = 10000000;
    // FILETIME is time in 1e-7s units since 1601's start: epoch is 1970's
    // start, 369 years (of which 3*24 +69/4 = 89 were leap) later.
    const qint64 offset = qint64(365 * 369 + 89) * 24 * 3600;
    const qint64 convert = (offset + seconds) * sec;
    FILETIME parts;
    parts.dwHighDateTime = convert >> 32;
    parts.dwLowDateTime = convert & 0xffffffff;
    return parts;
}
#endif

void tst_QFileSystemMetaData::timeSinceEpoch()
{
    // Regression test for QTBUG-48306, used to fail for TZ=Russia/Moscow
    // Oct 22 2014 6:00 UTC; TZ=Russia/Moscow changed from +4 to +3 on Oct 26.
    const long afterEpochUtc = 1413957600L;
    QFileSystemMetaData meta;
#ifdef Q_OS_WIN
    WIN32_FIND_DATA data;
    memset(&data, 0, sizeof(data));
    data.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    /* data.ftLastAccessTime = data.ftLastWriteTime = */
    data.ftCreationTime = epochToFileTime(afterEpochUtc);
    meta.fillFromFindData(data);
    QCOMPARE(meta.birthTime().toUTC(),
             QDateTime::fromMSecsSinceEpoch(afterEpochUtc * qint64(1000), QTimeZone::UTC));
#else
    QT_STATBUF data;
    memset(&data, 0, sizeof(data));
    data.st_ctime = afterEpochUtc;
    meta.fillFromStatBuf(data);
    QCOMPARE(meta.metadataChangeTime().toUTC(),
             QDateTime::fromMSecsSinceEpoch(afterEpochUtc * qint64(1000), QTimeZone::UTC));
#endif
}
#else // i.e. no Q_AUTOTEST_EXPORT
void tst_QFileSystemMetaData::timeSinceEpoch()
{
    QSKIP("QFileSystemMetaData methods aren't available to test");
}
#endif
QTEST_MAIN(tst_QFileSystemMetaData)
#include <tst_qfilesystemmetadata.moc>
