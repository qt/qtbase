/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextList>
#include <QTextTable>
#include <QBuffer>
#include <QDebug>

#include <private/qtextmarkdownimporter_p.h>

// #define DEBUG_WRITE_HTML

Q_LOGGING_CATEGORY(lcTests, "qt.text.tests")

static const QChar LineBreak = QChar(0x2028);
static const QChar Tab = QLatin1Char('\t');
static const QChar Space = QLatin1Char(' ');
static const QChar Period = QLatin1Char('.');

class tst_QTextMarkdownImporter : public QObject
{
    Q_OBJECT

private slots:
    void headingBulletsContinuations();
    void thematicBreaks();
    void lists_data();
    void lists();
    void avoidBlankLineAtBeginning_data();
    void avoidBlankLineAtBeginning();
    void pathological_data();
    void pathological();
};

void tst_QTextMarkdownImporter::headingBulletsContinuations()
{
    const QStringList expectedBlocks = QStringList() <<
        "heading" <<
        "bullet 1 continuation line 1, indented via tab" <<
        "bullet 2 continuation line 2, indented via 4 spaces" <<
        "bullet 3" <<
        "continuation paragraph 3, indented via tab" <<
        "bullet 3.1" <<
        "continuation paragraph 3.1, indented via 4 spaces" <<
        "bullet 3.2 continuation line, indented via 2 tabs" <<
        "bullet 4" <<
        "continuation paragraph 4, indented via 4 spaces and continuing onto another line too" <<
        "bullet 5" <<
        // indenting by only 2 spaces is perhaps non-standard but currently is OK
        "continuation paragraph 5, indented via 2 spaces and continuing onto another line too" <<
        "bullet 6" <<
        "plain old paragraph at the end";

    QFile f(QFINDTESTDATA("data/headingBulletsContinuations.md"));
    QVERIFY(f.open(QFile::ReadOnly | QIODevice::Text));
    QString md = QString::fromUtf8(f.readAll());
    f.close();

    QTextDocument doc;
    QTextMarkdownImporter(QTextMarkdownImporter::DialectGitHub).import(&doc, md);
    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    QTextFrame *currentFrame = iterator.currentFrame();
    QStringList::const_iterator expectedIt = expectedBlocks.constBegin();
    int i = 0;
    while (!iterator.atEnd()) {
        // There are no child frames
        QCOMPARE(iterator.currentFrame(), currentFrame);
        // Check whether we got the right child block
        QTextBlock block = iterator.currentBlock();
        QCOMPARE(block.text().contains(LineBreak), false);
        QCOMPARE(block.text().contains(Tab), false);
        QVERIFY(!block.text().startsWith(Space));
        int expectedIndentation = 0;
        if (block.text().contains(QLatin1String("continuation paragraph")))
            expectedIndentation = (block.text().contains(Period) ? 2 : 1);
        qCDebug(lcTests) << i << "child block" << block.text() << "indentation" << block.blockFormat().indent();
        QVERIFY(expectedIt != expectedBlocks.constEnd());
        QCOMPARE(block.text(), *expectedIt);
        if (i > 2)
            QCOMPARE(block.blockFormat().indent(), expectedIndentation);
        ++iterator;
        ++expectedIt;
        ++i;
    }
    QCOMPARE(expectedIt, expectedBlocks.constEnd());

#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/headingBulletsContinuations.html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif
}

void tst_QTextMarkdownImporter::thematicBreaks()
{
    int horizontalRuleCount = 0;
    int textLinesCount = 0;

    QFile f(QFINDTESTDATA("data/thematicBreaks.md"));
    QVERIFY(f.open(QFile::ReadOnly | QIODevice::Text));
    QString md = QString::fromUtf8(f.readAll());
    f.close();

    QTextDocument doc;
    QTextMarkdownImporter(QTextMarkdownImporter::DialectGitHub).import(&doc, md);
    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    QTextFrame *currentFrame = iterator.currentFrame();
    int i = 0;
    while (!iterator.atEnd()) {
        // There are no child frames
        QCOMPARE(iterator.currentFrame(), currentFrame);
        // Check whether the block is text or a horizontal rule
        QTextBlock block = iterator.currentBlock();
        if (block.blockFormat().hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth))
            ++horizontalRuleCount;
        else if (!block.text().isEmpty())
            ++textLinesCount;
        qCDebug(lcTests) << i << (block.blockFormat().hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth) ? QLatin1String("- - -") : block.text());
        ++iterator;
        ++i;
    }
    QCOMPARE(horizontalRuleCount, 5);
    QCOMPARE(textLinesCount, 9);

#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/thematicBreaks.html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif
}

void tst_QTextMarkdownImporter::lists_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expectedItemCount");
    QTest::addColumn<bool>("expectedEmptyItems");
    QTest::addColumn<QString>("rewrite");

    // Some of these cases show odd behavior, which is subject to change
    // as the importer and the writer are tweaked to fix bugs over time.
    QTest::newRow("dot newline") << ".\n" << 0 << true << ".\n\n";
    QTest::newRow("number dot newline") << "1.\n" << 1 << true << "1.  \n";
    QTest::newRow("star newline") << "*\n" << 1 << true << "* \n";
    QTest::newRow("hyphen newline") << "-\n" << 1 << true << "- \n";
    QTest::newRow("hyphen space newline") << "- \n" << 1 << true << "- \n";
    QTest::newRow("hyphen space letter newline") << "- a\n" << 1 << false << "- a\n";
    QTest::newRow("hyphen nbsp newline") <<
        QString::fromUtf8("-\u00A0\n") << 0 << true << "-\u00A0\n\n";
    QTest::newRow("nested empty lists") << "*\n  *\n  *\n" << 1 << true << "  * \n";
    QTest::newRow("list nested in empty list") << "-\n  * a\n" << 2 << false << "- \n  * a\n";
    QTest::newRow("lists nested in empty lists")
            << "-\n  * a\n  * b\n- c\n  *\n    + d\n" << 5 << false
            << "- \n  * a\n  * b\n- c *\n  + d\n";
    QTest::newRow("numeric lists nested in empty lists")
            << "- \n    1.  a\n    2.  b\n- c\n  1.\n       + d\n" << 4 << false
            << "- \n    1.  a\n    2.  b\n- c 1. + d\n";
}

void tst_QTextMarkdownImporter::lists()
{
    QFETCH(QString, input);
    QFETCH(int, expectedItemCount);
    QFETCH(bool, expectedEmptyItems);
    QFETCH(QString, rewrite);

    QTextDocument doc;
    doc.setMarkdown(input); // QTBUG-78870 : don't crash
    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    QTextFrame *currentFrame = iterator.currentFrame();
    int i = 0;
    int itemCount = 0;
    bool emptyItems = true;
    while (!iterator.atEnd()) {
        // There are no child frames
        QCOMPARE(iterator.currentFrame(), currentFrame);
        // Check whether the block is text or a horizontal rule
        QTextBlock block = iterator.currentBlock();
        if (block.textList()) {
            ++itemCount;
            if (!block.text().isEmpty())
                emptyItems = false;
        }
        qCDebug(lcTests, "%d %s%s", i,
                (block.textList() ? "<li>" : "<p>"), qPrintable(block.text()));
        ++iterator;
        ++i;
    }
    QCOMPARE(itemCount, expectedItemCount);
    QCOMPARE(emptyItems, expectedEmptyItems);
    QCOMPARE(doc.toMarkdown(), rewrite);
}

void tst_QTextMarkdownImporter::avoidBlankLineAtBeginning_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expectedNumberOfParagraphs");

    QTest::newRow("Text block") << QString("Markdown text") << 1;
    QTest::newRow("Headline") << QString("Markdown text\n============") << 1;
    QTest::newRow("Code block") << QString("    Markdown text") << 2;
    QTest::newRow("Unordered list") << QString("* Markdown text") << 1;
    QTest::newRow("Ordered list") << QString("1. Markdown text") << 1;
    QTest::newRow("Blockquote") << QString("> Markdown text") << 1;
}

void tst_QTextMarkdownImporter::avoidBlankLineAtBeginning() // QTBUG-81060
{
    QFETCH(QString, input);
    QFETCH(int, expectedNumberOfParagraphs);

    QTextDocument doc;
    QTextMarkdownImporter(QTextMarkdownImporter::DialectGitHub).import(&doc, input);
    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    int i = 0;
    while (!iterator.atEnd()) {
        QTextBlock block = iterator.currentBlock();
        // Make sure there is no empty paragraph at the beginning of the document
        if (i == 0)
            QVERIFY(!block.text().isEmpty());
        ++iterator;
        ++i;
    }
    QCOMPARE(i, expectedNumberOfParagraphs);
}

void tst_QTextMarkdownImporter::pathological_data()
{
    QTest::addColumn<QString>("warning");
    QTest::newRow("fuzz20450") << "attempted to insert into a list that no longer exists";
    QTest::newRow("fuzz20580") << "";
}

void tst_QTextMarkdownImporter::pathological() // avoid crashing on crazy input
{
    QFETCH(QString, warning);
    QString filename = QLatin1String("data/") + QTest::currentDataTag() + QLatin1String(".md");
    QFile f(QFINDTESTDATA(filename));
    QVERIFY(f.open(QFile::ReadOnly));
#ifdef QT_NO_DEBUG
    Q_UNUSED(warning)
#else
    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());
#endif
    QTextDocument().setMarkdown(f.readAll());
}

QTEST_MAIN(tst_QTextMarkdownImporter)
#include "tst_qtextmarkdownimporter.moc"
