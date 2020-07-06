/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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
}

QTEST_APPLESS_MAIN(tst_QStringTokenizer)
#include "tst_qstringtokenizer.moc"
