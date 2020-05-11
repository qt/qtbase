/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <qtextdocument.h>
#include <qtextdocumentfragment.h>
#include <qtexttable.h>
#include <qtextlist.h>
#include <qregularexpression.h>
#include <qdebug.h>
#include <private/qtextdocument_p.h>


#include <qtextcursor.h>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

class tst_QTextDocumentFragment : public QObject
{
    Q_OBJECT

public:
    tst_QTextDocumentFragment();
    ~tst_QTextDocumentFragment();

public slots:
    void init();
    void cleanup();
private slots:
    void listCopying();
    void listZeroCopying();
    void listCopying2();
    void tableCopying();
    void tableCopyingWithColSpans();
    void tableColSpanAndWidth();
    void tableImport();
    void tableImport2();
    void tableImport3();
    void tableImport4();
    void tableImport5();
    void textCopy();
    void copyWholeDocument();
    void title();
    void html_listIndents1();
    void html_listIndents2();
    void html_listIndents3();
    void html_listIndents4();
    void html_listIndents5();
    void html_listIndents6();
    void html_listIndents7();
    void blockCharFormat();
    void blockCharFormatCopied();
    void initialBlock();
    void clone();
    void dontRemoveInitialBlockIfItHoldsObjectIndexedCharFormat();
    void dosLineFeed();
    void unorderedListEnumeration();
    void resetHasBlockAfterClosedBlockTags();
    void ignoreStyleTags();
    void hrefAnchor();
    void namedAnchorFragments();
    void namedAnchorFragments2();
    void namedAnchorFragments3();
    void dontInheritAlignmentInTables();
    void cellBlockCount();
    void cellBlockCount2();
    void emptyTable();
    void emptyTable2();
    void emptyTable3();
    void doubleRowClose();
    void mayNotHaveChildren();
    void inheritAlignment();
    void dontEmitEmptyNodeWhenEmptyTagIsFollowedByCloseTag();
    void toPlainText();
    void copyTableRow();
    void copyTableColumn();
    void copySubTable();
    void html_textDecoration();
    void html_infiniteLoop();
    void html_blockIndent();
    void html_listIndent();
    void html_whitespace();
    void html_whitespace_data();
    void html_qt3Whitespace();
    void html_qt3WhitespaceWithFragments();
    void html_qt3WhitespaceAfterTags();
    void html_listStart1();
    void html_listStart2();
    void html_cssMargin();
    void html_hexEntities();
    void html_decEntities();
    void html_thCentered();
    void orderedListNumbering();
    void html_blockAfterList();
    void html_subAndSuperScript();
    void html_cssColors();
    void obeyFragmentMarkersInImport();
    void whitespaceWithFragmentMarkers();
    void html_emptyParapgraphs1();
    void html_emptyParapgraphs2();
    void html_emptyParagraphs3();
    void html_emptyParagraphs4();
    void html_font();
    void html_fontSize();
    void html_fontSizeAdjustment();
    void html_cssFontSize();
    void html_cssShorthandFont();
    void html_bodyBgColor();
    void html_qtBgColor();
    void html_blockLevelDiv();
    void html_spanNesting();
    void html_nestedLists();
    void noSpecialCharactersInPlainText();
    void html_doNotInheritBackground();
    void html_inheritBackgroundToInlineElements();
    void html_doNotInheritBackgroundFromBlockElements();
    void html_nobr();
    void fromPlainText();
    void fromPlainText2();
    void html_closingImageTag();
    void html_emptyDocument();
    void html_closingTag();
    void html_anchorAroundImage();
    void html_floatBorder();
    void html_frameImport();
    void html_frameImport2();
    void html_dontAddMarginsAcrossTableCells();
    void html_dontMergeCenterBlocks();
    void html_tableCellBgColor();
    void html_tableCellBgColor2();
    void html_cellSkip();
    void nonZeroMarginOnImport();
    void html_charFormatPropertiesUnset();
    void html_headings();
    void html_quotedFontFamily_data();
    void html_quotedFontFamily();
    void html_spanBackgroundColor();
    void defaultFont();
    void html_brokenTitle_data();
    void html_brokenTitle();
    void html_blockVsInline();
    void html_tbody();
    void html_nestedTables();
    void html_rowSpans();
    void html_rowSpans2();
    void html_implicitParagraphs();
    void html_missingCloseTag();
    void html_anchorColor();
    void html_lastParagraphClosing();
    void html_tableHeaderBodyFootParent();
    void html_columnWidths();
    void html_bodyBackground();
    void html_tableCellBackground();
    void css_bodyBackground();
    void css_tableCellBackground();
    void css_tableCellBorder();
    void css_tableCellBorderWidthOneValue();
    void css_tableCellBorderWidthTwoValues();
    void css_tableCellBorderShorthand();
    void css_tableCellAllBordersShorthand();
    void css_tableCellOverrideOneBorder();
    void css_tableBorderCollapse();
    void css_fontWeight();
    void css_float();
    void css_textIndent();
    void css_inline();
    void css_external();
    void css_import();
    void css_selectors_data();
    void css_selectors();
    void css_nodeNameCaseInsensitivity();
    void css_textUnderlineStyle_data();
    void css_textUnderlineStyle();
    void css_textUnderlineStyleAndDecoration();
    void css_listStyleType();
    void css_linkPseudo();
    void css_pageBreaks();
    void css_cellPaddings();
    void css_whiteSpace_data();
    void css_whiteSpace();
    void universalSelectors_data();
    void universalSelectors();
    void screenMedia();
    void htmlResourceLoading();
    void someCaseInsensitiveAttributeValues();
    void backgroundImage();
    void dontMergePreAndNonPre();
    void leftMarginInsideHtml();
    void html_margins();
    void newlineInsidePreShouldBecomeNewParagraph();
    void invalidColspan();
    void html_brokenTableWithJustTr();
    void html_brokenTableWithJustTd();
    void html_preNewlineHandling_data();
    void html_preNewlineHandling();
    void html_br();
    void html_dl();
    void html_tableStrangeNewline();
    void html_tableStrangeNewline2();
    void html_tableStrangeNewline3();
    void html_caption();
    void html_windowsEntities();
    void html_eatenText();
    void html_hr();
    void html_hrMargins();
    void html_blockQuoteMargins();
    void html_definitionListMargins();
    void html_listMargins();
    void html_titleAttribute();
    void html_compressDivs();
    void completeToPlainText();
    void copyContents();
    void html_textAfterHr();
    void blockTagClosing();
    void isEmpty();
    void html_alignmentInheritance();
    void html_ignoreEmptyDivs();
    void html_dontInheritAlignmentForFloatingImages();
    void html_verticalImageAlignment();
    void html_verticalCellAlignment();
    void html_borderColor();
    void html_borderStyle();
    void html_borderWidth();
    void html_userState();
    void html_rootFrameProperties();
    void html_alignmentPropertySet();
    void html_appendList();
    void html_appendList2();
    void html_qt3RichtextWhitespaceMode();
    void html_brAfterHr();
    void html_unclosedHead();
    void html_entities();
    void html_entities_data();
    void html_ignore_script();
    void html_directionWithHtml();
    void html_directionWithRichText();
    void html_metaInBody();
    void html_importImageWithoutAspectRatio();
    void html_fromFirefox();
    void html_emptyInlineInsideBlock();
    void css_fontAndWordSpacing();

private:
    inline void setHtml(const QString &html)
    // don't take the shortcut in QTextDocument::setHtml
    { doc->clear(); QTextCursor(doc).insertFragment(QTextDocumentFragment::fromHtml(html)); }

    inline void appendHtml(const QString &html)
    {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::End);
        cursor.insertHtml(html);
    }

    QTextDocument *doc;
    QTextCursor cursor;
};

tst_QTextDocumentFragment::tst_QTextDocumentFragment()
{
    QImage img(16, 16, QImage::Format_ARGB32_Premultiplied);
    img.save("foo.png");
}

tst_QTextDocumentFragment::~tst_QTextDocumentFragment()
{
    QFile::remove(QLatin1String("foo.png"));
}

void tst_QTextDocumentFragment::init()
{
    doc = new QTextDocument;
    cursor = QTextCursor(doc);
}

void tst_QTextDocumentFragment::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextDocumentFragment::listCopying()
{
    cursor.insertList(QTextListFormat::ListDecimal);

    QTextFormat originalBlockFormat = cursor.blockFormat();
    QVERIFY(originalBlockFormat.objectIndex() != -1);
    int originalListItemIdx = cursor.blockFormat().objectIndex();

    cursor.insertText("Hello World");

    QTextDocumentFragment fragment(doc);

    cursor.insertFragment(fragment);

    QVERIFY(cursor.currentList());
    QVERIFY(cursor.blockFormat() != originalBlockFormat);
    QVERIFY(cursor.blockFormat().objectIndex() != originalListItemIdx);
}

void tst_QTextDocumentFragment::listZeroCopying()
{
    // same testcase as above but using the zero-copying

    cursor.insertList(QTextListFormat::ListDecimal);

    QTextFormat originalBlockFormat = cursor.blockFormat();
    int originalListItemIdx = cursor.blockFormat().objectIndex();

    cursor.insertText("Hello World");

    QTextDocumentFragment fragment(doc);
    cursor.insertFragment(fragment);

    QVERIFY(cursor.currentList());
    QVERIFY(cursor.blockFormat() != originalBlockFormat);
    QVERIFY(cursor.blockFormat().objectIndex() != originalListItemIdx);
}

void tst_QTextDocumentFragment::listCopying2()
{
    cursor.insertList(QTextListFormat::ListDecimal);
    cursor.insertText("Hello World");

    cursor.insertList(QTextListFormat::ListDisc);
    cursor.insertText("Hello World");

    QTextDocumentFragment fragment(doc);

    cursor.insertFragment(fragment);

    cursor.movePosition(QTextCursor::Start);
    int listItemCount = 0;
    do {
        if (cursor.currentList())
            listItemCount++;
    } while (cursor.movePosition(QTextCursor::NextBlock));

    QCOMPARE(listItemCount, 4);

    // we call this here because it used to cause a failing assertion in the
    // list manager.
    doc->undo();
}

void tst_QTextDocumentFragment::tableCopying()
{
    // this tests both, the fragment to use the direction insertion instead of using the
    // cursor, which might adjuts its position when inserting a table step by step, as well
    // as the pasiveness of the tablemanager.
    QTextDocumentFragment fragment;
    {
        QTextDocument doc;
        QTextCursor cursor(&doc);

        QTextTableFormat fmt;
        QTextTable *table = cursor.insertTable(2, 2, fmt);

        table->cellAt(0, 0).firstCursorPosition().insertText("First Cell");
        table->cellAt(0, 1).firstCursorPosition().insertText("Second Cell");
        table->cellAt(1, 0).firstCursorPosition().insertText("Third Cell");
        table->cellAt(1, 1).firstCursorPosition().insertText("Fourth Cell");

        fragment = QTextDocumentFragment(&doc);
    }
    {
        QTextDocument doc;
        QTextCursor cursor(&doc);

        cursor.insertText("FooBar");
        cursor.insertBlock();
        cursor.movePosition(QTextCursor::Left);

        cursor.insertFragment(fragment);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);

        QTextTable *table = cursor.currentTable();
        QVERIFY(table);
        QCOMPARE(table->rows(), 2);
        QCOMPARE(table->columns(), 2);
    }
}

void tst_QTextDocumentFragment::tableCopyingWithColSpans()
{
    const char html[] = ""
"<table border>"
"  <tr>"
"    <td>First Cell"
"    <td>Second Cell"
"  </tr>"
"  <tr>"
"    <td colspan=\"2\">Third Cell"
"  </tr>"
"  <tr>"
"    <td>Fourth Cell"
"    <td>Fifth Cell"
"  </tr>"
"</table>";
    setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QVERIFY(table->columns() == 2 && table->rows() == 3);

    cursor = table->cellAt(2, 0).lastCursorPosition();
    cursor.setPosition(table->cellAt(0, 0).firstPosition(), QTextCursor::KeepAnchor);
    QVERIFY(cursor.hasComplexSelection());

    int firstRow = 0, numRows = 0, firstCol = 0, numCols = 0;
    cursor.selectedTableCells(&firstRow, &numRows, &firstCol, &numCols);
    QCOMPARE(firstRow, 0);
    QCOMPARE(numRows, 3);
    QCOMPARE(firstCol, 0);
    QCOMPARE(numCols, 1);

    QTextDocumentFragment frag = cursor.selection();
    cleanup();
    init();
    cursor.insertFragment(frag);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    table = cursor.currentTable();
    QVERIFY(table);
    QVERIFY(table->columns() == 1 && table->rows() == 3);
}

void tst_QTextDocumentFragment::tableColSpanAndWidth()
{
    const char html[] = ""
"<table border=\"0\">"
"  <tr>"
"    <td colspan=\"4\" width=\"400\">First Cell</td>"
"  </tr>"
"</table>";
    setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QVERIFY(table->columns() == 4 && table->rows() == 1);
    // make sure its approx 400 and not a multiple due to the colspan
    QVERIFY(doc->size().width()> 398.);
    QVERIFY(doc->size().width() < 420.);
}

void tst_QTextDocumentFragment::tableImport()
{
    // used to cause a failing assertion, as HTMLImporter::closeTag was
    // called twice with the last node.
    QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(QString::fromLatin1("<table><tr><td>Hey</td><td>Blah"));
    QVERIFY(!fragment.isEmpty());
}

void tst_QTextDocumentFragment::tableImport2()
{
    {
        const char html[] = ""
            "<table>"
            "<tr><td>First Cell</td><td>Second Cell</td></tr>"
            "<tr><td>Third Cell</td><td>Fourth Cell</td></tr>"
            "</table>";

        QTextDocument doc;
        QTextCursor cursor(&doc);
        cursor.insertFragment(QTextDocumentFragment::fromHtml(QByteArray::fromRawData(html, sizeof(html) / sizeof(html[0]))));

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();
        QVERIFY(table);
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 2);
    }
    {
        const char html[] = ""
            "<table>"
            "<tr><td>First Cell</td><td>Second Cell</td></tr>"
            "<tr><td>Third Cell</td><td>"
            "                           <table>"
            "                           <tr><td>First Nested Cell</td><td>Second Nested Cell</td></tr>"
            "                           <tr><td>Third Nested Cell</td><td>Fourth Nested Cell</td></tr>"
            "                           <tr><td>Fifth Nested Cell</td><td>Sixth Nested Cell</td></tr>"
            "                           </table></td></tr>"
            "</table>";

        QTextDocument doc;
        QTextCursor cursor(&doc);
        cursor.insertFragment(QTextDocumentFragment::fromHtml(QByteArray::fromRawData(html, sizeof(html) / sizeof(html[0]))));

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();
        QVERIFY(table);
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 2);

        /*
        QTextCursor fourthCell = table->cellAt(1, 1).firstCursorPosition();
        fourthCell.movePosition(QTextCursor::NextBlock);
        table = fourthCell.currentTable();
        QVERIFY(table);
        QVERIFY(table != cursor.currentTable());
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 3);
        */
    }
    {
        const char buggyHtml[] = ""
            "<table>"
            "<tr><td>First Cell<td>Second Cell"
            "<tr><td>Third Cell<td>Fourth Cell"
            "</table>";

        QTextDocument doc;
        QTextCursor cursor(&doc);
        cursor.insertFragment(QTextDocumentFragment::fromHtml(QByteArray::fromRawData(buggyHtml, sizeof(buggyHtml) / sizeof(buggyHtml[0]))));

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();
        QVERIFY(table);
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 2);
    }
    {
        const char buggyHtml[] = ""
            "<table>"
            "<tr><th>First Cell<th>Second Cell"
            "<tr><td>Third Cell<td>Fourth Cell"
            "</table>";

        QTextDocument doc;
        QTextCursor cursor(&doc);
        cursor.insertFragment(QTextDocumentFragment::fromHtml(QByteArray::fromRawData(buggyHtml, sizeof(buggyHtml) / sizeof(buggyHtml[0]))));

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();
        QVERIFY(table);
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 2);
    }

}

void tst_QTextDocumentFragment::tableImport3()
{
    // ### would be better to have tree tests for QTextHtmlParser
    // make sure the p is a child of the td. If not the following td
    // ends up outside the table, causing an assertion
    const char html[] = "<table><tr><td><p></p></td><td></td></tr></table>";
    QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(QString::fromLatin1(html));
    QVERIFY(!fragment.isEmpty());
}

void tst_QTextDocumentFragment::tableImport4()
{
    const char html[] = "<table>"
        "<tr><td>blah</td></tr>"
        "<tr><td>blah</td><td>blah</td></tr>"
        "</table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());
    QCOMPARE(cursor.currentTable()->columns(), 2);
}

void tst_QTextDocumentFragment::tableImport5()
{
    const char html[] = "<table>"
        "<tr>"
        " <td>Foo</td>"
        " <td>Bar</td>"
        " <td>Bleh</td>"
        "  <td>Blub</td>"
        "</tr>"
        "<tr>"
        "  <td>Ahh</td>"
        "  <td colspan=5>Gah</td>"
        "</tr>"
        "</table>";

    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());
    QCOMPARE(cursor.currentTable()->rows(), 2);
    QCOMPARE(cursor.currentTable()->columns(), 6);
}

void tst_QTextDocumentFragment::textCopy()
{
    /* this test used to cause failing assertions in QTextDocumentFragment */
    /* copy&paste 'lo\bwor' */
    cursor.insertText("Hello");
    cursor.insertBlock();
    cursor.insertText("World");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 3);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 3);

    QTextDocumentFragment fragment(cursor);
    QVERIFY(!fragment.isEmpty());
    cursor.insertFragment(fragment);
}

void tst_QTextDocumentFragment::copyWholeDocument()
{
    // used to cause the famous currentBlock.position() == pos + 1 failing assertion
    cursor.insertText("\nHey\nBlah\n");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setBackground(Qt::blue);
    doc->rootFrame()->setFrameFormat(fmt);

    QTextDocumentFragment fragment(cursor);
    QVERIFY(true); // good if we reach this point :)

    cleanup();
    init();

    fmt.setBackground(Qt::red);
    doc->rootFrame()->setFrameFormat(fmt);

    cursor.insertFragment(fragment);

    QVERIFY(doc->rootFrame()->frameFormat().background().color() == Qt::red);
}

void tst_QTextDocumentFragment::title()
{
    doc->setHtml(QString::fromLatin1("<html><head><title>Test</title></head><body>Blah</body></html>"));
    QCOMPARE(doc->metaInformation(QTextDocument::DocumentTitle), QString::fromLatin1("Test"));
}

void tst_QTextDocumentFragment::html_listIndents1()
{
    const char html[] = "<ul><li>Hey</li><li>Hah</li></ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);
    QCOMPARE(cursor.block().blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_listIndents2()
{
    const char html[] = "<ul><li>Hey<p>Hah</ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);
    QCOMPARE(cursor.block().blockFormat().indent(), 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().blockFormat().indent(), 1);
}

void tst_QTextDocumentFragment::html_listIndents3()
{
    const char html[] = "<ul><li><p>Hah</ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);
    QCOMPARE(cursor.block().blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_listIndents4()
{
    const char html[] = "<ul><li>Foo</ul><p>This should not have the same indent as Foo";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(!cursor.currentList());
    QCOMPARE(cursor.blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_listIndents5()
{
    const char html[] = "<ul><li>Foo<p><li>Bar</li></ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.currentList(), list);
    QCOMPARE(cursor.blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_listIndents6()
{
    const char html[] = "<ul><li>Outer List<div class=\"testclass\"><ul><li>Nested Item 1</li></ul></div></li></ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentList() != list);
    list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 2);

    QCOMPARE(cursor.blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_listIndents7()
{
    const char html[] = "<ul><li style=\"-qt-block-indent:1;\">Hey</ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextList *list = cursor.currentList();
    QVERIFY(list);
    QCOMPARE(list->format().indent(), 1);
    QCOMPARE(cursor.block().blockFormat().indent(), 1);
}

void tst_QTextDocumentFragment::blockCharFormat()
{
    const char html[] = "<p style=\"font-style:italic\"><span style=\"font-style:normal\">Test</span></p>";
    setHtml(QString::fromLatin1(html));
    QVERIFY(doc->begin().charFormat().fontItalic());
}

void tst_QTextDocumentFragment::blockCharFormatCopied()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::green);
    cursor.setBlockCharFormat(fmt);
    cursor.insertText("Test", QTextCharFormat());
    QTextDocumentFragment frag(doc);
    cleanup();
    init();
    cursor.insertFragment(frag);
    QCOMPARE(cursor.blockCharFormat(), fmt);
}

void tst_QTextDocumentFragment::initialBlock()
{
    const char html[] = "<p>Test</p>";
    setHtml(QString::fromLatin1(html));
    QCOMPARE(doc->blockCount(), 1);
}

void tst_QTextDocumentFragment::clone()
{
    QTextBlockFormat mod;
    mod.setAlignment(Qt::AlignCenter);
    cursor.mergeBlockFormat(mod);
    cursor.insertText("Blah");
    QCOMPARE(cursor.blockFormat().alignment(), Qt::AlignCenter);
    QTextDocumentFragment frag(doc);
    cleanup();
    init();
    cursor.insertFragment(frag);
    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.blockFormat().alignment(), Qt::AlignCenter);
}

void tst_QTextDocumentFragment::dontRemoveInitialBlockIfItHoldsObjectIndexedCharFormat()
{
    const char html[] = "<table><tr><td>cell one<td>cell two</tr><tr><td>cell three<td>cell four</tr></table>";
    QCOMPARE(doc->begin().charFormat().objectIndex(), -1);
    setHtml(QString::fromLatin1(html));
    int cnt = 0;

    int objectIndexOfLast = -1;
    for (QTextBlock blk = doc->begin(); blk.isValid(); blk = blk.next()) {
        ++cnt;
        objectIndexOfLast = blk.charFormat().objectIndex();
    }
    //   beginning of frame for first cell
    // + beginning of frame for second cell
    // + beginning of frame for third cell
    // + beginning of frame for fourth cell
    // + end of frame
    // + initial block
    // ==> 6
    QCOMPARE(cnt, 6);
    QVERIFY(objectIndexOfLast != -1);
    QVERIFY(doc->begin().next().charFormat().objectIndex() != -1);
}

void tst_QTextDocumentFragment::dosLineFeed()
{
    const char html[] = "<pre>Test\r\n</pre>Bar";
    setHtml(QString::fromLatin1(html));
    QVERIFY(!doc->toPlainText().contains('\r'));
    QCOMPARE(doc->toPlainText(), QString("Test\nBar"));
}

void tst_QTextDocumentFragment::unorderedListEnumeration()
{
    const char html[] = "<ul><ul><ul><li>Blah</li></ul></ul>";
    setHtml(QString::fromLatin1(html));
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListDisc);

    const char html2[] = "<ul><ul><ul type=circle><li>Blah</li></ul></ul>";
    setHtml(QString::fromLatin1(html2));
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListCircle);

}

void tst_QTextDocumentFragment::resetHasBlockAfterClosedBlockTags()
{
    // when closing tags we have to make sure hasBlock in import() gets reset
    const char html[] = "<body><table><tr><td><td><p></table><p></body>";
    setHtml(QString::fromLatin1(html));
    QVERIFY(!doc->isEmpty());
}

void tst_QTextDocumentFragment::ignoreStyleTags()
{
    const char html[] = "<body><style>Blah</style>Hello</body>";
    setHtml(QString::fromLatin1(html));
    QCOMPARE(doc->toPlainText(), QString("Hello"));
}

void tst_QTextDocumentFragment::hrefAnchor()
{
    {
        const char html[] = "<a href=\"test\">blah</a>";
        setHtml(QString::fromLatin1(html));
        QVERIFY(doc->begin().begin().fragment().charFormat().isAnchor());
        QCOMPARE(doc->begin().begin().fragment().charFormat().anchorHref(), QString::fromLatin1("test"));
        QVERIFY(doc->begin().begin().fragment().charFormat().fontUnderline());
    }

    {
        // only hyperlinks should have special formatting
        const char html[] = "<a>blah</a>";
        setHtml(QString::fromLatin1(html));
        QVERIFY(doc->begin().begin().fragment().charFormat().isAnchor());
        QVERIFY(!doc->begin().begin().fragment().charFormat().fontUnderline());
    }
}

void tst_QTextDocumentFragment::namedAnchorFragments()
{
    // named anchors should be 'invisible', but the fragment right after it should
    // hold the attribute
    const char html[] = "a<a name=\"test\" />blah";
    setHtml(QString::fromLatin1(html));

    QTextBlock firstBlock = doc->begin();
    QVERIFY(firstBlock.isValid());

    QTextBlock::Iterator it = firstBlock.begin();
    QVERIFY(!it.atEnd());

    // the 'a'
    QVERIFY(it.fragment().isValid());
    QCOMPARE(it.fragment().text(), QString::fromLatin1("a"));
    QVERIFY(!it.fragment().charFormat().isAnchor());

    // the 'b' of 'blah' as separate fragment with the anchor attribute
    ++it;
    QVERIFY(it.fragment().isValid());
    QCOMPARE(it.fragment().text(), QString::fromLatin1("b"));
    QVERIFY(it.fragment().charFormat().isAnchor());

    // the 'lah' of 'blah' as remainder
    ++it;
    QVERIFY(it.fragment().isValid());
    QVERIFY(it.fragment().text().startsWith("lah"));
    QVERIFY(!it.fragment().charFormat().isAnchor());
}

void tst_QTextDocumentFragment::namedAnchorFragments2()
{
    const char html[] = "<p>    <a name=\"foo\"> Hello";
    setHtml(QString::fromLatin1(html));

    QCOMPARE(doc->toPlainText(), QString("Hello"));

    QTextBlock::Iterator it = doc->begin().begin();
    QVERIFY(!it.atEnd());

    QCOMPARE(it.fragment().text(), QString::fromLatin1("H"));
    QVERIFY(it.fragment().charFormat().isAnchor());

    ++it;

    QCOMPARE(it.fragment().text(), QString::fromLatin1("ello"));
    QVERIFY(!it.fragment().charFormat().isAnchor());
}

void tst_QTextDocumentFragment::namedAnchorFragments3()
{
    setHtml("<a name=\"target\" /><a name=\"target2\"/><span>Text</span>");

    QCOMPARE(doc->toPlainText(), QString("Text"));

    QTextBlock::Iterator it = doc->begin().begin();
    QVERIFY(!it.atEnd());

    QCOMPARE(it.fragment().text(), QString::fromLatin1("T"));
    QVERIFY(it.fragment().charFormat().isAnchor());
    QCOMPARE(it.fragment().charFormat().anchorNames().constFirst(), QLatin1String("target"));
    QStringList targets; targets << "target" << "target2";
    QCOMPARE(it.fragment().charFormat().anchorNames(), targets);

    ++it;

    QCOMPARE(it.fragment().text(), QString::fromLatin1("ext"));
    QVERIFY(!it.fragment().charFormat().isAnchor());
}

void tst_QTextDocumentFragment::dontInheritAlignmentInTables()
{
    const char html[] = "<table align=center><tr><td>Hey</td></tr></table>";
    setHtml(QString::fromLatin1(html));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());
    QVERIFY(cursor.currentTable()->cellAt(0, 0).isValid());
    QVERIFY(cursor.currentTable()->cellAt(0, 0).firstCursorPosition().block().next().blockFormat().alignment() != Qt::AlignHCenter);
}

void tst_QTextDocumentFragment::cellBlockCount()
{
    const char html[] = "<table><tr><td>Hey</td></tr></table>";
    setHtml(QString::fromLatin1(html));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());

    QTextTableCell cell = cursor.currentTable()->cellAt(0, 0);
    QVERIFY(cell.isValid());

    int blockCount = 0;
    for (QTextFrame::iterator it = cell.begin(); !it.atEnd(); ++it) {
        QVERIFY(!it.currentFrame());
        QVERIFY(it.currentBlock().isValid());
        ++blockCount;
    }
    QCOMPARE(blockCount, 1);
}

void tst_QTextDocumentFragment::cellBlockCount2()
{
    const char html[] = "<table><tr><td><p>Hey</p></td></tr></table>";
    setHtml(QString::fromLatin1(html));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());

    QTextTableCell cell = cursor.currentTable()->cellAt(0, 0);
    QVERIFY(cell.isValid());

    int blockCount = 0;
    for (QTextFrame::iterator it = cell.begin(); !it.atEnd(); ++it) {
        QVERIFY(!it.currentFrame());
        QVERIFY(it.currentBlock().isValid());
        ++blockCount;
    }
    QCOMPARE(blockCount, 1);
}

void tst_QTextDocumentFragment::emptyTable()
{
    const char html[] = "<table></table>";
    setHtml(QString::fromLatin1(html));
    QVERIFY(true); // don't crash with a failing assertion
}

void tst_QTextDocumentFragment::emptyTable2()
{
    const char html[] = "<table></td></tr></table><p>blah</p>";
    setHtml(QString::fromLatin1(html));
    QVERIFY(true); // don't crash with a failing assertion
}

void tst_QTextDocumentFragment::emptyTable3()
{
    const char html[] = "<table><tr><td><table></table></td><td>Foobar</td></tr></table>";
    setHtml(QString::fromLatin1(html));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 2);
    QTextTableCell cell = table->cellAt(0, 0);
    QVERIFY(cell.isValid());
    QCOMPARE(cell.firstPosition(), cell.lastPosition());
    cell = table->cellAt(0, 1);
    QTextCursor cursor = cell.firstCursorPosition();
    cursor.setPosition(cell.lastPosition(), QTextCursor::KeepAnchor);
    QCOMPARE(cursor.selectedText(), QString("Foobar"));
}

void tst_QTextDocumentFragment::doubleRowClose()
{
    const char html[] = "<table><tr><td>Blah</td></tr></tr><tr><td>Hm</td></tr></table>";
    setHtml(QString::fromLatin1(html));
    QVERIFY(true); // don't crash with a failing assertion
}

void tst_QTextDocumentFragment::mayNotHaveChildren()
{
    // make sure the Hey does not end up as tag text for the img tag
    const char html[] = "<img />Hey";
    setHtml(QString::fromLatin1(html));
    QCOMPARE(doc->toPlainText().mid(1), QString::fromLatin1("Hey"));
}

void tst_QTextDocumentFragment::inheritAlignment()
{
    // make sure attributes from the body tag get inherited
    const char html[] = "<body align=right><p>Hey";
    setHtml(QString::fromLatin1(html));
    // html alignment is absolute
    QCOMPARE(doc->begin().blockFormat().alignment(), Qt::Alignment(Qt::AlignRight|Qt::AlignAbsolute));
}

void tst_QTextDocumentFragment::dontEmitEmptyNodeWhenEmptyTagIsFollowedByCloseTag()
{
    // make sure the Hey does not end up as tag text for the img tag
    const char html[] = "<body align=right><p align=left>Blah<img></img><p>Hey";
    setHtml(QString::fromLatin1(html));
    QCOMPARE(doc->begin().blockFormat().alignment(), Qt::Alignment(Qt::AlignLeft|Qt::AlignAbsolute));
    QCOMPARE(doc->begin().next().blockFormat().alignment(), Qt::Alignment(Qt::AlignRight|Qt::AlignAbsolute));
}

void tst_QTextDocumentFragment::toPlainText()
{
    QString input = "Hello\nWorld";
    input += QChar::ParagraphSeparator;
    input += "Blah";
    doc->setPlainText(input);
    QCOMPARE(doc->blockCount(), 3);
}

void tst_QTextDocumentFragment::copyTableRow()
{
    QTextDocumentFragment frag;
    {
        QTextTable *table = cursor.insertTable(2, 2);
        table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
        table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
        table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
        table->cellAt(1, 1).firstCursorPosition().insertText("Hah");

        // select second row
        cursor = table->cellAt(1, 1).firstCursorPosition();
        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);

        QCOMPARE(table->cellAt(cursor.position()).row(), 1);
        QCOMPARE(table->cellAt(cursor.position()).column(), 0);
        QCOMPARE(table->cellAt(cursor.anchor()).row(), 1);
        QCOMPARE(table->cellAt(cursor.anchor()).column(), 1);

        frag = QTextDocumentFragment(cursor);
    }
    {
        QTextDocument doc2;
        cursor = QTextCursor(&doc2);
        cursor.insertFragment(frag);

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();

        QVERIFY(table);
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 1);

        QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Bar"));
        QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Hah"));
    }
}

void tst_QTextDocumentFragment::copyTableColumn()
{
    QTextDocumentFragment frag;
    {
        QTextTable *table = cursor.insertTable(2, 2);
        table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
        table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
        table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
        table->cellAt(1, 1).firstCursorPosition().insertText("Hah");

        // select second column
        cursor = table->cellAt(0, 1).firstCursorPosition();
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);

        QCOMPARE(table->cellAt(cursor.anchor()).row(), 0);
        QCOMPARE(table->cellAt(cursor.anchor()).column(), 1);
        QCOMPARE(table->cellAt(cursor.position()).row(), 1);
        QCOMPARE(table->cellAt(cursor.position()).column(), 1);

        frag = QTextDocumentFragment(cursor);
    }
    {
        QTextDocument doc2;
        cursor = QTextCursor(&doc2);
        cursor.insertFragment(frag);

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();

        QVERIFY(table);
        QCOMPARE(table->columns(), 1);
        QCOMPARE(table->rows(), 2);

        QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Foo"));
        QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Hah"));
    }
}

void tst_QTextDocumentFragment::copySubTable()
{
    QTextDocumentFragment frag;
    {
        QTextTableFormat fmt;
        QVector<QTextLength> constraints;
        constraints << QTextLength(QTextLength::PercentageLength, 16);
        constraints << QTextLength(QTextLength::PercentageLength, 28);
        constraints << QTextLength(QTextLength::PercentageLength, 28);
        constraints << QTextLength(QTextLength::PercentageLength, 28);
        fmt.setColumnWidthConstraints(constraints);

        QTextTable *table = cursor.insertTable(4, 4, fmt);
        for (int row = 0; row < 4; ++row) {
            const QString rowS = QString::number(row) + QLatin1Char('/');
            for (int col = 0; col < 4; ++col)
                table->cellAt(row, col).firstCursorPosition().insertText(rowS + QString::number(col));
        }

        QCOMPARE(table->format().columnWidthConstraints().count(), table->columns());

        // select 2x2 subtable
        cursor = table->cellAt(1, 1).firstCursorPosition();
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);

        QCOMPARE(table->cellAt(cursor.anchor()).row(), 1);
        QCOMPARE(table->cellAt(cursor.anchor()).column(), 1);
        QCOMPARE(table->cellAt(cursor.position()).row(), 2);
        QCOMPARE(table->cellAt(cursor.position()).column(), 2);

        frag = QTextDocumentFragment(cursor);
    }
    {
        QTextDocument doc2;
        cursor = QTextCursor(&doc2);
        cursor.insertFragment(frag);

        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QTextTable *table = cursor.currentTable();

        QVERIFY(table);
        QVERIFY(table->format().columnWidthConstraints().isEmpty());
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->rows(), 2);

        QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("1/1"));
        QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("1/2"));
        QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("2/1"));
        QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("2/2"));
    }
}

void tst_QTextDocumentFragment::html_textDecoration()
{
    const char html[] = "<span style='text-decoration: overline line-through underline'>Blah</span>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QByteArray::fromRawData(html, sizeof(html) / sizeof(html[0]))));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().fontUnderline());
    QVERIFY(cursor.charFormat().fontOverline());
    QVERIFY(cursor.charFormat().fontStrikeOut());
}

void tst_QTextDocumentFragment::html_infiniteLoop()
{
    {
        // used to cause an infinite loop due to the lack of a space after the
        // tag name
        const char html[] = "<ahref=\"argl\">Link</a>";
        cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
        QVERIFY(true);
    }

    {
        const char html[] = "<a href=\"\"a<";
        cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
        QVERIFY(true);
    }
}

void tst_QTextDocumentFragment::html_blockIndent()
{
    const char html[] = "<p style=\"-qt-block-indent:3;\">Test</p>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
    QCOMPARE(cursor.blockFormat().indent(), 3);
}

void tst_QTextDocumentFragment::html_listIndent()
{
    const char html[] = "<ul style=\"-qt-list-indent:4;\"><li>Blah</ul>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().indent(), 4);
}

void tst_QTextDocumentFragment::html_whitespace_data()
{
    QTest::addColumn<QString>("html");
    QTest::addColumn<QString>("expectedPlainText");

    QTest::newRow("1") << QString("<span>This is some test</span><span> with spaces between words</span>")
                       << QString("This is some test with spaces between words");

    QTest::newRow("2") << QString("<span>  </span><span>nowhitespacehereplease</span>")
                       << QString::fromLatin1("nowhitespacehereplease");

    QTest::newRow("3") << QString("<span style=\"white-space: pre;\">  white  space  \n\n  here  </span>")
                       << QString::fromLatin1("  white  space  \n\n  here  ");

    QTest::newRow("4") << QString("<span style=\"white-space: pre-wrap;\">  white  space  \n\n  here  </span>")
                       << QString::fromLatin1("  white  space  \n\n  here  ");

    QTest::newRow("5") << QString("<a href=\"One.html\">One</a> <a href=\"Two.html\">Two</a> <b>Three</b>\n"
                                  "<b>Four</b>")
                       << QString::fromLatin1("One Two Three Four");

    QTest::newRow("6") << QString("<p>Testing:     <b><i><u>BoldItalic</u></i></b> <i>Italic</i></p>")
                       << QString("Testing: BoldItalic Italic");

    QTest::newRow("7") << QString("<table><tr><td>Blah</td></tr></table> <table border><tr><td>Foo</td></tr></table>")
                       << QString("\nBlah\n\nFoo\n");

    QTest::newRow("8") << QString("<table><tr><td><i>Blah</i></td></tr></table> <i>Blub</i>")
                       << QString("\nBlah\nBlub");

    QTest::newRow("9") << QString("<span style=\"white-space: nowrap;\">  white  space  \n\n  here  </span>")
                       << QString::fromLatin1("white space here ");

    QTest::newRow("10") << QString("<span style=\"white-space: pre-line;\">  white  space  \n\n  here  </span>")
                        << QString::fromLatin1("white space\n\nhere ");

    QTest::newRow("task116492") << QString("<p>a<font=\"Times\"> b </font>c</p>")
                                << QString("a b c");

    QTest::newRow("task121653") << QString("abc<b> def</b>")
                                << QString("abc def");

    QTest::newRow("task122650") << QString("<p>Foo</p>    Bar")
                                << QString("Foo\nBar");

    QTest::newRow("task122650-2") << QString("<p>Foo</p>  <p>  Bar")
                                  << QString("Foo \nBar");

    QTest::newRow("task122650-3") << QString("<html>Before<pre>\nTest</pre>")
                                  << QString("Before\nTest");

    QTest::newRow("br-with-whitespace") << QString("Foo<br>\nBlah")
                                        << QString("Foo\nBlah");

    QTest::newRow("collapse-p-with-newline") << QString("Foo<p>\n<p>\n<p>\n<p>\n<p>\n<p>\nBar")
            << QString("Foo\nBar");

    QTest::newRow("table") << QString("<table><tr><td>Blah</td></tr></table>\nTest")
                           << QString("\nBlah\nTest");

    QTest::newRow("table2") << QString("<table><tr><td><pre>\nTest\n</pre></td>\n </tr></table>")
                            << QString("\nTest\n");

    QTest::newRow("table3") << QString("<table><tr><td><pre>\nTest\n</pre> \n \n </td></tr></table>")
                            << QString("\nTest \n");
}

void tst_QTextDocumentFragment::html_whitespace()
{
    QFETCH(QString, html);
    QFETCH(QString, expectedPlainText);

    setHtml(html);

    QCOMPARE(doc->toPlainText(), expectedPlainText);
}

void tst_QTextDocumentFragment::html_qt3Whitespace()
{
    QString text = "This     text       has         some   whitespace"
                   "\n and \nnewlines that \n should be ignored\n\n";
    const QString html = QString("<html><head><meta name=\"qrichtext\" content=\"1\" /></head><body>")
                         + text
                         + QString("</body></html>");

    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    text.remove(QChar::fromLatin1('\n'));

    QCOMPARE(doc->toPlainText(), text);
}

void tst_QTextDocumentFragment::html_qt3WhitespaceWithFragments()
{
    QString text = "This     text       has         some   whitespace"
                   "\n and \nnewlines that \n should be ignored\n\n";
    const QString html = QString("<html><head><meta name=\"qrichtext\" content=\"1\" /></head><body>"
                                 "blah blah<!--StartFragment--><span>")
                         + text
                         + QString("</span><!--EndFragment--></body></html>");

    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    text.remove(QChar::fromLatin1('\n'));

    QCOMPARE(doc->toPlainText(), text);
}

void tst_QTextDocumentFragment::html_qt3WhitespaceAfterTags()
{
    QString text = "    This     text       has         some   whitespace   ";
    const QString html = QString("<html><head><meta name=\"qrichtext\" content=\"1\" /></head><body><span>")
                         + text
                         + QString("</span></body></html>");

    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    QCOMPARE(doc->toPlainText(), text);
}

void tst_QTextDocumentFragment::html_listStart1()
{
    // don't create a block for the <ul> element, even if there's some whitespace between
    // it and the <li>
    const QString html = QStringLiteral("<ul>        <li>list item</li><ul>");
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    QCOMPARE(doc->blockCount(), 1);
}

void tst_QTextDocumentFragment::html_listStart2()
{
    // unlike with html_listStart1 we want a block showing the 'buggy' text here
    const QString html = QStringLiteral("<ul>buggy, but text should appear<li>list item</li><ul>");
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    QCOMPARE(doc->blockCount(), 2);
}

void tst_QTextDocumentFragment::html_cssMargin()
{
    const char html[] = "<p style=\"margin-top: 1px; margin-bottom: 2px; margin-left: 3px; margin-right: 4px\">Test</p>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
    const QTextBlockFormat fmt = cursor.blockFormat();
    QCOMPARE(fmt.topMargin(), qreal(1));
    QCOMPARE(fmt.bottomMargin(), qreal(2));
    QCOMPARE(fmt.leftMargin(), qreal(3));
    QCOMPARE(fmt.rightMargin(), qreal(4));
}

void tst_QTextDocumentFragment::html_hexEntities()
{
    const char html[] = "&#x00040;";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
    QCOMPARE(doc->begin().begin().fragment().text(), QString("@"));
}

void tst_QTextDocumentFragment::html_decEntities()
{
    const char html[] = "&#64;";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));
    QCOMPARE(doc->begin().begin().fragment().text(), QString("@"));
}

void tst_QTextDocumentFragment::html_thCentered()
{
    const char html[] = "<table><tr><th>This should be centered</th></tr></table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    cursor.movePosition(QTextCursor::PreviousBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QVERIFY(table->cellAt(0, 0).begin().currentBlock().blockFormat().alignment() == Qt::AlignCenter);
}

void tst_QTextDocumentFragment::orderedListNumbering()
{
    // Supporter issue 45941 - make sure _two_ separate lists
    // are imported, which have their own numbering
    const char html[] = "<html><body>"
                        "<ol><li>elem 1</li></ol>"
                        "<ol><li>elem 1</li></ol>"
                        "</body></html>";

    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    int numberOfLists = 0;

    cursor.movePosition(QTextCursor::Start);
    QTextList *lastList = 0;
    do {
        QTextList *list = cursor.currentList();
        if (list && list != lastList) {
            lastList = list;
            ++numberOfLists;
        }
    } while (cursor.movePosition(QTextCursor::NextBlock));

    QCOMPARE(numberOfLists, 2);
}

void tst_QTextDocumentFragment::html_blockAfterList()
{
    const char html[] = "<ul><li>Foo</ul>This should be a separate paragraph and not be indented at the same level as Foo";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(html));

    cursor.movePosition(QTextCursor::Start);

    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().indent(), 1);

    QVERIFY(cursor.movePosition(QTextCursor::NextBlock));
    QVERIFY(!cursor.currentList());
    QCOMPARE(cursor.blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_subAndSuperScript()
{
    const char subHtml[] = "<sub>Subby</sub>";
    const char superHtml[] = "<sup>Super</sup>";
    const char subHtmlCss[] = "<span style=\"vertical-align: sub\">Subby</span>";
    const char superHtmlCss[] = "<span style=\"vertical-align: super\">Super</span>";
    const char alignmentInherited[] = "<sub><font face=\"Verdana\">Subby</font></sub>";

    setHtml(subHtml);
    QCOMPARE(cursor.charFormat().verticalAlignment(), QTextCharFormat::AlignSubScript);

    setHtml(subHtmlCss);
    QCOMPARE(cursor.charFormat().verticalAlignment(), QTextCharFormat::AlignSubScript);

    setHtml(superHtml);
    QCOMPARE(cursor.charFormat().verticalAlignment(), QTextCharFormat::AlignSuperScript);

    setHtml(superHtmlCss);
    QCOMPARE(cursor.charFormat().verticalAlignment(), QTextCharFormat::AlignSuperScript);

    setHtml(alignmentInherited);
    QCOMPARE(cursor.charFormat().verticalAlignment(), QTextCharFormat::AlignSubScript);
}

void tst_QTextDocumentFragment::html_cssColors()
{
    const char color[] = "<span style=\"color:red\"><span style=\"color:blue\">Blue</span></span>";
    setHtml(color);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::blue);

    const char rgbColor[] = "<span style=\"color:red\"><span style=\"color:rgb(0, 0, 255)\">Blue</span></span>";
    setHtml(rgbColor);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::blue);
}

void tst_QTextDocumentFragment::obeyFragmentMarkersInImport()
{
    const char html[] = "This leading text should not appear<!--StartFragment--><span>Text</span><!--EndFragment-->This text at the end should not appear";
    setHtml(html);

    QCOMPARE(doc->toPlainText(), QString("Text"));
}

void tst_QTextDocumentFragment::whitespaceWithFragmentMarkers()
{
    QString text("    text with leading and trailing whitespace    ");
    const char html[] = "This leading text should not appear<!--StartFragment-->%1<!--EndFragment-->This text at the end should not appear";
    setHtml(QString::fromLatin1(html).arg(text));

    QString expected("text with leading and trailing whitespace ");
    QCOMPARE(doc->toPlainText(), expected);
}

void tst_QTextDocumentFragment::html_emptyParapgraphs1()
{
    const char html[] = "<p style=\"-qt-paragraph-type:empty;\">&nbsp;</p><p>Two paragraphs</p>";
    setHtml(html);

    QCOMPARE(doc->blockCount(), 2);
    QVERIFY(doc->begin().text().isEmpty());
    QCOMPARE(doc->begin().next().text(), QString("Two paragraphs"));
}

void tst_QTextDocumentFragment::html_emptyParapgraphs2()
{
    const char html[] = "<p style=\"margin-left:80px\"></p><p>One paragraph</p>";
    setHtml(html);

    QCOMPARE(doc->blockCount(), 1);
    QCOMPARE(cursor.blockFormat().leftMargin(), qreal(0));

    const char html2[] = "<p style=\"margin-left:80px\"></p>One paragraph";
    setHtml(html2);
    QCOMPARE(doc->blockCount(), 1);
    QCOMPARE(cursor.blockFormat().leftMargin(), qreal(0));

    const char html3[] = "<p style=\"margin-left:80px\">Foo</p><p></p>Two paragraphs";
    setHtml(html3);
    QCOMPARE(doc->blockCount(), 2);
    cursor = QTextCursor(doc);
    QCOMPARE(cursor.blockFormat().leftMargin(), qreal(80));
    QCOMPARE(cursor.block().next().blockFormat().leftMargin(), qreal(0));
}

void tst_QTextDocumentFragment::html_emptyParagraphs3()
{
    const char html[] = "<ul><p>Foo</p><p></p></ul><h4>Bar</h4>";

    setHtml(html);

    QCOMPARE(doc->blockCount(), 2);

    cursor = QTextCursor(doc);
    QCOMPARE(cursor.block().next().blockFormat().indent(), 0);
}

void tst_QTextDocumentFragment::html_emptyParagraphs4()
{
    const char html[] = "<p>foo</p><p style=\"page-break-before: always\"></p><p>bar</p>";
    setHtml(html);

    QTextBlock block = doc->begin();
    QVERIFY(block.isValid());
    QCOMPARE(block.text(), QString("foo"));
    block = block.next();
    QVERIFY(block.isValid());
    QTextBlockFormat bf = block.blockFormat();
    QVERIFY(bf.hasProperty(QTextFormat::PageBreakPolicy));
    QCOMPARE(bf.pageBreakPolicy(), QTextFormat::PageBreak_AlwaysBefore);
    QCOMPARE(block.text(), QString("bar"));

    const char html2[] = "<p>foo</p><p style=\"page-break-after: always\"></p><p>bar</p>";
    setHtml(html2);

    block = doc->begin();
    QVERIFY(block.isValid());
    QCOMPARE(block.text(), QString("foo"));
    block = block.next();
    QVERIFY(block.isValid());
    bf = block.blockFormat();
    QVERIFY(bf.hasProperty(QTextFormat::PageBreakPolicy));
    QCOMPARE(bf.pageBreakPolicy(), QTextFormat::PageBreak_AlwaysBefore); // after the empty line means it should appear for 'bar'
    QCOMPARE(block.text(), QString("bar"));
}

void tst_QTextDocumentFragment::html_font()
{
    const char html[] = "<font color=\"blue\"><p>Hah</p></font>";
    setHtml(html);

    QVERIFY(cursor.charFormat().foreground().color() == Qt::blue);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::blue);
}

void tst_QTextDocumentFragment::html_fontSize()
{
    const char html[] = "<font size=\"2\">Hah</font>";
    setHtml(html);

    QCOMPARE(cursor.charFormat().property(QTextFormat::FontSizeAdjustment).toInt(), -1);
}

void tst_QTextDocumentFragment::html_fontSizeAdjustment()
{
    const char html[] = "<font size=\"7\"><b>Hah</b></font>";
    setHtml(html);

    QCOMPARE(cursor.charFormat().property(QTextFormat::FontSizeAdjustment).toInt(), 4);
    QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
}

void tst_QTextDocumentFragment::html_cssFontSize()
{
    const char html[] = "<span style=\"font-size: 50pt\">Foo</span>";
    setHtml(html);

    QCOMPARE(cursor.charFormat().property(QTextFormat::FontPointSize).toInt(), 50);

    const char html2[] = "<span style=\"font-size: 50px\">Foo</span>";
    setHtml(html2);

    QCOMPARE(cursor.charFormat().property(QTextFormat::FontPixelSize).toInt(), 50);

    const char html3[] = "<span style=\"font-size: large\">Foo</span>";
    setHtml(html3);

    QCOMPARE(cursor.charFormat().property(QTextFormat::FontSizeAdjustment).toInt(), 1);
}

void tst_QTextDocumentFragment::html_cssShorthandFont()
{
    {
        const char html[] = "<span style=\"font: 50px sans-serif\">Foo</span>";
        setHtml(html);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontPixelSize).toInt(), 50);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontFamily).toString(), QString("sans-serif"));
    }
    {
        const char html[] = "<span style=\"font: 50pt sans-serif\">Foo</span>";
        setHtml(html);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontPointSize).toInt(), 50);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontFamily).toString(), QString("sans-serif"));
    }
    {
        const char html[] = "<span style='font:7.0pt \"Times New Roman\"'>Foo</span>";
        setHtml(html);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontPointSize).toInt(), 7);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontFamily).toString(), QString("Times New Roman"));
    }
    {
        const char html[] = "<span style='font:bold 7.0pt'>Foo</span>";
        setHtml(html);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontWeight).toInt(), int(QFont::Bold));
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontPointSize).toInt(), 7);
    }
    {
        const char html[] = "<span style='font:bold italic 7.0pt'>Foo</span>";
        setHtml(html);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontWeight).toInt(), int(QFont::Bold));
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontItalic).toBool(), true);
    }
}

void tst_QTextDocumentFragment::html_bodyBgColor()
{
    const char html[] = "<body bgcolor=\"blue\">Foo</body>";
    doc->setHtml(html);

    QVERIFY(doc->rootFrame()->frameFormat().background().color() == Qt::blue);
}

void tst_QTextDocumentFragment::html_qtBgColor()
{
    const char html[] = "<qt bgcolor=\"blue\">Foo</qt>";
    doc->setHtml(html);

    QVERIFY(doc->rootFrame()->frameFormat().background().color() == Qt::blue);
}

void tst_QTextDocumentFragment::html_bodyBackground()
{
    const char html[] = "<body background=\"foo.png\">Foo</body>";
    doc->setHtml(html);

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on winrt. Investigate - QTBUG-68297", Continue);
#endif
    QCOMPARE(doc->rootFrame()->frameFormat().background().style(), Qt::TexturePattern);
}

void tst_QTextDocumentFragment::html_tableCellBackground()
{
    const char html[] = "<body><table><tr><td background=\"foo.png\">Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on winrt. Investigate - QTBUG-68297", Continue);
#endif
    QCOMPARE(cell.format().background().style(), Qt::TexturePattern);
}

void tst_QTextDocumentFragment::css_bodyBackground()
{
    const char html[] = "<body style=\"background-image:url('foo.png')\">Foo</body>";
    doc->setHtml(html);

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on winrt. Investigate - QTBUG-68297", Continue);
#endif
    QCOMPARE(doc->rootFrame()->frameFormat().background().style(), Qt::TexturePattern);
}

void tst_QTextDocumentFragment::css_tableCellBackground()
{
    const char html[] = "<body><table><tr><td style=\"background-image:url('foo.png')\">Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on winrt. Investigate - QTBUG-68297", Continue);
#endif
    QCOMPARE(cell.format().background().style(), Qt::TexturePattern);
}

void tst_QTextDocumentFragment::css_tableCellBorder()
{
    const char html[] = "<body><table><tr><td style=\"border-width:8px;border-color:green;border-style:groove;border-left-style:dashed;border-left-color:red;border-left-width:4px\">Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    QCOMPARE(cellFormat.leftBorder(), qreal(4));
    QCOMPARE(cellFormat.leftBorderBrush(), QBrush(QColor("red")));
    QCOMPARE(cellFormat.leftBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.rightBorder(), qreal(8));
    QCOMPARE(cellFormat.rightBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.rightBorderStyle(), QTextFrameFormat::BorderStyle_Groove);

    QCOMPARE(cellFormat.bottomBorder(), qreal(8));
    QCOMPARE(cellFormat.bottomBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.bottomBorderStyle(), QTextFrameFormat::BorderStyle_Groove);

    QCOMPARE(cellFormat.topBorder(), qreal(8));
    QCOMPARE(cellFormat.topBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.topBorderStyle(), QTextFrameFormat::BorderStyle_Groove);
}

void tst_QTextDocumentFragment::css_tableCellBorderWidthOneValue() // QTBUG-80496
{
    const char html[] = "<head><style type=\"text/css\"> body, td { border-width: 2px; }</style></head> <body> <table> <tr> <td></td> </tr> </table> </body> </html>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    QCOMPARE(cellFormat.leftBorder(), qreal(2));
    QCOMPARE(cellFormat.rightBorder(), qreal(2));
    QCOMPARE(cellFormat.bottomBorder(), qreal(2));
    QCOMPARE(cellFormat.topBorder(), qreal(2));
}

void tst_QTextDocumentFragment::css_tableCellBorderWidthTwoValues() // QTBUG-80496
{
    const char html[] = "<head><style type=\"text/css\"> body, td { border-width: 2px 3px; }</style></head> <body> <table> <tr> <td></td> </tr> </table> </body> </html>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    QCOMPARE(cellFormat.leftBorder(), qreal(3));
    QCOMPARE(cellFormat.rightBorder(), qreal(3));
    QCOMPARE(cellFormat.bottomBorder(), qreal(2));
    QCOMPARE(cellFormat.topBorder(), qreal(2));
}

void tst_QTextDocumentFragment::css_tableCellBorderShorthand()
{
    const char html[] = "<body><table><tr><td style=\"border-left:1px solid green;border-right:2px dashed red;border-bottom:3px dotted yellow;border-top:4px dot-dash blue\">Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    QCOMPARE(cellFormat.leftBorder(), qreal(1));
    QCOMPARE(cellFormat.leftBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.leftBorderStyle(), QTextFrameFormat::BorderStyle_Solid);

    QCOMPARE(cellFormat.rightBorder(), qreal(2));
    QCOMPARE(cellFormat.rightBorderBrush(), QBrush(QColor("red")));
    QCOMPARE(cellFormat.rightBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.bottomBorder(), qreal(3));
    QCOMPARE(cellFormat.bottomBorderBrush(), QBrush(QColor("yellow")));
    QCOMPARE(cellFormat.bottomBorderStyle(), QTextFrameFormat::BorderStyle_Dotted);

    QCOMPARE(cellFormat.topBorder(), qreal(4));
    QCOMPARE(cellFormat.topBorderBrush(), QBrush(QColor("blue")));
    QCOMPARE(cellFormat.topBorderStyle(), QTextFrameFormat::BorderStyle_DotDash);
}

void tst_QTextDocumentFragment::css_tableCellAllBordersShorthand()
{
    const char html[] = "<body><table><tr><td style=\"border:2px dashed green\">Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    QCOMPARE(cellFormat.leftBorder(), qreal(2));
    QCOMPARE(cellFormat.leftBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.leftBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.rightBorder(), qreal(2));
    QCOMPARE(cellFormat.rightBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.rightBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.bottomBorder(), qreal(2));
    QCOMPARE(cellFormat.bottomBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.bottomBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.topBorder(), qreal(2));
    QCOMPARE(cellFormat.topBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.topBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);
}

void tst_QTextDocumentFragment::css_tableCellOverrideOneBorder()
{
    const char html[] = "<body><table><tr><td style=\"border:2px dashed green;border-left:4px solid red\">Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    QCOMPARE(cellFormat.leftBorder(), qreal(4));
    QCOMPARE(cellFormat.leftBorderBrush(), QBrush(QColor("red")));
    QCOMPARE(cellFormat.leftBorderStyle(), QTextFrameFormat::BorderStyle_Solid);

    QCOMPARE(cellFormat.rightBorder(), qreal(2));
    QCOMPARE(cellFormat.rightBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.rightBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.bottomBorder(), qreal(2));
    QCOMPARE(cellFormat.bottomBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.bottomBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);

    QCOMPARE(cellFormat.topBorder(), qreal(2));
    QCOMPARE(cellFormat.topBorderBrush(), QBrush(QColor("green")));
    QCOMPARE(cellFormat.topBorderStyle(), QTextFrameFormat::BorderStyle_Dashed);
}

void tst_QTextDocumentFragment::css_tableBorderCollapse()
{
    const char html[] = "<body><table style=\"border-collapse:collapse\"><tr><td>Foo</td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QCOMPARE(table->format().borderCollapse(), true);
}

void tst_QTextDocumentFragment::css_cellPaddings()
{
    const char html[] = "<body><table><tr><td style=\"padding-left:1\">Foo</td>"
                        "<td style=\"padding-right:1\"></td><td style=\"padding-top:10\"></td>"
                        "<td style=\"padding-bottom:5\"></td><td style=\"padding:15\"></td></tr></table></body>";
    doc->setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QCOMPARE(cell.format().toTableCellFormat().leftPadding(), qreal(1));
    cell = table->cellAt(0, 1);
    QCOMPARE(cell.format().toTableCellFormat().rightPadding(), qreal(1));
    cell = table->cellAt(0, 2);
    QCOMPARE(cell.format().toTableCellFormat().topPadding(), qreal(10));
    cell = table->cellAt(0, 3);
    QCOMPARE(cell.format().toTableCellFormat().bottomPadding(), qreal(5));
    cell = table->cellAt(0, 4);
    QCOMPARE(cell.format().toTableCellFormat().leftPadding(), qreal(15));
    QCOMPARE(cell.format().toTableCellFormat().rightPadding(), qreal(15));
    QCOMPARE(cell.format().toTableCellFormat().topPadding(), qreal(15));
    QCOMPARE(cell.format().toTableCellFormat().bottomPadding(), qreal(15));
}

void tst_QTextDocumentFragment::css_whiteSpace_data()
{
    QTest::addColumn<QString>("htmlText");
    QTest::addColumn<bool>("nowrap");

    QTest::newRow("default") << QString("<p>Normal Text</p>") << false;
    QTest::newRow("white-space:nowrap") << QString("<p style=white-space:nowrap>Normal Text</p>") << true;
    QTest::newRow("white-space:pre") << QString("<p style=white-space:pre>Normal Text</p>") << true;
}

void tst_QTextDocumentFragment::css_whiteSpace()
{
    QFETCH(QString, htmlText);
    QFETCH(bool, nowrap);

    doc->setHtml(htmlText);
    QCOMPARE(doc->blockCount(), 1);
    QCOMPARE(doc->begin().blockFormat().nonBreakableLines(), nowrap);
}

void tst_QTextDocumentFragment::html_blockLevelDiv()
{
    const char html[] = "<div align=right><b>Hello World";
    setHtml(html);

    QCOMPARE(doc->begin().blockFormat().alignment(), Qt::AlignRight|Qt::AlignAbsolute);
    QCOMPARE(doc->begin().next(), doc->end());
}

void tst_QTextDocumentFragment::html_spanNesting()
{
    const char html[] = "<span style=\"color:black\">a<span style=\"color:red\">b<span style=\"color:black\">c</span></span>d</span>";
    setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground() == Qt::black);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground() == Qt::red);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground() == Qt::black);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground() == Qt::black);
}

void tst_QTextDocumentFragment::html_nestedLists()
{
    const char html[] = "<p><ul><li>Foo<ul><li>In nested list</li></ul></li><li>Last item</li></ul></p>";
    setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    QTextList *firstList = cursor.currentList();
    QVERIFY(firstList);
    QCOMPARE(firstList->format().indent(), 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QTextList *secondList = cursor.currentList();
    QVERIFY(secondList);
    QVERIFY(secondList != firstList);
    QCOMPARE(cursor.currentList()->format().indent(), 2);

    cursor.movePosition(QTextCursor::NextBlock);
    QTextList *thirdList = cursor.currentList();
    QVERIFY(thirdList);
    QCOMPARE(thirdList, firstList);
}

void tst_QTextDocumentFragment::noSpecialCharactersInPlainText()
{
    cursor.insertTable(2, 2);
    cursor.insertBlock();
    cursor.insertText(QString(QChar::LineSeparator));
    cursor.insertText(QString(QChar::Nbsp));

    QString plain = doc->toPlainText();
    QVERIFY(!plain.contains(QChar::ParagraphSeparator));
    QVERIFY(!plain.contains(QChar::Nbsp));
    QVERIFY(!plain.contains(QTextBeginningOfFrame));
    QVERIFY(!plain.contains(QTextEndOfFrame));
    QVERIFY(!plain.contains(QChar::LineSeparator));

    plain = QTextDocumentFragment(doc).toPlainText();
    QVERIFY(!plain.contains(QChar::ParagraphSeparator));
    QVERIFY(!plain.contains(QChar::Nbsp));
    QVERIFY(!plain.contains(QTextBeginningOfFrame));
    QVERIFY(!plain.contains(QTextEndOfFrame));
    QVERIFY(!plain.contains(QChar::LineSeparator));
}

void tst_QTextDocumentFragment::html_doNotInheritBackground()
{
    const char html[] = "<html><body bgcolor=\"blue\"><p>Blah</p></body></html>";
    doc->setHtml(html);

    for (QTextBlock block = doc->begin();
         block.isValid(); block = block.next()) {
        QVERIFY(!block.blockFormat().hasProperty(QTextFormat::BackgroundBrush));
    }

    QVERIFY(doc->rootFrame()->frameFormat().hasProperty(QTextFormat::BackgroundBrush));
    QVERIFY(doc->rootFrame()->frameFormat().background().color() == Qt::blue);
}

void tst_QTextDocumentFragment::html_inheritBackgroundToInlineElements()
{
    const char html[] = "<span style=\"background: blue\">Foo<span>Bar</span></span>";
    doc->setHtml(html);

    int fragmentCount = 0;

    QTextBlock block = doc->begin();
    for (QTextBlock::Iterator it = block.begin();
         !it.atEnd(); ++it, ++fragmentCount) {

        const QTextFragment fragment = it.fragment();
        if (fragmentCount == 0) {
            QCOMPARE(fragment.text(), QString("FooBar"));
            QVERIFY(fragment.charFormat().background().color() == Qt::blue);
        }
    }

    QCOMPARE(fragmentCount, 1);
}

void tst_QTextDocumentFragment::html_doNotInheritBackgroundFromBlockElements()
{
    const char html[] = "<p style=\"background: blue\"><span>Foo</span></span>";
    doc->setHtml(html);

    int fragmentCount = 0;

    QTextBlock block = doc->begin();
    for (QTextBlock::Iterator it = block.begin();
         !it.atEnd(); ++it, ++fragmentCount) {

        const QTextFragment fragment = it.fragment();
        if (fragmentCount == 0) {
            QCOMPARE(fragment.text(), QString("Foo"));
            QVERIFY(!fragment.charFormat().hasProperty(QTextFormat::BackgroundBrush));
        }
    }

    QCOMPARE(fragmentCount, 1);
}
void tst_QTextDocumentFragment::html_nobr()
{
    const QString input = "Blah Foo    Bar";
    const QString html = QString::fromLatin1("<html><body><p><nobr>") + input + QString::fromLatin1("</nobr></p></body></html>");
    setHtml(html);

    QString text = doc->begin().begin().fragment().text();
    QString expectedText = input;
    expectedText.replace(QRegularExpression("\\s+"), QString(QChar::Nbsp));
    QCOMPARE(text, expectedText);
}

void tst_QTextDocumentFragment::fromPlainText()
{
    QString plainText;
    plainText = "Hello\nWorld\r\nBlub";
    plainText += QChar::ParagraphSeparator;
    // TextEdit on OS 10 gives us OS 9 style linefeeds
    // when copy & pasteing multi-line plaintext.
    plainText += "OS9IsOldSchool\r";
    plainText += "Last Parag";

    doc->setPlainText(plainText);

    int blockCount = 0;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QVERIFY(!block.text().contains(QLatin1Char('\n')));
        QVERIFY(!block.text().contains(QLatin1Char('\r')));
        QVERIFY(!block.text().contains(QChar::ParagraphSeparator));

        if (blockCount == 0)
            QCOMPARE(block.text(), QString("Hello"));
        else if (blockCount == 1)
            QCOMPARE(block.text(), QString("World"));
        else if (blockCount == 2)
            QCOMPARE(block.text(), QString("Blub"));
        else if (blockCount == 3)
            QCOMPARE(block.text(), QString("OS9IsOldSchool"));
        else if (blockCount == 4)
            QCOMPARE(block.text(), QString("Last Parag"));


        ++blockCount;
    }

    QCOMPARE(blockCount, 5);
}

void tst_QTextDocumentFragment::fromPlainText2()
{
    doc->setPlainText("Hello World");
    QCOMPARE(QTextDocumentFragment(doc).toPlainText(), doc->toPlainText());
}

void tst_QTextDocumentFragment::html_closingImageTag()
{
    const char html[] = "<span style=\"font-size: 10pt\"><span style=\"font-size: 40pt\">Blah<img src=\"blah\"></img>Foo</span></span>";
    setHtml(html);

    int fragmentCount = 0;

    QTextBlock block = doc->begin();
    for (QTextBlock::Iterator it = block.begin();
         !it.atEnd(); ++it, ++fragmentCount) {

        const QTextFragment fragment = it.fragment();
        if (fragmentCount == 0) {
            QCOMPARE(fragment.text(), QString("Blah"));
            QCOMPARE(fragment.charFormat().fontPointSize(), qreal(40));
        } else if (fragmentCount == 1) {
            QCOMPARE(fragment.text(), QString(QChar::ObjectReplacementCharacter));
        } else if (fragmentCount == 2) {
            QCOMPARE(fragment.text(), QString("Foo"));
            QCOMPARE(fragment.charFormat().fontPointSize(), qreal(40));
        }
    }

    QCOMPARE(fragmentCount, 3);
}

void tst_QTextDocumentFragment::html_emptyDocument()
{
    const char html[] = "<html><body><p style=\"-qt-paragraph-type:empty;\"></p></body></html>";
    setHtml(html);
    QCOMPARE(doc->blockCount(), 1);
}

void tst_QTextDocumentFragment::html_closingTag()
{
    const char html[] = "<i />text";
    setHtml(html);

    QVERIFY(!cursor.charFormat().fontItalic());
}

void tst_QTextDocumentFragment::html_anchorAroundImage()
{
    const char html[] = "<a href=\"http://www.troll.no\"><img src=test.png></a>";
    setHtml(html);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QTextImageFormat fmt = cursor.charFormat().toImageFormat();
    QCOMPARE(fmt.name(), QString("test.png"));
    QVERIFY(fmt.isAnchor());
    QCOMPARE(fmt.anchorHref(), QString("http://www.troll.no"));
}

void tst_QTextDocumentFragment::html_floatBorder()
{
    const char html[] = "<table border=1.2><tr><td>Foo";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());
    QCOMPARE(cursor.currentTable()->format().border(), qreal(1.2));
}

void tst_QTextDocumentFragment::html_frameImport()
{
    QTextFrameFormat ffmt;
    ffmt.setBorder(1);
    ffmt.setPosition(QTextFrameFormat::FloatRight);
    ffmt.setMargin(2);
    ffmt.setWidth(100);
    ffmt.setHeight(50);
    ffmt.setBackground(QColor("#00ff00"));
    cursor.insertFrame(ffmt);
    cursor.insertText("Hello World");

    QTextDocumentFragment frag(doc);
    cleanup();
    init();
    frag = QTextDocumentFragment::fromHtml(frag.toHtml());
    cursor.insertFragment(frag);

    QList<QTextFrame *> childFrames = doc->rootFrame()->childFrames();
    QCOMPARE(childFrames.count(), 1);
    QTextFrame *frame = childFrames.first();
    QCOMPARE(frame->frameFormat().margin(), ffmt.margin());
    QCOMPARE(frame->frameFormat().border(), ffmt.border());
}

void tst_QTextDocumentFragment::html_frameImport2()
{
    QTextFrameFormat ffmt;
    ffmt.setBorder(1);
    ffmt.setPosition(QTextFrameFormat::FloatRight);
    ffmt.setLeftMargin(200);
    ffmt.setTopMargin(100);
    ffmt.setBottomMargin(50);
    ffmt.setRightMargin(250);
    ffmt.setWidth(100);
    ffmt.setHeight(50);
    ffmt.setBackground(QColor("#00ff00"));
    cursor.insertFrame(ffmt);
    cursor.insertText("Hello World");

    QTextDocumentFragment frag(doc);
    cleanup();
    init();
    frag = QTextDocumentFragment::fromHtml(frag.toHtml());
    cursor.insertFragment(frag);

    QList<QTextFrame *> childFrames = doc->rootFrame()->childFrames();
    QCOMPARE(childFrames.count(), 1);
    QTextFrame *frame = childFrames.first();
    QCOMPARE(frame->frameFormat().topMargin(), ffmt.topMargin());
    QCOMPARE(frame->frameFormat().bottomMargin(), ffmt.bottomMargin());
    QCOMPARE(frame->frameFormat().leftMargin(), ffmt.leftMargin());
    QCOMPARE(frame->frameFormat().rightMargin(), ffmt.rightMargin());
    QCOMPARE(frame->frameFormat().border(), ffmt.border());
}

void tst_QTextDocumentFragment::html_dontAddMarginsAcrossTableCells()
{
    const char html[] = "<table style=\"margin-left: 100px;\"><tr><td><p style=\"margin-left:50px;\">Foo</p></td></tr></table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));

    QList<QTextFrame *> childFrames = doc->rootFrame()->childFrames();
    QCOMPARE(childFrames.count(), 1);
    QTextFrame *frame = childFrames.first();
    cursor = frame->firstCursorPosition();
    QCOMPARE(cursor.blockFormat().leftMargin(), qreal(50.0));
}

void tst_QTextDocumentFragment::html_dontMergeCenterBlocks()
{
    const char html[] = "<center>This should be centered</center>And this should not be centered anymore";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));

    QCOMPARE(doc->blockCount(), 2);
    QTextBlock blk = doc->begin();
    QCOMPARE(blk.blockFormat().alignment(), Qt::AlignCenter);
    blk = blk.next();
    QVERIFY(blk.blockFormat().alignment() != Qt::AlignCenter);
}

void tst_QTextDocumentFragment::html_tableCellBgColor()
{
    const char html[] = "<table><tr><td bgcolor=\"blue\">Test<p>Second Parag</p></td></tr></table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QVERIFY(cell.format().background().color() == Qt::blue);
}

void tst_QTextDocumentFragment::html_tableCellBgColor2()
{
    const char html[] = "<table><tr><td bgcolor=\"blue\"><table><tr><td>Blah</td></tr></table></td></tr></table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextTableCell cell = table->cellAt(0, 0);
    QVERIFY(cell.format().background().color() == Qt::blue);

    QTextFrame::Iterator it = cell.begin();
    QVERIFY(!it.atEnd());
    QVERIFY(!it.currentFrame());
    QVERIFY(it.currentBlock().isValid());

    ++it;
    QVERIFY(!it.atEnd());
    QVERIFY(it.currentFrame() != 0);
    QVERIFY(!it.currentBlock().isValid());

    ++it;
    QVERIFY(!it.atEnd());
    QVERIFY(!it.currentFrame());
    QVERIFY(it.currentBlock().isValid());
    QCOMPARE(it.currentBlock().blockFormat().background(), QBrush(Qt::NoBrush));

    ++it;
    QVERIFY(it.atEnd());
}

void tst_QTextDocumentFragment::html_cellSkip()
{
    const char html[] = ""
"<table border>"
"  <tr>"
"    <td>First Cell</td>"
"  </tr>"
"  <tr>"
"    <td>Second Cell</td>"
"    <td>Third Cell</td>"
"  </tr>"
"</table>";

    setHtml(html);
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QVERIFY(table->columns() == 2 && table->rows() == 2);

    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First Cell"));
    QVERIFY(table->cellAt(0, 1).firstCursorPosition().block().text().isEmpty());
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Second Cell"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("Third Cell"));
}

void tst_QTextDocumentFragment::nonZeroMarginOnImport()
{
    // specify bgcolor so that the html import creates a root frame format
    setHtml("<html><body bgcolor=\"#00ff00\"><b>Hello World</b></body></html>");
    QVERIFY(doc->rootFrame()->frameFormat().margin() > 0.0);
}

void tst_QTextDocumentFragment::html_charFormatPropertiesUnset()
{
    setHtml("Hello World");
    QVERIFY(doc->begin().begin().fragment().charFormat().properties().isEmpty());
}

void tst_QTextDocumentFragment::html_headings()
{
    setHtml("<h1>foo</h1>bar");
    QCOMPARE(doc->blockCount(), 2);
}

void tst_QTextDocumentFragment::html_quotedFontFamily_data()
{
    QTest::addColumn<QString>("html");
    QTest::addColumn<QString>("fontFamily");
    QTest::addColumn<QStringList>("fontFamilies");

    const QString fooFamily = QLatin1String("Foo Bar");
    const QString weirdFamily = QLatin1String("'Weird, & font '' name',");

    QTest::newRow("data1") << QString("<div style=\"font-family: 'Foo Bar';\">Test</div>")
                           << fooFamily << QStringList(fooFamily);
    QTest::newRow("data2") << QString("<div style='font-family: \"Foo  Bar\";'>Test</div>")
                           << QString("Foo  Bar") << QStringList("Foo  Bar");
    QTest::newRow("data3") << QString("<div style='font-family: Foo\n  Bar;'>Test</div>")
                           << fooFamily << QStringList(fooFamily);
    QTest::newRow("data4") << QString("<div style='font-family: Foo\n  Bar, serif, \"bar foo\";'>Test"
                                      "</div>")
                           << fooFamily << (QStringList() << "Foo Bar" << "serif" << "bar foo");
    QTest::newRow("data5") << QString("<div style='font-family: \"\\'Weird, & font \\'\\' name\\',"
                                      "\";'>Test</div>")
                           << weirdFamily << QStringList(weirdFamily);
    QTest::newRow("data6") << QString("<div style='font-family: \"\\'Weird, & font \\'\\' name\\',"
                                      "\";'>Test</div>")
                           << weirdFamily << QStringList(weirdFamily);
    QTest::newRow("data7") << QString("<div style='font-family: \"\\'Weird, & font \\'\\' name\\',\", "
                                      "serif, \"bar foo\";'>Test</div>")
                            << weirdFamily
                            << (QStringList() << weirdFamily << "serif" << "bar foo");
}

void tst_QTextDocumentFragment::html_quotedFontFamily()
{
    QFETCH(QString, html);
    QFETCH(QString, fontFamily);
    QFETCH(QStringList, fontFamilies);

    setHtml(html);
    QCOMPARE(doc->begin().begin().fragment().charFormat().fontFamily(), fontFamily);
    QCOMPARE(doc->begin().begin().fragment().charFormat().font().families(), fontFamilies);
}

void tst_QTextDocumentFragment::defaultFont()
{
    QFont f;
    f.setFamily("Courier New");
    f.setBold(true);
    f.setItalic(true);
    f.setStrikeOut(true); // set here but deliberately ignored for the html export
    f.setPointSize(100);
    doc->setDefaultFont(f);
    doc->setPlainText("Hello World");
    const QString html = doc->toHtml();
    QLatin1String str("<body style=\" font-family:'Courier New'; font-size:100pt; font-weight:600; font-style:italic;\">");
    QVERIFY(html.contains(str));
}

void tst_QTextDocumentFragment::html_spanBackgroundColor()
{
    setHtml("<span style=\"background-color: blue\">Foo</span>");
    QVERIFY(doc->begin().begin().fragment().charFormat().background().color() == QColor(Qt::blue));
}

void tst_QTextDocumentFragment::html_brokenTitle_data()
{
    QTest::addColumn<QString>("html");
    QTest::addColumn<QString>("expectedBody");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("brokentitle") << QString("<html><head><title>Foo<b>bar</b></title></head><body>Blah</body></html>")
                                 << QString("Blah") << QString("Foo");
    QTest::newRow("brokentitle2") << QString("<html><head><title>Foo<font color=red>i</font>t<font color=red>i</font>Blub</title></head><body>Blah</body></html>")
                                 << QString("Blah") << QString("Foo");
    QTest::newRow("entities") << QString("<html><head><title>Foo&lt;bar</title></head><body>Blah</body></html>")
                              << QString("Blah") << QString("Foo<bar");
    QTest::newRow("unclosedtitle") << QString("<html><head><title>Foo</head><body>Blah</body></html>")
                                   << QString("Blah") << QString("Foo");
}

void tst_QTextDocumentFragment::html_brokenTitle()
{
    QFETCH(QString, html);
    QFETCH(QString, expectedBody);
    QFETCH(QString, expectedTitle);
    doc->setHtml(html);
    QCOMPARE(doc->toPlainText(), expectedBody);
    QCOMPARE(doc->metaInformation(QTextDocument::DocumentTitle), expectedTitle);
}

void tst_QTextDocumentFragment::html_blockVsInline()
{
    {
        setHtml("<html><body><div><b>Foo<div>Bar");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<html><body><p><b>Foo<p>Bar");
        QVERIFY(cursor.charFormat().fontWeight() != QFont::Bold);
        QVERIFY(cursor.blockCharFormat().fontWeight() != QFont::Bold);
    }
    {
        setHtml("<html><body><b><center>Foo</center></b>");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<html><body><b><p>Foo");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<html><body><b><p>Foo<p>Bar");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<div><b>Foo<div>Bar");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<p><b>Foo<p>Bar");
        QVERIFY(cursor.charFormat().fontWeight() != QFont::Bold);
        QVERIFY(cursor.blockCharFormat().fontWeight() != QFont::Bold);
    }
    {
        setHtml("<b><center>Foo</center></b>");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<b><p>Foo");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
    {
        setHtml("<b><p>Foo<p>Bar");
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        QCOMPARE(cursor.blockCharFormat().fontWeight(), int(QFont::Bold));
    }
}

void tst_QTextDocumentFragment::html_tbody()
{
    setHtml("<table><thead><tr><td>First Cell</td></tr></thead><tbody><tr><td>Second Cell</td></tr></tbody></table>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->format().headerRowCount(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First Cell"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Second Cell"));
}

void tst_QTextDocumentFragment::html_nestedTables()
{
    setHtml("<table>"
            "  <tr><td>"
            ""
            "    <table>"
            "      <tr><td>Hello</td></tr>"
            "    </table>"
            ""
            "    <table>"
            "      <tr><td>World</td></tr>"
            "    </table>"
            ""
            "  </td></tr>"
            "</table>"
           );

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 1);

    cursor = table->cellAt(0, 0).firstCursorPosition();
    cursor.movePosition(QTextCursor::NextBlock);

    QTextTable *firstNestedTable = cursor.currentTable();
    QVERIFY(firstNestedTable);
    QCOMPARE(firstNestedTable->parentFrame(), table);
    QCOMPARE(firstNestedTable->rows(), 1);
    QCOMPARE(firstNestedTable->columns(), 1);
    QCOMPARE(firstNestedTable->cellAt(0, 0).firstCursorPosition().block().text(), QString("Hello"));

    while (cursor.currentTable() == firstNestedTable
           && cursor.movePosition(QTextCursor::NextBlock))
        ;

    QVERIFY(!cursor.isNull());
    QCOMPARE(cursor.currentTable(), table);

    cursor.movePosition(QTextCursor::NextBlock);

    QTextTable *secondNestedTable = cursor.currentTable();
    QVERIFY(secondNestedTable);
    QCOMPARE(secondNestedTable->parentFrame(), table);
    QCOMPARE(secondNestedTable->rows(), 1);
    QCOMPARE(secondNestedTable->columns(), 1);
    QCOMPARE(secondNestedTable->cellAt(0, 0).firstCursorPosition().block().text(), QString("World"));
}

void tst_QTextDocumentFragment::html_rowSpans()
{
    setHtml(""
            "<table border=\"1\" width=\"100%\">"
            "  <tr>"
            "    <td rowspan=\"2\">blah</td>"
            "    <td rowspan=\"2\">foo</td>"
            "  </tr>"
            "  <tr></tr>"
            "  <tr>"
            "    <td rowspan=\"2\">blubb</td>"
            "    <td rowspan=\"2\">baz</td>"
            "  </tr>"
            "  <tr></tr>"
            "</table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("blah"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("foo"));

    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("blah"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("foo"));

    QCOMPARE(table->cellAt(2, 0).firstCursorPosition().block().text(), QString("blubb"));
    QCOMPARE(table->cellAt(2, 1).firstCursorPosition().block().text(), QString("baz"));

    QCOMPARE(table->cellAt(3, 0).firstCursorPosition().block().text(), QString("blubb"));
    QCOMPARE(table->cellAt(3, 1).firstCursorPosition().block().text(), QString("baz"));
}

void tst_QTextDocumentFragment::html_rowSpans2()
{
    setHtml(""
            "<html><body>"
            "<table border=\"1\">"
            "<tr>"
            "<td>Row 1 col 1</td>"
            "</tr>"
            "<tr>"
            "<td rowspan=\"3\">Row 2 col 1, rowspan 3</td>"
            "<td>Row 2 col 2</td>"
            "</tr>"
            "<tr>"
            "<td rowspan=\"2\">Row 3 col 2, rowspan 2</td>"
            "</tr>"
            "<tr>"
            "</tr>"
            "</table>"
            "</body></html>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 1).rowSpan(), 1);
    QCOMPARE(table->cellAt(1, 0).rowSpan(), 3);
    QCOMPARE(table->cellAt(2, 1).rowSpan(), 2);
}

void tst_QTextDocumentFragment::html_implicitParagraphs()
{
    setHtml("<p>foo</p>bar");
    QCOMPARE(doc->blockCount(), 2);
}

void tst_QTextDocumentFragment::html_missingCloseTag()
{
    setHtml("<font color=\"red\"><span style=\"color:blue\">blue</span></span>&nbsp;red</font>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::blue);
    cursor.movePosition(QTextCursor::NextWord);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::red);
}

void tst_QTextDocumentFragment::html_anchorColor()
{
    setHtml("<span style=\"color: red;\">Red</span>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::red);

    setHtml("<span style=\"color: red;\"><a href=\"http://www.kde.org/\">Blue</a></span>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().foreground().color(), QGuiApplication::palette().link().color());

    setHtml("<span style=\"color: red;\"><a href=\"http://www.kde.org/\" style=\"color: yellow;\">Green</a></span>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::yellow);
}

void tst_QTextDocumentFragment::html_lastParagraphClosing()
{
    setHtml("<p>Foo<b>Bar</b>Baz");
    QCOMPARE(doc->blockCount(), 1);
}

void tst_QTextDocumentFragment::html_tableHeaderBodyFootParent()
{
    // don't get confused by strange tags, keep tbody/thead/tfoot children of
    // the table tag
    setHtml("<table><col><col><col><tbody><tr><td>Hey</td></tr></tbody></table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Hey"));

    setHtml("<table><col><col><col><thead><tr><td>Hey</td></tr></thead></table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Hey"));

    setHtml("<table><col><col><col><tfoot><tr><td>Hey</td></tr></tfoot></table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Hey"));
}

void tst_QTextDocumentFragment::html_columnWidths()
{
    setHtml("<table>"
            " <tr>"
            "   <td colspan=\"2\">Foo</td>"
            " </tr>"
            " <tr>"
            "   <td>Bar</td>"
            "   <td width=\"1%\">Baz</td>"
            " </tr>"
            "</table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->rows(), 2);
    QTextTableFormat fmt = table->format();

    const QVector<QTextLength> columnWidths = fmt.columnWidthConstraints();
    QCOMPARE(columnWidths.count(), 2);
    QCOMPARE(columnWidths.at(0).type(), QTextLength::VariableLength);
    QCOMPARE(columnWidths.at(1).type(), QTextLength::PercentageLength);
    QCOMPARE(columnWidths.at(1).rawValue(), qreal(1));
}

void tst_QTextDocumentFragment::css_fontWeight()
{
    setHtml("<p style=\"font-weight:bold\">blah</p>");
    QCOMPARE(doc->begin().charFormat().fontWeight(), int(QFont::Bold));
    setHtml("<p style=\"font-weight:600\">blah</p>");
    QCOMPARE(doc->begin().charFormat().fontWeight(), int(QFont::Bold));

}

void tst_QTextDocumentFragment::css_float()
{
    setHtml("<img src=\"foo\" style=\"float: right\">");
    QTextCharFormat fmt = doc->begin().begin().fragment().charFormat();
    QVERIFY(fmt.isImageFormat());
    QTextObject *o = doc->objectForFormat(fmt);
    QVERIFY(o);
    QTextFormat f = o->format();
    QVERIFY(f.isFrameFormat());
    QCOMPARE(f.toFrameFormat().position(), QTextFrameFormat::FloatRight);

    setHtml("<img src=\"foo\" align=right>");
    fmt = doc->begin().begin().fragment().charFormat();
    QVERIFY(fmt.isImageFormat());
    o = doc->objectForFormat(fmt);
    QVERIFY(o);
    f = o->format();
    QVERIFY(f.isFrameFormat());
    QCOMPARE(f.toFrameFormat().position(), QTextFrameFormat::FloatRight);

    setHtml("<img src=\"foo\" align=left>");
    fmt = doc->begin().begin().fragment().charFormat();
    QVERIFY(fmt.isImageFormat());
    o = doc->objectForFormat(fmt);
    QVERIFY(o);
    f = o->format();
    QVERIFY(f.isFrameFormat());
    QCOMPARE(f.toFrameFormat().position(), QTextFrameFormat::FloatLeft);
}

void tst_QTextDocumentFragment::css_textIndent()
{
    setHtml("<p style=\"text-indent: 42px\">foo</p>");
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.textIndent(), qreal(42));
}

void tst_QTextDocumentFragment::css_inline()
{
    setHtml(""
            "<style>"
            " p { background-color: green;}"
            "</style>"
            "<p>test</p>"
            );
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), QColor("green"));
}

void tst_QTextDocumentFragment::css_external()
{
    doc->addResource(QTextDocument::StyleSheetResource, QUrl("test.css"), QString("p { background-color: green; }"));
    doc->setHtml(""
            "<link href=\"test.css\" type=\"text/css\" />"
            "<p>test</p>"
            );
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), QColor("green"));
}

void tst_QTextDocumentFragment::css_import()
{
    const QColor green("green");
    doc->addResource(QTextDocument::StyleSheetResource, QUrl("test.css"), QString("@import \"other.css\";"));
    doc->addResource(QTextDocument::StyleSheetResource, QUrl("other.css"), QString("@import url(\"other2.css\");"));
    doc->addResource(QTextDocument::StyleSheetResource, QUrl("other2.css"), QString("p { background-color: green; }"));
    doc->setHtml(""
            "<link href=\"test.css\" type=\"text/css\" />"
            "<p>test</p>"
            );
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), green);

    doc->setHtml(""
            "<style>@import \"test.css\" screen;</style>"
            "<p>test</p>"
            );
    fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), green);
}

void tst_QTextDocumentFragment::css_selectors_data()
{
    QTest::addColumn<bool>("match");
    QTest::addColumn<QString>("selector");
    QTest::addColumn<QString>("attributes");

    QTest::newRow("plain") << true << QString() << QString();

    QTest::newRow("class") << true << QString(".foo") << QString("class=foo");
    QTest::newRow("notclass") << false << QString(".foo") << QString("class=bar");

    QTest::newRow("attrset") << true << QString("[justset]") << QString("justset");
    QTest::newRow("notattrset") << false << QString("[justset]") << QString("otherattribute");

    QTest::newRow("attrmatch") << true << QString("[foo=bar]") << QString("foo=bar");
    QTest::newRow("noattrmatch") << false << QString("[foo=bar]") << QString("foo=xyz");

    QTest::newRow("contains") << true << QString("[foo~=bar]") << QString("foo=\"baz bleh bar\"");
    QTest::newRow("notcontains") << false << QString("[foo~=bar]") << QString("foo=\"test\"");

    QTest::newRow("beingswith") << true << QString("[foo|=bar]") << QString("foo=\"bar-bleh\"");
    QTest::newRow("notbeingswith") << false << QString("[foo|=bar]") << QString("foo=\"bleh-bar\"");

    QTest::newRow("attr2") << true << QString("[bar=foo]") << QString("bleh=bar bar=foo");
}

void tst_QTextDocumentFragment::css_selectors()
{
    QFETCH(bool, match);
    QFETCH(QString, selector);
    QFETCH(QString, attributes);

    QString html = QString(""
            "<style>"
            " p { background-color: green }"
            " p%1 { background-color: red }"
            "</style>"
            "<p %2>test</p>"
            ).arg(selector).arg(attributes);
    setHtml(html);

    QTextBlockFormat fmt = doc->begin().blockFormat();
    if (match)
        QCOMPARE(fmt.background().color(), QColor("red"));
    else
        QCOMPARE(fmt.background().color(), QColor("green"));
}

void tst_QTextDocumentFragment::css_nodeNameCaseInsensitivity()
{
    setHtml("<style>"
            "P { background-color: green }"
            "</style>"
            "<p>test</p>");
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), QColor("green"));
}

void tst_QTextDocumentFragment::css_textUnderlineStyle_data()
{
    QTest::addColumn<QString>("styleName");
    QTest::addColumn<int>("expectedStyle");

    QTest::newRow("none") << QString("none") << int(QTextCharFormat::NoUnderline);
    QTest::newRow("solid") << QString("solid") << int(QTextCharFormat::SingleUnderline);
    QTest::newRow("dash") << QString("dashed") << int(QTextCharFormat::DashUnderline);
    QTest::newRow("dot") << QString("dotted") << int(QTextCharFormat::DotLine);
    QTest::newRow("dashdot") << QString("dot-dash") << int(QTextCharFormat::DashDotLine);
    QTest::newRow("dashdotdot") << QString("dot-dot-dash") << int(QTextCharFormat::DashDotDotLine);
    QTest::newRow("wave") << QString("wave") << int(QTextCharFormat::WaveUnderline);
}

void tst_QTextDocumentFragment::css_textUnderlineStyle()
{
    QFETCH(QString, styleName);
    QFETCH(int, expectedStyle);

    QString html = QString::fromLatin1("<span style=\"text-underline-style: %1\">Blah</span>").arg(styleName);
    doc->setHtml(html);

    QTextFragment fragment = doc->begin().begin().fragment();
    QVERIFY(fragment.isValid());
    QCOMPARE(int(fragment.charFormat().underlineStyle()), expectedStyle);
}

void tst_QTextDocumentFragment::css_textUnderlineStyleAndDecoration()
{
    doc->setHtml("<span style=\"text-decoration: overline; text-underline-style: solid\">Test</span>");

    QTextFragment fragment = doc->begin().begin().fragment();
    QVERIFY(fragment.isValid());
    QCOMPARE(fragment.charFormat().underlineStyle(), QTextCharFormat::SingleUnderline);
    QVERIFY(fragment.charFormat().fontOverline());

    doc->setHtml("<span style=\"text-underline-style: solid; text-decoration: overline\">Test</span>");

    fragment = doc->begin().begin().fragment();
    QVERIFY(fragment.isValid());
    QCOMPARE(fragment.charFormat().underlineStyle(), QTextCharFormat::SingleUnderline);
    QVERIFY(fragment.charFormat().fontOverline());
}

void tst_QTextDocumentFragment::css_listStyleType()
{
    doc->setHtml("<ol style=\"list-style-type: disc\"><li>Blah</li></ol>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListDisc);

    doc->setHtml("<ul style=\"list-style-type: square\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListSquare);

    doc->setHtml("<ul style=\"list-style-type: circle\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListCircle);

    doc->setHtml("<ul style=\"list-style-type: decimal\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListDecimal);

    doc->setHtml("<ul style=\"list-style-type: lower-alpha\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListLowerAlpha);

    doc->setHtml("<ul style=\"list-style-type: upper-alpha\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListUpperAlpha);

    doc->setHtml("<ul style=\"list-style-type: upper-roman\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListUpperRoman);

    doc->setHtml("<ul style=\"list-style-type: lower-roman\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListLowerRoman);

    // ignore the unsupported list-style-position inside the list-style shorthand property
    doc->setHtml("<ul style=\"list-style: outside decimal\"><li>Blah</li></ul>");
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListDecimal);
}

void tst_QTextDocumentFragment::css_linkPseudo()
{
    doc->setHtml("<a href=\"foobar\">Blah</a>");
    QVERIFY(doc->begin().begin().fragment().charFormat().fontUnderline());

    doc->setHtml("<style>a { text-decoration: none; }</style><a href=\"foobar\">Blah</a>");
    QVERIFY(!doc->begin().begin().fragment().charFormat().fontUnderline());

    doc->setHtml("<style>a:link { text-decoration: none; }</style><a href=\"foobar\">Blah</a>");
    QVERIFY(!doc->begin().begin().fragment().charFormat().fontUnderline());
}

void tst_QTextDocumentFragment::css_pageBreaks()
{
    doc->setHtml("<p>Foo</p>");
    QCOMPARE(doc->begin().blockFormat().pageBreakPolicy(), QTextFormat::PageBreak_Auto);

    doc->setHtml("<p style=\" page-break-before:always;\">Foo</p>");
    QCOMPARE(doc->begin().blockFormat().pageBreakPolicy(), QTextFormat::PageBreak_AlwaysBefore);

    doc->setHtml("<p style=\" page-break-after:always;\">Foo</p>");
    QCOMPARE(doc->begin().blockFormat().pageBreakPolicy(), QTextFormat::PageBreak_AlwaysAfter);

    doc->setHtml("<p style=\" page-break-before:always; page-break-after:always;\">Foo</p>");
    QVERIFY(doc->begin().blockFormat().pageBreakPolicy() == (QTextFormat::PageBreak_AlwaysAfter | QTextFormat::PageBreak_AlwaysBefore));
}

void tst_QTextDocumentFragment::universalSelectors_data()
{
    QTest::addColumn<bool>("match");
    QTest::addColumn<QString>("selector");
    QTest::addColumn<QString>("attributes");

    QTest::newRow("1") << true << QString("*") << QString();
    QTest::newRow("2") << false << QString() << QString(); // invalid totally empty selector

    QTest::newRow("3") << false << QString("*[foo=bar]") << QString("foo=bleh");
    QTest::newRow("4") << true << QString("*[foo=bar]") << QString("foo=bar");

    QTest::newRow("5") << false << QString("[foo=bar]") << QString("foo=bleh");
    QTest::newRow("6") << true << QString("[foo=bar]") << QString("foo=bar");

    QTest::newRow("7") << true << QString(".charfmt1") << QString("class=charfmt1");
}

void tst_QTextDocumentFragment::universalSelectors()
{
    QFETCH(bool, match);
    QFETCH(QString, selector);
    QFETCH(QString, attributes);

    QString html = QString(""
            "<style>"
            "%1 { background-color: green }"
            "</style>"
            "<p %2>test</p>"
            ).arg(selector).arg(attributes);

    setHtml(html);

    QTextBlockFormat fmt = doc->begin().blockFormat();
    if (match)
        QCOMPARE(fmt.background().color(), QColor("green"));
    else
        QVERIFY(!fmt.hasProperty(QTextFormat::BackgroundBrush));
}

void tst_QTextDocumentFragment::screenMedia()
{
    const QColor green("green");
    setHtml("<style>"
            "@media screen {"
            "p { background-color: green }"
            "}"
            "</style>"
            "<p>test</p>"
            "");
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), green);

    setHtml("<style>"
            "@media foobar {"
            "p { background-color: green }"
            "}"
            "</style>"
            "<p>test</p>"
            "");
    fmt = doc->begin().blockFormat();
    QVERIFY(fmt.background().color() != green);

    setHtml("<style>"
            "@media sCrEeN {"
            "p { background-color: green }"
            "}"
            "</style>"
            "<p>test</p>"
            "");
    fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), green);
}

void tst_QTextDocumentFragment::htmlResourceLoading()
{
    const QString html("<link href=\"test.css\" type=\"text/css\" />"
                                   "<p>test</p>");

    QTextDocument tmp;
    tmp.addResource(QTextDocument::StyleSheetResource, QUrl("test.css"), QString("p { background-color: green; }"));
    QTextDocumentFragment frag = QTextDocumentFragment::fromHtml(html, &tmp);
    doc->clear();
    QTextCursor(doc).insertFragment(frag);
    QTextBlockFormat fmt = doc->begin().blockFormat();
    QCOMPARE(fmt.background().color(), QColor("green"));
}

void tst_QTextDocumentFragment::someCaseInsensitiveAttributeValues()
{
    const char html1[] = "<ul type=sQUarE><li>Blah</li></ul>";
    setHtml(QString::fromLatin1(html1));
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->format().style(), QTextListFormat::ListSquare);

    const char html2[] = "<div align=ceNTeR><b>Hello World";
    setHtml(html2);

    QCOMPARE(doc->begin().blockFormat().alignment(), Qt::AlignHCenter);

    const char html3[] = "<p dir=rTL><b>Hello World";
    setHtml(html3);

    QCOMPARE(doc->begin().blockFormat().layoutDirection(), Qt::RightToLeft);
}

class TestDocument : public QTextDocument
{
public:
    inline TestDocument() {}

    QPixmap testPixmap;

    virtual QVariant loadResource(int type, const QUrl &name) {
        if (name.toString() == QLatin1String("testPixmap")) {
            return testPixmap;
        }
        return QTextDocument::loadResource(type, name);
    }
};

void tst_QTextDocumentFragment::backgroundImage()
{
    TestDocument doc;
    doc.testPixmap = QPixmap(100, 100);
    doc.testPixmap.fill(Qt::blue);
    doc.setHtml("<p style=\"background-image: url(testPixmap)\">Hello</p>");
    QBrush bg = doc.begin().blockFormat().background();
    QCOMPARE(bg.style(), Qt::TexturePattern);
    QCOMPARE(bg.texture().cacheKey(), doc.testPixmap.cacheKey());
}

void tst_QTextDocumentFragment::dontMergePreAndNonPre()
{
    doc->setHtml("<pre>Pre text</pre>Text that should be wrapped");
    QCOMPARE(doc->blockCount(), 2);
    QCOMPARE(doc->begin().text(), QString("Pre text"));
    QCOMPARE(doc->begin().next().text(), QString("Text that should be wrapped"));
}

void tst_QTextDocumentFragment::leftMarginInsideHtml()
{
    doc->setHtml("<html><dl><dd>Blah");
    QCOMPARE(doc->blockCount(), 1);
    QVERIFY(doc->begin().blockFormat().leftMargin() > 0);
}

void tst_QTextDocumentFragment::html_margins()
{
    doc->setHtml("<p style=\"margin-left: 42px\">Test");
    QCOMPARE(doc->blockCount(), 1);
    QCOMPARE(doc->begin().blockFormat().topMargin(), 12.);
    QCOMPARE(doc->begin().blockFormat().bottomMargin(), 12.);
    QCOMPARE(doc->begin().blockFormat().leftMargin(), 42.);
}

void tst_QTextDocumentFragment::newlineInsidePreShouldBecomeNewParagraph()
{
    // rationale: we used to map newlines inside <pre> to QChar::LineSeparator, but
    // if you display a lot of text inside pre it all ended up inside one single paragraph,
    // which doesn't scale very well with our text engine. Paragraphs spanning thousands of
    // lines are not a common use-case otherwise.

    doc->setHtml("<pre>Foo\nBar</pre>");
    QCOMPARE(doc->blockCount(), 2);
    QTextBlock block = doc->begin();
    QCOMPARE(block.blockFormat().topMargin(), qreal(12));
    QVERIFY(qIsNull(block.blockFormat().bottomMargin()));

    block = block.next();

    QVERIFY(qIsNull(block.blockFormat().topMargin()));
    QCOMPARE(block.blockFormat().bottomMargin(), qreal(12));

    doc->setHtml("<pre style=\"margin-top: 32px; margin-bottom: 45px; margin-left: 50px\">Foo\nBar</pre>");
    QCOMPARE(doc->blockCount(), 2);
    block = doc->begin();
    QCOMPARE(block.blockFormat().topMargin(), qreal(32));
    QVERIFY(qIsNull(block.blockFormat().bottomMargin()));
    QCOMPARE(block.blockFormat().leftMargin(), qreal(50));

    block = block.next();

    QVERIFY(qIsNull(block.blockFormat().topMargin()));
    QCOMPARE(block.blockFormat().bottomMargin(), qreal(45));
    QCOMPARE(block.blockFormat().leftMargin(), qreal(50));

}

void tst_QTextDocumentFragment::invalidColspan()
{
    doc->setHtml("<table><tr rowspan=-1><td colspan=-1>Blah</td></tr></table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->rows(), 1);
}

void tst_QTextDocumentFragment::html_brokenTableWithJustTr()
{
    doc->setHtml("<tr><td>First Cell</td><tr><td>Second Cell");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First Cell"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Second Cell"));

    doc->setHtml(""
        "<col width=286 style='mso-width-source:userset;mso-width-alt:10459;width:215pt'>"
        "<col width=64 span=3 style='width:48pt'>"
        "<tr height=17 style='height:12.75pt'>"
        "<td height=17 width=286 style='height:12.75pt;width:215pt'>1a</td>"
        "<td width=64 style='width:48pt'>1b</td>"
        "<td width=64 style='width:48pt'>1c</td>"
        "<td width=64 style='width:48pt'>1d</td>"
        "</tr>"
        "<tr height=17 style='height:12.75pt'>"
        "<td height=17 style='height:12.75pt'>|2a</td>"
        "<td>2b</td>"
        "<td>2c</td>"
        "<td>2d</td>"
        "</tr>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 4);
}

void tst_QTextDocumentFragment::html_brokenTableWithJustTd()
{
    doc->setHtml("<td>First Cell</td><td>Second Cell");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First Cell"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Second Cell"));

    doc->setHtml("<td height=17 width=286 style='height:12.75pt;width:215pt'>1a</td>"
                 "<td width=64 style='width:48pt'>1b</td>"
                 "<td width=64 style='width:48pt'>1c</td>"
                 "<td width=64 style='width:48pt'>1d</td>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 4);
}

void tst_QTextDocumentFragment::html_preNewlineHandling_data()
{
    QTest::addColumn<QString>("html");
    QTest::addColumn<QString>("expectedPlainText");

    QTest::newRow("pre1") << QString("Foo<pre>Bar")
                          << QString("Foo\nBar");
    QTest::newRow("pre2") << QString("Foo<pre>\nBar")
                          << QString("Foo\nBar");
    QTest::newRow("pre3") << QString("Foo<pre>\n\nBar")
                          << QString("Foo\n\nBar");
    QTest::newRow("pre4") << QString("<html>Foo<pre>\nBar")
                          << QString("Foo\nBar");
    QTest::newRow("pre5") << QString("<pre>Foo\n</pre>\nBar")
                          << QString("Foo\nBar");
    QTest::newRow("pre6") << QString("<pre>Foo<b>Bar</b>Blah\n</pre>\nMooh")
                          << QString("FooBarBlah\nMooh");
    QTest::newRow("pre7") << QString("<pre>\nPara1\n</pre>\n<pre>\nPara2\n</pre>")
                          << QString("Para1\nPara2");
}

void tst_QTextDocumentFragment::html_preNewlineHandling()
{
    QFETCH(QString, html);

    doc->setHtml(html);
    QTEST(doc->toPlainText(), "expectedPlainText");
}

void tst_QTextDocumentFragment::html_br()
{
    doc->setHtml("Foo<br><br><br>Blah");
    QCOMPARE(doc->toPlainText(), QString("Foo\n\n\nBlah"));
}

void tst_QTextDocumentFragment::html_dl()
{
    doc->setHtml("<dl><dt>term<dd>data</dl>Text afterwards");
    QCOMPARE(doc->toPlainText(), QString("term\ndata\nText afterwards"));
}

void tst_QTextDocumentFragment::html_tableStrangeNewline()
{
    doc->setHtml("<table><tr><td>Foo</td></tr>\n</table>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 1);
    const QTextTableCell cell = table->cellAt(0, 0);
    QCOMPARE(cell.firstCursorPosition().block().text(), QString("Foo"));
    QCOMPARE(cell.firstCursorPosition().block(), cell.lastCursorPosition().block());
}

void tst_QTextDocumentFragment::html_tableStrangeNewline2()
{
    doc->setHtml("<table><tr><td>Foo</td></tr><tr>\n<td/></tr></table>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 1);
    const QTextTableCell cell = table->cellAt(0, 0);
    QCOMPARE(cell.firstCursorPosition().block().text(), QString("Foo"));
    QCOMPARE(cell.firstCursorPosition().block(), cell.lastCursorPosition().block());
}

void tst_QTextDocumentFragment::html_tableStrangeNewline3()
{
    doc->setHtml("<table border>"
                 "<tr>"
                 "<td>"
                 "<ul>"
                 "<li>Meh</li>"
                 "</ul>"
                 "</td>"
                 "<td>\n"
                 "<ul>"
                 "<li>Foo</li>"
                 "</ul>"
                 "</td>"
                 "</tr>"
                 "</table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 2);

    QTextTableCell cell = table->cellAt(0, 0);
    QCOMPARE(cell.firstCursorPosition().block().text(), QString("Meh"));
    QCOMPARE(cell.firstCursorPosition().block(), cell.lastCursorPosition().block());

    cell = table->cellAt(0, 1);
    QCOMPARE(cell.firstCursorPosition().block().text(), QString("Foo"));
    QCOMPARE(cell.firstCursorPosition().block(), cell.lastCursorPosition().block());
}

void tst_QTextDocumentFragment::html_caption()
{
    doc->setHtml("<table border align=center>"
                 "<caption>This <b>   is a</b> Caption!</caption>"
                 "<tr><td>Blah</td></tr>"
                 "</table>");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);

    QCOMPARE(cursor.block().text(), QString("This is a Caption!"));
    QCOMPARE(cursor.blockFormat().alignment(), Qt::AlignHCenter);

    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable *table = cursor.currentTable();
    QVERIFY(table);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 1);

    QTextTableCell cell = table->cellAt(0, 0);
    QCOMPARE(cell.firstCursorPosition().block().text(), QString("Blah"));
}

static const uint windowsLatin1ExtendedCharacters[0xA0 - 0x80] = {
    0x20ac, // 0x80
    0x0081, // 0x81 direct mapping
    0x201a, // 0x82
    0x0192, // 0x83
    0x201e, // 0x84
    0x2026, // 0x85
    0x2020, // 0x86
    0x2021, // 0x87
    0x02C6, // 0x88
    0x2030, // 0x89
    0x0160, // 0x8A
    0x2039, // 0x8B
    0x0152, // 0x8C
    0x008D, // 0x8D direct mapping
    0x017D, // 0x8E
    0x008F, // 0x8F directmapping
    0x0090, // 0x90 directmapping
    0x2018, // 0x91
    0x2019, // 0x92
    0x201C, // 0x93
    0X201D, // 0x94
    0x2022, // 0x95
    0x2013, // 0x96
    0x2014, // 0x97
    0x02DC, // 0x98
    0x2122, // 0x99
    0x0161, // 0x9A
    0x203A, // 0x9B
    0x0153, // 0x9C
    0x009D, // 0x9D direct mapping
    0x017E, // 0x9E
    0x0178  // 0x9F
};

void tst_QTextDocumentFragment::html_windowsEntities()
{
    for (uint i = 0; i < sizeof(windowsLatin1ExtendedCharacters)/sizeof(windowsLatin1ExtendedCharacters[0]); ++i) {
        QString html = QString::number(i + 0x80);
        html.prepend("<p>&#");
        html.append(";");
        doc->setHtml(html);
        QCOMPARE(doc->toPlainText(), QString(QChar(windowsLatin1ExtendedCharacters[i])));
    }
}

void tst_QTextDocumentFragment::html_eatenText()
{
    doc->setHtml("<h1>Test1</h1>\nTest2<h1>Test3</h1>");
    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.block().text(), QString("Test1"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("Test2"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("Test3"));
}

void tst_QTextDocumentFragment::html_hr()
{
    doc->setHtml("<hr />");
    QCOMPARE(doc->blockCount(), 1);
    QVERIFY(doc->begin().blockFormat().hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth));
}

void tst_QTextDocumentFragment::html_hrMargins()
{
    doc->setHtml("<p>Test<hr/>Blah");
    QCOMPARE(doc->blockCount(), 3);

    cursor.movePosition(QTextCursor::Start);
    QTextBlock block = cursor.block();
    QCOMPARE(block.text(), QString("Test"));
    QVERIFY(block.blockFormat().bottomMargin() <= qreal(12.));
    QTextBlock first = block;

    cursor.movePosition(QTextCursor::NextBlock);
    block = cursor.block();
    QVERIFY(qMax(first.blockFormat().bottomMargin(), block.blockFormat().topMargin()) > 0);

    cursor.movePosition(QTextCursor::NextBlock);
    block = cursor.block();

    QCOMPARE(block.text(), QString("Blah"));
}

void tst_QTextDocumentFragment::html_blockQuoteMargins()
{
    doc->setHtml("<blockquote>Bar</blockquote>");
    QCOMPARE(doc->blockCount(), 1);
    cursor.movePosition(QTextCursor::Start);
    QTextBlock block = cursor.block();
    QCOMPARE(block.text(), QString("Bar"));
    QCOMPARE(block.blockFormat().leftMargin(), qreal(40.));
    QCOMPARE(block.blockFormat().rightMargin(), qreal(40.));
    QCOMPARE(block.blockFormat().topMargin(), qreal(12.));
    QCOMPARE(block.blockFormat().bottomMargin(), qreal(12.));
}

void tst_QTextDocumentFragment::html_definitionListMargins()
{
    doc->setHtml("Foo<dl><dt>tag<dd>data</dl>Bar");
    QCOMPARE(doc->blockCount(), 4);

    cursor.movePosition(QTextCursor::Start);
    QTextBlock block = cursor.block();
    QCOMPARE(block.text(), QString("Foo"));

    block = block.next();
    QCOMPARE(block.text(), QString("tag"));
    QCOMPARE(block.blockFormat().topMargin(), qreal(8.));

    block = block.next();
    QCOMPARE(block.text(), QString("data"));
    QCOMPARE(block.blockFormat().bottomMargin(), qreal(8.));

    block = block.next();
    QCOMPARE(block.text(), QString("Bar"));
}

void tst_QTextDocumentFragment::html_listMargins()
{
    doc->setHtml("Foo<ol><li>First<li>Second</ol>Bar");
    QCOMPARE(doc->blockCount(), 4);

    cursor.movePosition(QTextCursor::Start);
    QTextBlock block = cursor.block();
    QCOMPARE(block.text(), QString("Foo"));

    block = block.next();
    QCOMPARE(block.text(), QString("First"));
    QCOMPARE(block.blockFormat().topMargin(), qreal(12.));

    block = block.next();
    QCOMPARE(block.text(), QString("Second"));
    QCOMPARE(block.blockFormat().bottomMargin(), qreal(12.));

    block = block.next();
    QCOMPARE(block.text(), QString("Bar"));
}

void tst_QTextDocumentFragment::html_titleAttribute()
{
    doc->setHtml("<span title=\"this is my title\">Test</span>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat().toolTip(), QString("this is my title"));
}

void tst_QTextDocumentFragment::html_compressDivs()
{
    doc->setHtml("<p/><div/><div/><div/><div/>Test");
    QCOMPARE(doc->blockCount(), 1);
    QCOMPARE(doc->begin().text(), QString("Test"));
}

void tst_QTextDocumentFragment::completeToPlainText()
{
    doc->setPlainText("Hello\nWorld");
    QCOMPARE(doc->toPlainText(), QString("Hello\nWorld"));
    QTextDocumentFragment fragment(doc);
    QCOMPARE(fragment.toPlainText(), QString("Hello\nWorld"));
}

void tst_QTextDocumentFragment::copyContents()
{
    doc->setPlainText("Hello");
    QFont f;
    doc->setDefaultFont(f);
    QTextFragment fragment = doc->begin().begin().fragment();
    QCOMPARE(fragment.text(), QString("Hello"));
    QCOMPARE(fragment.charFormat().font().pointSize(), f.pointSize());

    QTextDocumentFragment frag(doc);
    doc->clear();
    f.setPointSize(48);
    doc->setDefaultFont(f);
    QTextCursor(doc).insertFragment(QTextDocumentFragment::fromHtml(frag.toHtml()));
    fragment = doc->begin().begin().fragment();
    QCOMPARE(fragment.text(), QString("Hello"));
    QCOMPARE(fragment.charFormat().font().pointSize(), f.pointSize());
}

void tst_QTextDocumentFragment::html_textAfterHr()
{
    doc->setHtml("<hr><nobr><b>After the centered text</b></nobr>");
    QCOMPARE(doc->blockCount(), 2);
    QTextBlock block = doc->begin();
    QVERIFY(block.text().isEmpty());
    QVERIFY(block.blockFormat().hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth));
    block = block.next();

    QString txt("After the centered text");
    txt.replace(QLatin1Char(' '), QChar::Nbsp);
    QCOMPARE(block.text(), txt);
    QVERIFY(!block.blockFormat().hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth));
}

void tst_QTextDocumentFragment::blockTagClosing()
{
    doc->setHtml("<p>foo<p>bar<span>baz</span>");
    QCOMPARE(doc->blockCount(), 2);
    QTextBlock block = doc->begin();
    QCOMPARE(block.text(), QString("foo"));
    block = block.next();
    QCOMPARE(block.text(), QString("barbaz"));
}

void tst_QTextDocumentFragment::isEmpty()
{
    QTextDocumentFragment frag;
    QVERIFY(frag.isEmpty());
    frag = QTextDocumentFragment::fromHtml("test");
    QVERIFY(!frag.isEmpty());
    frag = QTextDocumentFragment::fromHtml("<hr />");
    QVERIFY(!frag.isEmpty());
}

void tst_QTextDocumentFragment::html_alignmentInheritance()
{
    doc->setHtml("<center>Centered text<hr></center><b>After the centered text</b>");
    QCOMPARE(doc->blockCount(), 3);
    QTextBlock block = doc->begin();
    QVERIFY(block.blockFormat().alignment() & Qt::AlignHCenter);
    block = block.next();
    QVERIFY(block.blockFormat().alignment() & Qt::AlignHCenter);
    block = block.next();
    QVERIFY(!(block.blockFormat().alignment() & Qt::AlignHCenter));
}

void tst_QTextDocumentFragment::html_ignoreEmptyDivs()
{
    doc->setHtml("<p><div/><b>Foo</b>");
    QCOMPARE(doc->blockCount(), 1);
    QCOMPARE(doc->begin().text(), QString("Foo"));
}

void tst_QTextDocumentFragment::html_dontInheritAlignmentForFloatingImages()
{
    doc->setHtml("<p align=right><img align=unknownignored src=\"foo\" /></p>");
    QTextCharFormat fmt = doc->begin().begin().fragment().charFormat();
    QVERIFY(fmt.isImageFormat());
    QTextObject *o = doc->objectForFormat(fmt);
    QVERIFY(o);
    QTextFormat f = o->format();
    QVERIFY(f.isFrameFormat());
    QCOMPARE(f.toFrameFormat().position(), QTextFrameFormat::InFlow);
}

void tst_QTextDocumentFragment::html_verticalImageAlignment()
{
    doc->setHtml("<img src=\"foo\"/>");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    QTextImageFormat fmt = cursor.charFormat().toImageFormat();
    QCOMPARE(fmt.verticalAlignment(), QTextCharFormat::AlignNormal);

    doc->setHtml("<img src=\"foo\" align=middle />");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    fmt = cursor.charFormat().toImageFormat();
    QCOMPARE(fmt.verticalAlignment(), QTextCharFormat::AlignMiddle);

    doc->setHtml("<img src=\"foo\" style=\"vertical-align: middle\" />");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    fmt = cursor.charFormat().toImageFormat();
    QCOMPARE(fmt.verticalAlignment(), QTextCharFormat::AlignMiddle);

    doc->setHtml("<img src=\"foo\" align=top />");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    fmt = cursor.charFormat().toImageFormat();
    QCOMPARE(fmt.verticalAlignment(), QTextCharFormat::AlignTop);

    doc->setHtml("<img src=\"foo\" style=\"vertical-align: top\" />");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    fmt = cursor.charFormat().toImageFormat();
    QCOMPARE(fmt.verticalAlignment(), QTextCharFormat::AlignTop);
}

void tst_QTextDocumentFragment::html_verticalCellAlignment()
{
    const char *alt[] =
    {
        // vertical-align property
        "<table>"
        "<tr>"
        "<td style=\"vertical-align: middle\"></td>"
        "<td style=\"vertical-align: top\"></td>"
        "<td style=\"vertical-align: bottom\"></td>"
        "</tr>"
        "</table>",
        // valign property
        "<table>"
        "<tr>"
        "<td valign=\"middle\"></td>"
        "<td valign=\"top\"></td>"
        "<td valign=\"bottom\"></td>"
        "</tr>"
        "</table>",
        // test td override of tr property
        "<table>"
        "<tr valign=\"bottom\">"
        "<td valign=\"middle\"></td>"
        "<td valign=\"top\"></td>"
        "<td></td>"
        "</tr>"
        "</table>"
    };

    const int numTestCases = sizeof(alt) / sizeof(*alt);
    for (int i = 0; i < numTestCases; ++i) {
        doc->setHtml(alt[i]);

        QTextTable *table = qobject_cast<QTextTable *>(doc->rootFrame()->childFrames().at(0));
        QVERIFY(table);

        QCOMPARE(table->cellAt(0, 0).format().verticalAlignment(), QTextCharFormat::AlignMiddle);
        QCOMPARE(table->cellAt(0, 1).format().verticalAlignment(), QTextCharFormat::AlignTop);
        QCOMPARE(table->cellAt(0, 2).format().verticalAlignment(), QTextCharFormat::AlignBottom);
    }
}

void tst_QTextDocumentFragment::html_borderColor()
{
    const char html[] = "<table border=1 style=\"border-color:#0000ff;\"><tr><td>Foo</td></tr></table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());
    QCOMPARE(cursor.currentTable()->format().borderStyle(), QTextFrameFormat::BorderStyle_Outset);
    QCOMPARE(cursor.currentTable()->format().borderBrush(), QBrush(QColor("#0000ff")));
}

void tst_QTextDocumentFragment::html_borderStyle()
{
    const char html[] = "<table border=1 style=\"border-style:solid;\"><tr><td>Foo</td></tr></table>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentTable());
    QCOMPARE(cursor.currentTable()->format().borderStyle(), QTextFrameFormat::BorderStyle_Solid);
    QCOMPARE(cursor.currentTable()->format().borderBrush(), QBrush(Qt::darkGray));
}

void tst_QTextDocumentFragment::html_borderWidth()
{
    const char *html[2] = { "<table style=\"border-width:2;\"><tr><td>Foo</td></tr></table>",
                            "<table style=\"border-width:2px;\"><tr><td>Foo</td></tr></table>" };

    for (int i = 0; i < 2; ++i) {
        cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html[i])));
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock);
        QVERIFY(cursor.currentTable());
        QCOMPARE(cursor.currentTable()->format().border(), qreal(2));
    }
}

void tst_QTextDocumentFragment::html_userState()
{
    const char html[] = "<p style=\"-qt-user-state:42;\">A</p><p style=\"-qt-user-state:0;\">B</p><p>C</p>";
    cursor.insertFragment(QTextDocumentFragment::fromHtml(QString::fromLatin1(html)));
    QTextBlock block = doc->begin();
    QCOMPARE(block.userState(), 42);
    QCOMPARE(block.next().userState(), 0);
    QCOMPARE(block.next().next().userState(), -1);
}

void tst_QTextDocumentFragment::html_rootFrameProperties()
{
    const char html[] = "<table border=1 style=\"-qt-table-type:root; margin-top:10px;\"><tr><td>Foo</tr></td>";
    doc->setHtml(html);

    QCOMPARE(doc->rootFrame()->childFrames().size(), 0);

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    QCOMPARE(fmt.topMargin(), qreal(10));
    QCOMPARE(fmt.bottomMargin(), qreal(0));
    QCOMPARE(fmt.leftMargin(), qreal(0));
    QCOMPARE(fmt.rightMargin(), qreal(0));
    QCOMPARE(fmt.border(), qreal(1));

    QString normalFrameHtml = QLatin1String(html);
    normalFrameHtml.replace(QLatin1String("root"), QLatin1String("frame"));

    doc->setHtml(normalFrameHtml);
    QCOMPARE(doc->rootFrame()->childFrames().size(), 1);
}

void tst_QTextDocumentFragment::html_appendList()
{
    appendHtml("<p>foo</p>");
    appendHtml("<ul><li>Line 1</li><li>Line 2</li></ul>");

    QCOMPARE(doc->blockCount(), 3);
    QVERIFY(doc->begin().next().textList() != 0);
}

void tst_QTextDocumentFragment::html_appendList2()
{
    appendHtml("1");
    appendHtml("<ul><li><img src=\"/foo/bar\" /></li></ul>");

    QCOMPARE(doc->blockCount(), 2);
    QVERIFY(doc->begin().next().textList() != 0);
}

void tst_QTextDocumentFragment::html_alignmentPropertySet()
{
    const char html[] = "<p>Test</p>";
    setHtml(QString::fromLatin1(html));
    QVERIFY(!doc->begin().blockFormat().hasProperty(QTextFormat::BlockAlignment));
}

void tst_QTextDocumentFragment::html_qt3RichtextWhitespaceMode()
{
    setHtml(QString::fromLatin1("<html><head><meta name=\"qrichtext\" content=\"1\" /></head><p>   line with whitespace</p><p>    another line with whitespace</p></body></html>"));
    QCOMPARE(doc->blockCount(), 2);

    QTextBlock block = doc->begin();
    QVERIFY(block.text().startsWith("   "));

    block = block.next();
    QVERIFY(block.text().startsWith("   "));
}

void tst_QTextDocumentFragment::html_brAfterHr()
{
    setHtml(QString::fromLatin1("Text A<br><hr><br>Text B<hr>"));

    QCOMPARE(doc->blockCount(), 4);

    QTextBlock block = doc->begin();
    QCOMPARE(block.text(), QString("Text A") + QChar(QChar::LineSeparator));

    block = block.next();
    QVERIFY(block.text().isEmpty());

    block = block.next();
    QCOMPARE(block.text(), QChar(QChar::LineSeparator) + QString("Text B"));

    block = block.next();
    QVERIFY(block.text().isEmpty());
}

void tst_QTextDocumentFragment::html_unclosedHead()
{
    doc->setHtml(QString::fromLatin1("<html><head><title>Test</title><body>Blah</body></html>"));
    QCOMPARE(doc->metaInformation(QTextDocument::DocumentTitle), QString::fromLatin1("Test"));
    QCOMPARE(doc->toPlainText(), QString::fromLatin1("Blah"));
}

// duplicated from qtexthtmlparser.cpp
#define MAX_ENTITY 258
static const struct { const char *name; quint16 code; } entities[MAX_ENTITY]= {
    { "AElig", 0x00c6 },
    { "Aacute", 0x00c1 },
    { "Acirc", 0x00c2 },
    { "Agrave", 0x00c0 },
    { "Alpha", 0x0391 },
    { "AMP", 38 },
    { "Aring", 0x00c5 },
    { "Atilde", 0x00c3 },
    { "Auml", 0x00c4 },
    { "Beta", 0x0392 },
    { "Ccedil", 0x00c7 },
    { "Chi", 0x03a7 },
    { "Dagger", 0x2021 },
    { "Delta", 0x0394 },
    { "ETH", 0x00d0 },
    { "Eacute", 0x00c9 },
    { "Ecirc", 0x00ca },
    { "Egrave", 0x00c8 },
    { "Epsilon", 0x0395 },
    { "Eta", 0x0397 },
    { "Euml", 0x00cb },
    { "Gamma", 0x0393 },
    { "GT", 62 },
    { "Iacute", 0x00cd },
    { "Icirc", 0x00ce },
    { "Igrave", 0x00cc },
    { "Iota", 0x0399 },
    { "Iuml", 0x00cf },
    { "Kappa", 0x039a },
    { "Lambda", 0x039b },
    { "LT", 60 },
    { "Mu", 0x039c },
    { "Ntilde", 0x00d1 },
    { "Nu", 0x039d },
    { "OElig", 0x0152 },
    { "Oacute", 0x00d3 },
    { "Ocirc", 0x00d4 },
    { "Ograve", 0x00d2 },
    { "Omega", 0x03a9 },
    { "Omicron", 0x039f },
    { "Oslash", 0x00d8 },
    { "Otilde", 0x00d5 },
    { "Ouml", 0x00d6 },
    { "Phi", 0x03a6 },
    { "Pi", 0x03a0 },
    { "Prime", 0x2033 },
    { "Psi", 0x03a8 },
    { "QUOT", 34 },
    { "Rho", 0x03a1 },
    { "Scaron", 0x0160 },
    { "Sigma", 0x03a3 },
    { "THORN", 0x00de },
    { "Tau", 0x03a4 },
    { "Theta", 0x0398 },
    { "Uacute", 0x00da },
    { "Ucirc", 0x00db },
    { "Ugrave", 0x00d9 },
    { "Upsilon", 0x03a5 },
    { "Uuml", 0x00dc },
    { "Xi", 0x039e },
    { "Yacute", 0x00dd },
    { "Yuml", 0x0178 },
    { "Zeta", 0x0396 },
    { "aacute", 0x00e1 },
    { "acirc", 0x00e2 },
    { "acute", 0x00b4 },
    { "aelig", 0x00e6 },
    { "agrave", 0x00e0 },
    { "alefsym", 0x2135 },
    { "alpha", 0x03b1 },
    { "amp", 38 },
    { "and", 0x22a5 },
    { "ang", 0x2220 },
    { "apos", 0x0027 },
    { "aring", 0x00e5 },
    { "asymp", 0x2248 },
    { "atilde", 0x00e3 },
    { "auml", 0x00e4 },
    { "bdquo", 0x201e },
    { "beta", 0x03b2 },
    { "brvbar", 0x00a6 },
    { "bull", 0x2022 },
    { "cap", 0x2229 },
    { "ccedil", 0x00e7 },
    { "cedil", 0x00b8 },
    { "cent", 0x00a2 },
    { "chi", 0x03c7 },
    { "circ", 0x02c6 },
    { "clubs", 0x2663 },
    { "cong", 0x2245 },
    { "copy", 0x00a9 },
    { "crarr", 0x21b5 },
    { "cup", 0x222a },
    { "curren", 0x00a4 },
    { "dArr", 0x21d3 },
    { "dagger", 0x2020 },
    { "darr", 0x2193 },
    { "deg", 0x00b0 },
    { "delta", 0x03b4 },
    { "diams", 0x2666 },
    { "divide", 0x00f7 },
    { "eacute", 0x00e9 },
    { "ecirc", 0x00ea },
    { "egrave", 0x00e8 },
    { "empty", 0x2205 },
    { "emsp", 0x2003 },
    { "ensp", 0x2002 },
    { "epsilon", 0x03b5 },
    { "equiv", 0x2261 },
    { "eta", 0x03b7 },
    { "eth", 0x00f0 },
    { "euml", 0x00eb },
    { "euro", 0x20ac },
    { "exist", 0x2203 },
    { "fnof", 0x0192 },
    { "forall", 0x2200 },
    { "frac12", 0x00bd },
    { "frac14", 0x00bc },
    { "frac34", 0x00be },
    { "frasl", 0x2044 },
    { "gamma", 0x03b3 },
    { "ge", 0x2265 },
    { "gt", 62 },
    { "hArr", 0x21d4 },
    { "harr", 0x2194 },
    { "hearts", 0x2665 },
    { "hellip", 0x2026 },
    { "iacute", 0x00ed },
    { "icirc", 0x00ee },
    { "iexcl", 0x00a1 },
    { "igrave", 0x00ec },
    { "image", 0x2111 },
    { "infin", 0x221e },
    { "int", 0x222b },
    { "iota", 0x03b9 },
    { "iquest", 0x00bf },
    { "isin", 0x2208 },
    { "iuml", 0x00ef },
    { "kappa", 0x03ba },
    { "lArr", 0x21d0 },
    { "lambda", 0x03bb },
    { "lang", 0x2329 },
    { "laquo", 0x00ab },
    { "larr", 0x2190 },
    { "lceil", 0x2308 },
    { "ldquo", 0x201c },
    { "le", 0x2264 },
    { "lfloor", 0x230a },
    { "lowast", 0x2217 },
    { "loz", 0x25ca },
    { "lrm", 0x200e },
    { "lsaquo", 0x2039 },
    { "lsquo", 0x2018 },
    { "lt", 60 },
    { "macr", 0x00af },
    { "mdash", 0x2014 },
    { "micro", 0x00b5 },
    { "middot", 0x00b7 },
    { "minus", 0x2212 },
    { "mu", 0x03bc },
    { "nabla", 0x2207 },
    { "nbsp", 0x00a0 },
    { "ndash", 0x2013 },
    { "ne", 0x2260 },
    { "ni", 0x220b },
    { "not", 0x00ac },
    { "notin", 0x2209 },
    { "nsub", 0x2284 },
    { "ntilde", 0x00f1 },
    { "nu", 0x03bd },
    { "oacute", 0x00f3 },
    { "ocirc", 0x00f4 },
    { "oelig", 0x0153 },
    { "ograve", 0x00f2 },
    { "oline", 0x203e },
    { "omega", 0x03c9 },
    { "omicron", 0x03bf },
    { "oplus", 0x2295 },
    { "or", 0x22a6 },
    { "ordf", 0x00aa },
    { "ordm", 0x00ba },
    { "oslash", 0x00f8 },
    { "otilde", 0x00f5 },
    { "otimes", 0x2297 },
    { "ouml", 0x00f6 },
    { "para", 0x00b6 },
    { "part", 0x2202 },
    { "percnt", 0x0025 },
    { "permil", 0x2030 },
    { "perp", 0x22a5 },
    { "phi", 0x03c6 },
    { "pi", 0x03c0 },
    { "piv", 0x03d6 },
    { "plusmn", 0x00b1 },
    { "pound", 0x00a3 },
    { "prime", 0x2032 },
    { "prod", 0x220f },
    { "prop", 0x221d },
    { "psi", 0x03c8 },
    { "quot", 34 },
    { "rArr", 0x21d2 },
    { "radic", 0x221a },
    { "rang", 0x232a },
    { "raquo", 0x00bb },
    { "rarr", 0x2192 },
    { "rceil", 0x2309 },
    { "rdquo", 0x201d },
    { "real", 0x211c },
    { "reg", 0x00ae },
    { "rfloor", 0x230b },
    { "rho", 0x03c1 },
    { "rlm", 0x200f },
    { "rsaquo", 0x203a },
    { "rsquo", 0x2019 },
    { "sbquo", 0x201a },
    { "scaron", 0x0161 },
    { "sdot", 0x22c5 },
    { "sect", 0x00a7 },
    { "shy", 0x00ad },
    { "sigma", 0x03c3 },
    { "sigmaf", 0x03c2 },
    { "sim", 0x223c },
    { "spades", 0x2660 },
    { "sub", 0x2282 },
    { "sube", 0x2286 },
    { "sum", 0x2211 },
    { "sup1", 0x00b9 },
    { "sup2", 0x00b2 },
    { "sup3", 0x00b3 },
    { "sup", 0x2283 },
    { "supe", 0x2287 },
    { "szlig", 0x00df },
    { "tau", 0x03c4 },
    { "there4", 0x2234 },
    { "theta", 0x03b8 },
    { "thetasym", 0x03d1 },
    { "thinsp", 0x2009 },
    { "thorn", 0x00fe },
    { "tilde", 0x02dc },
    { "times", 0x00d7 },
    { "trade", 0x2122 },
    { "uArr", 0x21d1 },
    { "uacute", 0x00fa },
    { "uarr", 0x2191 },
    { "ucirc", 0x00fb },
    { "ugrave", 0x00f9 },
    { "uml", 0x00a8 },
    { "upsih", 0x03d2 },
    { "upsilon", 0x03c5 },
    { "uuml", 0x00fc },
    { "weierp", 0x2118 },
    { "xi", 0x03be },
    { "yacute", 0x00fd },
    { "yen", 0x00a5 },
    { "yuml", 0x00ff },
    { "zeta", 0x03b6 },
    { "zwj", 0x200d },
    { "zwnj", 0x200c }
};

void tst_QTextDocumentFragment::html_entities_data()
{
    QTest::addColumn<QString>("html");
    QTest::addColumn<quint16>("code");

    for (int i = 0; i < MAX_ENTITY; ++i) {
        QTest::newRow(entities[i].name) << QString("<pre>&") + QString::fromLatin1(entities[i].name) + QString(";</pre>")
                                        << entities[i].code;
    }
}

void tst_QTextDocumentFragment::html_entities()
{
    QFETCH(QString, html);
    QFETCH(quint16, code);

    setHtml(html);
    QCOMPARE(doc->blockCount(), 1);
    QString txt = doc->begin().text();
    QCOMPARE(txt.length(), 1);
    QCOMPARE(txt.at(0).unicode(), code);
}

void tst_QTextDocumentFragment::html_ignore_script()
{
    doc->setHtml(QString::fromLatin1("<html><script>Test</script><body>Blah</body></html>"));
    QCOMPARE(doc->toPlainText(), QString("Blah"));
}

void tst_QTextDocumentFragment::html_directionWithHtml()
{
    doc->setHtml(QString::fromLatin1("<html><body><p>Test<p dir=rtl>RTL<p dir=ltr>LTR"));
    QCOMPARE(doc->blockCount(), 3);

    QTextBlock block = doc->firstBlock();
    QVERIFY(block.blockFormat().hasProperty(QTextFormat::LayoutDirection));
    QVERIFY(block.blockFormat().layoutDirection() == Qt::LeftToRight); // HTML default

    block = block.next();
    QVERIFY(block.blockFormat().hasProperty(QTextFormat::LayoutDirection));
    QCOMPARE(block.blockFormat().layoutDirection(), Qt::RightToLeft);

    block = block.next();
    QVERIFY(block.blockFormat().hasProperty(QTextFormat::LayoutDirection));
    QCOMPARE(block.blockFormat().layoutDirection(), Qt::LeftToRight);
}

void tst_QTextDocumentFragment::html_directionWithRichText()
{
    doc->setHtml(QString::fromLatin1("<p>Test<p dir=rtl>RTL<p dir=ltr>LTR"));
    QCOMPARE(doc->blockCount(), 3);

    QTextBlock block = doc->firstBlock();
    QVERIFY(!block.blockFormat().hasProperty(QTextFormat::LayoutDirection));

    block = block.next();
    QVERIFY(block.blockFormat().hasProperty(QTextFormat::LayoutDirection));
    QCOMPARE(block.blockFormat().layoutDirection(), Qt::RightToLeft);

    block = block.next();
    QVERIFY(block.blockFormat().hasProperty(QTextFormat::LayoutDirection));
    QCOMPARE(block.blockFormat().layoutDirection(), Qt::LeftToRight);
}

void tst_QTextDocumentFragment::html_metaInBody()
{
    setHtml("<body>Hello<meta>World</body>");
    QCOMPARE(doc->toPlainText(), QString("HelloWorld"));
}

void tst_QTextDocumentFragment::html_importImageWithoutAspectRatio()
{
    doc->setHtml("<img src=\"foo\" width=\"100%\" height=\"43\">");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    QTextImageFormat fmt = cursor.charFormat().toImageFormat();
    // qDebug() << fmt.width() << fmt.height();
    QVERIFY (fmt.hasProperty(QTextFormat::ImageWidth));
    QCOMPARE (fmt.height(), 43.);

    doc->setHtml("<img src=\"foo\" height=\"43\">");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    fmt = cursor.charFormat().toImageFormat();
    QVERIFY (! fmt.hasProperty(QTextFormat::ImageWidth));
    QCOMPARE (fmt.height(), 43.);

    doc->setHtml("<img src=\"foo\" width=\"200\">");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().isImageFormat());
    fmt = cursor.charFormat().toImageFormat();
    QVERIFY (! fmt.hasProperty(QTextFormat::ImageHeight));
    QCOMPARE (fmt.width(), 200.);
}

void tst_QTextDocumentFragment::html_fromFirefox()
{
    // if you have a html loaded in firefox like <html>Test\nText</html>  then selecting all and copying will
    // result in the following text on the clipboard (for text/html)
    doc->setHtml(QString::fromLatin1("<!--StartFragment-->Test\nText\n\n<!--EndFragment-->"));
    QCOMPARE(doc->toPlainText(), QString::fromLatin1("Test Text "));
}

void tst_QTextDocumentFragment::html_emptyInlineInsideBlock()
{
    doc->setHtml(QString::fromLatin1("<!--StartFragment--><blockquote><span/>Foobar</blockquote><!--EndFragment-->"));
    QVERIFY(doc->firstBlock().blockFormat().leftMargin() > 0);
}

void tst_QTextDocumentFragment::css_fontAndWordSpacing()
{
    {
        const char html[] = "<body style=\"letter-spacing:13px; word-spacing:15px;\">Foo</span>";
        doc->setHtml(html);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextCharacter);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacing).toInt(), 13);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacingType).toUInt(),
                 (uint)(QFont::AbsoluteSpacing));
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontWordSpacing).toInt(), 15);
    }
    {
        const char html[] = "<body style=\"letter-spacing:1em; word-spacing:0px;\">Foo</span>";
        doc->setHtml(html);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextCharacter);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacing).toInt(), 200);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacingType).toUInt(),
                 (uint)(QFont::PercentageSpacing));
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontWordSpacing).toInt(), 0);
    }
    {
        const char html[] = "<body style=\"letter-spacing:0em;\">Foo</span>";
        doc->setHtml(html);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextCharacter);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacing).toInt(), 100);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacingType).toUInt(),
                 (uint)(QFont::PercentageSpacing));
    }
    {
        const char html[] = "<body style=\"letter-spacing:-0.5em;\">Foo</span>";
        doc->setHtml(html);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextCharacter);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacing).toInt(), 50);
        QCOMPARE(cursor.charFormat().property(QTextFormat::FontLetterSpacingType).toUInt(),
                 (uint)(QFont::PercentageSpacing));
    }
}

QTEST_MAIN(tst_QTextDocumentFragment)
#include "tst_qtextdocumentfragment.moc"
