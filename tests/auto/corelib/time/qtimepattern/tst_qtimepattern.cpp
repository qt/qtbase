// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "private/qttemporalpattern_p.h"

#include "QtCore/qlatin1stringview.h"
#include <QTest>

using namespace Qt::StringLiterals;

class tst_QTimePattern : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fromQtFormat_data();
    void fromQtFormat();
    void forLocale_data();
    void forLocale();

private:
    static constexpr struct { int hr; int min; int sec; int mil; } sampleTimes[] = {
        {0, 0, 0, 0}, {1, 0, 0, 0}, {12, 0, 0, 0}, {23, 0, 0, 0},
        {0, 30, 0, 0}, {2, 30, 0, 0}, {6, 30, 0, 0}, {10, 30, 0, 0}, {15, 30, 0, 0}, {20, 30, 0, 0},
        {0, 0, 30, 0}, {2, 0, 30, 0}, {6, 0, 30, 0}, {10, 0, 30, 0}, {15, 0, 30, 0}, {20, 0, 30, 0},
        {0, 59, 59, 999}, {11, 59, 59, 999}, {23, 59, 59, 999},
    };
    static constexpr QLatin1StringView localeTags[] = {
        "en"_L1, "en-GB"_L1,
        "ff-Adlm-GN"_L1, // non-BMP-digits
    };
};

void tst_QTimePattern::fromQtFormat_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QTime>("time");

    const auto addRows = [](QByteArrayView stem, QString &&format, bool valid) {
        if (valid) {
            for (const auto &loc : localeTags) {
                const QString locName(loc);
                for (const auto &sam : sampleTimes) {
                    QTest::addRow("%s/%s/%02d:%02d:%02d.%03d",
                                  stem.constData(), loc.constData(),
                                  sam.hr, sam.min, sam.sec, sam.mil)
                        << format << locName << QTime(sam.hr, sam.min, sam.sec, sam.mil);
                }
            }
        } else {
            QTest::addRow("%s", stem.constData()) << format << u"C"_s << QTime();
        }
    };
    addRows("empy", u""_s, false);
    // 'h' is treated as 'H' unless [aA] is present:
    addRows("h:m:s.z", u"h:m:s.z"_s, true);
    addRows("H:m:s.z", u"H:m:s.z"_s, true);
    addRows("H''m''s''z", u"H''m''s''z"_s, true);
    addRows("HHmmss.zzz", u"HHmmss.zzz"_s, true);
    addRows("Hms.z", u"Hms.z"_s, true);
}

void tst_QTimePattern::fromQtFormat()
{
    QFETCH(const QString, format);
    QFETCH(const QString, localeName);
    // QFETCH(const QTime, time);
    const QLocale locale(localeName);
    auto pattern = QTimePattern::fromQtFormat(format);
    pattern.setLocale(locale);

    QCOMPARE(pattern.locale(), locale);

    // TODO: round-trip tests, once serialization is implemented
}

void tst_QTimePattern::forLocale_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QLocale::FormatType>("type");
    QTest::addColumn<QTime>("time");

    constexpr QLocale::FormatType types[] = {
        QLocale::LongFormat, QLocale::ShortFormat, QLocale::NarrowFormat
    };

    for (const auto loc : localeTags) {
        const QString locName(loc);
        for (const auto type : types) {
            const char *fmt = [](QLocale::FormatType type) {
                switch (type) {
                case QLocale::LongFormat: return "long";
                case QLocale::ShortFormat: return "shrt";
                case QLocale::NarrowFormat: return "nrow";
                }
                Q_UNREACHABLE_RETURN("<unknown>");
            }(type);
            for (const auto sam : sampleTimes) {
                QTest::addRow("%s/%s/%02d:%02d:%02d.%03d",
                              loc.constData(), fmt, sam.hr, sam.min, sam.sec, sam.mil)
                    << locName << type << QTime(sam.hr, sam.min, sam.sec, sam.mil);
            }
        }
    };
}

void tst_QTimePattern::forLocale()
{
    QFETCH(const QString, localeName);
    QFETCH(const QLocale::FormatType, type);
    // QFETCH(const QTime, time);
    const QLocale locale(localeName);
    const auto pattern = QTimePattern::forLocale(locale, type);

    QCOMPARE(pattern.locale(), locale);

    // TODO: round-trip tests, once serialization is implemented
}

QTEST_APPLESS_MAIN(tst_QTimePattern)
#include "tst_qtimepattern.moc"
