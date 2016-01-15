/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtTest/QtTest>

/*!
  \internal
 */
class tst_DateTime: public QObject
{
    Q_OBJECT

private slots:
    void dateTime() const;
    void qurl() const;
    void qurl_data() const;
};

void tst_DateTime::dateTime() const
{
    const QDateTime utc(QDate(2000, 5, 3), QTime(4, 3, 4), Qt::UTC);
    const QDateTime local(QDate(2000, 5, 3), QTime(4, 3, 4), Qt::OffsetFromUTC, 120 /* 2 minutes */);

    QCOMPARE(local, utc);
}

void tst_DateTime::qurl() const
{
    QFETCH(QUrl, operandA);
    QFETCH(QUrl, operandB);

    QCOMPARE(operandA, operandB);
}

void tst_DateTime::qurl_data() const
{
    QTest::addColumn<QUrl>("operandA");
    QTest::addColumn<QUrl>("operandB");

    QTest::newRow("empty urls") << QUrl() << QUrl();
    QTest::newRow("empty rhs") << QUrl(QLatin1String("http://example.com")) << QUrl();
    QTest::newRow("empty lhs") << QUrl() << QUrl(QLatin1String("http://example.com"));
    QTest::newRow("same urls") << QUrl(QLatin1String("http://example.com")) << QUrl(QLatin1String("http://example.com"));
}

QTEST_MAIN(tst_DateTime)

#include "tst_datetime.moc"
