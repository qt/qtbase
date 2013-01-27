/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <QtXml/QtXml>
#if defined(Q_OS_WINCE)
#include <QtGui/QFontDatabase>
#endif
#include <QtGui/QFontInfo>
#include <QtGui/QFontMetrics>

#include "private/qcssparser_p.h"

class tst_QCssParser : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void scanner_data();
    void scanner();
    void term_data();
    void term();
    void expr_data();
    void expr();
    void import();
    void media();
    void page();
    void ruleset();
    void selector_data();
    void selector();
    void prio();
    void escapes();
    void malformedDeclarations_data();
    void malformedDeclarations();
    void invalidAtKeywords();
    void marginValue();
    void marginValue_data();
    void colorValue_data();
    void colorValue();
    void styleSelector_data();
    void styleSelector();
    void specificity_data();
    void specificity();
    void specificitySort_data();
    void specificitySort();
    void rulesForNode_data();
    void rulesForNode();
    void shorthandBackgroundProperty_data();
    void shorthandBackgroundProperty();
    void pseudoElement_data();
    void pseudoElement();
    void gradient_data();
    void gradient();
    void extractFontFamily_data();
    void extractFontFamily();
    void extractBorder_data();
    void extractBorder();
    void noTextDecoration();
    void quotedAndUnquotedIdentifiers();

private:
#if defined(Q_OS_WINCE)
    int m_timesFontId;
#endif
};

void tst_QCssParser::initTestCase()
{
#if defined(Q_OS_WINCE)
    QFontDatabase fontDB;
    m_timesFontId = -1;
    if (!fontDB.families().contains("Times New Roman")) {
        m_timesFontId = QFontDatabase::addApplicationFont("times.ttf");
        QVERIFY(m_timesFontId != -1);
    }
#endif
}

void tst_QCssParser::cleanupTestCase()
{
#if defined(Q_OS_WINCE)
    if (m_timesFontId != -1)
        QFontDatabase::removeApplicationFont(m_timesFontId);
#endif
}

void tst_QCssParser::scanner_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

#if !defined(Q_OS_IRIX) && !defined(Q_OS_WINCE)
    QDir d(SRCDIR);
#else
    QDir d(QDir::current());
#endif
    d.cd("testdata");
    d.cd("scanner");
    foreach (QFileInfo test, d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString dir = test.absoluteFilePath() + QDir::separator();
        QTest::newRow(qPrintable(test.baseName()))
            << dir + "input"
            << dir + "output"
            ;
    }
}


static const char *tokenName(QCss::TokenType t)
{
    switch (t) {
        case QCss::NONE: return "NONE";
        case QCss::S: return "S";
        case QCss::CDO: return "CDO";
        case QCss::CDC: return "CDC";
        case QCss::INCLUDES: return "INCLUDES";
        case QCss::DASHMATCH: return "DASHMATCH";
        case QCss::LBRACE: return "LBRACE";
        case QCss::PLUS: return "PLUS";
        case QCss::GREATER: return "GREATER";
        case QCss::COMMA: return "COMMA";
        case QCss::STRING: return "STRING";
        case QCss::INVALID: return "INVALID";
        case QCss::IDENT: return "IDENT";
        case QCss::HASH: return "HASH";
        case QCss::ATKEYWORD_SYM: return "ATKEYWORD_SYM";
        case QCss::EXCLAMATION_SYM: return "EXCLAMATION_SYM";
        case QCss::LENGTH: return "LENGTH";
        case QCss::PERCENTAGE: return "PERCENTAGE";
        case QCss::NUMBER: return "NUMBER";
        case QCss::FUNCTION: return "FUNCTION";
        case QCss::COLON: return "COLON";
        case QCss::SEMICOLON: return "SEMICOLON";
        case QCss::RBRACE: return "RBRACE";
        case QCss::SLASH: return "SLASH";
        case QCss::MINUS: return "MINUS";
        case QCss::DOT: return "DOT";
        case QCss::STAR: return "STAR";
        case QCss::LBRACKET: return "LBRACKET";
        case QCss::RBRACKET: return "RBRACKET";
        case QCss::EQUAL: return "EQUAL";
        case QCss::LPAREN: return "LPAREN";
        case QCss::RPAREN: return "RPAREN";
        case QCss::OR: return "OR";
    }
    return "";
}

static void debug(const QVector<QCss::Symbol> &symbols, int index = -1)
{
    qDebug() << "all symbols:";
    for (int i = 0; i < symbols.count(); ++i)
        qDebug() << "(" << i << "); Token:" << tokenName(symbols.at(i).token) << "; Lexem:" << symbols.at(i).lexem();
    if (index != -1)
        qDebug() << "failure at index" << index;
}

//static void debug(const QCss::Parser &p) { debug(p.symbols); }

void tst_QCssParser::scanner()
{
    QFETCH(QString, input);
    QFETCH(QString, output);

    QFile inputFile(input);
    QVERIFY(inputFile.open(QIODevice::ReadOnly|QIODevice::Text));
    QVector<QCss::Symbol> symbols;
    QCss::Scanner::scan(QCss::Scanner::preprocess(QString::fromUtf8(inputFile.readAll())), &symbols);

    QVERIFY(symbols.count() > 1);
    QVERIFY(symbols.last().token == QCss::S);
    QVERIFY(symbols.last().lexem() == QLatin1String("\n"));
    symbols.remove(symbols.count() - 1, 1);

    QFile outputFile(output);
    QVERIFY(outputFile.open(QIODevice::ReadOnly|QIODevice::Text));
    QStringList lines;
    while (!outputFile.atEnd()) {
        QString line = QString::fromUtf8(outputFile.readLine());
        if (line.endsWith(QLatin1Char('\n')))
            line.chop(1);
        lines.append(line);
    }

    if (lines.count() != symbols.count()) {
        debug(symbols);
        QCOMPARE(lines.count(), symbols.count());
    }

    for (int i = 0; i < lines.count(); ++i) {
        QStringList l = lines.at(i).split(QChar::fromLatin1('|'));
        QCOMPARE(l.count(), 2);
        const QString expectedToken = l.at(0);
        const QString expectedLexem = l.at(1);
        QString actualToken = QString::fromLatin1(tokenName(symbols.at(i).token));
        if (actualToken != expectedToken) {
            debug(symbols, i);
            QCOMPARE(actualToken, expectedToken);
        }
        if (symbols.at(i).lexem() != expectedLexem) {
            debug(symbols, i);
            QCOMPARE(symbols.at(i).lexem(), expectedLexem);
        }
    }
}

Q_DECLARE_METATYPE(QCss::Value)

void tst_QCssParser::term_data()
{
    QTest::addColumn<bool>("parseSuccess");
    QTest::addColumn<QString>("css");
    QTest::addColumn<QCss::Value>("expectedValue");

    QCss::Value val;

    val.type = QCss::Value::Percentage;
    val.variant = QVariant(double(200));
    QTest::newRow("percentage") << true << "200%" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10px");
    QTest::newRow("px") << true << "10px" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10cm");
    QTest::newRow("cm") << true << "10cm" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10mm");
    QTest::newRow("mm") << true << "10mm" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10pt");
    QTest::newRow("pt") << true << "10pt" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10pc");
    QTest::newRow("pc") << true << "10pc" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("42in");
    QTest::newRow("inch") << true << "42in" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10deg");
    QTest::newRow("deg") << true << "10deg" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10rad");
    QTest::newRow("rad") << true << "10rad" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10grad");
    QTest::newRow("grad") << true << "10grad" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10ms");
    QTest::newRow("time") << true << "10ms" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10s");
    QTest::newRow("times") << true << "10s" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10hz");
    QTest::newRow("hz") << true << "10hz" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10khz");
    QTest::newRow("khz") << true << "10khz" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10myunit");
    QTest::newRow("dimension") << true << "10myunit" << val;

    val.type = QCss::Value::Percentage;

    val.type = QCss::Value::Percentage;
    val.variant = QVariant(double(-200));
    QTest::newRow("minuspercentage") << true << "-200%" << val;

    val.type = QCss::Value::Length;
    val.variant = QString("10em");
    QTest::newRow("ems") << true << "10em" << val;

    val.type = QCss::Value::String;
    val.variant = QVariant(QString("foo"));
    QTest::newRow("string") << true << "\"foo\"" << val;

    val.type = QCss::Value::Function;
    val.variant = QVariant(QStringList() << "myFunc" << "23, (nested text)");
    QTest::newRow("function") << true << "myFunc(23, (nested text))" << val;

    QTest::newRow("function_failure") << false << "myFunction((blah)" << val;
    QTest::newRow("function_failure2") << false << "+myFunc(23, (nested text))" << val;

    val.type = QCss::Value::Color;
    val.variant = QVariant(QColor("#12ff34"));
    QTest::newRow("hexcolor") << true << "#12ff34" << val;

    val.type = QCss::Value::Color;
    val.variant = QVariant(QColor("#ffbb00"));
    QTest::newRow("hexcolor2") << true << "#fb0" << val;

    QTest::newRow("hexcolor_failure") << false << "#cafebabe" << val;

    val.type = QCss::Value::Uri;
    val.variant = QString("www.kde.org");
    QTest::newRow("uri1") << true << "url(\"www.kde.org\")" << val;

    QTest::newRow("uri2") << true << "url(www.kde.org)" << val;

    val.type = QCss::Value::KnownIdentifier;
    val.variant = int(QCss::Value_Italic);
    QTest::newRow("italic") << true << "italic" << val;

    val.type = QCss::Value::KnownIdentifier;
    val.variant = int(QCss::Value_Italic);
    QTest::newRow("ItaLIc") << true << "ItaLIc" << val;
}

void tst_QCssParser::term()
{
    QFETCH(bool, parseSuccess);
    QFETCH(QString, css);
    QFETCH(QCss::Value, expectedValue);

    if (strcmp(QTest::currentDataTag(), "hexcolor_failure") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QCssParser::parseHexColor: Unknown color name '#cafebabe'");

    QCss::Parser parser(css);
    QCss::Value val;
    QVERIFY(parser.testTerm());
    QCOMPARE(parser.parseTerm(&val), parseSuccess);
    if (parseSuccess) {
        QCOMPARE(int(val.type), int(expectedValue.type));
        if (val.variant != expectedValue.variant) {
            qDebug() << "val.variant:" << val.variant << "expectedValue.variant:" << expectedValue.variant;
            QCOMPARE(val.variant, expectedValue.variant);
        }
    }
}

void tst_QCssParser::expr_data()
{
    QTest::addColumn<bool>("parseSuccess");
    QTest::addColumn<QString>("css");
    QTest::addColumn<QVector<QCss::Value> >("expectedValues");

    QVector<QCss::Value> values;
    QCss::Value val;

    QCss::Value comma;
    comma.type = QCss::Value::TermOperatorComma;

    val.type = QCss::Value::Identifier;
    val.variant = QLatin1String("foo");
    values << val;
    values << comma;
    val.variant = QLatin1String("bar");
    values << val;
    values << comma;
    val.variant = QLatin1String("baz");
    values << val;
    QTest::newRow("list") << true << "foo, bar, baz" << values;
    values.clear();
}

void tst_QCssParser::expr()
{
    QFETCH(bool, parseSuccess);
    QFETCH(QString, css);
    QFETCH(QVector<QCss::Value>, expectedValues);

    QCss::Parser parser(css);
    QVector<QCss::Value> values;
    QVERIFY(parser.testExpr());
    QCOMPARE(parser.parseExpr(&values), parseSuccess);
    if (parseSuccess) {
        QCOMPARE(values.count(), expectedValues.count());

        for (int i = 0; i < values.count(); ++i) {
            QCOMPARE(int(values.at(i).type), int(expectedValues.at(i).type));
            QCOMPARE(values.at(i).variant, expectedValues.at(i).variant);
        }
    }
}

void tst_QCssParser::import()
{
    QCss::Parser parser("@import \"plainstring\";");
    QVERIFY(parser.testImport());
    QCss::ImportRule rule;
    QVERIFY(parser.parseImport(&rule));
    QCOMPARE(rule.href, QString("plainstring"));

    parser = QCss::Parser("@import url(\"www.kde.org\") print/*comment*/,screen;");
    QVERIFY(parser.testImport());
    QVERIFY(parser.parseImport(&rule));
    QCOMPARE(rule.href, QString("www.kde.org"));
    QCOMPARE(rule.media.count(), 2);
    QCOMPARE(rule.media.at(0), QString("print"));
    QCOMPARE(rule.media.at(1), QString("screen"));
}

void tst_QCssParser::media()
{
    QCss::Parser parser("@media print/*comment*/,screen /*comment to ignore*/{ }");
    QVERIFY(parser.testMedia());
    QCss::MediaRule rule;
    QVERIFY(parser.parseMedia(&rule));
    QCOMPARE(rule.media.count(), 2);
    QCOMPARE(rule.media.at(0), QString("print"));
    QCOMPARE(rule.media.at(1), QString("screen"));
    QVERIFY(rule.styleRules.isEmpty());
}

void tst_QCssParser::page()
{
    QCss::Parser parser("@page :first/*comment to ignore*/{ }");
    QVERIFY(parser.testPage());
    QCss::PageRule rule;
    QVERIFY(parser.parsePage(&rule));
    QCOMPARE(rule.selector, QString("first"));
    QVERIFY(rule.declarations.isEmpty());
}

void tst_QCssParser::ruleset()
{
    {
        QCss::Parser parser("p/*foo*/{ }");
        QVERIFY(parser.testRuleset());
        QCss::StyleRule rule;
        QVERIFY(parser.parseRuleset(&rule));
        QCOMPARE(rule.selectors.count(), 1);
        QCOMPARE(rule.selectors.at(0).basicSelectors.count(), 1);
        QCOMPARE(rule.selectors.at(0).basicSelectors.at(0).elementName, QString("p"));
        QVERIFY(rule.declarations.isEmpty());
    }

    {
        QCss::Parser parser("p/*comment*/,div{ }");
        QVERIFY(parser.testRuleset());
        QCss::StyleRule rule;
        QVERIFY(parser.parseRuleset(&rule));
        QCOMPARE(rule.selectors.count(), 2);
        QCOMPARE(rule.selectors.at(0).basicSelectors.count(), 1);
        QCOMPARE(rule.selectors.at(0).basicSelectors.at(0).elementName, QString("p"));
        QCOMPARE(rule.selectors.at(1).basicSelectors.count(), 1);
        QCOMPARE(rule.selectors.at(1).basicSelectors.at(0).elementName, QString("div"));
        QVERIFY(rule.declarations.isEmpty());
    }

    {
        QCss::Parser parser(":before, :after { }");
        QVERIFY(parser.testRuleset());
        QCss::StyleRule rule;
        QVERIFY(parser.parseRuleset(&rule));
        QCOMPARE(rule.selectors.count(), 2);

        QCOMPARE(rule.selectors.at(0).basicSelectors.count(), 1);
        QCOMPARE(rule.selectors.at(0).basicSelectors.at(0).pseudos.count(), 1);
        QCOMPARE(rule.selectors.at(0).basicSelectors.at(0).pseudos.at(0).name, QString("before"));

        QCOMPARE(rule.selectors.at(1).basicSelectors.count(), 1);
        QCOMPARE(rule.selectors.at(1).basicSelectors.at(0).pseudos.count(), 1);
        QCOMPARE(rule.selectors.at(1).basicSelectors.at(0).pseudos.at(0).name, QString("after"));

        QVERIFY(rule.declarations.isEmpty());
    }

}

Q_DECLARE_METATYPE(QCss::Selector)

void tst_QCssParser::selector_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QCss::Selector>("expectedSelector");

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "p";
        basic.relationToNext = QCss::BasicSelector::MatchNextSelectorIfPreceeds;
        sel.basicSelectors << basic;

        basic = QCss::BasicSelector();
        basic.elementName = "div";
        sel.basicSelectors << basic;

        QTest::newRow("comment") << QString("p/* */+ div") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = QString();
        sel.basicSelectors << basic;

        QTest::newRow("any") << QString("*") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        sel.basicSelectors << basic;

        QTest::newRow("element") << QString("e") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        basic.relationToNext = QCss::BasicSelector::MatchNextSelectorIfAncestor;
        sel.basicSelectors << basic;

        basic.elementName = "f";
        basic.relationToNext = QCss::BasicSelector::NoRelation;
        sel.basicSelectors << basic;

        QTest::newRow("descendant") << QString("e f") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        basic.relationToNext = QCss::BasicSelector::MatchNextSelectorIfParent;
        sel.basicSelectors << basic;

        basic.elementName = "f";
        basic.relationToNext = QCss::BasicSelector::NoRelation;
        sel.basicSelectors << basic;

        QTest::newRow("parent") << QString("e > f") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        QCss::Pseudo pseudo;
        pseudo.name = "first-child";
        basic.pseudos.append(pseudo);
        sel.basicSelectors << basic;

        QTest::newRow("first-child") << QString("e:first-child") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        QCss::Pseudo pseudo;
        pseudo.name = "c";
        pseudo.function = "lang";
        basic.pseudos.append(pseudo);
        sel.basicSelectors << basic;

        QTest::newRow("lang") << QString("e:lang(c)") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        basic.relationToNext = QCss::BasicSelector::MatchNextSelectorIfPreceeds;
        sel.basicSelectors << basic;

        basic.elementName = "f";
        basic.relationToNext = QCss::BasicSelector::NoRelation;
        sel.basicSelectors << basic;

        QTest::newRow("precede") << QString("e + f") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        QCss::AttributeSelector attrSel;
        attrSel.name = "foo";
        basic.attributeSelectors << attrSel;
        sel.basicSelectors << basic;

        QTest::newRow("attr") << QString("e[foo]") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        QCss::AttributeSelector attrSel;
        attrSel.name = "foo";
        attrSel.value = "warning";
        attrSel.valueMatchCriterium = QCss::AttributeSelector::MatchEqual;
        basic.attributeSelectors << attrSel;
        sel.basicSelectors << basic;

        QTest::newRow("attr-equal") << QString("e[foo=\"warning\"]") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        QCss::AttributeSelector attrSel;
        attrSel.name = "foo";
        attrSel.value = "warning";
        attrSel.valueMatchCriterium = QCss::AttributeSelector::MatchContains;
        basic.attributeSelectors << attrSel;
        sel.basicSelectors << basic;

        QTest::newRow("attr-contains") << QString("e[foo~=\"warning\"]") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        QCss::AttributeSelector attrSel;
        attrSel.name = "lang";
        attrSel.value = "en";
        attrSel.valueMatchCriterium = QCss::AttributeSelector::MatchBeginsWith;
        basic.attributeSelectors << attrSel;
        sel.basicSelectors << basic;

        QTest::newRow("attr-contains") << QString("e[lang|=\"en\"]") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "div";

        QCss::AttributeSelector attrSel;
        attrSel.name = "class";
        attrSel.valueMatchCriterium = QCss::AttributeSelector::MatchContains;
        attrSel.value = "warning";
        basic.attributeSelectors.append(attrSel);

        attrSel.value = "foo";
        basic.attributeSelectors.append(attrSel);

        sel.basicSelectors << basic;

        QTest::newRow("class") << QString("div.warning.foo") << sel;
    }

    {
        QCss::Selector sel;
        QCss::BasicSelector basic;

        basic.elementName = "e";
        basic.ids << "myid";
        sel.basicSelectors << basic;

        QTest::newRow("id") << QString("e#myid") << sel;
    }
}

void tst_QCssParser::selector()
{
    QFETCH(QString, css);
    QFETCH(QCss::Selector, expectedSelector);

    QCss::Parser parser(css);
    QVERIFY(parser.testSelector());
    QCss::Selector selector;
    QVERIFY(parser.parseSelector(&selector));

    QCOMPARE(selector.basicSelectors.count(), expectedSelector.basicSelectors.count());
    for (int i = 0; i < selector.basicSelectors.count(); ++i) {
        const QCss::BasicSelector sel = selector.basicSelectors.at(i);
        const QCss::BasicSelector expectedSel = expectedSelector.basicSelectors.at(i);
        QCOMPARE(sel.elementName, expectedSel.elementName);
        QCOMPARE(int(sel.relationToNext), int(expectedSel.relationToNext));

        QCOMPARE(sel.pseudos.count(), expectedSel.pseudos.count());
        for (int i = 0; i < sel.pseudos.count(); ++i) {
            QCOMPARE(sel.pseudos.at(i).name, expectedSel.pseudos.at(i).name);
            QCOMPARE(sel.pseudos.at(i).function, expectedSel.pseudos.at(i).function);
        }

        QCOMPARE(sel.attributeSelectors.count(), expectedSel.attributeSelectors.count());
        for (int i = 0; i < sel.attributeSelectors.count(); ++i) {
            QCOMPARE(sel.attributeSelectors.at(i).name, expectedSel.attributeSelectors.at(i).name);
            QCOMPARE(sel.attributeSelectors.at(i).value, expectedSel.attributeSelectors.at(i).value);
            QCOMPARE(int(sel.attributeSelectors.at(i).valueMatchCriterium), int(expectedSel.attributeSelectors.at(i).valueMatchCriterium));
        }
    }
}

void tst_QCssParser::prio()
{
    {
        QCss::Parser parser("!important");
        QVERIFY(parser.testPrio());
    }
    {
        QCss::Parser parser("!impOrTAnt");
        QVERIFY(parser.testPrio());
    }
    {
        QCss::Parser parser("!\"important\"");
        QVERIFY(!parser.testPrio());
        QCOMPARE(parser.index, 0);
    }
    {
        QCss::Parser parser("!importbleh");
        QVERIFY(!parser.testPrio());
        QCOMPARE(parser.index, 0);
    }
}

void tst_QCssParser::escapes()
{
    QCss::Parser parser("\\hello");
    parser.test(QCss::IDENT);
    QCOMPARE(parser.lexem(), QString("hello"));
}

void tst_QCssParser::malformedDeclarations_data()
{
    QTest::addColumn<QString>("css");

    QTest::newRow("1") << QString("p { color:green }");
    QTest::newRow("2") << QString("p { color:green; color }  /* malformed declaration missing ':', value */");
    QTest::newRow("3") << QString("p { color:red;   color; color:green }  /* same with expected recovery */");
    QTest::newRow("4") << QString("p { color:green; color: } /* malformed declaration missing value */");
    QTest::newRow("5") << QString("p { color:red;   color:; color:green } /* same with expected recovery */");
    QTest::newRow("6") << QString("p { color:green; color{;color:maroon} } /* unexpected tokens { } */");
    QTest::newRow("7") << QString("p { color:red;   color{;color:maroon}; color:green } /* same with recovery */");
}

void tst_QCssParser::malformedDeclarations()
{
    QFETCH(QString, css);
    QCss::Parser parser(css);
    QVERIFY(parser.testRuleset());
    QCss::StyleRule rule;
    QVERIFY(parser.parseRuleset(&rule));

    QCOMPARE(rule.selectors.count(), 1);
    QCOMPARE(rule.selectors.at(0).basicSelectors.count(), 1);
    QCOMPARE(rule.selectors.at(0).basicSelectors.at(0).elementName, QString("p"));

    QVERIFY(rule.declarations.count() >= 1);
    QCOMPARE(int(rule.declarations.last().d->propertyId), int(QCss::Color));
    QCOMPARE(rule.declarations.last().d->values.count(), 1);
    QCOMPARE(int(rule.declarations.last().d->values.at(0).type), int(QCss::Value::Identifier));
    QCOMPARE(rule.declarations.last().d->values.at(0).variant.toString(), QString("green"));
}

void tst_QCssParser::invalidAtKeywords()
{
    QCss::Parser parser(""
    "@three-dee {"
    "  @background-lighting {"
    "    azimuth: 30deg;"
    "    elevation: 190deg;"
    "  }"
    "  h1 { color: red }"
    "}"
    "h1 { color: blue }");

    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QCOMPARE(sheet.styleRules.count() + sheet.nameIndex.count(), 1);
    QCss::StyleRule rule =  (!sheet.styleRules.isEmpty()) ?
            sheet.styleRules.at(0) : *sheet.nameIndex.begin();

    QCOMPARE(rule.selectors.count(), 1);
    QCOMPARE(rule.selectors.at(0).basicSelectors.count(), 1);
    QCOMPARE(rule.selectors.at(0).basicSelectors.at(0).elementName, QString("h1"));

    QCOMPARE(rule.declarations.count(), 1);
    QCOMPARE(int(rule.declarations.at(0).d->propertyId), int(QCss::Color));
    QCOMPARE(rule.declarations.at(0).d->values.count(), 1);
    QCOMPARE(int(rule.declarations.at(0).d->values.at(0).type), int(QCss::Value::Identifier));
    QCOMPARE(rule.declarations.at(0).d->values.at(0).variant.toString(), QString("blue"));
}


void tst_QCssParser::colorValue_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QColor>("expectedColor");

    QTest::newRow("identifier") << "color: black" << QColor("black");
    QTest::newRow("string") << "color: \"green\"" << QColor("green");
    QTest::newRow("hexcolor") << "color: #12af0e" << QColor(0x12, 0xaf, 0x0e);
    QTest::newRow("functional1") << "color: rgb(21, 45, 73)" << QColor(21, 45, 73);
    QTest::newRow("functional2") << "color: rgb(100%, 0%, 100%)" << QColor(0xff, 0, 0xff);
    QTest::newRow("rgba") << "color: rgba(10, 20, 30, 40)" << QColor(10, 20, 30, 40);
    QTest::newRow("rgbaf") << "color: rgba(10, 20, 30, 0.5)" << QColor(10, 20, 30, 127);
    QTest::newRow("rgb") << "color: rgb(10, 20, 30, 40)" << QColor(10, 20, 30, 40);
    QTest::newRow("hsl") << "color: hsv(10, 20, 30)" << QColor::fromHsv(10, 20, 30, 255);
    QTest::newRow("hsla") << "color: hsva(10, 20, 30, 40)" << QColor::fromHsv(10, 20, 30, 40);
    QTest::newRow("invalid1") << "color: rgb(why, does, it, always, rain, on, me)" << QColor();
    QTest::newRow("invalid2") << "color: rgba(i, meant, norway)" << QColor();
    QTest::newRow("role") << "color: palette(base)" << qApp->palette().color(QPalette::Base);
    QTest::newRow("role2") << "color: palette( window-text ) " << qApp->palette().color(QPalette::WindowText);
    QTest::newRow("transparent") << "color: transparent" << QColor(Qt::transparent);
}

void tst_QCssParser::colorValue()
{
    QFETCH(QString, css);
    QFETCH(QColor, expectedColor);

    QCss::Parser parser(css);
    QCss::Declaration decl;
    QVERIFY(parser.parseNextDeclaration(&decl));
    const QColor col = decl.colorValue();
    QVERIFY(expectedColor.isValid() == col.isValid());
    QCOMPARE(col, expectedColor);
}

class DomStyleSelector : public QCss::StyleSelector
{
public:
    inline DomStyleSelector(const QDomDocument &doc, const QCss::StyleSheet &sheet)
        : doc(doc)
    {
        styleSheets.append(sheet);
    }

    virtual QStringList nodeNames(NodePtr node) const { return QStringList(reinterpret_cast<QDomElement *>(node.ptr)->tagName()); }
    virtual QString attribute(NodePtr node, const QString &name) const { return reinterpret_cast<QDomElement *>(node.ptr)->attribute(name); }
    virtual bool hasAttribute(NodePtr node, const QString &name) const { return reinterpret_cast<QDomElement *>(node.ptr)->hasAttribute(name); }
    virtual bool hasAttributes(NodePtr node) const { return reinterpret_cast<QDomElement *>(node.ptr)->hasAttributes(); }

    virtual bool isNullNode(NodePtr node) const {
        return reinterpret_cast<QDomElement *>(node.ptr)->isNull();
    }
    virtual NodePtr parentNode(NodePtr node) const {
        NodePtr parent;
        parent.ptr = new QDomElement(reinterpret_cast<QDomElement *>(node.ptr)->parentNode().toElement());
        return parent;
    }
    virtual NodePtr duplicateNode(NodePtr node) const {
        NodePtr n;
        n.ptr = new QDomElement(*reinterpret_cast<QDomElement *>(node.ptr));
        return n;
    }
    virtual NodePtr previousSiblingNode(NodePtr node) const {
        NodePtr sibling;
        sibling.ptr = new QDomElement(reinterpret_cast<QDomElement *>(node.ptr)->previousSiblingElement());
        return sibling;
    }
    virtual void freeNode(NodePtr node) const {
        delete reinterpret_cast<QDomElement *>(node.ptr);
    }

private:
    QDomDocument doc;
};

Q_DECLARE_METATYPE(QDomDocument)

void tst_QCssParser::marginValue_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QString>("expectedMargin");

    QFont f;
    int ex = QFontMetrics(f).xHeight();
    int em = QFontMetrics(f).height();

    QTest::newRow("one value") << "margin: 1px" << "1 1 1 1";
    QTest::newRow("two values") << "margin: 1px 2px" << "1 2 1 2";
    QTest::newRow("three value") << "margin: 1px 2px 3px" << "1 2 3 2";
    QTest::newRow("four values") << "margin: 1px 2px 3px 4px" << "1 2 3 4";
    QTest::newRow("default px") << "margin: 1 2 3 4" << "1 2 3 4";
    QTest::newRow("no unit") << "margin: 1 2 3 4" << "1 2 3 4";
    QTest::newRow("em") << "margin: 1ex 2ex 3ex 4ex" << QString("%1 %2 %3 %4").arg(ex).arg(2*ex).arg(3*ex).arg(4*ex);
    QTest::newRow("ex") << "margin: 1 2em 3px 4ex" << QString("%1 %2 %3 %4").arg(1).arg(2*em).arg(3).arg(4*ex);

    f.setPointSize(20);
    f.setBold(true);
    ex = QFontMetrics(f).xHeight();
    em = QFontMetrics(f).height();
    QTest::newRow("em2") << "font: bold 20pt; margin: 1ex 2ex 3ex 4ex" << QString("%1 %2 %3 %4").arg(ex).arg(2*ex).arg(3*ex).arg(4*ex);
    QTest::newRow("ex2") << "margin: 1 2em 3px 4ex; font-size: 20pt; font-weight: bold;" << QString("%1 %2 %3 %4").arg(1).arg(2*em).arg(3).arg(4*ex);

    QTest::newRow("crap") << "margin: crap" << "0 0 0 0";
}

void tst_QCssParser::marginValue()
{
    QFETCH(QString, css);
    QFETCH(QString, expectedMargin);

    QDomDocument doc;
    QVERIFY(doc.setContent(QLatin1String("<!DOCTYPE test><test> <dummy/> </test>")));

    css.prepend("dummy {");
    css.append("}");

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    DomStyleSelector testSelector(doc, sheet);
    QDomElement e = doc.documentElement().firstChildElement();
    QCss::StyleSelector::NodePtr n;
    n.ptr = &e;
    QVector<QCss::StyleRule> rules = testSelector.styleRulesForNode(n);
    QVector<QCss::Declaration> decls = rules.at(0).declarations;
    QCss::ValueExtractor v(decls);

    {
    int m[4];
    int p[4];
    int spacing;
    v.extractBox(m, p, &spacing);
    QString str = QString("%1 %2 %3 %4").arg(m[0]).arg(m[1]).arg(m[2]).arg(m[3]);
    QCOMPARE(str, expectedMargin);
    }
}

void tst_QCssParser::styleSelector_data()
{
    QTest::addColumn<bool>("match");
    QTest::addColumn<QString>("selector");
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("elementToCheck");

    QTest::newRow("plain") << true << QString("p") << QString("<p />") << QString();
    QTest::newRow("noplain") << false << QString("bar") << QString("<p />") << QString();

    QTest::newRow("class") << true << QString(".foo") << QString("<p class=\"foo\" />") << QString();
    QTest::newRow("noclass") << false << QString(".bar") << QString("<p class=\"foo\" />") << QString();

    QTest::newRow("attrset") << true << QString("[justset]") << QString("<p justset=\"bar\" />") << QString();
    QTest::newRow("notattrset") << false << QString("[justset]") << QString("<p otherattribute=\"blub\" />") << QString();

    QTest::newRow("attrmatch") << true << QString("[foo=bar]") << QString("<p foo=\"bar\" />") << QString();
    QTest::newRow("noattrmatch") << false << QString("[foo=bar]") << QString("<p foo=\"xyz\" />") << QString();

    QTest::newRow("contains") << true << QString("[foo~=bar]") << QString("<p foo=\"baz bleh bar\" />") << QString();
    QTest::newRow("notcontains") << false << QString("[foo~=bar]") << QString("<p foo=\"test\" />") << QString();

    QTest::newRow("beingswith") << true << QString("[foo|=bar]") << QString("<p foo=\"bar-bleh\" />") << QString();
    QTest::newRow("notbeingswith") << false << QString("[foo|=bar]") << QString("<p foo=\"bleh-bar\" />") << QString();

    QTest::newRow("attr2") << true << QString("[bar=foo]") << QString("<p bleh=\"bar\" bar=\"foo\" />") << QString();

    QTest::newRow("universal1") << true << QString("*") << QString("<p />") << QString();

    QTest::newRow("universal3") << false << QString("*[foo=bar]") << QString("<p foo=\"bleh\" />") << QString();
    QTest::newRow("universal4") << true << QString("*[foo=bar]") << QString("<p foo=\"bar\" />") << QString();

    QTest::newRow("universal5") << false << QString("[foo=bar]") << QString("<p foo=\"bleh\" />") << QString();
    QTest::newRow("universal6") << true << QString("[foo=bar]") << QString("<p foo=\"bar\" />") << QString();

    QTest::newRow("universal7") << true << QString(".charfmt1") << QString("<p class=\"charfmt1\" />") << QString();

    QTest::newRow("id") << true << QString("#blub") << QString("<p id=\"blub\" />") << QString();
    QTest::newRow("noid") << false << QString("#blub") << QString("<p id=\"other\" />") << QString();

    QTest::newRow("childselector") << true << QString("parent > child")
                                   << QString("<parent><child /></parent>")
                                   << QString("parent/child");

    QTest::newRow("nochildselector2") << false << QString("parent > child")
                                   << QString("<child><parent /></child>")
                                   << QString("child/parent");

    QTest::newRow("nochildselector3") << false << QString("parent > child")
                                   << QString("<parent><intermediate><child /></intermediate></parent>")
                                   << QString("parent/intermediate/child");

    QTest::newRow("childselector2") << true << QString("parent[foo=bar] > child")
                                   << QString("<parent foo=\"bar\"><child /></parent>")
                                   << QString("parent/child");

    QTest::newRow("nochildselector4") << false << QString("parent[foo=bar] > child")
                                   << QString("<parent><child /></parent>")
                                   << QString("parent/child");

    QTest::newRow("nochildselector5") << false << QString("parent[foo=bar] > child")
                                   << QString("<parent foo=\"bar\"><parent><child /></parent></parent>")
                                   << QString("parent/parent/child");

    QTest::newRow("childselectors") << true << QString("grandparent > parent > child")
                                   << QString("<grandparent><parent><child /></parent></grandparent>")
                                   << QString("grandparent/parent/child");

    QTest::newRow("descendant") << true << QString("grandparent child")
                                   << QString("<grandparent><parent><child /></parent></grandparent>")
                                   << QString("grandparent/parent/child");

    QTest::newRow("nodescendant") << false << QString("grandparent child")
                                   << QString("<other><parent><child /></parent></other>")
                                   << QString("other/parent/child");

    QTest::newRow("descendant2") << true << QString("grandgrandparent grandparent child")
                                   << QString("<grandgrandparent><inbetween><grandparent><parent><child /></parent></grandparent></inbetween></grandgrandparent>")
                                   << QString("grandgrandparent/inbetween/grandparent/parent/child");

    QTest::newRow("combined") << true << QString("grandparent parent > child")
                              << QString("<grandparent><inbetween><parent><child /></parent></inbetween></grandparent>")
                              << QString("grandparent/inbetween/parent/child");

    QTest::newRow("combined2") << true << QString("grandparent > parent child")
                              << QString("<grandparent><parent><inbetween><child /></inbetween></parent></grandparent>")
                              << QString("grandparent/parent/inbetween/child");

    QTest::newRow("combined3") << true << QString("grandparent > parent child")
                              << QString("<grandparent><parent><inbetween><child /></inbetween></parent></grandparent>")
                              << QString("grandparent/parent/inbetween/child");

    QTest::newRow("nocombined") << false << QString("grandparent parent > child")
                              << QString("<inbetween><parent><child /></parent></inbetween>")
                              << QString("inbetween/parent/child");

    QTest::newRow("nocombined2") << false << QString("grandparent parent > child")
                              << QString("<parent><child /></parent>")
                              << QString("parent/child");

    QTest::newRow("previoussibling") << true << QString("p1 + p2")
                                     << QString("<p1 /><p2 />")
                                     << QString("p2");

    QTest::newRow("noprevioussibling") << false << QString("p2 + p1")
                                     << QString("<p1 /><p2 />")
                                     << QString("p2");

    QTest::newRow("ancestry_firstmismatch") << false << QString("parent child[foo=bar]")
                                            << QString("<parent><child /></parent>")
                                            << QString("parent/child");

    QTest::newRow("unknown-pseudo") << false << QString("p:enabled:foobar") << QString("<p/>") << QString();
}

void tst_QCssParser::styleSelector()
{
    QFETCH(bool, match);
    QFETCH(QString, selector);
    QFETCH(QString, xml);
    QFETCH(QString, elementToCheck);

    QString css = QString("%1 { background-color: green }").arg(selector);
    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QDomDocument doc;
    xml.prepend("<!DOCTYPE test><test>");
    xml.append("</test>");
    QVERIFY(doc.setContent(xml));

    DomStyleSelector testSelector(doc, sheet);

    QDomElement e = doc.documentElement();
    if (elementToCheck.isEmpty()) {
        e = e.firstChildElement();
    } else {
        QStringList path = elementToCheck.split(QLatin1Char('/'));
        do {
            e = e.namedItem(path.takeFirst()).toElement();
        } while (!path.isEmpty());
    }
    QVERIFY(!e.isNull());
    QCss::StyleSelector::NodePtr n;
    n.ptr = &e;
    QVector<QCss::Declaration> decls = testSelector.declarationsForNode(n);

    if (match) {
        QCOMPARE(decls.count(), 1);
        QCOMPARE(int(decls.at(0).d->propertyId), int(QCss::BackgroundColor));
        QCOMPARE(decls.at(0).d->values.count(), 1);
        QCOMPARE(int(decls.at(0).d->values.at(0).type), int(QCss::Value::Identifier));
        QCOMPARE(decls.at(0).d->values.at(0).variant.toString(), QString("green"));
    } else {
        QVERIFY(decls.isEmpty());
    }
}

void tst_QCssParser::specificity_data()
{
    QTest::addColumn<QString>("selector");
    QTest::addColumn<int>("specificity");

    QTest::newRow("universal") << QString("*") << 0;

    QTest::newRow("elements+pseudos1") << QString("foo") << 1;
    QTest::newRow("elements+pseudos2") << QString("foo *[blah]") << 1 + (1 * 0x10);

    // should strictly speaking be '2', but we don't support pseudo-elements yet,
    // only pseudo-classes
    QTest::newRow("elements+pseudos3") << QString("li:first-line") << 1 + (1 * 0x10);

    QTest::newRow("elements+pseudos4") << QString("ul li") << 2;
    QTest::newRow("elements+pseudos5") << QString("ul ol+li") << 3;
    QTest::newRow("elements+pseudos6") << QString("h1 + *[rel=up]") << 1 + (1 * 0x10);

    QTest::newRow("elements+pseudos7") << QString("ul ol li.red") << 3 + (1 * 0x10);
    QTest::newRow("elements+pseudos8") << QString("li.red.level") << 1 + (2 * 0x10);
    QTest::newRow("id") << QString("#x34y") << 1 * 0x100;
}

void tst_QCssParser::specificity()
{
    QFETCH(QString, selector);

    QString css = QString("%1 { }").arg(selector);
    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QCOMPARE(sheet.styleRules.count() + sheet.nameIndex.count() + sheet.idIndex.count() , 1);
    QCss::StyleRule rule =  (!sheet.styleRules.isEmpty()) ? sheet.styleRules.at(0)
                        :  (!sheet.nameIndex.isEmpty())  ? *sheet.nameIndex.begin()
                        :  *sheet.idIndex.begin();
    QCOMPARE(rule.selectors.count(), 1);
    QTEST(rule.selectors.at(0).specificity(), "specificity");
}

void tst_QCssParser::specificitySort_data()
{
    QTest::addColumn<QString>("firstSelector");
    QTest::addColumn<QString>("secondSelector");
    QTest::addColumn<QString>("xml");

    QTest::newRow("universal1") << QString("*") << QString("p") << QString("<p />");
    QTest::newRow("attr") << QString("p") << QString("p[foo=bar]") << QString("<p foo=\"bar\" />");
    QTest::newRow("id") << QString("p") << QString("#hey") << QString("<p id=\"hey\" />");
    QTest::newRow("id2") << QString("[id=hey]") << QString("#hey") << QString("<p id=\"hey\" />");
    QTest::newRow("class") << QString("p") << QString(".hey") << QString("<p class=\"hey\" />");
}

void tst_QCssParser::specificitySort()
{
    QFETCH(QString, firstSelector);
    QFETCH(QString, secondSelector);
    QFETCH(QString, xml);

    firstSelector.append(" { color: green; }");
    secondSelector.append(" { color: red; }");

    QDomDocument doc;
    xml.prepend("<!DOCTYPE test><test>");
    xml.append("</test>");
    QVERIFY(doc.setContent(xml));

    for (int i = 0; i < 2; ++i) {
        QString css;
        if (i == 0)
            css = firstSelector + secondSelector;
        else
            css = secondSelector + firstSelector;

        QCss::Parser parser(css);
        QCss::StyleSheet sheet;
        QVERIFY(parser.parse(&sheet));

        DomStyleSelector testSelector(doc, sheet);

        QDomElement e = doc.documentElement().firstChildElement();
        QCss::StyleSelector::NodePtr n;
        n.ptr = &e;
        QVector<QCss::Declaration> decls = testSelector.declarationsForNode(n);

        QCOMPARE(decls.count(), 2);

        QCOMPARE(int(decls.at(0).d->propertyId), int(QCss::Color));
        QCOMPARE(decls.at(0).d->values.count(), 1);
        QCOMPARE(int(decls.at(0).d->values.at(0).type), int(QCss::Value::Identifier));
        QCOMPARE(decls.at(0).d->values.at(0).variant.toString(), QString("green"));

        QCOMPARE(int(decls.at(1).d->propertyId), int(QCss::Color));
        QCOMPARE(decls.at(1).d->values.count(), 1);
        QCOMPARE(int(decls.at(1).d->values.at(0).type), int(QCss::Value::Identifier));
        QCOMPARE(decls.at(1).d->values.at(0).variant.toString(), QString("red"));
    }
}

void tst_QCssParser::rulesForNode_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("css");
    QTest::addColumn<quint64>("pseudoClass");
    QTest::addColumn<int>("declCount");
    QTest::addColumn<QString>("value0");
    QTest::addColumn<QString>("value1");

    QTest::newRow("universal1") << QString("<p/>") << QString("* { color: red }")
                                << (quint64)QCss::PseudoClass_Unspecified << 1 << "red" << "";

    QTest::newRow("basic") << QString("<p/>") << QString("p:enabled { color: red; bg:blue; }")
        << (quint64)QCss::PseudoClass_Enabled << 2 << "red" << "blue";

    QTest::newRow("single") << QString("<p/>")
        << QString("p:enabled { color: red; } *:hover { color: white }")
        << (quint64)QCss::PseudoClass_Hover << 1 << "white" << "";

    QTest::newRow("multisel") << QString("<p/>")
        << QString("p:enabled { color: red; } p:hover { color: gray } *:hover { color: white } ")
        << (quint64)QCss::PseudoClass_Hover << 2 << "white" << "gray";

    QTest::newRow("multisel2") << QString("<p/>")
        << QString("p:enabled { color: red; } p:hover:focus { color: gray } *:hover { color: white } ")
        << quint64(QCss::PseudoClass_Hover|QCss::PseudoClass_Focus) << 2 << "white" << "gray";

    QTest::newRow("multisel3-diffspec") << QString("<p/>")
        << QString("p:enabled { color: red; } p:hover:focus { color: gray } *:hover { color: white } ")
        << quint64(QCss::PseudoClass_Hover) << 1 << "white" << "";

    QTest::newRow("!-1") << QString("<p/>")
        << QString("p:checked:!hover { color: red; } p:checked:hover { color: gray } p:checked { color: white }")
        << quint64(QCss::PseudoClass_Hover|QCss::PseudoClass_Checked) << 2 << "white" << "gray";

    QTest::newRow("!-2") << QString("<p/>")
        << QString("p:checked:!hover:!pressed { color: red; } p:!checked:hover { color: gray } p:!focus { color: blue }")
        << quint64(QCss::PseudoClass_Focus) << 0 << "" << "";

    QTest::newRow("!-3") << QString("<p/>")
        << QString("p:checked:!hover:!pressed { color: red; } p:!checked:hover { color: gray } p:!focus { color: blue; }")
        << quint64(QCss::PseudoClass_Pressed) << 1 << "blue" << "";
}

void tst_QCssParser::rulesForNode()
{
    QFETCH(QString, xml);
    QFETCH(QString, css);
    QFETCH(quint64, pseudoClass);
    QFETCH(int, declCount);
    QFETCH(QString, value0);
    QFETCH(QString, value1);

    QDomDocument doc;
    xml.prepend("<!DOCTYPE test><test>");
    xml.append("</test>");
    QVERIFY(doc.setContent(xml));

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    DomStyleSelector testSelector(doc, sheet);
    QDomElement e = doc.documentElement().firstChildElement();
    QCss::StyleSelector::NodePtr n;
    n.ptr = &e;
    QVector<QCss::StyleRule> rules = testSelector.styleRulesForNode(n);

    QVector<QCss::Declaration> decls;
    for (int i = 0; i < rules.count(); i++) {
        const QCss::Selector &selector = rules.at(i).selectors.at(0);
        quint64 negated = 0;
        quint64 cssClass = selector.pseudoClass(&negated);
        if ((cssClass == QCss::PseudoClass_Unspecified)
            || ((((cssClass & pseudoClass) == cssClass)) && ((negated & pseudoClass) == 0)))
            decls += rules.at(i).declarations;
    }

    QVERIFY(decls.count() == declCount);

    if (declCount > 0)
        QCOMPARE(decls.at(0).d->values.at(0).variant.toString(), value0);
    if (declCount > 1)
        QCOMPARE(decls.at(1).d->values.at(0).variant.toString(), value1);
}

void tst_QCssParser::shorthandBackgroundProperty_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QBrush>("expectedBrush");
    QTest::addColumn<QString>("expectedImage");
    QTest::addColumn<int>("expectedRepeatValue");
    QTest::addColumn<int>("expectedAlignment");

    QTest::newRow("simple color") << "background: red" << QBrush(QColor("red")) << QString() << int(QCss::Repeat_XY) << int(Qt::AlignLeft | Qt::AlignTop);
    QTest::newRow("plain color") << "background-color: red" << QBrush(QColor("red")) << QString() << int(QCss::Repeat_XY) << int(Qt::AlignLeft | Qt::AlignTop);
    QTest::newRow("palette color") << "background-color: palette(mid)" << qApp->palette().mid() << QString() << int(QCss::Repeat_XY) << int(Qt::AlignLeft | Qt::AlignTop);
    QTest::newRow("multiple") << "background: url(chess.png) blue repeat-y" << QBrush(QColor("blue")) << QString("chess.png") << int(QCss::Repeat_Y) << int(Qt::AlignLeft | Qt::AlignTop);
    QTest::newRow("plain alignment") << "background-position: center" << QBrush() << QString() << int(QCss::Repeat_XY) << int(Qt::AlignCenter);
    QTest::newRow("plain alignment2") << "background-position: left top" << QBrush() << QString() << int(QCss::Repeat_XY) << int(Qt::AlignLeft | Qt::AlignTop);
    QTest::newRow("plain alignment3") << "background-position: left" << QBrush() << QString() << int(QCss::Repeat_XY) << int(Qt::AlignLeft | Qt::AlignVCenter);
    QTest::newRow("multi") << "background: left url(blah.png) repeat-x" << QBrush() << QString("blah.png") << int(QCss::Repeat_X) << int(Qt::AlignLeft | Qt::AlignVCenter);
    QTest::newRow("multi2") << "background: url(blah.png) repeat-x top" << QBrush() << QString("blah.png") << int(QCss::Repeat_X) << int(Qt::AlignTop | Qt::AlignHCenter);
    QTest::newRow("multi3") << "background: url(blah.png) top right" << QBrush() << QString("blah.png") << int(QCss::Repeat_XY) << int(Qt::AlignTop | Qt::AlignRight);
}

void tst_QCssParser::shorthandBackgroundProperty()
{
    QFETCH(QString, css);

    QDomDocument doc;
    QVERIFY(doc.setContent(QLatin1String("<!DOCTYPE test><test> <dummy/> </test>")));

    css.prepend("dummy {");
    css.append("}");

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    DomStyleSelector testSelector(doc, sheet);
    QDomElement e = doc.documentElement().firstChildElement();
    QCss::StyleSelector::NodePtr n;
    n.ptr = &e;
    QVector<QCss::StyleRule> rules = testSelector.styleRulesForNode(n);
    QVector<QCss::Declaration> decls = rules.at(0).declarations;
    QCss::ValueExtractor v(decls);

    QBrush brush;
    QString image;
    QCss::Repeat repeat = QCss::Repeat_XY;
    Qt::Alignment alignment = Qt::AlignTop | Qt::AlignLeft;
    QCss::Origin origin = QCss::Origin_Padding;
    QCss::Attachment attachment;
    QCss::Origin ignoredOrigin;
    v.extractBackground(&brush, &image, &repeat, &alignment, &origin, &attachment, &ignoredOrigin);

    QFETCH(QBrush, expectedBrush);
    QVERIFY(expectedBrush.color() == brush.color());

    QTEST(image, "expectedImage");
    QTEST(int(repeat), "expectedRepeatValue");
    QTEST(int(alignment), "expectedAlignment");

    //QTBUG-9674  : a second evaluation should give the same results
    QVERIFY(v.extractBackground(&brush, &image, &repeat, &alignment, &origin, &attachment, &ignoredOrigin));
    QVERIFY(expectedBrush.color() == brush.color());
    QTEST(image, "expectedImage");
    QTEST(int(repeat), "expectedRepeatValue");
    QTEST(int(alignment), "expectedAlignment");
}

void tst_QCssParser::pseudoElement_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QString>("pseudoElement");
    QTest::addColumn<int>("declCount");

    // QComboBox::dropDown { border-image: blah; }
    QTest::newRow("no pseudo-elements") << QString("dummy:hover { color: red }") << "" << 1;
    QTest::newRow("no pseudo-elements") << QString("dummy:hover { color: red }") << "pe" << 0;

    QTest::newRow("1 pseudo-element (1)") << QString("dummy::pe:hover { color: red }") << "pe" << 1;
    QTest::newRow("1 pseudo-element (2)") << QString("dummy::pe:hover { color: red }") << "x" << 0;
    QTest::newRow("1 pseudo-element (2)") << QString("whatever::pe:hover { color: red }") << "pe" << 0;

    QTest::newRow("1 pseudo-element (3)")
        << QString("dummy { color: white; } dummy::pe:hover { color: red }") << "x" << 0;
    QTest::newRow("1 pseudo-element (4)")
        << QString("dummy { color: white; } dummy::pe:hover { color: red } dummy { x:y }") << "" << 2;
    QTest::newRow("1 pseudo-element (5)")
        << QString("dummy { color: white; } dummy::pe:hover { color: red }") << "pe" << 1;
    QTest::newRow("1 pseudo-element (6)")
      << QString("dummy { color: white; } dummy::pe:hover { color: red } dummy::pe:checked { x: y} ") << "pe" << 2;

    QTest::newRow("2 pseudo-elements (1)")
      << QString("dummy { color: white; } dummy::pe1:hover { color: red } dummy::pe2:checked { x: y} ")
      << "" << 1;
    QTest::newRow("2 pseudo-elements (1)")
      << QString("dummy { color: white; } dummy::pe1:hover { color: red } dummy::pe2:checked { x: y} ")
      << "pe1" << 1;
    QTest::newRow("2 pseudo-elements (2)")
      << QString("dummy { color: white; } dummy::pe1:hover { color: red } dummy::pe2:checked { x: y} ")
      << "pe2" << 1;
}

void tst_QCssParser::pseudoElement()
{
    QFETCH(QString, css);
    QFETCH(QString, pseudoElement);
    QFETCH(int, declCount);

    QDomDocument doc;
    QVERIFY(doc.setContent(QLatin1String("<!DOCTYPE test><test> <dummy/> </test>")));

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    DomStyleSelector testSelector(doc, sheet);
    QDomElement e = doc.documentElement().firstChildElement();
    QCss::StyleSelector::NodePtr n;
    n.ptr = &e;
    QVector<QCss::StyleRule> rules = testSelector.styleRulesForNode(n);
    QVector<QCss::Declaration> decls;
    for (int i = 0; i < rules.count(); i++) {
        const QCss::Selector& selector = rules.at(i).selectors.at(0);
        if (pseudoElement.compare(selector.pseudoElement(), Qt::CaseInsensitive) != 0)
            continue;
        decls += rules.at(i).declarations;

    }
    QVERIFY(decls.count() == declCount);
}

void tst_QCssParser::gradient_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QPointF>("start");
    QTest::addColumn<QPointF>("finalStop");
    QTest::addColumn<int>("spread");
    QTest::addColumn<qreal>("stop0");
    QTest::addColumn<QColor>("color0");
    QTest::addColumn<qreal>("stop1");
    QTest::addColumn<QColor>("color1");

    QTest::newRow("color-string") <<
     "selection-background-color: qlineargradient(x1:1, y1:2, x2:3, y2:4, "
         "stop:0.2 red, stop:0.5 green)" << "linear" << QPointF(1, 2) << QPointF(3, 4)
                                  << 0 << qreal(0.2) << QColor("red") << qreal(0.5) << QColor("green");

    QTest::newRow("color-#") <<
     "selection-background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
         "spread: reflect, stop:0.2 #123, stop:0.5 #456)" << "linear" << QPointF(0, 0) << QPointF(0, 1)
                             << 1 << qreal(0.2) << QColor("#123") << qreal(0.5) << QColor("#456");

    QTest::newRow("color-rgb") <<
     "selection-background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
         "spread: reflect, stop:0.2 rgb(1, 2, 3), stop:0.5 rgba(1, 2, 3, 4))" << "linear" << QPointF(0, 0) << QPointF(0, 1)
                             << 1 << qreal(0.2) << QColor(1, 2, 3) << qreal(0.5) << QColor(1, 2, 3, 4);

    QTest::newRow("color-spaces") <<
     "selection-background-color: qlineargradient(x1: 0, y1 :0,x2:0, y2 : 1 , "
         "spread: reflect, stop:0.2 rgb(1, 2, 3), stop: 0.5   rgba(1, 2, 3, 4))" << "linear" << QPointF(0, 0) << QPointF(0, 1)
                             << 1 << qreal(0.2) << QColor(1, 2, 3) << qreal(0.5) << QColor(1, 2, 3, 4);

    QTest::newRow("conical gradient") <<
     "selection-background-color: qconicalgradient(cx: 4, cy : 2, angle: 23, "
         "spread: repeat, stop:0.2 rgb(1, 2, 3), stop:0.5 rgba(1, 2, 3, 4))" << "conical" << QPointF(4, 2) << QPointF()
                             << 2 << qreal(0.2) << QColor(1, 2, 3) << qreal(0.5) << QColor(1, 2, 3, 4);

    /* won't pass: stop values are expected to be sorted
     QTest::newRow("unsorted-stop") <<
     "selection-background: lineargradient(x1:0, y1:0, x2:0, y2:1, "
         "stop:0.5 green, stop:0.2 red)" << QPointF(0, 0) << QPointF(0, 1)
         0 << 0.2 << QColor("red") << 0.5 << QColor("green");
    */
}

void tst_QCssParser::gradient()
{
    QFETCH(QString, css);
    QFETCH(QString, type);
    QFETCH(QPointF, finalStop);
    QFETCH(QPointF, start);
    QFETCH(int, spread);
    QFETCH(qreal, stop0); QFETCH(QColor, color0);
    QFETCH(qreal, stop1); QFETCH(QColor, color1);

    QDomDocument doc;
    QVERIFY(doc.setContent(QLatin1String("<!DOCTYPE test><test> <dummy/> </test>")));

    css.prepend("dummy {");
    css.append("}");

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    DomStyleSelector testSelector(doc, sheet);
    QDomElement e = doc.documentElement().firstChildElement();
    QCss::StyleSelector::NodePtr n;
    n.ptr = &e;
    QVector<QCss::StyleRule> rules = testSelector.styleRulesForNode(n);
    QVector<QCss::Declaration> decls = rules.at(0).declarations;
    QCss::ValueExtractor ve(decls);
    QBrush fg, sfg;
    QBrush sbg, abg;
    QVERIFY(ve.extractPalette(&fg, &sfg, &sbg, &abg));
    if (type == "linear") {
        QVERIFY(sbg.style() == Qt::LinearGradientPattern);
        const QLinearGradient *lg = static_cast<const QLinearGradient *>(sbg.gradient());
        QCOMPARE(lg->start(), start);
        QCOMPARE(lg->finalStop(), finalStop);
    } else if (type == "conical") {
        QVERIFY(sbg.style() == Qt::ConicalGradientPattern);
        const QConicalGradient *cg = static_cast<const QConicalGradient *>(sbg.gradient());
        QCOMPARE(cg->center(), start);
    }
    const QGradient *g = sbg.gradient();
    QCOMPARE(g->spread(), QGradient::Spread(spread));
    QVERIFY(g->stops().at(0).first == stop0);
    QVERIFY(g->stops().at(0).second == color0);
    QVERIFY(g->stops().at(1).first == stop1);
    QVERIFY(g->stops().at(1).second == color1);
}

void tst_QCssParser::extractFontFamily_data()
{
    if (QFontInfo(QFont("Times New Roman")).family() != "Times New Roman")
        QSKIP("'Times New Roman' font not found");

    QTest::addColumn<QString>("css");
    QTest::addColumn<QString>("expectedFamily");

    QTest::newRow("quoted-family-name") << "font-family: 'Times New Roman'" << QString("Times New Roman");
    QTest::newRow("unquoted-family-name") << "font-family: Times New Roman" << QString("Times New Roman");
    QTest::newRow("unquoted-family-name2") << "font-family: Times        New     Roman" << QString("Times New Roman");
    QTest::newRow("multiple") << "font-family: Times New Roman  , foobar, 'baz'" << QString("Times New Roman");
    QTest::newRow("multiple2") << "font-family: invalid,  Times New   Roman " << QString("Times New Roman");
    QTest::newRow("invalid") << "font-family: invalid" << QFontInfo(QFont("invalid font")).family();
    QTest::newRow("shorthand") << "font: 12pt Times New Roman" << QString("Times New Roman");
    QTest::newRow("shorthand multiple quote") << "font: 12pt invalid, \"Times New Roman\" " << QString("Times New Roman");
    QTest::newRow("shorthand multiple") << "font: 12pt invalid, Times New Roman " << QString("Times New Roman");
    QTest::newRow("invalid spaces") << "font-family: invalid spaces, Times New Roman " << QString("Times New Roman");
    QTest::newRow("invalid spaces quotes") << "font-family: 'invalid spaces', 'Times New Roman' " << QString("Times New Roman");
}


void tst_QCssParser::extractFontFamily()
{
    QFETCH(QString, css);
    css.prepend("dummy {");
    css.append("}");

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QCOMPARE(sheet.styleRules.count() + sheet.nameIndex.count(), 1);
    QCss::StyleRule rule =  (!sheet.styleRules.isEmpty()) ?
            sheet.styleRules.at(0) : *sheet.nameIndex.begin();

    const QVector<QCss::Declaration> decls = rule.declarations;
    QVERIFY(!decls.isEmpty());
    QCss::ValueExtractor extractor(decls);

    int adjustment = 0;
    QFont fnt;
    extractor.extractFont(&fnt, &adjustment);
    QFontInfo info(fnt);

    QTEST(info.family(), "expectedFamily");
}

void tst_QCssParser::extractBorder_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<int>("expectedTopWidth");
    QTest::addColumn<int>("expectedTopStyle");
    QTest::addColumn<QColor>("expectedTopColor");

    QTest::newRow("all values") << "border: 2px solid green" << 2 << (int)QCss::BorderStyle_Solid << QColor("green");
    QTest::newRow("palette") << "border: 2px solid palette(highlight)" << 2 << (int)QCss::BorderStyle_Solid << qApp->palette().color(QPalette::Highlight);
    QTest::newRow("just width") << "border: 2px" << 2 << (int)QCss::BorderStyle_None << QColor();
    QTest::newRow("just style") << "border: solid" << 0 << (int)QCss::BorderStyle_Solid << QColor();
    QTest::newRow("just color") << "border: green" << 0 << (int)QCss::BorderStyle_None << QColor("green");
    QTest::newRow("width+style") << "border: 2px solid" << 2 << (int)QCss::BorderStyle_Solid << QColor();
    QTest::newRow("style+color") << "border: solid green" << 0 << (int)QCss::BorderStyle_Solid << QColor("green");
    QTest::newRow("width+color") << "border: 3px green" << 3 << (int)QCss::BorderStyle_None << QColor("green");
    QTest::newRow("groove style") << "border: groove" << 0 << (int)QCss::BorderStyle_Groove << QColor();
    QTest::newRow("ridge style") << "border: ridge" << 0 << (int)QCss::BorderStyle_Ridge << QColor();
    QTest::newRow("double style") << "border: double" << 0 << (int)QCss::BorderStyle_Double << QColor();
    QTest::newRow("inset style") << "border: inset" << 0 << (int)QCss::BorderStyle_Inset << QColor();
    QTest::newRow("outset style") << "border: outset" << 0 << (int)QCss::BorderStyle_Outset << QColor();
    QTest::newRow("dashed style") << "border: dashed" << 0 << (int)QCss::BorderStyle_Dashed << QColor();
    QTest::newRow("dotted style") << "border: dotted" << 0 << (int)QCss::BorderStyle_Dotted << QColor();
    QTest::newRow("dot-dash style") << "border: dot-dash" << 0 << (int)QCss::BorderStyle_DotDash << QColor();
    QTest::newRow("dot-dot-dash style") << "border: dot-dot-dash" << 0 << (int)QCss::BorderStyle_DotDotDash << QColor();

    QTest::newRow("top-width+color") << "border-top: 3px green" << 3 << (int)QCss::BorderStyle_None << QColor("green");
}

void tst_QCssParser::extractBorder()
{
    QFETCH(QString, css);
    QFETCH(int, expectedTopWidth);
    QFETCH(int, expectedTopStyle);
    QFETCH(QColor, expectedTopColor);

    css.prepend("dummy {");
    css.append("}");

    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QCOMPARE(sheet.styleRules.count() + sheet.nameIndex.count(), 1);
    QCss::StyleRule rule =  (!sheet.styleRules.isEmpty()) ?
            sheet.styleRules.at(0) : *sheet.nameIndex.begin();
    const QVector<QCss::Declaration> decls = rule.declarations;
    QVERIFY(!decls.isEmpty());
    QCss::ValueExtractor extractor(decls);

    int widths[4];
    QBrush colors[4];
    QCss::BorderStyle styles[4];
    QSize radii[4];

    extractor.extractBorder(widths, colors, styles, radii);
    QVERIFY(widths[QCss::TopEdge] == expectedTopWidth);
    QVERIFY(styles[QCss::TopEdge] == expectedTopStyle);
    QVERIFY(colors[QCss::TopEdge] == expectedTopColor);

    //QTBUG-9674  : a second evaluation should give the same results
    QVERIFY(extractor.extractBorder(widths, colors, styles, radii));
    QVERIFY(widths[QCss::TopEdge] == expectedTopWidth);
    QVERIFY(styles[QCss::TopEdge] == expectedTopStyle);
    QVERIFY(colors[QCss::TopEdge] == expectedTopColor);
}

void tst_QCssParser::noTextDecoration()
{
    QCss::Parser parser("dummy { text-decoration: none; }");
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QCOMPARE(sheet.styleRules.count() + sheet.nameIndex.count(), 1);
    QCss::StyleRule rule =  (!sheet.styleRules.isEmpty()) ?
            sheet.styleRules.at(0) : *sheet.nameIndex.begin();
    const QVector<QCss::Declaration> decls = rule.declarations;
    QVERIFY(!decls.isEmpty());
    QCss::ValueExtractor extractor(decls);

    int adjustment = 0;
    QFont f;
    f.setUnderline(true);
    f.setOverline(true);
    f.setStrikeOut(true);
    QVERIFY(extractor.extractFont(&f, &adjustment));

    QVERIFY(!f.underline());
    QVERIFY(!f.overline());
    QVERIFY(!f.strikeOut());
}

void tst_QCssParser::quotedAndUnquotedIdentifiers()
{
    QCss::Parser parser("foo { font-style: \"italic\"; font-weight: bold }");
    QCss::StyleSheet sheet;
    QVERIFY(parser.parse(&sheet));

    QCOMPARE(sheet.styleRules.count() + sheet.nameIndex.count(), 1);
    QCss::StyleRule rule = (!sheet.styleRules.isEmpty()) ?
           sheet.styleRules.at(0) : *sheet.nameIndex.begin();
    const QVector<QCss::Declaration> decls = rule.declarations;
    QCOMPARE(decls.size(), 2);

    QCOMPARE(decls.at(0).d->values.first().type, QCss::Value::String);
    QCOMPARE(decls.at(0).d->property, QLatin1String("font-style"));
    QCOMPARE(decls.at(0).d->values.first().toString(), QLatin1String("italic"));

    QCOMPARE(decls.at(1).d->values.first().type, QCss::Value::KnownIdentifier);
    QCOMPARE(decls.at(1).d->property, QLatin1String("font-weight"));
    QCOMPARE(decls.at(1).d->values.first().toString(), QLatin1String("bold"));
}

QTEST_MAIN(tst_QCssParser)
#include "tst_qcssparser.moc"

