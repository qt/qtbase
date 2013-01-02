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

#include <qtextdocument.h>
#include <qtextdocumentfragment.h>
#include <qtextlist.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextcursor.h>
#include "../qtextdocument/common.h"

class tst_QTextList : public QObject
{
    Q_OBJECT

public:
    tst_QTextList();


public slots:
    void init();
    void cleanup();
private slots:
    void item();
    void autoNumbering();
    void autoNumberingRTL();
    void autoNumberingPrefixAndSuffix();
    void autoNumberingPrefixAndSuffixRTL();
    void autoNumberingPrefixAndSuffixHtmlExportImport();
    void romanNumbering();
    void romanNumberingLimit();
    void formatChange();
    void cursorNavigation();
    void partialRemoval();
    void formatReferenceChange();
    void ensureItemOrder();
    void add();
    void defaultIndent();
    void blockUpdate();
    void numbering_data();
    void numbering();

private:
    QTextDocument *doc;
    QTextCursor cursor;
    QTestDocumentLayout *layout;
};

tst_QTextList::tst_QTextList()
{}

void tst_QTextList::init()
{
    doc = new QTextDocument();
    layout = new QTestDocumentLayout(doc);
    doc->setDocumentLayout(layout);
    cursor = QTextCursor(doc);
}

void tst_QTextList::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextList::item()
{
    // this is basically a test for the key() + 1 in QTextList::item.
    QTextList *list = cursor.createList(QTextListFormat());
    QVERIFY(list->item(0).blockFormat().objectIndex() != -1);
}

void tst_QTextList::autoNumbering()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListLowerAlpha);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 27; ++i)
	cursor.insertBlock();

    QVERIFY(list->count() == 28);

    QVERIFY(cursor.currentList());
    QVERIFY(cursor.currentList()->itemNumber(cursor.block()) == 27);
    QVERIFY(cursor.currentList()->itemText(cursor.block()) == "ab.");
}

void tst_QTextList::autoNumberingPrefixAndSuffix()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListLowerAlpha);
    fmt.setNumberPrefix("-");
    fmt.setNumberSuffix(")");
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 27; ++i)
        cursor.insertBlock();

    QVERIFY(list->count() == 28);

    QVERIFY(cursor.currentList());
    QVERIFY(cursor.currentList()->itemNumber(cursor.block()) == 27);
    QVERIFY(cursor.currentList()->itemText(cursor.block()) == "-ab)");
}

void tst_QTextList::autoNumberingPrefixAndSuffixRTL()
{
    QTextBlockFormat bfmt;
    bfmt.setLayoutDirection(Qt::RightToLeft);
    cursor.setBlockFormat(bfmt);

    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListUpperAlpha);
    fmt.setNumberPrefix("-");
    fmt.setNumberSuffix("*");
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    cursor.insertBlock();

    QVERIFY(list->count() == 2);

    QVERIFY(cursor.currentList()->itemText(cursor.block()) == "*B-");
}

void tst_QTextList::autoNumberingPrefixAndSuffixHtmlExportImport()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListLowerAlpha);
    fmt.setNumberPrefix("\"");
    fmt.setNumberSuffix("#");
    fmt.setIndent(10);
    // FIXME: Would like to test "'" but there's a problem in the css parser (Scanner::preprocess
    // is called before the values are being parsed), so the quoting does not work.
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 27; ++i)
        cursor.insertBlock();

    QVERIFY(list->count() == 28);

    QString htmlExport = doc->toHtml();
    QTextDocument importDoc;
    importDoc.setHtml(htmlExport);

    QTextCursor importCursor(&importDoc);
    for (int i = 0; i < 27; ++i)
        importCursor.movePosition(QTextCursor::NextBlock);

    QVERIFY(importCursor.currentList());
    QVERIFY(importCursor.currentList()->itemNumber(importCursor.block()) == 27);
    QVERIFY(importCursor.currentList()->itemText(importCursor.block()) == "\"ab#");
    QVERIFY(importCursor.currentList()->format().indent() == 10);
}

void tst_QTextList::autoNumberingRTL()
{
    QTextBlockFormat bfmt;
    bfmt.setLayoutDirection(Qt::RightToLeft);
    cursor.setBlockFormat(bfmt);

    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListUpperAlpha);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    cursor.insertBlock();

    QVERIFY(list->count() == 2);

    QVERIFY(cursor.currentList()->itemText(cursor.block()) == ".B");
}

void tst_QTextList::romanNumbering()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListUpperRoman);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 4998; ++i)
      cursor.insertBlock();

    QVERIFY(list->count() == 4999);

    QVERIFY(cursor.currentList());
    QVERIFY(cursor.currentList()->itemNumber(cursor.block()) == 4998);
    QVERIFY(cursor.currentList()->itemText(cursor.block()) == "MMMMCMXCIX.");
}

void tst_QTextList::romanNumberingLimit()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListLowerRoman);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 4999; ++i)
      cursor.insertBlock();

    QVERIFY(list->count() == 5000);

    QVERIFY(cursor.currentList());
    QVERIFY(cursor.currentList()->itemNumber(cursor.block()) == 4999);
    QVERIFY(cursor.currentList()->itemText(cursor.block()) == "?.");
}

void tst_QTextList::formatChange()
{
    // testing the formatChanged slot in QTextListManager

    /* <initial block>
     * 1.
     * 2.
     */
    QTextList *list = cursor.insertList(QTextListFormat::ListDecimal);
    QTextList *firstList = list;
    cursor.insertBlock();

    QVERIFY(list && list->count() == 2);

    QTextBlockFormat bfmt = cursor.blockFormat();
//     QVERIFY(bfmt.object() == list);

    bfmt.setObjectIndex(-1);
    cursor.setBlockFormat(bfmt);

    QVERIFY(firstList->count() == 1);
}

void tst_QTextList::cursorNavigation()
{
    // testing some cursor list methods

    /* <initial block>
     * 1.
     * 2.
     */
    cursor.insertList(QTextListFormat::ListDecimal);
    cursor.insertBlock();

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.currentList());
    cursor.movePosition(QTextCursor::PreviousBlock);
    QVERIFY(cursor.currentList());
    QVERIFY(cursor.currentList()->itemNumber(cursor.block()) == 0);
}

void tst_QTextList::partialRemoval()
{
    /* this is essentially a test for PieceTable::removeBlock to not miss any
       blocks with the blockChanged signal emission that actually get removed.

       It creates two lists, like this:

       1. Hello World
       a. Foobar

       and then removes from within the 'Hello World' into the 'Foobar' .
       There used to be no emission for the removal of the second (a.) block,
       causing list inconsistencies.

       */

    QTextList *firstList = cursor.insertList(QTextListFormat::ListDecimal);

    QTextCursor selStart = cursor;
    selStart.movePosition(QTextCursor::PreviousCharacter);

    cursor.insertText("Hello World");

    // position it well into the 'hello world' text.
    selStart.movePosition(QTextCursor::NextCharacter);
    selStart.movePosition(QTextCursor::NextCharacter);
    selStart.clearSelection();

    QPointer<QTextList> secondList = cursor.insertList(QTextListFormat::ListCircle);
    cursor.insertText("Foobar");

    // position it into the 'foo bar' text.
    cursor.movePosition(QTextCursor::PreviousCharacter);
    QTextCursor selEnd = cursor;

    // this creates a selection that includes parts of both text-fragments and also the list item of the second list.
    QTextCursor selection = selStart;
    selection.setPosition(selEnd.position(),  QTextCursor::KeepAnchor);

    selection.deleteChar(); // deletes the second list

    QVERIFY(!secondList);
    QVERIFY(!firstList->isEmpty());

    doc->undo();
}

void tst_QTextList::formatReferenceChange()
{
    QTextList *list = cursor.insertList(QTextListFormat::ListDecimal);
    cursor.insertText("Some Content...");
    cursor.insertBlock(QTextBlockFormat());

    cursor.setPosition(list->item(0).position());
    int listItemStartPos = cursor.position();
    cursor.movePosition(QTextCursor::NextBlock);
    int listItemLen = cursor.position() - listItemStartPos;
    layout->expect(listItemStartPos, listItemLen, listItemLen);

    QTextListFormat fmt = list->format();
    fmt.setStyle(QTextListFormat::ListCircle);
    list->setFormat(fmt);

    QVERIFY(layout->called);
    QVERIFY(!layout->error);
}

void tst_QTextList::ensureItemOrder()
{
    /*
     * Insert a new list item before the first one and verify the blocks
     * are sorted after that.
     */
    QTextList *list = cursor.insertList(QTextListFormat::ListDecimal);

    QTextBlockFormat fmt = cursor.blockFormat();
    cursor.movePosition(QTextCursor::Start);
    cursor.insertBlock(fmt);

    QCOMPARE(list->item(0).position(), 1);
    QCOMPARE(list->item(1).position(), 2);
}

void tst_QTextList::add()
{
    QTextList *list = cursor.insertList(QTextListFormat::ListDecimal);
    cursor.insertBlock(QTextBlockFormat());
    QCOMPARE(list->count(), 1);
    cursor.insertBlock(QTextBlockFormat());
    list->add(cursor.block());
    QCOMPARE(list->count(), 2);
}

// Task #72036
void tst_QTextList::defaultIndent()
{
    QTextListFormat fmt;
    QCOMPARE(fmt.indent(), 1);
}

void tst_QTextList::blockUpdate()
{
    // three items
    QTextList *list = cursor.insertList(QTextListFormat::ListDecimal);
    cursor.insertBlock();
    cursor.insertBlock();

    // remove second, needs also update on the third
    // since the numbering might have changed
    const int len = cursor.position() + cursor.block().length() - 1;
    layout->expect(1, len, len);
    list->remove(list->item(1));
    QVERIFY(!layout->error);
}

void tst_QTextList::numbering_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("number");
    QTest::addColumn<QString>("result");

    QTest::newRow("E.") << int(QTextListFormat::ListUpperAlpha) << 5 << "E.";
    QTest::newRow("abc.") << int(QTextListFormat::ListLowerAlpha) << (26 + 2) * 26 + 3 << "abc.";
    QTest::newRow("12.") << int(QTextListFormat::ListDecimal) << 12 << "12.";
    QTest::newRow("XXIV.") << int(QTextListFormat::ListUpperRoman) << 24 << "XXIV.";
    QTest::newRow("VIII.") << int(QTextListFormat::ListUpperRoman) << 8 << "VIII.";
    QTest::newRow("xxx.") << int(QTextListFormat::ListLowerRoman) << 30 << "xxx.";
    QTest::newRow("xxix.") << int(QTextListFormat::ListLowerRoman) << 29 << "xxix.";
//    QTest::newRow("xxx. alpha") << int(QTextListFormat::ListLowerAlpha) << (24 * 26 + 24) * 26 + 24  << "xxx."; //Too slow
}

void tst_QTextList::numbering()
{
    QFETCH(int, format);
    QFETCH(int, number);
    QFETCH(QString, result);


    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::Style(format));
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 1; i < number; ++i)
        cursor.insertBlock();

    QCOMPARE(list->count(), number);

    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->itemNumber(cursor.block()), number - 1);
    QCOMPARE(cursor.currentList()->itemText(cursor.block()), result);
}

QTEST_MAIN(tst_QTextList)
#include "tst_qtextlist.moc"
