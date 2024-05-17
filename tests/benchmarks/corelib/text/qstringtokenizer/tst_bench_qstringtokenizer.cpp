// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QStringTokenizer>

class tst_QStringTokenizer : public QObject
{
    Q_OBJECT

    void tokenize_data() const;
    template <typename T, typename U>
    void tokenize() const;
private slots:
    void tokenize_qlatin1string_qlatin1string_data() const { tokenize_data(); }
    void tokenize_qlatin1string_qlatin1string() const { tokenize<QLatin1String, QLatin1String>(); }
    void tokenize_qstring_qstring_data() const { tokenize_data(); }
    void tokenize_qstring_qstring() const { tokenize<QString, QString>(); }
    void tokenize_qlatin1string_qstring_data() const { tokenize_data(); }
    void tokenize_qlatin1string_qstring() const { tokenize<QLatin1String, QString>(); }
    void tokenize_qstring_qlatin1string_data() const { tokenize_data(); }
    void tokenize_qstring_qlatin1string() const { tokenize<QString, QLatin1String>(); }
};

template<typename T>
T fromByteArray(QByteArrayView v);

template<>
QString fromByteArray<QString>(QByteArrayView v)
{
    return QString::fromLatin1(v);
}

template<>
QLatin1String fromByteArray<QLatin1String>(QByteArrayView v)
{
    return QLatin1String(v.data(), v.size());
}

void tst_QStringTokenizer::tokenize_data() const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("separator");
    QTest::addColumn<bool>("caseSensitive");
    QTest::addColumn<int>("expectedCount");

    QByteArray shortSentence = "A seriously short sentence.";
    QTest::addRow("short-sentence-spaces") << shortSentence << QByteArray(" ") << true << 4;
    QTest::addRow("short-sentence-spaces-case-insensitive")
            << shortSentence << QByteArray(" ") << false << 4;

    QTest::addRow("short-sentence-se") << shortSentence << QByteArray("se") << true << 3;
    QTest::addRow("short-sentence-se-case-insensitive")
            << shortSentence << QByteArray("Se") << false << 3;

    QFile file(":/data/lorem.txt");
    if (!file.open(QFile::ReadOnly))
        qFatal("Can't open lorem.txt");

    const QByteArray content = file.readAll();
    QTest::addRow("lorem-ipsum-spaces") << content << QByteArray(" ") << true << 3250;
    QTest::addRow("lorem-ipsum-spaces-case-insensitive")
            << content << QByteArray(" ") << false << 3250;

    QTest::addRow("lorem-ipsum-l") << content << QByteArray("l") << true << 771;
    QTest::addRow("lorem-ipsum-l-case-insensitive")
            << content << QByteArray("l") << false << 772;

    QTest::addRow("lorem-ipsum-lo") << content << QByteArray("lo") << true << 130;
    QTest::addRow("lorem-ipsum-lo-case-insensitive")
            << content << QByteArray("lo") << false << 131;

    QTest::addRow("lorem-ipsum-lor") << content << QByteArray("lor") << true << 122;
    QTest::addRow("lorem-ipsum-lor-case-insensitive")
            << content << QByteArray("lor") << false << 123;

    QTest::addRow("lorem-ipsum-lore") << content << QByteArray("lore") << true << 73;
    QTest::addRow("lorem-ipsum-lore-case-insensitive")
            << content << QByteArray("lore") << false << 74;

    QTest::addRow("lorem-ipsum-lorem") << content << QByteArray("lorem") << true << 34;
    QTest::addRow("lorem-ipsum-lorem-case-insensitive")
            << content << QByteArray("lorem") << false << 35;

    QTest::addRow("lorem-ipsum-lorem i") << content << QByteArray("lorem i") << true << 5;
    QTest::addRow("lorem-ipsum-lorem i-case-insensitive")
            << content << QByteArray("lorem i") << false << 6;

    QTest::addRow("lorem-ipsum-et explicabo s") << content << QByteArray("et explicabo s") << true << 3;
    QTest::addRow("lorem-ipsum-et explicabo s-case-insensitive")
            << content << QByteArray("et explicabo s") << false << 3;
}

template<typename T, typename U>
void tst_QStringTokenizer::tokenize() const
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, separator);
    QFETCH(bool, caseSensitive);
    QFETCH(int, expectedCount);

    T haystack = fromByteArray<T>(input);
    U needle = fromByteArray<U>(separator);

    const Qt::CaseSensitivity sensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QBENCHMARK {
        QStringTokenizer tok(haystack, needle, sensitivity);
        qsizetype count = 0;
        for (auto res : tok) {
            count++;
            Q_UNUSED(res);
        }
        QCOMPARE(count, expectedCount);
    }
}

QTEST_MAIN(tst_QStringTokenizer)

#include "tst_bench_qstringtokenizer.moc"
