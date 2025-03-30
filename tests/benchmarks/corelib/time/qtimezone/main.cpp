/****************************************************************************
**
** Copyright (C) 2019 Crimson AS <info@crimson.no>
** Copyright (C) 2018 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QTimeZone>
#include <QTest>
#include <qdebug.h>

Q_DECLARE_METATYPE(QTimeZone::NameType)

// Enable to test *every* zone, rather than a hand-picked few, in some _data() sets:
// #define EXHAUSTIVE

class tst_QTimeZone : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void isTimeZoneIdAvailable();
    void systemTimeZone();
    void zoneByName_data();
    void zoneByName();
    void displayName_data();
    void displayName();
    void transitionList_data();
    void transitionList();
    void transitionsForward_data() { transitionList_data(); }
    void transitionsForward();
    void transitionsReverse_data() { transitionList_data(); }
    void transitionsReverse();
};

static QVector<QByteArray> enoughZones()
{
#ifdef EXHAUSTIVE
    QVector<QByteArray> result = QTimeZone::availableTimeZoneIds();
#else
    QVector<QByteArray> result{
        QByteArray("UTC"),
        // Those named overtly in tst_QDateTime:
        QByteArray("Europe/Oslo"),
        QByteArray("America/Vancouver"),
        QByteArray("Europe/Berlin"),
        QByteArray("America/Sao_Paulo"),
        QByteArray("Pacific/Auckland"),
        QByteArray("Australia/Eucla"),
        QByteArray("Asia/Kathmandu"),
        QByteArray("Pacific/Kiritimati"),
        QByteArray("Pacific/Apia"),
        QByteArray("UTC+12:00"),
        QByteArray("Australia/Sydney"),
        QByteArray("Asia/Singapore"),
        QByteArray("Australia/Brisbane")
    };
#endif
    result << QByteArray("Vulcan/ShiKahr"); // invalid: also worth testing
    return result;
}

void tst_QTimeZone::isTimeZoneIdAvailable()
{
    const QList<QByteArray> available = QTimeZone::availableTimeZoneIds();
    QBENCHMARK {
        for (const QByteArray &id : available)
            QVERIFY(QTimeZone::isTimeZoneIdAvailable(id));
    }
}

void tst_QTimeZone::systemTimeZone()
{
    QBENCHMARK {
        QTimeZone::systemTimeZone();
    }
}

void tst_QTimeZone::zoneByName_data()
{
    QTest::addColumn<QByteArray>("name");

    const auto names = enoughZones();
    for (const auto &name : names)
        QTest::newRow(name.constData()) << name;
}

void tst_QTimeZone::zoneByName()
{
    QFETCH(QByteArray, name);
    QTimeZone zone;
    QBENCHMARK {
        zone = QTimeZone(name);
    }
    Q_UNUSED(zone);
}

void tst_QTimeZone::displayName_data()
{
    QTest::addColumn<QByteArray>("ianaId");
    QTest::addColumn<QDateTime>("when");
    QTest::addColumn<QLocale>("where");
    QTest::addColumn<QTimeZone::NameType>("type");

    const QLocale sys = QLocale::system();
    const QLocale locs[] = {
        QLocale::c(),
        // System locale and its CLDR-equivalent:
        sys, QLocale(sys.language(), sys.script(), sys.country()),
        // An assortment of CLDR ones:
        QLocale(QLocale::Arabic, QLocale::Lebanon),
        QLocale(QLocale::Chinese, QLocale::China),
        QLocale(QLocale::English, QLocale::UnitedStates),
        QLocale(QLocale::French, QLocale::France),
        QLocale(QLocale::Finnish, QLocale::Finland),
        QLocale(QLocale::German, QLocale::Germany),
        QLocale(QLocale::NorwegianBokmal, QLocale::Norway),
        QLocale(QLocale::Ukrainian, QLocale::Ukraine),
    };
    const QByteArray locName[] = { "C", "system", "CLDR-sys" };
    const qsizetype locCount = sizeof(locName) / sizeof(locName[1]);
    Q_ASSERT(locCount == 3);
    const QDateTime times[] = {
        QDate(1970, 1, 1).startOfDay(Qt::UTC),
        QDate(2000, 2, 29).startOfDay(Qt::UTC),
        QDate(2020, 4, 30).startOfDay(Qt::UTC),
    };

    const auto names = enoughZones();
    for (const auto &name : names) {
        for (const auto &when : times) {
            qsizetype locIndex = 0;
            for (const auto &where : locs) {
                const QByteArray dt = when.toString(Qt::ISODate).toUtf8();
                const QByteArray loc =
                    locIndex < locCount ? locName[locIndex] : where.bcp47Name().toUtf8();
                QTest::addRow("%s@%s/%s:default", name.data(), dt.data(), loc.data())
                    << name << when << where << QTimeZone::DefaultName;
                QTest::addRow("%s@%s/%s:long", name.data(), dt.data(), loc.data())
                    << name << when << where << QTimeZone::LongName;
                QTest::addRow("%s@%s/%s:short", name.data(), dt.data(), loc.data())
                    << name << when << where << QTimeZone::ShortName;
                QTest::addRow("%s@%s/%s:offset", name.data(), dt.data(), loc.data())
                    << name << when << where << QTimeZone::OffsetName;
                ++locIndex;
            }
        }
    }
}

void tst_QTimeZone::displayName()
{
    QFETCH(const QByteArray, ianaId);
    QFETCH(const QDateTime, when);
    QFETCH(const QLocale, where);
    QFETCH(const QTimeZone::NameType, type);

    const QTimeZone zone(ianaId);
    QString name;
    QBENCHMARK {
        name = zone.displayName(when, type, where);
    }
    Q_UNUSED(name);
}

void tst_QTimeZone::transitionList_data()
{
    QTest::addColumn<QByteArray>("name");
    QTest::newRow("system") << QByteArray(); // Handled specially in the test.

    const auto names = enoughZones();
    for (const auto &name : names) {
        QTimeZone zone(name);
        if (zone.isValid() && zone.hasTransitions())
            QTest::newRow(name.constData()) << name;
    }
}

void tst_QTimeZone::transitionList()
{
    QFETCH(QByteArray, name);
    const QTimeZone zone = name.isEmpty() ? QTimeZone::systemTimeZone() : QTimeZone(name);
    const QDateTime early = QDate(1625, 6, 8).startOfDay(Qt::UTC); // Cassini's birth date
    const QDateTime late // End of 32-bit signed time_t
        = QDateTime::fromSecsSinceEpoch(std::numeric_limits<qint32>::max(), Qt::UTC);
    QTimeZone::OffsetDataList seq;
    QBENCHMARK {
        seq = zone.transitions(early, late);
    }
    Q_UNUSED(seq);
}

void tst_QTimeZone::transitionsForward()
{
    QFETCH(QByteArray, name);
    const QTimeZone zone = name.isEmpty() ? QTimeZone::systemTimeZone() : QTimeZone(name);
    const QDateTime early = QDate(1625, 6, 8).startOfDay(Qt::UTC); // Cassini's birth date
    QBENCHMARK {
        QTimeZone::OffsetData tran = zone.nextTransition(early);
        while (tran.atUtc.isValid())
            tran = zone.nextTransition(tran.atUtc);
    }
}

void tst_QTimeZone::transitionsReverse()
{
    QFETCH(QByteArray, name);
    const QTimeZone zone = name.isEmpty() ? QTimeZone::systemTimeZone() : QTimeZone(name);
    const QDateTime late // End of 32-bit signed time_t
        = QDateTime::fromSecsSinceEpoch(std::numeric_limits<qint32>::max(), Qt::UTC);
    QBENCHMARK {
        QTimeZone::OffsetData tran = zone.previousTransition(late);
        while (tran.atUtc.isValid())
            tran = zone.previousTransition(tran.atUtc);
    }
}

QTEST_MAIN(tst_QTimeZone)

#include "main.moc"
