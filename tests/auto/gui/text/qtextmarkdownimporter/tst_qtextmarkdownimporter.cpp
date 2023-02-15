// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QBuffer>
#include <QDebug>
#include <QFontInfo>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextList>
#include <QTextTable>
#include <QLoggingCategory>

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
    void nestedSpans_data();
    void nestedSpans();
    void avoidBlankLineAtBeginning_data();
    void avoidBlankLineAtBeginning();
    void fragmentsAndProperties_data();
    void fragmentsAndProperties();
    void pathological_data();
    void pathological();
    void fencedCodeBlocks_data();
    void fencedCodeBlocks();

private:
    bool isMainFontFixed();

public:
    enum CharFormat {
        Normal = 0x0,
        Italic = 0x1,
        Bold = 0x02,
        Underlined = 0x04,
        Strikeout = 0x08,
        Mono = 0x10,
        Link = 0x20
    };
    Q_ENUM(CharFormat)
    Q_DECLARE_FLAGS(CharFormats, CharFormat)
};

Q_DECLARE_METATYPE(tst_QTextMarkdownImporter::CharFormats)
Q_DECLARE_OPERATORS_FOR_FLAGS(tst_QTextMarkdownImporter::CharFormats)

bool tst_QTextMarkdownImporter::isMainFontFixed()
{
    bool ret = QFontInfo(QGuiApplication::font()).fixedPitch();
    if (ret) {
        qCWarning(lcTests) << "QFontDatabase::GeneralFont is monospaced: markdown writing is likely to use too many backticks";
        qCWarning(lcTests) << "system fonts: fixed" << QFontDatabase::systemFont(QFontDatabase::FixedFont)
                           << "fixed?" << QFontInfo(QFontDatabase::systemFont(QFontDatabase::FixedFont)).fixedPitch()
                           << "general" << QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    }
    return ret;
}

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
    QTest::addColumn<int>("skipToCheckStart");
    QTest::addColumn<int>("expectedListStart");
    QTest::addColumn<int>("expectedItemCount");
    QTest::addColumn<bool>("expectedEmptyItems");
    QTest::addColumn<QString>("rewrite");

    // Some of these cases show odd behavior, which is subject to change
    // as the importer and the writer are tweaked to fix bugs over time.
    QTest::newRow("dot newline") << ".\n" << 0 << 1 << 0 << true << ".\n\n";
    QTest::newRow("number dot newline") << "1.\n" << 0 << 1 << 1 << true << "1.  \n";
    QTest::newRow("number offset start") << "2. text\n" << 0 << 2 << 1 << false << "2.  text\n";
    QTest::newRow("second list offset start")
            << "1. text\n\nintervening paragraph\n\n4. second list item"
            << 2 << 4 << 2 << false
            << "1.  text\n\nintervening paragraph\n\n4.  second list item\n";
    QTest::newRow("list continuation offset start")
            << "3. text\n\n   next paragraph in item 1\n10. second list item"
            << 2 << 3 << 2 << false
            << "3.  text\n\n    next paragraph in item 1\n\n4.  second list item\n";
    QTest::newRow("nested list offset start")
            << "1. text\n\n    0. indented list item\n\n4. second item in first list"
            << 1 << 0 << 3 << false
            << "1.  text\n    0.  indented list item\n2.  second item in first list\n";
    QTest::newRow("offset start after nested list")
            << "1. text\n\n    0. indented list item\n\n4. second item in first list"
            << 2 << 1 << 3 << false
            << "1.  text\n    0.  indented list item\n2.  second item in first list\n";
    QTest::newRow("star newline") << "*\n" << 0 << 1 << 1 << true << "* \n";
    QTest::newRow("hyphen newline") << "-\n" << 0 << 1 << 1 << true << "- \n";
    QTest::newRow("hyphen space newline") << "- \n" << 0 << 1 << 1 << true << "- \n";
    QTest::newRow("hyphen space letter newline") << "- a\n" << 0 << 1 << 1 << false << "- a\n";
    QTest::newRow("hyphen nbsp newline") <<
        QString::fromUtf8("-\u00A0\n") << 0 << 1 << 0 << true << "-\u00A0\n\n";
    QTest::newRow("nested empty lists") << "*\n  *\n  *\n" << 0 << 1 << 1 << true << "  * \n";
    QTest::newRow("list nested in empty list") << "-\n  * a\n" << 0 << 1 << 2 << false << "- \n  * a\n";
    QTest::newRow("lists nested in empty lists")
            << "-\n  * a\n  * b\n- c\n  *\n    + d\n" << 0 << 1 << 5 << false
            << "- \n  * a\n  * b\n- c *\n  + d\n";
    QTest::newRow("numeric lists nested in empty lists")
            << "- \n    1.  a\n    2.  b\n- c\n  1.\n       + d\n" << 0 << 1 << 4 << false
            << "- \n    1.  a\n    2.  b\n- c 1. + d\n";
    QTest::newRow("styled spans in list items")
            << "1.  normal text\n2.  **bold** text\n3.  `code` in the item\n4.  *italic* text\n5.  _underlined_ text\n"
            << 0 << 1 << 5 << false
            << "1.  normal text\n2.  **bold** text\n3.  `code` in the item\n4.  *italic* text\n5.  _underlined_ text\n";
}

void tst_QTextMarkdownImporter::lists()
{
    QFETCH(QString, input);
    QFETCH(int, skipToCheckStart);
    QFETCH(int, expectedListStart);
    QFETCH(int, expectedItemCount);
    QFETCH(bool, expectedEmptyItems);
    QFETCH(QString, rewrite);

    QTextDocument doc;
    doc.setMarkdown(input); // QTBUG-78870 : don't crash

#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif
    qCDebug(lcTests) << " original:" << input;
    qCDebug(lcTests) << "rewritten:" << doc.toMarkdown();

    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    QTextFrame *currentFrame = iterator.currentFrame();
    int i = 0;
    int itemCount = 0;
    bool emptyItems = true;
    QString firstItemFontFamily;
    while (!iterator.atEnd()) {
        // There are no child frames
        QCOMPARE(iterator.currentFrame(), currentFrame);
        // Check whether the block is text or a horizontal rule
        QTextBlock block = iterator.currentBlock();
        QTextListFormat listFmt;
        if (block.textList()) {
            ++itemCount;
            if (!block.text().isEmpty())
                emptyItems = false;
            listFmt = block.textList()->format();
        }
        qCDebug(lcTests, "%d %s%s", i,
                (block.textList() ? "<li>" : "<p>"), qPrintable(block.text()));
        QTextCharFormat listItemFmt = block.charFormat();
        QFont listItemFont = listItemFmt.font();
        // QTextDocumentLayoutPrivate::drawListItem() uses listItemFont to render numbers in an ordered list.
        // We want that to be consistent, regardless whether the list item's text begins with a styled span.
        if (firstItemFontFamily.isEmpty())
            firstItemFontFamily = listItemFont.family();
        else
            QCOMPARE(listItemFont.family(), firstItemFontFamily);
        QCOMPARE(listItemFont.bold(), false);
        QCOMPARE(listItemFont.italic(), false);
        QCOMPARE(listItemFont.underline(), false);
        QCOMPARE(listItemFont.fixedPitch(), false);
        QCOMPARE(listItemFmt.fontItalic(), false);
        QCOMPARE(listItemFmt.fontUnderline(), false);
        QCOMPARE(listItemFmt.fontFixedPitch(), false);
        if (i == skipToCheckStart) {
            qCDebug(lcTests) << "skipped to list item" << i << block.text()
                             << "start" << listFmt.start() << "expected" << expectedListStart;
            QCOMPARE(listFmt.start(), expectedListStart);
        }
        ++iterator;
        ++i;
    }
    QCOMPARE(itemCount, expectedItemCount);
    QCOMPARE(emptyItems, expectedEmptyItems);
    if (doc.toMarkdown() != rewrite && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(doc.toMarkdown(), rewrite);
}

void tst_QTextMarkdownImporter::nestedSpans_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("wordToCheck");
    QTest::addColumn<CharFormats>("expectedFormat");

    QTest::newRow("bold italic")
            << "before ***bold italic*** after"
            << 1 << (Bold | Italic);
    QTest::newRow("bold underlined")
            << "before **_bold underlined_** after"
            << 1 << (Bold | Underlined);
    QTest::newRow("italic underlined")
            << "before *_italic underlined_* after"
            << 1 << (Italic | Underlined);
    QTest::newRow("bold strikeout")
            << "before **~~bold strikeout~~** after"
            << 1 << (Bold | Strikeout);
    QTest::newRow("italic strikeout")
            << "before *~~italic strikeout~~* after"
            << 1 << (Italic | Strikeout);
    QTest::newRow("bold italic strikeout")
            << "before ***~~bold italic strikeout~~*** after"
            << 1 << (Bold | Italic | Strikeout);
    QTest::newRow("bold link text")
            << "before [**bold link**](https://qt.io) after"
            << 1 << (Bold | Link);
    QTest::newRow("italic link text")
            << "before [*italic link*](https://qt.io) after"
            << 1 << (Italic | Link);
    QTest::newRow("bold italic link text")
            << "before [***bold italic link***](https://qt.io) after"
            << 1 << (Bold | Italic | Link);
    QTest::newRow("strikeout link text")
            << "before [~~strikeout link~~](https://qt.io) after"
            << 1 << (Strikeout | Link);
    QTest::newRow("strikeout bold italic link text")
            << "before [~~***strikeout bold italic link***~~](https://qt.io) after"
            << 1 << (Strikeout | Bold | Italic | Link);
    QTest::newRow("bold image alt")
            << "before [**bold image alt**](/path/to/image.png) after"
            << 1 << (Bold | Link);
    QTest::newRow("bold strikeout italic image alt")
            << "before [**~~*bold strikeout italic image alt*~~**](/path/to/image.png) after"
            << 1 << (Strikeout | Bold | Italic | Link);
    // code spans currently override all surrounding formatting
    QTest::newRow("code in italic span")
            << "before *italic `code` and* after"
            << 2 << (Mono | Normal);
    // but the format after the code span ends should revert to what it was before
    QTest::newRow("code in italic strikeout bold span")
            << "before *italic ~~strikeout **bold `code` and**~~* after"
            << 5 << (Bold | Italic | Strikeout);
}

void tst_QTextMarkdownImporter::nestedSpans()
{
    QFETCH(QString, input);
    QFETCH(int, wordToCheck);
    QFETCH(CharFormats, expectedFormat);

    QTextDocument doc;
    doc.setMarkdown(input);

#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif

    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    QTextFrame *currentFrame = iterator.currentFrame();
    while (!iterator.atEnd()) {
        // There are no child frames
        QCOMPARE(iterator.currentFrame(), currentFrame);
        // Check the QTextCharFormat of the specified word
        QTextCursor cur(iterator.currentBlock());
        cur.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, wordToCheck);
        cur.select(QTextCursor::WordUnderCursor);
        QTextCharFormat fmt = cur.charFormat();
        qCDebug(lcTests) << "word" << wordToCheck << cur.selectedText() << "font" << fmt.font()
                         << "weight" << fmt.fontWeight() << "italic" << fmt.fontItalic()
                         << "underlined" << fmt.fontUnderline()
                         << "strikeout" << fmt.fontStrikeOut() << "anchor" << fmt.isAnchor()
                         << "monospace" << QFontInfo(fmt.font()).fixedPitch() // depends on installed fonts (QTBUG-75649)
                                        << fmt.fontFixedPitch()
                                        << fmt.hasProperty(QTextFormat::FontFixedPitch)
                         << "expected" << expectedFormat;
        QCOMPARE(fmt.fontWeight() > QFont::Normal, expectedFormat.testFlag(Bold));
        QCOMPARE(fmt.fontItalic(), expectedFormat.testFlag(Italic));
        QCOMPARE(fmt.fontUnderline(), expectedFormat.testFlag(Underlined));
        QCOMPARE(fmt.fontStrikeOut(), expectedFormat.testFlag(Strikeout));
        QCOMPARE(fmt.isAnchor(), expectedFormat.testFlag(Link));
        QCOMPARE(fmt.fontFixedPitch(), expectedFormat.testFlag(Mono));
        QCOMPARE(fmt.hasProperty(QTextFormat::FontFixedPitch), expectedFormat.testFlag(Mono));
        ++iterator;
    }
}

void tst_QTextMarkdownImporter::avoidBlankLineAtBeginning_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expectedNumberOfParagraphs");

    QTest::newRow("Text block") << QString("Markdown text") << 1;
    QTest::newRow("Headline") << QString("Markdown text\n============") << 1;
    QTest::newRow("Code block") << QString("    Markdown text") << 1;
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

void tst_QTextMarkdownImporter::fragmentsAndProperties_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("fragmentToCheck");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QTextFormat::Property>("propertyToCheck");
    QTest::addColumn<QVariant>("expectedPropertyValue");
    QTest::addColumn<int>("expectedNumberOfBlocks");
    QTest::addColumn<int>("expectedNumberOfFragments");

    QTest::newRow("entitiesInHtmlFontBlock") // QTBUG-94245
            << QString("<font color='red'>&lt;123 test&gt;</font>&nbsp;test")
            << 0 << "<123 test>" << QTextFormat::ForegroundBrush << QVariant(QBrush(QColor("red")))
            << 1 << 2;
    QTest::newRow("entitiesInHtmlBoldBlock") // QTBUG-91222
            << QString("<b>x&amp;lt;</b>")
            << 0 << "x&lt;" << QTextFormat::FontWeight << QVariant(700)
            << 1 << 1;
}

void tst_QTextMarkdownImporter::fragmentsAndProperties()
{
    QFETCH(QString, input);
    QFETCH(int, fragmentToCheck);
    QFETCH(QString, expectedText);
    QFETCH(QTextFormat::Property, propertyToCheck);
    QFETCH(QVariant, expectedPropertyValue);
    QFETCH(int, expectedNumberOfBlocks);
    QFETCH(int, expectedNumberOfFragments);

    QTextDocument doc;
    QTextMarkdownImporter(QTextMarkdownImporter::DialectGitHub).import(&doc, input);
#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif
    QTextFrame::iterator blockIter = doc.rootFrame()->begin();
    int blockCount = 0;
    int fragCount = 0;
    while (!blockIter.atEnd()) {
        QTextBlock block = blockIter.currentBlock();
        auto fragIter = block.begin();
        while (!fragIter.atEnd()) {
            auto frag = fragIter.fragment();
            qCDebug(lcTests) << "fragment" << fragCount << ':' << frag.text() << Qt::hex << frag.charFormat().properties();
            if (fragCount == fragmentToCheck) {
                QVariant prop = frag.charFormat().property(propertyToCheck);
                QCOMPARE(prop, expectedPropertyValue);
                QCOMPARE(frag.text(), expectedText);
            }
            ++fragIter;
            ++fragCount;
        }
        ++blockIter;
        ++blockCount;
    }
    QCOMPARE(blockCount, expectedNumberOfBlocks);
    QCOMPARE(fragCount, expectedNumberOfFragments);
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
    Q_UNUSED(warning);
#else
    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());
#endif
    QTextDocument().setMarkdown(f.readAll());
}

void tst_QTextMarkdownImporter::fencedCodeBlocks_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expectedCodeBlockCount");
    QTest::addColumn<int>("expectedPlainBlockCount");
    QTest::addColumn<QString>("expectedLanguage");
    QTest::addColumn<QString>("expectedFenceChar");
    QTest::addColumn<QString>("rewrite");

    QTest::newRow("backtick fence with language")
            << "```pseudocode\nprint('hello world\\n')\n```\n"
            << 1 << 0 << "pseudocode" << "`"
            << "```pseudocode\nprint('hello world\\n')\n```\n\n";
    QTest::newRow("tilde fence with language")
            << "~~~pseudocode\nprint('hello world\\n')\n~~~\n"
            << 1 << 0 << "pseudocode" << "~"
            << "~~~pseudocode\nprint('hello world\\n')\n~~~\n\n";
    QTest::newRow("embedded backticks")
            << "```\nnone `one` ``two``\n```\nplain\n```\n```three``` ````four````\n```\nplain\n"
            << 2 << 2 << QString() << "`"
            << "```\nnone `one` ``two``\n```\nplain\n\n```\n```three``` ````four````\n```\nplain\n\n";
}

void tst_QTextMarkdownImporter::fencedCodeBlocks()
{
    QFETCH(QString, input);
    QFETCH(int, expectedCodeBlockCount);
    QFETCH(int, expectedPlainBlockCount);
    QFETCH(QString, expectedLanguage);
    QFETCH(QString, expectedFenceChar);
    QFETCH(QString, rewrite);

    QTextDocument doc;
    doc.setMarkdown(input);

#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif

    QTextFrame::iterator iterator = doc.rootFrame()->begin();
    QTextFrame *currentFrame = iterator.currentFrame();
    int codeBlockCount = 0;
    int plainBlockCount = 0;
    while (!iterator.atEnd()) {
        // There are no child frames
        QCOMPARE(iterator.currentFrame(), currentFrame);
        // Check whether the block is code or plain
        QTextBlock block = iterator.currentBlock();
        const bool codeBlock = block.blockFormat().hasProperty(QTextFormat::BlockCodeFence);
        QCOMPARE(block.blockFormat().nonBreakableLines(), codeBlock);
        QCOMPARE(block.blockFormat().stringProperty(QTextFormat::BlockCodeLanguage), codeBlock ? expectedLanguage : QString());
        if (codeBlock) {
            QCOMPARE(block.blockFormat().stringProperty(QTextFormat::BlockCodeFence), expectedFenceChar);
            ++codeBlockCount;
        } else {
            ++plainBlockCount;
        }
        qCDebug(lcTests) << (codeBlock ? "code" : "text") << block.text() << block.charFormat().fontFamilies();
        ++iterator;
    }
    QCOMPARE(codeBlockCount, expectedCodeBlockCount);
    QCOMPARE(plainBlockCount, expectedPlainBlockCount);
    if (doc.toMarkdown() != rewrite && isMainFontFixed())
        QEXPECT_FAIL("", "fixed-pitch main font (QTBUG-103484)", Continue);
    QCOMPARE(doc.toMarkdown(), rewrite);
}

QTEST_MAIN(tst_QTextMarkdownImporter)
#include "tst_qtextmarkdownimporter.moc"
