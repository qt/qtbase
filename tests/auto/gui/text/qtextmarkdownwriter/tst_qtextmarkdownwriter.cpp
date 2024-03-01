// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextList>
#include <QTextTable>
#include <QBuffer>
#include <QDebug>
#include <QFontInfo>
#include <QLoggingCategory>

#include <private/qtextmarkdownwriter_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.text.tests")

// #define DEBUG_WRITE_OUTPUT

class tst_QTextMarkdownWriter : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();

private slots:
    void testWriteParagraph_data();
    void testWriteParagraph();
    void testWriteList();
    void testWriteEmptyList();
    void testWriteCheckboxListItemEndingWithCode();
    void testWriteNestedBulletLists_data();
    void testWriteNestedBulletLists();
    void testWriteNestedNumericLists();
    void testWriteNumericListWithStart();
    void testWriteTable();
    void frontMatter();
    void charFormatWrapping_data();
    void charFormatWrapping();
    void charFormat_data();
    void charFormat();
    void rewriteDocument_data();
    void rewriteDocument();
    void fromHtml_data();
    void fromHtml();
    void fromPlainTextAndBack_data();
    void fromPlainTextAndBack();
    void escapeSpecialCharacters_data();
    void escapeSpecialCharacters();

private:
    bool isMainFontFixed();
    bool isFixedFontProportional();
    QString documentToUnixMarkdown();

private:
    QTextDocument *document;
    QFont m_monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QFont m_defaultFont;
};

void tst_QTextMarkdownWriter::init()
{
    document = new QTextDocument();
}

void tst_QTextMarkdownWriter::cleanup()
{
    delete document;
}

bool tst_QTextMarkdownWriter::isMainFontFixed()
{
    bool ret = QFontInfo(QGuiApplication::font()).fixedPitch();
    if (ret) {
        qCWarning(lcTests) << "QFontDatabase::GeneralFont is monospaced: markdown writing is likely to use too many backticks"
                           << QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    }
    return ret;
}

bool tst_QTextMarkdownWriter::isFixedFontProportional()
{
    bool ret = !QFontInfo(QFontDatabase::systemFont(QFontDatabase::FixedFont)).fixedPitch();
    if (ret) {
        qCWarning(lcTests) << "QFontDatabase::FixedFont is NOT monospaced: markdown writing is likely to use too few backticks"
                           << QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
    return ret;
}

QString tst_QTextMarkdownWriter::documentToUnixMarkdown()
{
    QString ret;
    QTextStream ts(&ret, QIODevice::WriteOnly);
    QTextMarkdownWriter writer(ts, QTextDocument::MarkdownDialectGitHub);
    writer.writeAll(document);
    return ret;
}

void tst_QTextMarkdownWriter::testWriteParagraph_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("empty") << "" <<
        "";
    QTest::newRow("spaces") << "foobar   word" <<
        "foobar   word\n\n";
    QTest::newRow("starting spaces") << "  starting spaces" <<
        "  starting spaces\n\n";
    QTest::newRow("trailing spaces") << "trailing spaces  " <<
        "trailing spaces  \n\n";
    QTest::newRow("tab") << "word\ttab x" <<
        "word\ttab x\n\n";
    QTest::newRow("tab2") << "word\t\ttab\tx" <<
        "word\t\ttab\tx\n\n";
    QTest::newRow("misc") << "foobar   word\ttab x" <<
        "foobar   word\ttab x\n\n";
    QTest::newRow("misc2") << "\t     \tFoo" <<
        "\t     \tFoo\n\n";
}

void tst_QTextMarkdownWriter::testWriteParagraph()
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    QTextCursor cursor(document);
    cursor.insertText(input);

    const QString output = documentToUnixMarkdown();
    if (output != expectedOutput && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expectedOutput);
}

void tst_QTextMarkdownWriter::testWriteList()
{
    QTextCursor cursor(document);
    QTextList *list = cursor.createList(QTextListFormat::ListDisc);
    cursor.insertText("ListItem 1");
    list->add(cursor.block());
    cursor.insertBlock();
    cursor.insertText("ListItem 2");
    list->add(cursor.block());

    const QString output = documentToUnixMarkdown();
    const QString expected = QString::fromLatin1("- ListItem 1\n- ListItem 2\n");
    if (output != expected && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expected);
}

void tst_QTextMarkdownWriter::testWriteEmptyList()
{
    QTextCursor cursor(document);
    cursor.createList(QTextListFormat::ListDisc);

    QCOMPARE(documentToUnixMarkdown(), QString::fromLatin1("- \n"));
}

void tst_QTextMarkdownWriter::testWriteCheckboxListItemEndingWithCode()
{
    QTextCursor cursor(document);
    QTextList *list = cursor.createList(QTextListFormat::ListDisc);
    cursor.insertText("Image.originalSize property (not necessary; PdfDocument.pagePointSize() substitutes)");
    list->add(cursor.block());
    {
        auto fmt = cursor.block().blockFormat();
        fmt.setMarker(QTextBlockFormat::MarkerType::Unchecked);
        cursor.setBlockFormat(fmt);
    }
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor, 4);
    QCOMPARE(cursor.selectedText(), QString::fromLatin1("PdfDocument.pagePointSize()"));
    auto fmt = cursor.charFormat();
    fmt.setFontFixedPitch(true);
    cursor.setCharFormat(fmt);
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 5);
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor, 4);
    QCOMPARE(cursor.selectedText(), QString::fromLatin1("Image.originalSize"));
    cursor.setCharFormat(fmt);

    const QString output = documentToUnixMarkdown();
    const QString expected = QString::fromLatin1(
                "- [ ] `Image.originalSize` property (not necessary; `PdfDocument.pagePointSize()`\n  substitutes)\n");
    if (output != expected && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expected);
}

void tst_QTextMarkdownWriter::testWriteNestedBulletLists_data()
{
    QTest::addColumn<bool>("checkbox");
    QTest::addColumn<bool>("checked");
    QTest::addColumn<bool>("continuationLine");
    QTest::addColumn<bool>("continuationParagraph");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("plain bullets") << false << false << false << false <<
        "- ListItem 1\n  * ListItem 2\n    + ListItem 3\n- ListItem 4\n  * ListItem 5\n";
    QTest::newRow("bullets with continuation lines") << false << false << true << false <<
        "- ListItem 1\n  * ListItem 2\n    + ListItem 3 with text that won't fit on one line and thus needs a\n      continuation\n- ListItem 4\n  * ListItem 5 with text that won't fit on one line and thus needs a\n    continuation\n";
    QTest::newRow("bullets with continuation paragraphs") << false << false << false << true <<
        "- ListItem 1\n\n  * ListItem 2\n    + ListItem 3\n\n      continuation\n\n- ListItem 4\n\n  * ListItem 5\n\n    continuation\n\n";
    QTest::newRow("unchecked") << true << false << false << false <<
        "- [ ] ListItem 1\n  * [ ] ListItem 2\n    + [ ] ListItem 3\n- [ ] ListItem 4\n  * [ ] ListItem 5\n";
    QTest::newRow("checked") << true << true << false << false <<
        "- [x] ListItem 1\n  * [x] ListItem 2\n    + [x] ListItem 3\n- [x] ListItem 4\n  * [x] ListItem 5\n";
    QTest::newRow("checked with continuation lines") << true << true << true << false <<
        "- [x] ListItem 1\n  * [x] ListItem 2\n    + [x] ListItem 3 with text that won't fit on one line and thus needs a\n      continuation\n- [x] ListItem 4\n  * [x] ListItem 5 with text that won't fit on one line and thus needs a\n    continuation\n";
    QTest::newRow("checked with continuation paragraphs") << true << true << false << true <<
        "- [x] ListItem 1\n\n  * [x] ListItem 2\n    + [x] ListItem 3\n\n      continuation\n\n- [x] ListItem 4\n\n  * [x] ListItem 5\n\n    continuation\n\n";
}

void tst_QTextMarkdownWriter::testWriteNestedBulletLists()
{
    QFETCH(bool, checkbox);
    QFETCH(bool, checked);
    QFETCH(bool, continuationParagraph);
    QFETCH(bool, continuationLine);
    QFETCH(QString, expectedOutput);

    QTextCursor cursor(document);
    QTextBlockFormat blockFmt = cursor.blockFormat();
    if (checkbox) {
        blockFmt.setMarker(checked ? QTextBlockFormat::MarkerType::Checked : QTextBlockFormat::MarkerType::Unchecked);
        cursor.setBlockFormat(blockFmt);
    }

    QTextList *list1 = cursor.createList(QTextListFormat::ListDisc);
    cursor.insertText("ListItem 1");
    list1->add(cursor.block());

    QTextListFormat fmt2;
    fmt2.setStyle(QTextListFormat::ListCircle);
    fmt2.setIndent(2);
    QTextList *list2 = cursor.insertList(fmt2);
    cursor.insertText("ListItem 2");

    QTextListFormat fmt3;
    fmt3.setStyle(QTextListFormat::ListSquare);
    fmt3.setIndent(3);
    cursor.insertList(fmt3);
    cursor.insertText(continuationLine ?
                          "ListItem 3 with text that won't fit on one line and thus needs a continuation" :
                          "ListItem 3");
    if (continuationParagraph) {
        QTextBlockFormat blockFmt;
        blockFmt.setIndent(2);
        cursor.insertBlock(blockFmt);
        cursor.insertText("continuation");
    }

    cursor.insertBlock(blockFmt);
    cursor.insertText("ListItem 4");
    list1->add(cursor.block());

    cursor.insertBlock();
    cursor.insertText(continuationLine ?
                          "ListItem 5 with text that won't fit on one line and thus needs a continuation" :
                          "ListItem 5");
    list2->add(cursor.block());
    if (continuationParagraph) {
        QTextBlockFormat blockFmt;
        blockFmt.setIndent(2);
        cursor.insertBlock(blockFmt);
        cursor.insertText("continuation");
    }

    const QString output = documentToUnixMarkdown();
#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".md");
        out.open(QFile::WriteOnly);
        out.write(output.toUtf8());
        out.close();
    }
#endif
    if (output != expectedOutput && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expectedOutput);
}

void tst_QTextMarkdownWriter::testWriteNestedNumericLists()
{
    QTextCursor cursor(document);

    QTextList *list1 = cursor.createList(QTextListFormat::ListDecimal);
    cursor.insertText("ListItem 1");
    list1->add(cursor.block());

    QTextListFormat fmt2;
    // Alpha "numbering" is not supported in markdown, so we'll actually get decimal.
    fmt2.setStyle(QTextListFormat::ListLowerAlpha);
    fmt2.setNumberSuffix(QLatin1String(")"));
    fmt2.setIndent(2);
    QTextList *list2 = cursor.insertList(fmt2);
    cursor.insertText("ListItem 2");

    QTextListFormat fmt3;
    fmt3.setStyle(QTextListFormat::ListDecimal);
    fmt3.setIndent(3);
    cursor.insertList(fmt3);
    cursor.insertText("ListItem 3");

    cursor.insertBlock();
    cursor.insertText("ListItem 4");
    list1->add(cursor.block());

    cursor.insertBlock();
    cursor.insertText("ListItem 5");
    list2->add(cursor.block());

    const QString output = documentToUnixMarkdown();

 #ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out(QDir::temp().filePath(QLatin1String(QTest::currentTestFunction()) + ".md"));
        out.open(QFile::WriteOnly);
        out.write(output.toUtf8());
        out.close();
    }
    {
        QFile out(QDir::temp().filePath(QLatin1String(QTest::currentTestFunction()) + ".html"));
        out.open(QFile::WriteOnly);
        out.write(document->toHtml().toUtf8());
        out.close();
    }
#endif

    // While we can set the start index for a block, if list items intersect each other, they will
    // still use the list numbering.
    const QString expected = QString::fromLatin1(
                "1.  ListItem 1\n    1)  ListItem 2\n        1.  ListItem 3\n2.  ListItem 4\n    2)  ListItem 5\n");
    if (output != expected && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expected);
}

void tst_QTextMarkdownWriter::testWriteNumericListWithStart()
{
    QTextCursor cursor(document);

    // The first list will start at 2.
    QTextListFormat fmt1;
    fmt1.setStyle(QTextListFormat::ListDecimal);
    fmt1.setStart(2);
    QTextList *list1 = cursor.createList(fmt1);
    cursor.insertText("ListItem 1");
    list1->add(cursor.block());

    // This list uses the default start (1) again.
    QTextListFormat fmt2;
    // Alpha "numbering" is not supported in markdown, so we'll actually get decimal.
    fmt2.setStyle(QTextListFormat::ListLowerAlpha);
    fmt2.setNumberSuffix(QLatin1String(")"));
    fmt2.setIndent(2);
    QTextList *list2 = cursor.insertList(fmt2);
    cursor.insertText("ListItem 2");

    // Negative list numbers are disallowed by most Markdown implementations. This list will start
    // at 1 for that reason.
    QTextListFormat fmt3;
    fmt3.setStyle(QTextListFormat::ListDecimal);
    fmt3.setIndent(3);
    fmt3.setStart(-1);
    cursor.insertList(fmt3);
    cursor.insertText("ListItem 3");

    // Continuing list1, so the second item will have the number 3.
    cursor.insertBlock();
    cursor.insertText("ListItem 4");
    list1->add(cursor.block());

    // This will look out of place: it's in a different position than its list would suggest.
    // Generates invalid markdown numbering (OK for humans, but md4c will parse it differently than we "meant").
    // TODO QTBUG-111707: the writer needs to add newlines, otherwise ListItem 5 becomes part of the text for ListItem 4.
    cursor.insertBlock();
    cursor.insertText("ListItem 5");
    list2->add(cursor.block());

    // 0 indexed lists are fine.
    QTextListFormat fmt4;
    fmt4.setStyle(QTextListFormat::ListDecimal);
    fmt4.setStart(0);
    QTextList *list4 = cursor.insertList(fmt4);
    cursor.insertText("SecondList Item 0");
    list4->add(cursor.block());

    // Ensure list numbers are incremented properly.
    cursor.insertBlock();
    cursor.insertText("SecondList Item 1");
    list4->add(cursor.block());

    const QString output = documentToUnixMarkdown();
    const QString expected = QString::fromLatin1(
            R"(2.  ListItem 1
    1)  ListItem 2
        1.  ListItem 3
3.  ListItem 4
    2)  ListItem 5
0.  SecondList Item 0
1.  SecondList Item 1
)");

#ifdef DEBUG_WRITE_OUTPUT
   {
       QFile out(QDir::temp().filePath(QLatin1String(QTest::currentTestFunction()) + ".md"));
       out.open(QFile::WriteOnly);
       out.write(output.toUtf8());
       out.close();
   }
   {
       QFile out(QDir::temp().filePath(QLatin1String(QTest::currentTestFunction()) + ".html"));
       out.open(QFile::WriteOnly);
       out.write(document->toHtml().toUtf8());
       out.close();
   }
#endif

    if (output != expected && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expected);
}

void tst_QTextMarkdownWriter::testWriteTable()
{
    QTextCursor cursor(document);
    QTextTable * table = cursor.insertTable(4, 3);
    cursor = table->cellAt(0, 0).firstCursorPosition();
    // valid Markdown tables need headers, but QTextTable doesn't make that distinction
    // so QTextMarkdownWriter assumes the first row of any table is a header
    cursor.insertText("one");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("two");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("three");
    cursor.movePosition(QTextCursor::NextCell);

    cursor.insertText("alice");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("bob");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("carl");
    cursor.movePosition(QTextCursor::NextCell);

    cursor.insertText("dennis");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("eric");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("fiona");
    cursor.movePosition(QTextCursor::NextCell);

    cursor.insertText("gina");
    /*
        |one   |two |three|
        |------|----|-----|
        |alice |bob |carl |
        |dennis|eric|fiona|
        |gina  |    |     |
    */

    QString md = documentToUnixMarkdown();

#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out("/tmp/table.md");
        out.open(QFile::WriteOnly);
        out.write(md.toUtf8());
        out.close();
    }
#endif

    QString expected = QString::fromLatin1(
        "\n|one   |two |three|\n|------|----|-----|\n|alice |bob |carl |\n|dennis|eric|fiona|\n|gina  |    |     |\n\n");
    if (md != expected && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(md, expected);

    // create table with merged cells
    document->clear();
    cursor = QTextCursor(document);
    table = cursor.insertTable(3, 3);
    table->mergeCells(0, 0, 1, 2);
    table->mergeCells(1, 1, 1, 2);
    cursor = table->cellAt(0, 0).firstCursorPosition();
    cursor.insertText("a");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("b");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("c");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("d");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("e");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("f");
    /*
      +---+-+
      |a  |b|
      +---+-+
      |c|  d|
      +-+-+-+
      |e|f| |
      +-+-+-+

      generates

      |a ||b|
      |-|-|-|
      |c|d ||
      |e|f| |

    */

    md = documentToUnixMarkdown();

#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out("/tmp/table-merged-cells.md");
        out.open(QFile::WriteOnly);
        out.write(md.toUtf8());
        out.close();
    }
#endif

    expected = QString::fromLatin1("\n|a ||b|\n|-|-|-|\n|c|d ||\n|e|f| |\n\n");
    if (md != expected && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(md, expected);
}

void tst_QTextMarkdownWriter::frontMatter()
{
    QTextCursor cursor(document);
    cursor.insertText("bar");
    document->setMetaInformation(QTextDocument::FrontMatter, "foo");

    const QString output = documentToUnixMarkdown();
    const QString expectedOutput("---\nfoo\n---\nbar\n\n");
    if (output != expectedOutput && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(output, expectedOutput);
}

void tst_QTextMarkdownWriter::charFormatWrapping_data()
{
    QTest::addColumn<QTextFormat::Property>("property");
    QTest::addColumn<QVariant>("propertyValue");
    QTest::addColumn<QString>("followingText");
    QTest::addColumn<QString>("expectedIndicator");

    const QString spaced = " after";
    const QString unspaced = ", and some more after";

    QTest::newRow("FontFixedPitch-spaced")
            << QTextFormat::FontFixedPitch << QVariant(true) << spaced << "`";
    QTest::newRow("FontFixedPitch-unspaced")
            << QTextFormat::FontFixedPitch << QVariant(true) << unspaced << "`";
    QTest::newRow("FontItalic")
            << QTextFormat::FontItalic << QVariant(true) << spaced << "*";
    QTest::newRow("FontUnderline")
            << QTextFormat::FontUnderline << QVariant(true) << spaced << "_";
    QTest::newRow("FontStrikeOut")
            << QTextFormat::FontStrikeOut << QVariant(true) << spaced << "~~";
    QTest::newRow("FontWeight-spaced")
            << QTextFormat::FontWeight << QVariant(700) << spaced << "**";
    QTest::newRow("FontWeight-unspaced")
            << QTextFormat::FontWeight << QVariant(700) << unspaced << "**";
}

void tst_QTextMarkdownWriter::charFormatWrapping() // QTBUG-116927
{
    QFETCH(QTextFormat::Property, property);
    QFETCH(QVariant, propertyValue);
    QFETCH(QString, expectedIndicator);
    QFETCH(QString, followingText);

    const QString newLine("\n");
    QTextCursor cursor(document);
    cursor.insertText("around sixty-four characters to go before some formatted words ");
    QTextCharFormat fmt;
    fmt.setProperty(property, propertyValue);
    cursor.setCharFormat(fmt);
    cursor.insertText("formatted text");

    cursor.setCharFormat({});
    cursor.insertText(followingText);
    qsizetype lastNewLineIndex = 100;

    for (int push = 0; push < 10; ++push) {
        if (push > 0) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.insertText("a");
        }

        const QString output = documentToUnixMarkdown().trimmed(); // get rid of trailing newlines
        const auto nlIdx = output.indexOf(newLine);
        qCDebug(lcTests) << "push" << push << ":" << output << "newline @" << nlIdx;
        // we're always wrapping in this test: expect to find a newline
        QCOMPARE_GT(nlIdx, 70);
        // don't expect the newline to be more than one character to the right of where we found it last time
        // i.e. if we already started breaking in the middle: "`formatted\ntext`",
        // then we would not expect that prepending one more character would make it go
        // back to breaking afterwards: "`formatted text`\n" (because then the line becomes longer than necessary)
        QCOMPARE_LE(nlIdx, lastNewLineIndex + 1);
        lastNewLineIndex = nlIdx;
        const QString nextChars = output.sliced(nlIdx + newLine.size(), expectedIndicator.size());
        const auto startingIndicatorIdx = output.indexOf(expectedIndicator);
        // the starting indicator always exists, except in case of font problems on some CI platforms
        if (startingIndicatorIdx <= 0)
            QSKIP("starting indicator not found, probably due to platform font problems (QTBUG-103484 etc.)");
        const auto endingIndicatorIdx = output.indexOf(expectedIndicator, startingIndicatorIdx + 5);
        qCDebug(lcTests) << "next chars past newline" << nextChars
                         << "indicators @" << startingIndicatorIdx << endingIndicatorIdx;
        // the closing indicator must exist
        QCOMPARE_GT(endingIndicatorIdx, startingIndicatorIdx);
        // don't start a new line with an ending indicator:
        // we can have "**formatted\ntext**" or "**formatted text**\n" or "\n**formatted text**"
        // but not "**formatted text\n**"
        if (startingIndicatorIdx < nlIdx)
            QCOMPARE_NE(nextChars, expectedIndicator);
    }
}

void tst_QTextMarkdownWriter::charFormat_data()
{
    QTest::addColumn<QTextFormat::Property>("property");
    QTest::addColumn<QVariant>("propertyValue");
    QTest::addColumn<QFont>("explicitFont");
    QTest::addColumn<QString>("expectedOutput");

    const QTextFormat::Property NoProperty = QTextFormat::ObjectIndex;

    QTest::newRow("FontFixedPitch")
            << QTextFormat::FontFixedPitch << QVariant(true) << m_defaultFont
            << "before `formatted` after";
    if (!isFixedFontProportional()) {
        // QTBUG-54623 QTBUG-75649 QTBUG-79900 QTBUG-103484 etc.
        QTest::newRow("mono font") << NoProperty << QVariant() << m_monoFont
                                        << "before `formatted` after";
    }

    {
        QFont font;
        font.setItalic(true);
        QTest::newRow("italic font")
                << NoProperty << QVariant() << font
                << "before *formatted* after";
    }
    QTest::newRow("FontItalic")
            << QTextFormat::FontItalic << QVariant(true) << m_defaultFont
            << "before *formatted* after";

    {
        QFont font;
        font.setUnderline(true);
        QTest::newRow("underline font")
                << NoProperty << QVariant() << font
                << "before _formatted_ after";
    }
    QTest::newRow("FontUnderline")
            << QTextFormat::FontUnderline << QVariant(true) << m_defaultFont
            << "before _formatted_ after";

    {
        QFont font;
        font.setStrikeOut(true);
        QTest::newRow("strikeout font")
                << NoProperty << QVariant() << font
                << "before ~~formatted~~ after";
    }
    QTest::newRow("FontStrikeOut")
            << QTextFormat::FontStrikeOut << QVariant(true) << m_defaultFont
            << "before ~~formatted~~ after";

    {
        QFont font;
        font.setBold(true);
        QTest::newRow("bold font")
                << NoProperty << QVariant() << font
                << "before **formatted** after";
    }
    {
        QFont font;
        font.setWeight(QFont::Black);
        QTest::newRow("black font")
                << NoProperty << QVariant() << font
                << "before **formatted** after";
    }
    QTest::newRow("FontWeight")
            << QTextFormat::FontWeight << QVariant(700) << m_defaultFont
            << "before **formatted** after";

    QTest::newRow("AnchorHref")
            << QTextFormat::AnchorHref << QVariant("linky linky") << m_defaultFont
            << "before [formatted](linky linky) after";

    QTest::newRow("TextToolTip") // no effect without AnchorHref
            << QTextFormat::TextToolTip << QVariant("such a tool") << m_defaultFont
            << "before formatted after";
}

void tst_QTextMarkdownWriter::charFormat()
{
    if (isMainFontFixed())
        QSKIP("QTextMarkdownWriter would generate bogus backticks");

    QFETCH(QTextFormat::Property, property);
    QFETCH(QVariant, propertyValue);
    QFETCH(QFont, explicitFont);
    QFETCH(QString, expectedOutput);

    QTextCursor cursor(document);
    cursor.insertText("before ");

    QTextCharFormat fmt;
    if (explicitFont != m_defaultFont)
        fmt.setFont(explicitFont);
    if (property != QTextFormat::ObjectIndex) // != 0
        fmt.setProperty(property, propertyValue);
    if (explicitFont == m_monoFont) {
        QFontInfo fontInfo(fmt.font());
        qCDebug(lcTests) << "mono font" << explicitFont << "fontInfo fixedPitch" << fontInfo.fixedPitch() << "fmt fixedPitch" << fmt.fontFixedPitch();
    }
    cursor.setCharFormat(fmt);
    cursor.insertText("formatted");

    cursor.setCharFormat({});
    cursor.insertText(" after");

    const QString output = documentToUnixMarkdown();
#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out(QDir::temp().filePath(QLatin1String(QTest::currentDataTag()) + ".md"));
        out.open(QFile::WriteOnly);
        out.write(output.toUtf8());
        out.close();
    }
#endif
    QCOMPARE(output.trimmed(), expectedOutput);
}

void tst_QTextMarkdownWriter::rewriteDocument_data()
{
    QTest::addColumn<QString>("inputFile");

    QTest::newRow("block quotes") << "blockquotes.md";
    QTest::newRow("block quotes with lists") << "blockquotesWithLists.md";
    // QTest::newRow("list item with block quote") << "listItemWithBlockquote.md"; // not supported for now
    QTest::newRow("example") << "example.md";
    QTest::newRow("list items after headings") << "headingsAndLists.md";
    QTest::newRow("word wrap") << "wordWrap.md";
    QTest::newRow("links") << "links.md";
    QTest::newRow("lists and code blocks") << "listsAndCodeBlocks.md";
    QTest::newRow("front matter") << "yaml.md";
    QTest::newRow("long headings") << "longHeadings.md";
}

void tst_QTextMarkdownWriter::rewriteDocument()
{
    QFETCH(QString, inputFile);
    QTextDocument doc;
    QFile f(QFINDTESTDATA("data/" + inputFile));
    QVERIFY(f.open(QFile::ReadOnly | QIODevice::Text));
    QString orig = QString::fromUtf8(f.readAll());
    f.close();
    doc.setMarkdown(orig);
    QString md = doc.toMarkdown();

#ifdef DEBUG_WRITE_OUTPUT
    QFile out("/tmp/rewrite-" + inputFile);
    out.open(QFile::WriteOnly);
    out.write(md.toUtf8());
    out.close();
#endif

    if (md != orig && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(md, orig);
}

void tst_QTextMarkdownWriter::fromHtml_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("long URL") <<
        "<span style=\"font-style:italic;\">https://www.example.com/dir/subdir/subsubdir/subsubsubdir/subsubsubsubdir/subsubsubsubsubdir/</span>" <<
        "\n*https://www.example.com/dir/subdir/subsubdir/subsubsubdir/subsubsubsubdir/subsubsubsubsubdir/*\n\n";
    QTest::newRow("non-emphasis inline asterisk") << "3 * 4" << "3 * 4\n\n";
    QTest::newRow("arithmetic") << "(2 * a * x + b)^2 = b^2 - 4 * a * c" << "(2 * a * x + b)^2 = b^2 - 4 * a * c\n\n";
    QTest::newRow("escaped asterisk after newline") <<
        "The first sentence of this paragraph holds 80 characters, then there's a star. * This is wrapped, but is <em>not</em> a bullet point." <<
        "The first sentence of this paragraph holds 80 characters, then there's a star.\n\\* This is wrapped, but is *not* a bullet point.\n\n";
    QTest::newRow("escaped plus after newline") <<
        "The first sentence of this paragraph holds 80 characters, then there's a plus. + This is wrapped, but is <em>not</em> a bullet point." <<
        "The first sentence of this paragraph holds 80 characters, then there's a plus.\n\\+ This is wrapped, but is *not* a bullet point.\n\n";
    QTest::newRow("escaped hyphen after newline") <<
        "The first sentence of this paragraph holds 80 characters, then there's a minus. - This is wrapped, but is <em>not</em> a bullet point." <<
        "The first sentence of this paragraph holds 80 characters, then there's a minus.\n\\- This is wrapped, but is *not* a bullet point.\n\n";
    QTest::newRow("list items with indented continuations") <<
        "<ul><li>bullet<p>continuation paragraph</p></li><li>another bullet<br/>continuation line</li></ul>" <<
        "- bullet\n\n  continuation paragraph\n\n- another bullet\n  continuation line\n";
    QTest::newRow("nested list items with continuations") <<
        "<ul><li>bullet<p>continuation paragraph</p></li><li>another bullet<br/>continuation line</li><ul><li>bullet<p>continuation paragraph</p></li><li>another bullet<br/>continuation line</li></ul></ul>" <<
        "- bullet\n\n  continuation paragraph\n\n- another bullet\n  continuation line\n\n  - bullet\n\n    continuation paragraph\n\n  - another bullet\n    continuation line\n";
    QTest::newRow("nested ordered list items with continuations") <<
        "<ol><li>item<p>continuation paragraph</p></li><li>another item<br/>continuation line</li><ol><li>item<p>continuation paragraph</p></li><li>another item<br/>continuation line</li></ol><li>another</li><li>another</li></ol>" <<
        "1.  item\n\n    continuation paragraph\n\n2.  another item\n    continuation line\n\n    1.  item\n\n        continuation paragraph\n\n    2.  another item\n        continuation line\n\n3.  another\n4.  another\n";
    QTest::newRow("thematic break") <<
        "something<hr/>something else" <<
        "something\n\n- - -\nsomething else\n\n";
    QTest::newRow("block quote") <<
        "<p>In 1958, Mahatma Gandhi was quoted as follows:</p><blockquote>The Earth provides enough to satisfy every man's need but not for every man's greed.</blockquote>" <<
        "In 1958, Mahatma Gandhi was quoted as follows:\n\n> The Earth provides enough to satisfy every man's need but not for every man's\n> greed.\n\n";
    QTest::newRow("image") <<
        "<img src=\"/url\" alt=\"foo\" title=\"title\"/>" <<
        "![foo](/url \"title\")\n\n";
    QTest::newRow("code") <<
        "<pre class=\"language-pseudocode\">\n#include \"foo.h\"\n\nblock {\n    statement();\n}\n\n</pre>" <<
        "```pseudocode\n#include \"foo.h\"\n\nblock {\n    statement();\n}\n\n```\n\n";
    QTest::newRow("escaped number and paren after single newline") <<
        "<p>(The first sentence of this paragraph is a line, next paragraph has a number 13) but that's not part of an ordered list</p>" <<
        "(The first sentence of this paragraph is a line, next paragraph has a number\n13\\) but that's not part of an ordered list\n\n";
    QTest::newRow("escaped number and paren after double newline") <<
        "<p>(The first sentence of this paragraph is a line, the next paragraph has a number</p>13) but that's not part of an ordered list" <<
        "(The first sentence of this paragraph is a line, the next paragraph has a number\n\n13\\) but that's not part of an ordered list\n\n";
    QTest::newRow("preformats with embedded backticks") <<
        "<pre>none `one` ``two``</pre>plain<pre>```three``` ````four````</pre>plain" <<
        "```\nnone `one` ``two``\n\n```\nplain\n\n```\n```three``` ````four````\n\n```\nplain\n\n";
    QTest::newRow("list items with and without checkboxes") <<
        "<ul><li>bullet</li><li class=\"unchecked\">unchecked item</li><li class=\"checked\">checked item</li></ul>" <<
        "- bullet\n- [ ] unchecked item\n- [x] checked item\n";
    QTest::newRow("table with backslash in cell") << // QTBUG-96051
            "<table><tr><td>1011011 [</td><td>1011100 backslash \\</td></tr></table>" <<
            "|1011011 [|1011100 backslash \\\\|";
    // https://spec.commonmark.org/0.31.2/#example-12
    // escaping punctuation is ok, but QTextMarkdownWriter currently doesn't do that (which is also ok)
    QTest::newRow("punctuation") <<
            R"(<p>!&quot;#$%&amp;'()*+,-./:;&lt;=&gt;?@[\]^_`{|}~</p>)" <<
            R"(!"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~)";
    // https://spec.commonmark.org/0.31.2/#example-14
    QTest::newRow("backslash asterisk no emphasis") << //  QTBUG-122083
            R"(\*no emphasis*)" <<
            R"(\\\*no emphasis*)";
    // https://spec.commonmark.org/0.31.2/#example-15
    QTest::newRow("backslash before emphasis") <<
            R"(\<em>emphasis</em>)" <<
            R"(\\*emphasis*)";
    // https://spec.commonmark.org/0.31.2/#example-20
    QTest::newRow("backslash-asterisk in autolink") <<
            R"(<p><a href="https://example.com?find=\\*">https://example.com?find=\*</a></p>)" <<
            R"(<https://example.com?find=\\*>)";
    // https://spec.commonmark.org/0.31.2/#example-24
    QTest::newRow("plus in fenced code lang") <<
            "<pre class=\"language-foo+bar\">foo</pre>" <<
            "```foo+bar\nfoo\n```";
}

void tst_QTextMarkdownWriter::fromHtml()
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    document->setHtml(input);
    QString output = documentToUnixMarkdown();

#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".md");
        out.open(QFile::WriteOnly);
        out.write(output.toUtf8());
        out.close();
    }
#endif

    output = output.trimmed();
    expectedOutput = expectedOutput.trimmed();
    if (output != expectedOutput && (isMainFontFixed() || isFixedFontProportional()))
        QEXPECT_FAIL("", "fixed main font or proportional fixed font (QTBUG-103484)", Continue);
    QCOMPARE(output, expectedOutput);
}

void tst_QTextMarkdownWriter::fromPlainTextAndBack_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedMarkdown");

    // tests to verify that fixing QTBUG-122083 is safe
    QTest::newRow("single backslashes") <<
            R"(\ again: \ not esc: \* \-\-\ \*abc*)" <<
            R"(\\ again: \\ not esc: \\* \\-\\-\\ \\\*abc*)";
    // https://spec.commonmark.org/0.31.2/#example-12
    QTest::newRow("punctuation") <<
            R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)" <<
            R"(!"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~)";
    // https://spec.commonmark.org/0.31.2/#example-13
    QTest::newRow("literal backslashes") <<
            QString(uR"(\→\A\a\ \3\φ\«)") <<
            "\\\\\u2192\\\\A\\\\a\\\\ \\\\3\\\\\u03C6\\\\\u00AB";
    // https://spec.commonmark.org/0.31.2/#example-14
    QTest::newRow("escape to avoid em") <<
            R"(*not emphasized*)" <<
            R"(\*not emphasized*)";
    QTest::newRow("escape to avoid html") <<
            R"(<br/> not a tag)" <<
            R"(\<br/> not a tag)";
    QTest::newRow("escape to avoid link") <<
            R"([not a link](/foo))" <<
            R"(\[not a link](/foo))";
    QTest::newRow("escape to avoid mono") <<
            R"(`not code`)" <<
            R"(\`not code`)";
    QTest::newRow("escape to avoid num list") <<
            R"(1. not a list)" <<
            R"(1\. not a list)";
    QTest::newRow("escape to avoid list") <<
            R"(* not a list)" <<
            R"(\* not a list)";
    QTest::newRow("escape to avoid heading") <<
            R"(# not a heading)" <<
            R"(\# not a heading)";
    QTest::newRow("escape to avoid reflink") <<
            R"([foo]: /url "not a reference")" <<
            R"(\[foo]: /url "not a reference")";
    QTest::newRow("escape to avoid entity") <<
            R"(&ouml; not a character entity)" <<
            R"(\&ouml; not a character entity)";
    // end of tests to verify that fixing QTBUG-122083 is safe
    // (it's ok to add unrelated plain-to-markdown-to-plaintext cases later)
}

void tst_QTextMarkdownWriter::fromPlainTextAndBack()
{
    QFETCH(QString, input);
    QFETCH(QString, expectedMarkdown);

    document->setPlainText(input);
    QString output = documentToUnixMarkdown();

#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".md");
        out.open(QFile::WriteOnly);
        out.write(output.toUtf8());
        out.close();
    }
#endif

    output = output.trimmed();
    expectedMarkdown = expectedMarkdown.trimmed();
    if (output != expectedMarkdown && (isMainFontFixed() || isFixedFontProportional()))
        QSKIP("", "fixed main font or proportional fixed font (QTBUG-103484)");
    QCOMPARE(output, expectedMarkdown);
    QCOMPARE(document->toPlainText(), input);
    document->setMarkdown(output);
    QCOMPARE(document->toPlainText(), input);
    if (document->blockCount() == 1)
        QCOMPARE(document->firstBlock().text(), input);
}

void tst_QTextMarkdownWriter::escapeSpecialCharacters_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("backslash") << "foo \\ bar \\\\ baz \\" << "foo \\\\ bar \\\\\\\\ baz \\\\";
    QTest::newRow("not emphasized") << "*normal* **normal too**" << "\\*normal* \\**normal too**";
    QTest::newRow("not code") << "`normal` `normal too`" << "\\`normal` \\`normal too`";
    QTest::newRow("code fence") << "```not a fence; ``` no risk here; ```not a fence" // TODO slightly inconsistent
                                << "\\```not a fence; ``` no risk here; \\```not a fence";
    QTest::newRow("not html") << "<p>not a tag: <br/> nope</p>" << "\\<p>not a tag: \\<br/> nope\\</p>";
    QTest::newRow("not a link") << "text [not a link](/foo)" << "text \\[not a link](/foo)";
    QTest::newRow("not a circle") << "* polaris" << "\\* polaris";
    QTest::newRow("not a square") << "+ groovy" << "\\+ groovy";
    QTest::newRow("not a bullet") << "- stayin alive" << "\\- stayin alive";
    QTest::newRow("arithmetic") << "1 + 2 - 3 * 4" << "1 + 2 - 3 * 4";
    QTest::newRow("not a list") << "1. not a list" << "1\\. not a list";
    QTest::newRow("not a list either") << "Jupiter and 10." << "Jupiter and 10.";
    QTest::newRow("not a heading") << "# not a heading" << "\\# not a heading";
    QTest::newRow("a non-entity") << "&ouml; not a character entity" << "\\&ouml; not a character entity";
}

/*! \internal
    If the user types into a Qt-based editor plain text that the
    markdown parser would misinterpret, escape it when we save to markdown
    to clarify that it's plain text.
    https://spec.commonmark.org/0.31.2/#backslash-escapes
*/
void tst_QTextMarkdownWriter::escapeSpecialCharacters() // QTBUG-96051, QTBUG-122083
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    document->setPlainText(input);
    QString output = documentToUnixMarkdown();

#ifdef DEBUG_WRITE_OUTPUT
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".md");
        out.open(QFile::WriteOnly);
        out.write(output.toUtf8());
        out.close();
    }
#endif

    output = output.trimmed();
    if (output != expectedOutput && (isMainFontFixed() || isFixedFontProportional()))
        QEXPECT_FAIL("", "fixed main font or proportional fixed font (QTBUG-103484)", Continue);
    QCOMPARE(output, expectedOutput);
}

QTEST_MAIN(tst_QTextMarkdownWriter)
#include "tst_qtextmarkdownwriter.moc"
