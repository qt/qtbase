// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QStringTokenizer>
#include <QStringBuilder>

#include <QTest>

#include <string>

Q_DECLARE_METATYPE(Qt::SplitBehavior)

class tst_QStringTokenizer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr() const;
    void basics_data() const;
    void basics() const;
    void toContainer() const;
};

static QStringList skipped(const QStringList &sl)
{
    QStringList result;
    result.reserve(sl.size());
    for (const QString &s : sl) {
        if (!s.isEmpty())
            result.push_back(s);
    }
    return result;
}

QString toQString(QStringView str)
{
    return str.toString();
}

template <typename Container>
QStringList toQStringList(const Container &c)
{
    QStringList r;
    for (auto &&e : c)
        r.push_back(toQString(e));
    return r;
}

void tst_QStringTokenizer::constExpr() const
{
    // compile-time checks
    {
        constexpr auto tok = qTokenize(u"a,b,c", u",");
        Q_UNUSED(tok);
    }
    {
        constexpr auto tok = qTokenize(u"a,b,c", u',');
        Q_UNUSED(tok);
    }
}

void tst_QStringTokenizer::basics_data() const
{
    QTest::addColumn<Qt::SplitBehavior>("sb");
    QTest::addColumn<Qt::CaseSensitivity>("cs");

#define ROW(sb, cs) \
    do { QTest::addRow("%s/%s", #sb, #cs) << Qt::SplitBehavior{Qt::sb} << Qt::cs; } while (0)

    ROW(KeepEmptyParts, CaseSensitive);
    ROW(KeepEmptyParts, CaseInsensitive);
    ROW(SkipEmptyParts, CaseSensitive);
    ROW(SkipEmptyParts, CaseInsensitive);

#undef ROW
}

void tst_QStringTokenizer::basics() const
{
    QFETCH(const Qt::SplitBehavior, sb);
    QFETCH(const Qt::CaseSensitivity, cs);

    auto expected = QStringList{"", "a", "b", "c", "d", "e", ""};
    if (sb & Qt::SkipEmptyParts)
        expected = skipped(expected);
    QCOMPARE(toQStringList(qTokenize(u",a,b,c,d,e,", u',', sb, cs)), expected);
    QCOMPARE(toQStringList(qTokenize(u",a,b,c,d,e,", u',', cs, sb)), expected);

    {
        auto tok = qTokenize(expected.join(u'x'), u"X" % QString(), Qt::CaseInsensitive);
        // the temporary QStrings returned from join() and the QStringBuilder expression
        // are now destroyed, but 'tok' should keep both alive
        QCOMPARE(toQStringList(tok), expected);
    }

    using namespace std::string_literals;

    {
        auto tok = qTokenize(expected.join(u'x'), u"X"s, Qt::CaseInsensitive);
        QCOMPARE(toQStringList(tok), expected);
    }

    {
        auto tok = qTokenize(expected.join(u'x'), QLatin1Char('x'), cs, sb);
        QCOMPARE(toQStringList(tok), expected);
    }
}

void tst_QStringTokenizer::toContainer() const
{
    // QStringView value_type:
    {
        auto tok = qTokenize(u"a,b,c", u',');
        auto v = tok.toContainer();
        QVERIFY((std::is_same_v<decltype(v), QList<QStringView>>));
    }
    // QLatin1String value_type
    {
        auto tok = qTokenize(QLatin1String{"a,b,c"}, u',');
        auto v = tok.toContainer();
        QVERIFY((std::is_same_v<decltype(v), QList<QLatin1String>>));
    }
    // QLatin1String value_type into QStringList
    {
        auto tok = qTokenize(QLatin1String{"a,b,c"}, u',');
        QStringList result;
        tok.toContainer(result);
        QCOMPARE(result, QStringList({"a", "b", "c"}));
    }
    // QLatin1String value_type into QStringList: rvalue overload
    {
        QStringList result;
        qTokenize(QLatin1String{"a,b,c"}, u',').toContainer(result);
        QCOMPARE(result, QStringList({"a", "b", "c"}));
    }
}

QTEST_APPLESS_MAIN(tst_QStringTokenizer)
#include "tst_qstringtokenizer.moc"
