// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>
#include <QTest>

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
    const auto twoMinutes = std::chrono::minutes{2};
    const QDateTime utc(QDate(2000, 5, 3), QTime(4, 3, 4), QTimeZone::UTC);
    const QDateTime local(QDate(2000, 5, 3), QTime(4, 3, 4),
                          QTimeZone::fromDurationAheadOfUtc(twoMinutes));

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
