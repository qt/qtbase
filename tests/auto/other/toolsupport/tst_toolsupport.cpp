/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include <QtTest>

//
// Note:
//
// When this test here fails and the change leading to the failure
// intentionally changed a private class, adjust the test here and bump
// the TypeInformationVersion field in src/corelib/global/qhooks.cpp
// in the same commit as the modification to the private class.
//


// Don't do this at home. This is test code, not production.
#define protected public
#define private public

#include <private/qdatetime_p.h>
#include <private/qfile_p.h>
#include <private/qfileinfo_p.h>
#include <private/qobject_p.h>
#include <qobject.h>

#if defined(Q_CC_GNU) || defined(Q_CC_MSVC)
#define RUN_MEMBER_OFFSET_TEST 1
#else
#define RUN_MEMBER_OFFSET_TEST 0
#endif

#if RUN_MEMBER_OFFSET_TEST
template <typename T, typename K>
size_t pmm_to_offsetof(T K:: *pmm)
{
#ifdef Q_CC_MSVC
    // Even on 64 bit MSVC uses 4 byte offsets.
    quint32 ret;
#else
    size_t ret;
#endif
    Q_STATIC_ASSERT(sizeof(ret) == sizeof(pmm));
    memcpy(&ret, &pmm, sizeof(ret));
    return ret;
}
#endif

class tst_toolsupport : public QObject
{
    Q_OBJECT

private slots:
    void offsets();
    void offsets_data();
};

void tst_toolsupport::offsets()
{
    QFETCH(size_t, actual);
    QFETCH(int, expected32);
    QFETCH(int, expected64);
    size_t expect = sizeof(void *) == 4 ? expected32 : expected64;
    QCOMPARE(actual, expect);
}

void tst_toolsupport::offsets_data()
{
    QTest::addColumn<size_t>("actual");
    QTest::addColumn<int>("expected32");
    QTest::addColumn<int>("expected64");

    {
        QTestData &data = QTest::newRow("sizeof(QObjectData)")
                << sizeof(QObjectData);
        data << 28 << 48; // vptr + 3 ptr + 2 int + ptr
    }

#if RUN_MEMBER_OFFSET_TEST
    {
        QTestData &data = QTest::newRow("QObjectPrivate::extraData")
                << pmm_to_offsetof(&QObjectPrivate::extraData);
        data << 28 << 48;    // sizeof(QObjectData)
    }

    {
        QTestData &data = QTest::newRow("QFileInfoPrivate::fileEntry")
                << pmm_to_offsetof(&QFileInfoPrivate::fileEntry);
        data << 4 << 8;
    }

    {
        QTestData &data = QTest::newRow("QFileSystemEntry::filePath")
                << pmm_to_offsetof(&QFileSystemEntry::m_filePath);
        data << 0 << 0;
    }

#ifdef Q_OS_LINUX
    {
        QTestData &data = QTest::newRow("QFilePrivate::fileName")
                << pmm_to_offsetof(&QFilePrivate::fileName);
#ifdef Q_PROCESSOR_X86
        // x86 32-bit has weird alignment rules. Refer to QtPrivate::AlignOf in
        // qglobal.h for more details.
        data << 168 << 248;
#else
        data << 172 << 248;
#endif
    }
#endif

    {
        QTest::newRow("QDateTimePrivate::m_msecs")
            << pmm_to_offsetof(&QDateTimePrivate::m_msecs) << 0 << 0;
        QTest::newRow("QDateTimePrivate::m_status")
            << pmm_to_offsetof(&QDateTimePrivate::m_status) << 8 << 8;
        QTest::newRow("QDateTimePrivate::m_offsetFromUtc")
            << pmm_to_offsetof(&QDateTimePrivate::m_offsetFromUtc) << 12 << 12;
        QTest::newRow("QDateTimePrivate::m_timeZone")
            << pmm_to_offsetof(&QDateTimePrivate::m_timeZone) << 20 << 24;
    }
#endif // RUN_MEMBER_OFFSET_TEST
}


QTEST_APPLESS_MAIN(tst_toolsupport);

#include "tst_toolsupport.moc"

