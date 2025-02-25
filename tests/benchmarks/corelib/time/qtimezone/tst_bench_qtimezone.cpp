// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Crimson AS <info@crimson.no>
// Copyright (C) 2018 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTimeZone>
#include <QTest>
#include <qdebug.h>

// Enable to test *every* zone, rather than a hand-picked few, in some _data() sets:
// #define EXHAUSTIVE

class tst_QTimeZone : public QObject
{
    Q_OBJECT

private Q_SLOTS:
#if QT_CONFIG(timezone)
    void isTimeZoneIdAvailable();
    void systemTimeZone();
    void utc();
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
#endif
};

static QList<QByteArray> enoughZones()
{
#ifdef EXHAUSTIVE
    QList<QByteArray> result = QTimeZone::availableTimeZoneIds();
#else
    QList<QByteArray> result {
        QByteArray("UTC"),
        // Those named overtly in tst_QDateTime - special cases first:
        QByteArray("UTC-02:00"), QByteArray("UTC+02:00"), QByteArray("UTC+12:00"),
        QByteArray("Etc/GMT+3"), QByteArray("GMT-2"), QByteArray("GMT"),
        // ... then ordinary names in alphabetic order:
        QByteArray("America/New_York"), QByteArray("America/Sao_Paulo"),
        QByteArray("America/Vancouver"),
        QByteArray("Asia/Kathmandu"), QByteArray("Asia/Singapore"),
        QByteArray("Australia/Brisbane"), QByteArray("Australia/Eucla"),
        QByteArray("Australia/Sydney"),
        QByteArray("Europe/Berlin"), QByteArray("Europe/Helsinki"),
        QByteArray("Europe/Rome"), QByteArray("Europe/Oslo"),
        QByteArray("Pacific/Apia"), QByteArray("Pacific/Auckland"),
        QByteArray("Pacific/Kiritimati")
    };
#endif
    result << QByteArray("Vulcan/ShiKahr"); // invalid: also worth testing
    return result;
}
#if QT_CONFIG(timezone)
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
        [[maybe_unused]] const auto r = QTimeZone::systemTimeZone();
    }
}

void tst_QTimeZone::utc()
{
    QBENCHMARK {
        [[maybe_unused]] const auto r = QTimeZone::utc();
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
        sys, QLocale(sys.language(), sys.script(), sys.territory()),
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
    const QDateTime times[] = {
        QDate(1970, 1, 1).startOfDay(QTimeZone::UTC),
        QDate(2000, 2, 29).startOfDay(QTimeZone::UTC),
        QDate(2020, 4, 30).startOfDay(QTimeZone::UTC),
    };

    const auto names = enoughZones();
    for (const auto &name : names) {
        for (const auto &when : times) {
            std::size_t locIndex = 0;
            for (const auto &where : locs) {
                const QByteArray dt = when.toString(Qt::ISODate).toUtf8();
                const QByteArray loc =
                    locIndex < std::size(locName) ? locName[locIndex] : where.bcp47Name().toUtf8();
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
    const QDateTime early = QDate(1625, 6, 8).startOfDay(QTimeZone::UTC); // Cassini's birth date
    const QDateTime late // End of 32-bit signed time_t
        = QDateTime::fromSecsSinceEpoch(std::numeric_limits<qint32>::max(), QTimeZone::UTC);
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
    const QDateTime early = QDate(1625, 6, 8).startOfDay(QTimeZone::UTC); // Cassini's birth date
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
        = QDateTime::fromSecsSinceEpoch(std::numeric_limits<qint32>::max(), QTimeZone::UTC);
    QBENCHMARK {
        QTimeZone::OffsetData tran = zone.previousTransition(late);
        while (tran.atUtc.isValid())
            tran = zone.previousTransition(tran.atUtc);
    }
}
#endif

QTEST_MAIN(tst_QTimeZone)

#include "tst_bench_qtimezone.moc"
