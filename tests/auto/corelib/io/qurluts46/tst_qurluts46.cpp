// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QTest>
#include <QSet>
#include <QByteArray>
#include <algorithm>

class tst_QUrlUts46 : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void idnaTestV2_data();
    void idnaTestV2();

private:
    // All error codes in UTR #46 revision 31 (Unicode 15.1):
    //      A4_1, A4_2,
    //      B1, B2, B3, B4, B5, B6,
    //      C1, C2,
    //      P4,
    //      V1, V2, V3, V5, V6,
    //      X4_2
    //
    // NOTE: moving this inside idnaTestV2_data() results in ICE with MSVC 2019
    static const QSet<QByteArray> fatalErrors;
};

const QSet<QByteArray> tst_QUrlUts46::fatalErrors = {
        "A4_2", // Empty ASCII label
};

/**
 * Replace \uXXXX escapes in test case fields.
 * Turn "" into empty string, and empty string into null.
 */
static QString postProcessField(const QString &field)
{
    if (field == u"\"\"")
        return QString{u""}; // Empty, but not null
    if (field == u"")
        return {}; // Null

    static const QRegularExpression re(R"(\\u([[:xdigit:]]{4}))");

    QString result;
    qsizetype lastIdx = 0;

    for (const auto &match : re.globalMatch(field)) {
        // Add stuff before the match
        result.append(field.mid(lastIdx, match.capturedStart() - lastIdx));
        bool ok = false;
        auto c = match.captured(1).toUInt(&ok, 16);
        if (!ok) {
            qFatal("Failed to parse a Unicode escape: %s", qPrintable(match.captured(1)));
        }

        result.append(QChar(c));
        lastIdx = match.capturedEnd();
    }

    // Append the unescaped end
    result.append(field.mid(lastIdx));

    return result;
}

void tst_QUrlUts46::idnaTestV2_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("toUnicode");
    QTest::addColumn<bool>("toUnicodeOk");
    QTest::addColumn<QString>("toAsciiN");
    QTest::addColumn<bool>("toAsciiNOk");
    QTest::addColumn<QString>("toAsciiT");
    QTest::addColumn<bool>("toAsciiTOk");

    QFile dataFile(QFINDTESTDATA("testdata/IdnaTestV2.txt"));
    qDebug() << "Data file:" << dataFile.fileName();
    QVERIFY(dataFile.open(QFile::ReadOnly));

    auto isToAsciiOk = [](const QByteArray &s, bool ifEmpty) {
        if (s.isEmpty())
            return ifEmpty;

        Q_ASSERT(s.startsWith('[') && s.endsWith(']'));

        const auto errors = s.sliced(1, s.size() - 2).split(',');
        // NOTE: empty string is not in fatalErrors and it's ok
        return std::all_of(errors.begin(), errors.end(),
                           [](auto &e) { return !fatalErrors.contains(e.trimmed()); });
    };

    for (unsigned int lineNo = 1; !dataFile.atEnd(); lineNo++) {
        auto line = dataFile.readLine().trimmed();

        int commentIdx = line.indexOf('#');
        if (commentIdx != -1)
            line = line.left(commentIdx).trimmed();
        if (line.isEmpty())
            continue;

        auto fields = line.split(';');
        Q_ASSERT(fields.size() == 7);

        for (auto &field : fields)
            field = postProcessField(field.trimmed()).toUtf8();

        const QString &source = fields[0];
        QString toUnicode = fields[1].isNull() ? source : fields[1];
        bool toUnicodeOk = fields[2].isNull();
        bool toUnicodeOkForAscii = isToAsciiOk(fields[2], true);
        QString toAsciiN = fields[3].isNull() ? toUnicode : fields[3];
        bool toAsciiNOk = isToAsciiOk(fields[4], toUnicodeOkForAscii);
        QString toAsciiT = fields[5].isNull() ? toAsciiN : fields[5];
        bool toAsciiTOk = isToAsciiOk(fields[6], toAsciiNOk);

        QTest::addRow("line %u", lineNo) << source << toUnicode << toUnicodeOk << toAsciiN
                                         << toAsciiNOk << toAsciiT << toAsciiTOk;
    }
}

void tst_QUrlUts46::idnaTestV2()
{
    QFETCH(QString, source);
    QFETCH(QString, toUnicode);
    QFETCH(bool, toUnicodeOk);
    QFETCH(QString, toAsciiN);
    QFETCH(bool, toAsciiNOk);
    QFETCH(QString, toAsciiT);
    QFETCH(bool, toAsciiTOk);

#define TEST_TRAILING_EMPTY_ROOT(x)                                                               \
    do {                                                                                          \
        if ((x).endsWith(u".")) {                                                                 \
            QEXPECT_FAIL("", "Qt doesn't yet reject domain names with trailing empty root label", \
                         Abort);                                                                  \
        }                                                                                         \
    } while (false)

    QString toAceN = QUrl::toAce(source);
    TEST_TRAILING_EMPTY_ROOT(toAceN);

    if (toUnicodeOk && toAsciiNOk)
        QCOMPARE(toAceN, toAsciiN);
    else if (toAsciiNOk)
        QVERIFY(toAceN.isEmpty() || toAceN == toAsciiN);
    else
        QCOMPARE(toAceN, QString());

    QString toAceT = QUrl::toAce(source, QUrl::AceTransitionalProcessing);
    TEST_TRAILING_EMPTY_ROOT(toAceT);

    if (toUnicodeOk && toAsciiTOk)
        QCOMPARE(toAceT, toAsciiT);
    else if (toAsciiTOk)
        QVERIFY(toAceT.isEmpty() || toAceT == toAsciiT);
    else
        QCOMPARE(toAceT, QString());

    QString normalized = QUrl::fromAce(toAceN.toUtf8(), QUrl::IgnoreIDNWhitelist);
    if (toUnicodeOk && !toAceN.isEmpty())
        QCOMPARE(normalized, toUnicode);
    else
        QCOMPARE(normalized, toAceN);
}

QTEST_APPLESS_MAIN(tst_QUrlUts46)

#include "tst_qurluts46.moc"
