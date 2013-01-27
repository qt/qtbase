/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qtimezone.h>
#include <private/qtimezoneprivate_p.h>
#include <qlocale.h>

class tst_QTimeZone : public QObject
{
    Q_OBJECT

public:
    tst_QTimeZone();

private slots:
    // Public class default system tests
    void createTest();
    void nullTest();
    void dataStreamTest();
    void availableTimeZoneIds();
    void windowsId();
    // Backend tests
    void utcTest();

private:
    void printTimeZone(const QTimeZone tz);
    bool debug;
};

tst_QTimeZone::tst_QTimeZone()
{
    // Set to true to print debug output
    debug = false;
}

void tst_QTimeZone::printTimeZone(const QTimeZone tz)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    qDebug() << "";
    qDebug() << "Time Zone               = " << tz;
    qDebug() << "";
    qDebug() << "Is Valid                = " << tz.isValid();
    qDebug() << "";
    qDebug() << "Zone ID                 = " << tz.id();
    qDebug() << "Country                 = " << QLocale::countryToString(tz.country());
    qDebug() << "Comment                 = " << tz.comment();
    qDebug() << "";
    qDebug() << "Locale                  = " << QLocale().name();
    qDebug() << "Name Long               = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName);
    qDebug() << "Name Short              = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::ShortName);
    qDebug() << "Name Offset             = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName);
    qDebug() << "Name Long DST           = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::LongName);
    qDebug() << "Name Short DST          = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName);
    qDebug() << "Name Offset DST         = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName);
    qDebug() << "Name Long Generic       = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::LongName);
    qDebug() << "Name Short Generic      = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::ShortName);
    qDebug() << "Name Offset Generic     = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName);
    qDebug() << "";
    QLocale locale = QLocale(QStringLiteral("de_DE"));
    qDebug() << "Locale                  = " << locale.name();
    qDebug() << "Name Long               = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName, locale);
    qDebug() << "Name Short              = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, locale);
    qDebug() << "Name Offset             = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, locale);
    qDebug() << "Name Long DST           = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::LongName,locale);
    qDebug() << "Name Short DST          = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, locale);
    qDebug() << "Name Offset DST         = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, locale);
    qDebug() << "Name Long Generic       = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::LongName, locale);
    qDebug() << "Name Short Generic      = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, locale);
    qDebug() << "Name Offset Generic     = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, locale);
    qDebug() << "";
    qDebug() << "Abbreviation Now        = " << tz.abbreviation(now);
    qDebug() << "Abbreviation on 1 Jan   = " << tz.abbreviation(jan);
    qDebug() << "Abbreviation on 1 June  = " << tz.abbreviation(jun);
    qDebug() << "";
    qDebug() << "Offset on 1 January     = " << tz.offsetFromUtc(jan);
    qDebug() << "Offset on 1 June        = " << tz.offsetFromUtc(jun);
    qDebug() << "Offset Now              = " << tz.offsetFromUtc(now);
    qDebug() << "";
    qDebug() << "UTC Offset Now          = " << tz.standardTimeOffset(now);
    qDebug() << "UTC Offset on 1 January = " << tz.standardTimeOffset(jan);
    qDebug() << "UTC Offset on 1 June    = " << tz.standardTimeOffset(jun);
    qDebug() << "";
    qDebug() << "DST Offset on 1 January = " << tz.daylightTimeOffset(jan);
    qDebug() << "DST Offset on 1 June    = " << tz.daylightTimeOffset(jun);
    qDebug() << "DST Offset Now          = " << tz.daylightTimeOffset(now);
    qDebug() << "";
    qDebug() << "Has DST                 = " << tz.hasDaylightTime();
    qDebug() << "Is DST Now              = " << tz.isDaylightTime(now);
    qDebug() << "Is DST on 1 January     = " << tz.isDaylightTime(jan);
    qDebug() << "Is DST on 1 June        = " << tz.isDaylightTime(jun);
    qDebug() << "";
    qDebug() << "Has Transitions         = " << tz.hasTransitions();
    qDebug() << "Transition after 1 Jan  = " << tz.nextTransition(jan).atUtc;
    qDebug() << "Transition after 1 Jun  = " << tz.nextTransition(jun).atUtc;
    qDebug() << "Transition before 1 Jan = " << tz.previousTransition(jan).atUtc;
    qDebug() << "Transition before 1 Jun = " << tz.previousTransition(jun).atUtc;
    qDebug() << "";
}

void tst_QTimeZone::createTest()
{
    QTimeZone tz("Pacific/Auckland");

    if (debug)
        printTimeZone(tz);

    // If the tz is not valid then skip as is probably using the UTC backend which is tested later
    if (!tz.isValid())
        return;

    // Validity tests
    QCOMPARE(tz.isValid(), true);

    // Comparison tests
    QTimeZone tz2("Pacific/Auckland");
    QTimeZone tz3("Australia/Sydney");
    QCOMPARE((tz == tz2), true);
    QCOMPARE((tz != tz2), false);
    QCOMPARE((tz == tz3), false);
    QCOMPARE((tz != tz3), true);

    QCOMPARE(tz.country(), QLocale::NewZealand);

    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime janPrev = QDateTime(QDate(2011, 1, 1), QTime(0, 0, 0), Qt::UTC);

    QCOMPARE(tz.offsetFromUtc(jan), 46800);
    QCOMPARE(tz.offsetFromUtc(jun), 43200);

    QCOMPARE(tz.standardTimeOffset(jan), 43200);
    QCOMPARE(tz.standardTimeOffset(jun), 43200);

    QCOMPARE(tz.daylightTimeOffset(jan), 3600);
    QCOMPARE(tz.daylightTimeOffset(jun), 0);

    QCOMPARE(tz.hasDaylightTime(), true);
    QCOMPARE(tz.isDaylightTime(jan), true);
    QCOMPARE(tz.isDaylightTime(jun), false);

    // Only test transitions if host system supports them
    if (tz.hasTransitions()) {
        QTimeZone::OffsetData tran = tz.nextTransition(jan);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1333202400000);
        QCOMPARE(tran.offsetFromUtc, 43200);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tz.nextTransition(jun);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1348927200000);
        QCOMPARE(tran.offsetFromUtc, 46800);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tz.previousTransition(jan);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1316872800000);
        QCOMPARE(tran.offsetFromUtc, 46800);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tz.previousTransition(jun);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1333202400000);
        QCOMPARE(tran.offsetFromUtc, 43200);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 0);

        QTimeZone::OffsetDataList expected;
        tran.atUtc = QDateTime::fromMSecsSinceEpoch(1301752800000, Qt::UTC);
        tran.offsetFromUtc = 46800;
        tran.standardTimeOffset = 43200;
        tran.daylightTimeOffset = 3600;
        expected << tran;
        tran.atUtc = QDateTime::fromMSecsSinceEpoch(1316872800000, Qt::UTC);
        tran.offsetFromUtc = 43200;
        tran.standardTimeOffset = 43200;
        tran.daylightTimeOffset = 0;
        expected << tran;
        QTimeZone::OffsetDataList result = tz.transitions(janPrev, jan);
        QCOMPARE(result.count(), expected.count());
        for (int i = 0; i > expected.count(); ++i) {
            QCOMPARE(result.at(i).atUtc, expected.at(i).atUtc);
            QCOMPARE(result.at(i).offsetFromUtc, expected.at(i).offsetFromUtc);
            QCOMPARE(result.at(i).standardTimeOffset, expected.at(i).standardTimeOffset);
            QCOMPARE(result.at(i).daylightTimeOffset, expected.at(i).daylightTimeOffset);
        }
    }
}

void tst_QTimeZone::nullTest()
{
    QTimeZone nullTz1;
    QTimeZone nullTz2;
    QTimeZone utc("UTC");

    // Validity tests
    QCOMPARE(nullTz1.isValid(), false);
    QCOMPARE(nullTz2.isValid(), false);
    QCOMPARE(utc.isValid(), true);

    // Comparison tests
    QCOMPARE((nullTz1 == nullTz2), true);
    QCOMPARE((nullTz1 != nullTz2), false);
    QCOMPARE((nullTz1 == utc), false);
    QCOMPARE((nullTz1 != utc), true);

    // Assignment tests
    nullTz2 = utc;
    QCOMPARE(nullTz2.isValid(), true);
    utc = nullTz1;
    QCOMPARE(utc.isValid(), false);

    QCOMPARE(nullTz1.id(), QByteArray());
    QCOMPARE(nullTz1.country(), QLocale::AnyCountry);
    QCOMPARE(nullTz1.comment(), QString());

    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime janPrev = QDateTime(QDate(2011, 1, 1), QTime(0, 0, 0), Qt::UTC);

    QCOMPARE(nullTz1.abbreviation(jan), QString());
    QCOMPARE(nullTz1.displayName(jan), QString());
    QCOMPARE(nullTz1.displayName(QTimeZone::StandardTime), QString());

    QCOMPARE(nullTz1.offsetFromUtc(jan), 0);
    QCOMPARE(nullTz1.offsetFromUtc(jun), 0);

    QCOMPARE(nullTz1.standardTimeOffset(jan), 0);
    QCOMPARE(nullTz1.standardTimeOffset(jun), 0);

    QCOMPARE(nullTz1.daylightTimeOffset(jan), 0);
    QCOMPARE(nullTz1.daylightTimeOffset(jun), 0);

    QCOMPARE(nullTz1.hasDaylightTime(), false);
    QCOMPARE(nullTz1.isDaylightTime(jan), false);
    QCOMPARE(nullTz1.isDaylightTime(jun), false);

    QTimeZone::OffsetData data = nullTz1.offsetData(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, std::numeric_limits<int>::min());
    QCOMPARE(data.standardTimeOffset, std::numeric_limits<int>::min());
    QCOMPARE(data.daylightTimeOffset, std::numeric_limits<int>::min());

    QCOMPARE(nullTz1.hasTransitions(), false);

    data = nullTz1.nextTransition(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, std::numeric_limits<int>::min());
    QCOMPARE(data.standardTimeOffset, std::numeric_limits<int>::min());
    QCOMPARE(data.daylightTimeOffset, std::numeric_limits<int>::min());

    data = nullTz1.previousTransition(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, std::numeric_limits<int>::min());
    QCOMPARE(data.standardTimeOffset, std::numeric_limits<int>::min());
    QCOMPARE(data.daylightTimeOffset, std::numeric_limits<int>::min());
}

void tst_QTimeZone::dataStreamTest()
{
    // Test the OffsetFromUtc backend serialization
    QTimeZone tz1("QST", 123456, "Qt Standard Time", "QST", QLocale::Norway, "Qt Testing");
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
    }
    QTimeZone tz2("UTC");
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
    }
    QCOMPARE(tz2.id(), QByteArray("QST"));
    QCOMPARE(tz2.comment(), QString("Qt Testing"));
    QCOMPARE(tz2.country(), QLocale::Norway);
    QCOMPARE(tz2.abbreviation(QDateTime::currentDateTime()), QString("QST"));
    QCOMPARE(tz2.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QString()),
             QString("Qt Standard Time"));
    QCOMPARE(tz2.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, QString()),
             QString("Qt Standard Time"));
    QCOMPARE(tz2.offsetFromUtc(QDateTime::currentDateTime()), 123456);

    // Test the system backend serialization
    tz1 = QTimeZone("Pacific/Auckland");

    // If not valid then probably using the UTC system backend so skip
    if (!tz1.isValid())
        return;

    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
    }
    tz2 = QTimeZone("UTC");
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
    }
    QCOMPARE(tz2.id(), tz1.id());
}

void tst_QTimeZone::availableTimeZoneIds()
{
    if (debug) {
        qDebug() << "";
        qDebug() << "Available Time Zones" ;
        qDebug() << QTimeZone::availableTimeZoneIds();
        qDebug() << "";
        qDebug() << "Available Time Zones in the US";
        qDebug() << QTimeZone::availableTimeZoneIds(QLocale::UnitedStates);
        qDebug() << "";
        qDebug() << "Available Time Zones with UTC Offset 0";
        qDebug() << QTimeZone::availableTimeZoneIds(0);
        qDebug() << "";
    } else {
        //Just test the calls work, we cannot know what any test machine has available
        QList<QByteArray> listAll = QTimeZone::availableTimeZoneIds();
        QList<QByteArray> listUs = QTimeZone::availableTimeZoneIds(QLocale::UnitedStates);
        QList<QByteArray> listZero = QTimeZone::availableTimeZoneIds(0);
    }
}

void tst_QTimeZone::windowsId()
{
/*
    Current Windows zones for "Central Standard Time":
    Region      Olsen Id(s)
    Default     "America/Chicago"
    Canada      "America/Winnipeg America/Rainy_River America/Rankin_Inlet America/Resolute"
    Mexico      "America/Matamoros"
    USA         "America/Chicago America/Indiana/Knox America/Indiana/Tell_City America/Menominee"
                "America/North_Dakota/Beulah America/North_Dakota/Center"
                "America/North_Dakota/New_Salem"
    AnyCountry  "CST6CDT"
*/
    QCOMPARE(QTimeZone::olsenIdToWindowsId("America/Chicago"),
             QByteArray("Central Standard Time"));
    QCOMPARE(QTimeZone::olsenIdToWindowsId("America/Resolute"),
             QByteArray("Central Standard Time"));

    // Partials shouldn't match
    QCOMPARE(QTimeZone::olsenIdToWindowsId("America/Chi"), QByteArray());
    QCOMPARE(QTimeZone::olsenIdToWindowsId("InvalidZone"), QByteArray());
    QCOMPARE(QTimeZone::olsenIdToWindowsId(QByteArray()), QByteArray());

    // Check default value
    QCOMPARE(QTimeZone::windowsIdToDefaultOlsenId("Central Standard Time"),
             QByteArray("America/Chicago"));
    QCOMPARE(QTimeZone::windowsIdToDefaultOlsenId("Central Standard Time", QLocale::Canada),
             QByteArray("America/Winnipeg"));
    QCOMPARE(QTimeZone::windowsIdToDefaultOlsenId("Central Standard Time", QLocale::AnyCountry),
             QByteArray("CST6CDT"));
    QCOMPARE(QTimeZone::windowsIdToDefaultOlsenId(QByteArray()), QByteArray());

    // No country is sorted list of all zones
    QList<QByteArray> list;
    list << "America/Chicago" << "America/Indiana/Knox" << "America/Indiana/Tell_City"
         << "America/Matamoros" << "America/Menominee" << "America/North_Dakota/Beulah"
         << "America/North_Dakota/Center" << "America/North_Dakota/New_Salem"
         << "America/Rainy_River" << "America/Rankin_Inlet" << "America/Resolute"
         << "America/Winnipeg" << "CST6CDT";
    QCOMPARE(QTimeZone::windowsIdToOlsenIds("Central Standard Time"), list);

    // Check country with no match returns empty list
    list.clear();
    QCOMPARE(QTimeZone::windowsIdToOlsenIds("Central Standard Time", QLocale::NewZealand),
             list);

    // Check valid country returns list in preference order
    list.clear();
    list << "America/Winnipeg" << "America/Rainy_River" << "America/Rankin_Inlet"
         << "America/Resolute";
    QCOMPARE(QTimeZone::windowsIdToOlsenIds("Central Standard Time", QLocale::Canada), list);

    list.clear();
    list << "America/Matamoros";
    QCOMPARE(QTimeZone::windowsIdToOlsenIds("Central Standard Time", QLocale::Mexico), list);

    list.clear();
    list << "America/Chicago" << "America/Indiana/Knox" << "America/Indiana/Tell_City"
         << "America/Menominee" << "America/North_Dakota/Beulah" << "America/North_Dakota/Center"
         << "America/North_Dakota/New_Salem";
    QCOMPARE(QTimeZone::windowsIdToOlsenIds("Central Standard Time", QLocale::UnitedStates),
             list);

    list.clear();
    list << "CST6CDT";
    QCOMPARE(QTimeZone::windowsIdToOlsenIds("Central Standard Time", QLocale::AnyCountry),
             list);

    // Check no windowsId return empty
    list.clear();
    QCOMPARE(QTimeZone::windowsIdToOlsenIds(QByteArray()), list);
    QCOMPARE(QTimeZone::windowsIdToOlsenIds(QByteArray(), QLocale::AnyCountry), list);
}

void tst_QTimeZone::utcTest()
{
#ifdef QT_BUILD_INTERNAL
    // Test default UTC constructor
    QUtcTimeZonePrivate tzp;
    QCOMPARE(tzp.isValid(),   true);
    QCOMPARE(tzp.id(), QByteArray("UTC"));
    QCOMPARE(tzp.country(), QLocale::AnyCountry);
    QCOMPARE(tzp.abbreviation(0), QString("UTC"));
    QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QString()), QString("UTC"));
    QCOMPARE(tzp.offsetFromUtc(0), 0);
    QCOMPARE(tzp.standardTimeOffset(0), 0);
    QCOMPARE(tzp.daylightTimeOffset(0), 0);
    QCOMPARE(tzp.hasDaylightTime(), false);
    QCOMPARE(tzp.hasTransitions(), false);

    // Test create from UTC Offset
    QDateTime now = QDateTime::currentDateTime();
    QTimeZone tz(36000);
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("UTC+10:00"));
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test invalid UTC offset, must be in range -14 to +14 hours
    int min = -14*60*60;
    int max = 14*60*60;
    QCOMPARE(QTimeZone(min - 1).isValid(), false);
    QCOMPARE(QTimeZone(min).isValid(), true);
    QCOMPARE(QTimeZone(min + 1).isValid(), true);
    QCOMPARE(QTimeZone(max - 1).isValid(), true);
    QCOMPARE(QTimeZone(max).isValid(), true);
    QCOMPARE(QTimeZone(max + 1).isValid(), false);

    // Test create from standard name
    tz = QTimeZone("UTC+10:00");
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("UTC+10:00"));
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test invalid UTC ID, must be in available list
    tz = QTimeZone("UTC+00:01");
    QCOMPARE(tz.isValid(),   false);

    // Test create custom zone
    tz = QTimeZone("QST", 123456, "Qt Standard Time", "QST", QLocale::Norway, "Qt Testing");
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("QST"));
    QCOMPARE(tz.comment(), QString("Qt Testing"));
    QCOMPARE(tz.country(), QLocale::Norway);
    QCOMPARE(tz.abbreviation(now), QString("QST"));
    QCOMPARE(tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QString()),
             QString("Qt Standard Time"));
    QCOMPARE(tz.offsetFromUtc(now), 123456);
    QCOMPARE(tz.standardTimeOffset(now), 123456);
    QCOMPARE(tz.daylightTimeOffset(now), 0);
#endif // QT_BUILD_INTERNAL
}

QTEST_APPLESS_MAIN(tst_QTimeZone)
#include "tst_qtimezone.moc"
