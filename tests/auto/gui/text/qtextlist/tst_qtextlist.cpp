// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qtextdocument.h>
#include <qtextdocumentfragment.h>
#include <qtextlist.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextcursor.h>
#include "../qtextdocument/common.h"

class tst_QTextList : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
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
    void start_data();
    void start();

private:
    QTextDocument *doc;
    QTextCursor cursor;
    QTestDocumentLayout *layout;
};

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

    QCOMPARE(list->count(), 28);

    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->itemNumber(cursor.block()), 27);
    QCOMPARE(cursor.currentList()->itemText(cursor.block()), QLatin1String("ab."));
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

    QCOMPARE(list->count(), 28);

    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->itemNumber(cursor.block()), 27);
    QCOMPARE(cursor.currentList()->itemText(cursor.block()), QLatin1String("-ab)"));
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

    QCOMPARE(list->count(), 2);

    QCOMPARE(cursor.currentList()->itemText(cursor.block()), QLatin1String("*B-"));
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

    QCOMPARE(list->count(), 28);

    QString htmlExport = doc->toHtml();
    QTextDocument importDoc;
    importDoc.setHtml(htmlExport);

    QTextCursor importCursor(&importDoc);
    for (int i = 0; i < 27; ++i)
        importCursor.movePosition(QTextCursor::NextBlock);

    QVERIFY(importCursor.currentList());
    QCOMPARE(importCursor.currentList()->itemNumber(importCursor.block()), 27);
    QCOMPARE(importCursor.currentList()->itemText(importCursor.block()), QLatin1String("\"ab#"));
    QCOMPARE(importCursor.currentList()->format().indent(), 10);
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

    QCOMPARE(list->count(), 2);

    QCOMPARE(cursor.currentList()->itemText(cursor.block()), QLatin1String(".B"));
}

void tst_QTextList::romanNumbering()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListUpperRoman);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 4998; ++i)
      cursor.insertBlock();

    QCOMPARE(list->count(), 4999);

    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->itemNumber(cursor.block()), 4998);
    QCOMPARE(cursor.currentList()->itemText(cursor.block()), QLatin1String("MMMMCMXCIX."));
}

void tst_QTextList::romanNumberingLimit()
{
    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::ListLowerRoman);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    for (int i = 0; i < 4999; ++i)
      cursor.insertBlock();

    QCOMPARE(list->count(), 5000);

    QVERIFY(cursor.currentList());
    QCOMPARE(cursor.currentList()->itemNumber(cursor.block()), 4999);
    QCOMPARE(cursor.currentList()->itemText(cursor.block()), QLatin1String("?."));
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
//     QCOMPARE(bfmt.object(), list);

    bfmt.setObjectIndex(-1);
    cursor.setBlockFormat(bfmt);

    QCOMPARE(firstList->count(), 1);
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
    QCOMPARE(cursor.currentList()->itemNumber(cursor.block()), 0);
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
    QVERIFY(firstList->count() > 0);

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

void tst_QTextList::start_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("start");
    QTest::addColumn<QStringList>("expectedItemTexts");

    QTest::newRow("-1.") << int(QTextListFormat::ListDecimal) << -1
                         << QStringList{ "-1.", "0.", "1." };
    QTest::newRow("0.") << int(QTextListFormat::ListDecimal) << 0
                        << QStringList{ "0.", "1.", "2." };
    QTest::newRow("1.") << int(QTextListFormat::ListDecimal) << 1
                        << QStringList{ "1.", "2.", "3." };

    QTest::newRow("A. -1") << int(QTextListFormat::ListUpperAlpha) << -1
                           << QStringList{ "-1.", "0.", "A." };
    QTest::newRow("A. 0.") << int(QTextListFormat::ListUpperAlpha) << 0
                           << QStringList{ "0.", "A.", "B." };
    QTest::newRow("a. -1") << int(QTextListFormat::ListLowerAlpha) << -1
                           << QStringList{ "-1.", "0.", "a." };
    QTest::newRow("a. 0.") << int(QTextListFormat::ListLowerAlpha) << 0
                           << QStringList{ "0.", "a.", "b." };
    QTest::newRow("d. 4.") << int(QTextListFormat::ListLowerAlpha) << 4
                           << QStringList{ "d.", "e.", "f." };

    QTest::newRow("I. -1") << int(QTextListFormat::ListUpperRoman) << -1
                           << QStringList{ "-1.", "0.", "I." };
    QTest::newRow("I. 0.") << int(QTextListFormat::ListUpperRoman) << 0
                           << QStringList{ "0.", "I.", "II." };
    QTest::newRow("i. -1") << int(QTextListFormat::ListLowerRoman) << -1
                           << QStringList{ "-1.", "0.", "i." };
    QTest::newRow("i. 0.") << int(QTextListFormat::ListLowerRoman) << 0
                           << QStringList{ "0.", "i.", "ii." };
}

void tst_QTextList::start()
{
    QFETCH(int, format);
    QFETCH(int, start);
    QFETCH(QStringList, expectedItemTexts);

    QTextListFormat fmt;
    fmt.setStyle(QTextListFormat::Style(format));
    fmt.setStart(start);
    QTextList *list = cursor.createList(fmt);
    QVERIFY(list);

    while (list->count() < int(expectedItemTexts.size()))
        cursor.insertBlock();

    QCOMPARE(list->count(), expectedItemTexts.size());

    for (int i = 0; i < list->count(); ++i)
        QCOMPARE(cursor.currentList()->itemText(cursor.currentList()->item(i)),
                 expectedItemTexts[i]);
}

QTEST_MAIN(tst_QTextList)
#include "tst_qtextlist.moc"
