/****************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include <QString>

class tst_QLatin1String : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nullString();
    void emptyString();
};

void tst_QLatin1String::nullString()
{
    // default ctor
    {
        QLatin1String l1;
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(Q_NULLPTR));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isNull());
    }

    // from nullptr
    {
        const char *null = Q_NULLPTR;
        QLatin1String l1(null);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(Q_NULLPTR));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isNull());
    }

    // from null QByteArray
    {
        const QByteArray null;
        QVERIFY(null.isNull());

        QLatin1String l1(null);
        QEXPECT_FAIL("", "null QByteArrays become non-null QLatin1Strings...", Continue);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(Q_NULLPTR));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QEXPECT_FAIL("", "null QByteArrays become non-null QLatin1Strings become non-null QStrings...", Continue);
        QVERIFY(s.isNull());
    }
}

void tst_QLatin1String::emptyString()
{
    {
        const char *empty = "";
        QLatin1String l1(empty);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(empty));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const char *notEmpty = "foo";
        QLatin1String l1(notEmpty, 0);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(notEmpty));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const QByteArray empty = "";
        QLatin1String l1(empty);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(empty.constData()));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }
}



QTEST_APPLESS_MAIN(tst_QLatin1String)

#include "tst_qlatin1string.moc"
