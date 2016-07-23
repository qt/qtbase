/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
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
             QDateTime::fromMSecsSinceEpoch(afterEpochUtc * qint64(1000), Qt::UTC));
#else
    QT_STATBUF data;
    memset(&data, 0, sizeof(data));
    data.st_ctime = afterEpochUtc;
    meta.fillFromStatBuf(data);
    QCOMPARE(meta.metadataChangeTime().toUTC(),
             QDateTime::fromMSecsSinceEpoch(afterEpochUtc * qint64(1000), Qt::UTC));
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
