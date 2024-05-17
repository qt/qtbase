// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <private/qdatetimeparser_p.h>

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

// access to needed members in QDateTimeParser
class QDTPUnitTestParser : public QDateTimeParser
{
public:
    QDTPUnitTestParser() : QDateTimeParser(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit) { }

    // forward data structures
    using QDateTimeParser::ParsedSection;
    using QDateTimeParser::State;

    // function to manipulate private internals
    void setText(QString text) { m_text = text; }

    // forwarding of methods
    using QDateTimeParser::parseSection;
};

bool operator==(const QDTPUnitTestParser::ParsedSection &a,
                const QDTPUnitTestParser::ParsedSection &b)
{
    return a.value == b.value && a.used == b.used && a.zeroes == b.zeroes && a.state == b.state;
}

// pretty printing for ParsedSection
char *toString(const QDTPUnitTestParser::ParsedSection &section)
{
    using QTest::toString;
    return toString(QByteArray("ParsedSection(") + "state=" + QByteArray::number(section.state)
                    + ", value=" + QByteArray::number(section.value)
                    + ", used=" + QByteArray::number(section.used)
                    + ", zeros=" + QByteArray::number(section.zeroes) + ")");
}

QT_END_NAMESPACE

class tst_QDateTimeParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void reparse();
    void parseSection_data();
    void parseSection();

    void intermediateYear_data();
    void intermediateYear();
};

void tst_QDateTimeParser::reparse()
{
    const QDateTime when = QDate(2023, 6, 15).startOfDay();
    // QTBUG-114575: 6.2 through 6.5 got back a bogus Qt::TimeZone (with zero offset):
    const auto expect = ([](QStringView name) {
        // When local time is UTC or a fixed offset from it, the parser prefers
        // to interpret a UTC or offset suffix as such, rather than as local
        // time (thereby avoiding DST-ness checks). We have to match that here.
        if (name == "UTC"_L1)
            return Qt::UTC;
        if (name.startsWith(u'+') || name.startsWith(u'-')) {
            if (std::all_of(name.begin() + 1, name.end(), [](QChar ch) { return ch == u'0'; }))
                return Qt::UTC;
            if (std::all_of(name.begin() + 1, name.end(), [](QChar ch) { return ch.isDigit(); }))
                return Qt::OffsetFromUTC;
            // Potential hh:mm offset ?  Not yet seen as local tzname[] entry.
        }
        return Qt::LocalTime;
    });

    const QStringView format = u"dd/MM/yyyy HH:mm t";
    QDateTimeParser who(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit);
    QVERIFY(who.parseFormat(format));
    {
        // QDTP defaults to the system locale.
        const auto state = who.parse(QLocale::system().toString(when, format), -1, when, false);
        QCOMPARE(state.state, QDateTimeParser::Acceptable);
        QVERIFY(!state.conflicts);
        QCOMPARE(state.padded, 0);
        QCOMPARE(state.value.timeSpec(), expect(when.timeZoneAbbreviation()));
        QCOMPARE(state.value, when);
    }
    {
        // QDT::toString() uses the C locale:
        who.setDefaultLocale(QLocale::c());
        const QString zoneName = ([when]() {
#if QT_CONFIG(timezone)
            if (QLocale::c() != QLocale::system()) {
                const QString local = when.timeRepresentation().displayName(
                    when, QTimeZone::ShortName, QLocale::c());
                if (!local.isEmpty())
                    return local;
            }
#endif
            return when.timeZoneAbbreviation();
        })();
        const auto state = who.parse(when.toString(format), -1, when, false);
        QCOMPARE(state.state, QDateTimeParser::Acceptable);
        QVERIFY(!state.conflicts);
        QCOMPARE(state.padded, 0);
        QCOMPARE(state.value.timeSpec(), expect(zoneName));
        QCOMPARE(state.value, when);
    }
}

void tst_QDateTimeParser::parseSection_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("sectionIndex");
    QTest::addColumn<int>("offset");
    QTest::addColumn<QDTPUnitTestParser::ParsedSection>("expected");

    using ParsedSection = QDTPUnitTestParser::ParsedSection;
    using State = QDTPUnitTestParser::State;
    QTest::newRow("short-year-begin")
        << "yyyy_MM_dd" << "200_12_15" << 0 << 0
        << ParsedSection(State::Intermediate ,200, 3, 0);

    QTest::newRow("short-year-middle")
        << "MM-yyyy-dd" << "12-200-15" << 1 << 3
        << ParsedSection(State::Intermediate, 200, 3, 0);

    QTest::newRow("negative-year-middle-5")
        << "MM-yyyy-dd" << "12--2000-15" << 1 << 3
        << ParsedSection(State::Acceptable, -2000, 5, 0);

    QTest::newRow("short-negative-year-middle-4")
        << "MM-yyyy-dd" << "12--200-15" << 1 << 3
        << ParsedSection(State::Intermediate, -200, 4, 0);

    QTest::newRow("short-negative-year-middle-3")
        << "MM-yyyy-dd" << "12--20-15" << 1 << 3
        << ParsedSection(State::Intermediate, -20, 3, 0);

    QTest::newRow("short-negative-year-middle-2")
        << "MM-yyyy-dd" << "12--2-15" << 1 << 3
        << ParsedSection(State::Intermediate, -2, 2, 0);

    QTest::newRow("short-negative-year-middle-1")
        << "MM-yyyy-dd" << "12---15"  << 1 << 3
        << ParsedSection(State::Intermediate, 0, 1, 0);

    // Here the -15 will be understood as year, with separator and day omitted,
    // although it could equally be read as month and day with missing year.
    QTest::newRow("short-negative-year-middle-0")
        << "MM-yyyy-dd" << "12--15"  << 1 << 3
        << ParsedSection(State::Intermediate, -15, 3, 0);
}

void tst_QDateTimeParser::parseSection()
{
    QFETCH(QString, format);
    QFETCH(QString, input);
    QFETCH(int, sectionIndex);
    QFETCH(int, offset);
    QFETCH(QDTPUnitTestParser::ParsedSection, expected);

    QDTPUnitTestParser testParser;

    QVERIFY(testParser.parseFormat(format));
    QDateTime val(QDate(1900, 1, 1).startOfDay());

    testParser.setText(input);
    auto result = testParser.parseSection(val, sectionIndex, offset);
    QCOMPARE(result, expected);
}

void tst_QDateTimeParser::intermediateYear_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QDate>("expected");

    QTest::newRow("short-year-begin")
        << "yyyy_MM_dd" << "200_12_15" << QDate(200, 12, 15);
    QTest::newRow("short-year-mid")
        << "MM_yyyy_dd" << "12_200_15" << QDate(200, 12, 15);
    QTest::newRow("short-year-end")
        << "MM_dd_yyyy" << "12_15_200" << QDate(200, 12, 15);
}

void tst_QDateTimeParser::intermediateYear()
{
    QFETCH(QString, format);
    QFETCH(QString, input);
    QFETCH(QDate, expected);

    QDateTimeParser testParser(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit);

    QVERIFY(testParser.parseFormat(format));

    // Indian/Cocos has a transition at the start of 1900, so it started this
    // day at 00:02:20, throwing a time offset into QDTP.
    QDateTime val(QDate(1900, 1, 1).startOfDay());
    const QDateTimeParser::StateNode tmp = testParser.parse(input, -1, val, false);
    QCOMPARE(tmp.state, QDateTimeParser::Intermediate);
    QCOMPARE(tmp.value.date(), expected);
}

QTEST_APPLESS_MAIN(tst_QDateTimeParser)

#include "tst_qdatetimeparser.moc"
