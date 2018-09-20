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
#include <qtexttable.h>
#include <qvariant.h>
#include <qtextdocumentfragment.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextlayout.h>
#include <qtextcursor.h>
#include <qtextobject.h>
#include <qdebug.h>

#include <private/qtextcursor_p.h>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

class tst_QTextCursor : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void navigation1();
    void navigation2_data();
    void navigation2();
    void navigation3();
    void navigation4();
    void navigation5();
    void navigation6();
    void navigation7();
    void navigation8();
    void navigation9();
    void navigation10();
    void movePositionEndOfLine();
    void insertBlock();
    void insertWithBlockSeparator1();
    void insertWithBlockSeparator2();
    void insertWithBlockSeparator3();
    void insertWithBlockSeparator4();
    void clearObjectType1();
    void clearObjectType2();
    void clearObjectType3();
    void comparisonOperators1();
    void comparisonOperators2();
    void selection1();
    void dontCopyTableAttributes();

    void checkFrame1();
    void checkFrame2();

    void tableMovement();
    void selectionsInTable();

    void insertBlockToUseCharFormat();

    void selectedText();

    void insertBlockShouldRemoveSelection();
    void insertBlockShouldRemoveSelection2();
    void mergeCellShouldUpdateSelection();

    void joinPreviousEditBlock();

    void setBlockFormatInTable();

    void blockCharFormat();
    void blockCharFormat2();
    void blockCharFormat3();
    void blockCharFormatOnSelection();

    void anchorInitialized1();
    void anchorInitialized2();
    void anchorInitialized3();

    void selectWord();
    void selectWordWithSeparators_data();
    void selectWordWithSeparators();
    void startOfWord();
    void selectBlock();
    void selectVisually();

    void insertText();

    void insertFragmentShouldUseCurrentCharFormat();

    void endOfLine();

    void editBlocksDuringRemove();
    void selectAllDuringRemove();

    void update_data();
    void update();

    void disallowSettingObjectIndicesOnCharFormats();

    void blockAndColumnNumber();

    void clearCells();

    void task244408_wordUnderCursor_data();
    void task244408_wordUnderCursor();

    void adjustCursorsOnInsert();

    void cursorPositionWithBlockUndoAndRedo();
    void cursorPositionWithBlockUndoAndRedo2();
    void cursorPositionWithBlockUndoAndRedo3();

    void joinNonEmptyRemovedBlockUserState();
    void crashOnDetachingDanglingCursor();

private:
    int blockCount();

    QTextDocument *doc;
    QTextCursor cursor;
};

void tst_QTextCursor::init()
{
    doc = new QTextDocument;
    cursor = QTextCursor(doc);
}

void tst_QTextCursor::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextCursor::navigation1()
{

    cursor.insertText("Hello World");
    QCOMPARE(doc->toPlainText(), QLatin1String("Hello World"));

    cursor.movePosition(QTextCursor::End);
    QCOMPARE(cursor.position(), 11);
    cursor.deletePreviousChar();
    QCOMPARE(cursor.position(), 10);
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    QCOMPARE(doc->toPlainText(), QLatin1String("Hello"));

    QTextCursor otherCursor(doc);
    otherCursor.movePosition(QTextCursor::Start);
    otherCursor.movePosition(QTextCursor::Right);
    cursor = otherCursor;
    cursor.movePosition(QTextCursor::Right);
    QVERIFY(cursor != otherCursor);
    otherCursor.insertText("Hey");
    QCOMPARE(cursor.position(), 5);

    doc->undo();
    QCOMPARE(cursor.position(), 2);
    doc->redo();
    QCOMPARE(cursor.position(), 5);

    doc->undo();

    doc->undo();
    QCOMPARE(doc->toPlainText(), QLatin1String("Hello World"));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 6);
    QCOMPARE(cursor.position(), 6);
    otherCursor = cursor;
    otherCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 2);
    otherCursor.deletePreviousChar();
    otherCursor.deletePreviousChar();
    otherCursor.deletePreviousChar();
    QCOMPARE(cursor.position(), 5);

    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();
    {
        int oldPos = cursor.position();
        cursor.movePosition(QTextCursor::End);
        QCOMPARE(cursor.position(), oldPos);
    }
    QVERIFY(cursor.atBlockStart());
    QCOMPARE(cursor.position(), 9);

    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertText("Test", fmt);
    QCOMPARE(fmt, cursor.charFormat());
    QCOMPARE(cursor.position(), 13);
}

void tst_QTextCursor::navigation2_data()
{
    QTest::addColumn<QStringList>("sl");
    QTest::addColumn<QList<QVariant> >("movement");
    QTest::addColumn<int>("finalPos");

    QTest::newRow("startBlock1") << QStringList("Happy happy happy joy joy joy")
                              << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)) << 0;
    QTest::newRow("endBlock1") << QStringList("Happy happy happy joy joy joy")
                            << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)) << 29;
    QTest::newRow("startBlock2") << QStringList("Happy happy happy joy joy joy")
                              << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)
                                     << QVariant(QTextCursor::StartOfBlock)) << 0;
    QTest::newRow("endBlock2") << QStringList("Happy happy happy joy joy joy")
                            << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)
                                     << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)
                                     ) << 29;
    QTest::newRow("multiBlock1") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock))
                             << 18;
    QTest::newRow("multiBlock2") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 29;
    QTest::newRow("multiBlock3") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                                   << QVariant(QTextCursor::StartOfBlock))
                             << 18;
    QTest::newRow("multiBlock4") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 17;
    QTest::newRow("multiBlock5") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                   << QVariant(QTextCursor::EndOfBlock)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 17;
    QTest::newRow("multiBlock6") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::End)
                                                   << QVariant(QTextCursor::StartOfBlock))
                             << 18;
    QTest::newRow("multiBlock7") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock))
                             << 0;
    QTest::newRow("multiBlock8") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 17;
    QTest::newRow("multiBlock9") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                   << QVariant(QTextCursor::NextBlock))
                             << 18;
    QTest::newRow("multiBlock10") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                               << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                     << QVariant(QTextCursor::NextBlock)
                                                     << QVariant(QTextCursor::NextBlock))
                               << 18;
    QTest::newRow("multiBlock11") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                               << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                     << QVariant(QTextCursor::NextBlock)
                                                     << QVariant(QTextCursor::EndOfBlock))
                               << 29;
    QTest::newRow("PreviousWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord))
                                << 26;
    QTest::newRow("PreviousWord2") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::PreviousWord))
                                << 22;
    QTest::newRow("EndWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::EndOfWord))
                                << 25;
    QTest::newRow("NextWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::NextWord))
                                << 26;
    QTest::newRow("NextWord2") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                      << QVariant(QTextCursor::NextWord)
                                                      << QVariant(QTextCursor::EndOfWord))
                                << 11;
    QTest::newRow("StartWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                   << QVariant(QTextCursor::PreviousWord)
                                                   << QVariant(QTextCursor::StartOfWord))
                             << 22;
    QTest::newRow("StartWord3") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                   << QVariant(QTextCursor::NextWord)
                                                   << QVariant(QTextCursor::EndOfWord)
                                                   << QVariant(QTextCursor::StartOfWord))
                             << 6;

    QTest::newRow("PreviousCharacter") << (QStringList() << QString("Happy happy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousCharacter)
                                                   << QVariant(QTextCursor::PreviousCharacter))
                             << 17;
}

void tst_QTextCursor::navigation2()
{
    QFETCH(QStringList, sl);
    QFETCH(QList<QVariant>, movement);
    int i;
    for (i = 0; i < sl.size(); ++i) {
        cursor.insertText(sl.at(i));
        if (i < sl.size() - 1)
            cursor.insertBlock();
    }

    for (i = 0; i < movement.size(); ++i)
        cursor.movePosition(QTextCursor::MoveOperation(movement.at(i).toInt()));
    QTEST(cursor.position(), "finalPos");
}

void tst_QTextCursor::navigation3()
{
    cursor.insertText("a");
    cursor.deletePreviousChar();
    QCOMPARE(cursor.position(), 0);
    QVERIFY(doc->toPlainText().isEmpty());
}

void tst_QTextCursor::navigation4()
{
    cursor.insertText("  Test  ");

    cursor.setPosition(4);
    cursor.movePosition(QTextCursor::EndOfWord);
    QCOMPARE(cursor.position(), 6);
}

void tst_QTextCursor::navigation5()
{
    cursor.insertText("Test");
    cursor.insertBlock();
    cursor.insertText("Test");

    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::EndOfBlock);
    QCOMPARE(cursor.position(), 4);
}

void tst_QTextCursor::navigation6()
{
    // triger creation of document layout, so that QTextLines are there
    doc->documentLayout();
    doc->setTextWidth(1000);

    cursor.insertText("Test    ");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfLine);
    QCOMPARE(cursor.position(), 8);
}

void tst_QTextCursor::navigation7()
{
    QVERIFY(doc->isEmpty());
    for (int i = QTextCursor::Start; i <= QTextCursor::WordRight; ++i)
        QVERIFY(!cursor.movePosition(QTextCursor::MoveOperation(i)));

    doc->setPlainText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    do {
    } while (cursor.movePosition(QTextCursor::NextCharacter));
    QVERIFY(true /*reached*/);
}

void tst_QTextCursor::navigation8()
{
    cursor.insertList(QTextListFormat::ListDecimal);
    QCOMPARE(cursor.position(), 1);
    cursor.insertText("foo");
    QCOMPARE(cursor.position(), 4);

    cursor.insertList(QTextListFormat::ListCircle);
    QCOMPARE(cursor.position(), 5);
    cursor.insertText("something");
    QCOMPARE(cursor.position(), 14);

    cursor.movePosition(QTextCursor::PreviousCharacter);
    QCOMPARE(cursor.position(), 13);

    cursor.setPosition(2);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.position(), 3);
}

void tst_QTextCursor::navigation9()
{
    cursor.insertText("Hello  &-=+\t   World");
    cursor.movePosition(QTextCursor::PreviousWord);
    QCOMPARE(cursor.position(), 15);
    cursor.movePosition(QTextCursor::PreviousWord);
    QCOMPARE(cursor.position(), 7);
    cursor.movePosition(QTextCursor::PreviousWord);
    QCOMPARE(cursor.position(), 0);
    cursor.movePosition(QTextCursor::NextWord);
    QCOMPARE(cursor.position(), 7);
    cursor.movePosition(QTextCursor::NextWord);
    QCOMPARE(cursor.position(), 15);
}

void tst_QTextCursor::navigation10()
{
    doc->setHtml("<html><p>just a simple paragraph.</p>"
        "<table>"
          "<tr><td>Cell number 1</td><td>another cell</td><td></td><td>previous</br>is</br>empty</td></tr>"
          "<tr><td>row 2</td><td colspan=\"2\">foo bar</td><td>last cell</td></tr>"
          "<tr><td colspan=\"3\">row 3</td><td>a</td></tr>"
        "</table></html");
    QCOMPARE(cursor.position(), 101); // end of document
    cursor.setPosition(0);
    QCOMPARE(cursor.position(), 0);
    bool ok = cursor.movePosition(QTextCursor::EndOfLine);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 24);
    ok = cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 25); // cell 1
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 39); // another..
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 52); // empty
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 53); // last on row 1
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 69); // row 2
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 75);
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 83);
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 93); // row 3
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 99);
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(!ok);
    QCOMPARE(cursor.position(), 99); // didn't move.
    QVERIFY(cursor.currentTable());

    // same thing in reverse...
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 93);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 83);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 75);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 69);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 53);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 52);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 39);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 25);
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(!ok);
    QCOMPARE(cursor.position(), 25); // can't leave the table

    ok = cursor.movePosition(QTextCursor::NextRow);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 69);
    ok = cursor.movePosition(QTextCursor::NextRow);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 93);
    ok = cursor.movePosition(QTextCursor::NextRow);
    QVERIFY(!ok);
    QCOMPARE(cursor.position(), 93); // didn't move

    ok = cursor.movePosition(QTextCursor::PreviousRow);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 83); // last col in row 2
    ok = cursor.movePosition(QTextCursor::PreviousRow);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 53); // last col in row 1
    ok = cursor.movePosition(QTextCursor::PreviousRow);
    QVERIFY(!ok);
    QCOMPARE(cursor.position(), 53);

    // test usecase of jumping over a cell
    doc->clear();
    doc->setHtml("<html><table>tr><td rowspan=\"2\">a</td><td>b</td></tr><tr><td>c</td></tr></table></html>");
    cursor.setPosition(1); // a
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 3); // b
    ok = cursor.movePosition(QTextCursor::NextCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 5); // c
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 3); // b
    ok = cursor.movePosition(QTextCursor::PreviousCell);
    QVERIFY(ok);
    QCOMPARE(cursor.position(), 1); // a
}

void tst_QTextCursor::insertBlock()
{
    QTextBlockFormat fmt;
    fmt.setTopMargin(100);
    cursor.insertBlock(fmt);
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.blockFormat(), fmt);
}

void tst_QTextCursor::insertWithBlockSeparator1()
{
    QString text = "Hello" + QString(QChar::ParagraphSeparator) + "World";

    cursor.insertText(text);

    cursor.movePosition(QTextCursor::PreviousBlock);
    QCOMPARE(cursor.position(), 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 6);
}

void tst_QTextCursor::insertWithBlockSeparator2()
{
    cursor.insertText(QString(QChar::ParagraphSeparator));
    QCOMPARE(cursor.position(), 1);
}

void tst_QTextCursor::insertWithBlockSeparator3()
{
    cursor.insertText(QString(QChar::ParagraphSeparator) + "Hi" + QString(QChar::ParagraphSeparator));
    QCOMPARE(cursor.position(), 4);
}

void tst_QTextCursor::insertWithBlockSeparator4()
{
    cursor.insertText(QString(QChar::ParagraphSeparator) + QString(QChar::ParagraphSeparator));
    QCOMPARE(cursor.position(), 2);
}

void tst_QTextCursor::clearObjectType1()
{
    cursor.insertImage("test.png");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(cursor.charFormat().isImageFormat());
    cursor.insertText("Hey");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(!cursor.charFormat().isImageFormat());
}

void tst_QTextCursor::clearObjectType2()
{
    cursor.insertImage("test.png");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(cursor.charFormat().isImageFormat());
    cursor.insertBlock();
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(!cursor.charFormat().isImageFormat());
}

void tst_QTextCursor::clearObjectType3()
{
    // like clearObjectType2 but tests different insertBlock overload
    cursor.insertImage("test.png");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(cursor.charFormat().isImageFormat());
    QTextBlockFormat bfmt;
    bfmt.setAlignment(Qt::AlignRight);
    cursor.insertBlock(bfmt);
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(!cursor.charFormat().isImageFormat());
}

void tst_QTextCursor::comparisonOperators1()
{
    cursor.insertText("Hello World");

    cursor.movePosition(QTextCursor::PreviousWord);

    QTextCursor startCursor = cursor;
    startCursor.movePosition(QTextCursor::Start);

    QVERIFY(startCursor < cursor);

    QTextCursor midCursor = startCursor;
    midCursor.movePosition(QTextCursor::NextWord);

    QVERIFY(midCursor <= cursor);
    QCOMPARE(midCursor, cursor);
    QVERIFY(midCursor >= cursor);

    QVERIFY(midCursor > startCursor);

    QVERIFY(midCursor != startCursor);
    QVERIFY(!(midCursor == startCursor));

    QTextCursor nullCursor;

    QVERIFY(!(startCursor < nullCursor));
    QVERIFY(!(nullCursor < nullCursor));
    QVERIFY(nullCursor < startCursor);

    QVERIFY(nullCursor <= startCursor);
    QVERIFY(!(startCursor <= nullCursor));

    QVERIFY(!(nullCursor >= startCursor));
    QVERIFY(startCursor >= nullCursor);

    QVERIFY(!(nullCursor > startCursor));
    QVERIFY(!(nullCursor > nullCursor));
    QVERIFY(startCursor > nullCursor);
}

void tst_QTextCursor::comparisonOperators2()
{
    QTextDocument doc1;
    QTextDocument doc2;

    QTextCursor cursor1(&doc1);
    QTextCursor cursor2(&doc2);

    QVERIFY(cursor1 != cursor2);
    QCOMPARE(cursor1, QTextCursor(&doc1));
}

void tst_QTextCursor::selection1()
{
    cursor.insertText("Hello World");

    cursor.setPosition(0);
    cursor.clearSelection();
    cursor.setPosition(4, QTextCursor::KeepAnchor);

    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 4);
}

void tst_QTextCursor::dontCopyTableAttributes()
{
    /* when pressing 'enter' inside a cell it shouldn't
     * enlarge the table by adding another cell but just
     * extend the cell */
    QTextTable *table = cursor.insertTable(2, 2);
    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());
    cursor.insertBlock();
    QCOMPARE(table->columns(), 2);
}

void tst_QTextCursor::checkFrame1()
{
    QCOMPARE(cursor.position(), 0);
    QPointer<QTextFrame> frame = cursor.insertFrame(QTextFrameFormat());
    QVERIFY(frame != 0);

    QTextFrame *root = frame->parentFrame();
    QVERIFY(root != 0);

    QCOMPARE(frame->firstPosition(), 1);
    QCOMPARE(frame->lastPosition(), 1);
    QVERIFY(frame->parentFrame() != 0);
    QCOMPARE(root->childFrames().size(), 1);

    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);

    doc->undo();

    QVERIFY(!frame);
    QCOMPARE(root->childFrames().size(), 0);

    QCOMPARE(cursor.position(), 0);
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 0);

    doc->redo();

    frame = doc->frameAt(1);

    QVERIFY(frame);
    QCOMPARE(frame->firstPosition(), 1);
    QCOMPARE(frame->lastPosition(), 1);
    QVERIFY(frame->parentFrame() != 0);
    QCOMPARE(root->childFrames().size(), 1);

    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);

//     cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
//     QCOMPARE(cursor.position(), 2);
//     QCOMPARE(cursor.selectionStart(), 0);
//     QCOMPARE(cursor.selectionEnd(), 2);
}

void tst_QTextCursor::checkFrame2()
{
    QCOMPARE(cursor.position(), 0);
    cursor.insertText("A");
    QCOMPARE(cursor.position(), 1);
    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);

    QPointer<QTextFrame> frame = cursor.insertFrame(QTextFrameFormat());
    QTextFrame *root = frame->parentFrame();

    QCOMPARE(frame->firstPosition(), 1);
    QCOMPARE(frame->lastPosition(), 2);
    QVERIFY(frame->parentFrame() != 0);
    QCOMPARE(root->childFrames().size(), 1);

    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 2);

    doc->undo();

    QVERIFY(!frame);
    QCOMPARE(root->childFrames().size(), 0);

    QCOMPARE(cursor.position(), 0);
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 1);

    doc->redo();

    frame = doc->frameAt(1);

    QVERIFY(frame);
    QCOMPARE(frame->firstPosition(), 1);
    QCOMPARE(frame->lastPosition(), 2);
    QVERIFY(frame->parentFrame() != 0);
    QCOMPARE(root->childFrames().size(), 1);

    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 2);

    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.position(), 0);
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 3);
}

void tst_QTextCursor::insertBlockToUseCharFormat()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertText("Hello", fmt);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));

    cursor.insertBlock();
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));

    fmt.setForeground(Qt::red);
    cursor.insertText("Hello\nWorld", fmt);
    cursor.insertText("Blah");
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::red));

    // ### we might want a testcase for createTable, too, as it calls insertBlock, too,
    // and we might want to have the char format copied (the one that gets inserted
    // as table separators, that are undeletable)
}

void tst_QTextCursor::tableMovement()
{
    QCOMPARE(cursor.position(), 0);
    cursor.insertText("AA");
    QCOMPARE(cursor.position(), 2);
    cursor.movePosition(QTextCursor::Left);

    cursor.insertTable(3, 3);
    QCOMPARE(cursor.position(), 2);

    cursor.movePosition(QTextCursor::Down);
    QCOMPARE(cursor.position(), 5);

    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.position(), 6);

    cursor.movePosition(QTextCursor::Up);
    QCOMPARE(cursor.position(), 3);

    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.position(), 4);

    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.position(), 5);

    cursor.movePosition(QTextCursor::Up);
    QCOMPARE(cursor.position(), 2);

    cursor.movePosition(QTextCursor::Up);
    QCOMPARE(cursor.position(), 0);

}

void tst_QTextCursor::selectionsInTable()
{
    QTextTable *table = cursor.insertTable(3, 3);
    table->cellAt(0, 0).firstCursorPosition().insertText("A a"); // first = 1
    table->cellAt(0, 1).firstCursorPosition().insertText("B b"); // first = 5
    table->cellAt(0, 2).firstCursorPosition().insertText("C c"); // first = 9
    table->cellAt(1, 0).firstCursorPosition().insertText("D d"); // first = 13
    table->cellAt(1, 1).firstCursorPosition().insertText("E e"); // first = 17
    table->cellAt(1, 2).firstCursorPosition().insertText("F f"); // first = 21
    table->cellAt(2, 0).firstCursorPosition().insertText("G g"); // first = 25
    table->cellAt(2, 1).firstCursorPosition().insertText("H h"); // first = 29
    table->cellAt(2, 2).firstCursorPosition().insertText("I i"); // first = 33

    cursor = table->cellAt(0, 0).lastCursorPosition();
    QCOMPARE(cursor.position(), 4);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor) == false);
    QCOMPARE(cursor.position(), 1);

    cursor = table->cellAt(1, 0).lastCursorPosition();
    QCOMPARE(cursor.position(), 16);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor) == false);
    QCOMPARE(cursor.position(), 13);

    cursor = table->cellAt(0, 2).firstCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor) == false);

    cursor = table->cellAt(1, 2).firstCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor) == false);

    // Next let's test selecting entire cells one at a time
    cursor = table->cellAt(0, 0).firstCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 5);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 9);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 13);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 17);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 21);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 25);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 29);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 33);
    QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor) == false);

    // And now lets walk all the way back
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 29);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 25);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 21);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 17);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 13);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 9);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 5);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor));
    QCOMPARE(cursor.position(), 1);
    QVERIFY(cursor.movePosition(QTextCursor::PreviousCell, QTextCursor::KeepAnchor) == false);

    QTextCursor::MoveOperation leftMovements[5] = {
          QTextCursor::PreviousBlock
        , QTextCursor::PreviousCharacter
        , QTextCursor::PreviousWord
        , QTextCursor::Left
        , QTextCursor::WordLeft
    };

    QTextCursor::MoveOperation rightMovements[5] = {
        QTextCursor::NextBlock
        , QTextCursor::NextCharacter
        , QTextCursor::NextWord
        , QTextCursor::Right
        , QTextCursor::WordRight
    };

    for (int i = 0; i < 5; ++i) {
        QTextCursor::MoveOperation left = leftMovements[i];
        QTextCursor::MoveOperation right = rightMovements[i];

        // Lets walk circle around anchor placed at 1,1 using up, down, left and right
        cursor = table->cellAt(1, 1).firstCursorPosition();
        QCOMPARE(cursor.position(), 17);
        QVERIFY(cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 18); // First right should not jump more than one char
        QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 21); // Lets jump to the next cell
        QVERIFY(cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 33);
        QVERIFY(cursor.movePosition(left, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 29);
        QVERIFY(cursor.movePosition(left, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 25);
        QVERIFY(cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 13);
        QVERIFY(cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 1);
        QVERIFY(cursor.movePosition(right, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 5);
        QVERIFY(cursor.movePosition(right, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 9);
        QVERIFY(cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 21);

        // Lets walk to the side 2 cells and back, first right
        cursor = table->cellAt(0, 0).firstCursorPosition();
        QVERIFY(cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 5); // Lets jump to the next cell
        QVERIFY(cursor.movePosition(right, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 9);
        QVERIFY(cursor.movePosition(left, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 5);
        QVERIFY(cursor.movePosition(left, QTextCursor::KeepAnchor));
        QVERIFY(cursor.position() < 5);

        // Then left
        cursor = table->cellAt(0, 2).firstCursorPosition();
        QCOMPARE(cursor.position(), 9);
        QVERIFY(cursor.movePosition(left, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 5); // A single left should do
        QVERIFY(cursor.movePosition(left, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 1);
        QVERIFY(cursor.movePosition(right, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 5);
        QVERIFY(cursor.movePosition(right, QTextCursor::KeepAnchor));
        QCOMPARE(cursor.position(), 9);
    }
}

void tst_QTextCursor::selectedText()
{
    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    QCOMPARE(cursor.selectedText(), QString("Hello World"));
}

void tst_QTextCursor::insertBlockShouldRemoveSelection()
{
    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectedText(), QString("Hello"));

    cursor.insertBlock();

    QVERIFY(!cursor.hasSelection());
    QCOMPARE(doc->toPlainText().indexOf("Hello"), -1);
}

void tst_QTextCursor::insertBlockShouldRemoveSelection2()
{
    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectedText(), QString("Hello"));

    QTextBlockFormat fmt = cursor.blockFormat();
    cursor.insertBlock(fmt);

    QVERIFY(!cursor.hasSelection());
    QCOMPARE(doc->toPlainText().indexOf("Hello"), -1);
}

void tst_QTextCursor::mergeCellShouldUpdateSelection()
{
    QTextTable *table = cursor.insertTable(4, 4);
    cursor.setPosition(table->cellAt(0, 0).firstPosition());
    cursor.setPosition(table->cellAt(3, 0).firstPosition(), QTextCursor::KeepAnchor); // aka bottom left
    int firstRow, numRows, firstColumn, numColumns;
    cursor.selectedTableCells(&firstRow, &numRows, &firstColumn, &numColumns);
    QCOMPARE(firstRow, 0);
    QCOMPARE(numRows, 4);
    QCOMPARE(firstColumn, 0);
    QCOMPARE(numColumns, 1);

    table->removeColumns(firstColumn, numColumns);

    QCOMPARE(cursor.anchor(), table->cellAt(0, 0).firstPosition());
    QCOMPARE(cursor.position(), table->cellAt(0, 0).firstPosition());
    QCOMPARE(cursor.position(), cursor.anchor()); // empty. I don't really care where it ends up.

    // prepare for another test with multiple cursors.
    // note we have a 4 rows, 3 cols table now.
    cursor.setPosition(table->cellAt(0, 0).firstPosition());
    cursor.setPosition(table->cellAt(0, 2).firstPosition(), QTextCursor::KeepAnchor);

    // now create a selection of a whole row.
    QTextCursor c2 = table->cellAt(2, 0).firstCursorPosition();
    c2.setPosition(table->cellAt(2, 2).firstPosition(), QTextCursor::KeepAnchor);

    // just for good measure, another one for a block of cells.
    QTextCursor c3 = table->cellAt(2, 1).firstCursorPosition();
    c3.setPosition(table->cellAt(3, 2).firstPosition(), QTextCursor::KeepAnchor);

    table->removeRows(2, 1);

    QCOMPARE(cursor.anchor(), table->cellAt(0, 0).firstPosition());
    QCOMPARE(cursor.position(), table->cellAt(0, 2).firstPosition());

    QCOMPARE(c2.position(), c2.anchor()); // empty. I don't really care where it ends up.

    QCOMPARE(c3.anchor(), table->cellAt(2, 1).firstPosition());
    QCOMPARE(c3.position(), table->cellAt(2, 2).firstPosition());


    // prepare for another test where we remove a column
    // note we have a 3 rows, 3 cols table now.
    cursor.setPosition(table->cellAt(0, 0).firstPosition());
    cursor.setPosition(table->cellAt(2, 1).firstPosition(), QTextCursor::KeepAnchor);

    c2.setPosition(table->cellAt(0, 1).firstPosition());
    c2.setPosition(table->cellAt(2, 2).firstPosition(), QTextCursor::KeepAnchor);

    table->removeColumns(1, 1);

    QCOMPARE(cursor.anchor(), table->cellAt(0, 0).firstPosition());
    QCOMPARE(cursor.position(), table->cellAt(2, 0).firstPosition());

    QCOMPARE(c2.anchor(), table->cellAt(0, 1).firstPosition());
    QCOMPARE(c2.position(), table->cellAt(2, 1).firstPosition());

    // test for illegal cursor positions.
    // note we have a 3 rows, 2 cols table now.
    cursor.setPosition(table->cellAt(2, 0).firstPosition());
    cursor.setPosition(table->cellAt(2, 1).firstPosition(), QTextCursor::KeepAnchor);

    c2.setPosition(table->cellAt(0, 0).firstPosition());
    c2.setPosition(table->cellAt(2, 1).firstPosition(), QTextCursor::KeepAnchor);

    c3.setPosition(table->cellAt(2, 1).firstPosition());

    table->removeRows(2, 1);

    QCOMPARE(cursor.anchor(), table->cellAt(1, 1).lastPosition()+1);
    QCOMPARE(cursor.position(), cursor.anchor());

    QCOMPARE(c2.anchor(), table->cellAt(0, 0).firstPosition());
    QCOMPARE(c2.position(), table->cellAt(1, 1).firstPosition());

    QCOMPARE(c3.anchor(), table->cellAt(1, 1).firstPosition());
    QCOMPARE(c3.position(), table->cellAt(1, 1).firstPosition());
}

void tst_QTextCursor::joinPreviousEditBlock()
{
    cursor.beginEditBlock();
    cursor.insertText("Hello");
    cursor.insertText("World");
    cursor.endEditBlock();
    QVERIFY(doc->toPlainText().startsWith("HelloWorld"));

    cursor.joinPreviousEditBlock();
    cursor.insertText("Hey");
    cursor.endEditBlock();
    QVERIFY(doc->toPlainText().startsWith("HelloWorldHey"));

    doc->undo();
    QVERIFY(!doc->toPlainText().contains("HelloWorldHey"));
}

void tst_QTextCursor::setBlockFormatInTable()
{
    // someone reported this on qt4-preview-feedback
    QTextBlockFormat fmt;
    fmt.setBackground(Qt::blue);
    cursor.setBlockFormat(fmt);

    QTextTable *table = cursor.insertTable(2, 2);
    cursor = table->cellAt(0, 0).firstCursorPosition();
    fmt.setBackground(Qt::red);
    cursor.setBlockFormat(fmt);

    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.blockFormat().background().color(), QColor(Qt::blue));
}

void tst_QTextCursor::blockCharFormat2()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::green);
    cursor.mergeBlockCharFormat(fmt);

    fmt.setForeground(Qt::red);

    cursor.insertText("Test", fmt);
    cursor.movePosition(QTextCursor::Start);
    cursor.insertText("Red");
    cursor.movePosition(QTextCursor::PreviousCharacter);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::red));
}

void tst_QTextCursor::blockCharFormat3()
{
    QVERIFY(cursor.atBlockStart());
    QVERIFY(cursor.atBlockEnd());
    QVERIFY(cursor.atStart());

    QTextCharFormat fmt;
    fmt.setForeground(Qt::green);
    cursor.setBlockCharFormat(fmt);
    cursor.insertText("Test");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    const QColor red(Qt::red);
    const QColor green(Qt::green);
    QCOMPARE(cursor.charFormat().foreground().color(), green);

    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.charFormat().foreground().color(), green);

    fmt.setForeground(Qt::red);
    cursor.setBlockCharFormat(fmt);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), red);

    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.charFormat().foreground().color(), green);

    cursor.insertText("Test");
    QCOMPARE(cursor.charFormat().foreground().color(), green);

    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    QVERIFY(cursor.atBlockStart());
    QVERIFY(cursor.atBlockEnd());
    QVERIFY(cursor.atStart());

    cursor.insertText("Test");
    QCOMPARE(cursor.charFormat().foreground().color(), red);
}

void tst_QTextCursor::blockCharFormat()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    cursor.insertText("Hm");

    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::blue));

    fmt.setForeground(Qt::red);

    cursor.setBlockCharFormat(fmt);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::red));
}

void tst_QTextCursor::blockCharFormatOnSelection()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertBlock(QTextBlockFormat(), fmt);

    fmt.setForeground(Qt::green);
    cursor.insertText("Hm", fmt);

    fmt.setForeground(Qt::red);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    cursor.insertText("Ah");

    fmt.setForeground(Qt::white);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    cursor.insertText("bleh");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::blue));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::red));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::white));

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);

    fmt.setForeground(Qt::cyan);
    cursor.setBlockCharFormat(fmt);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::cyan));

    cursor.movePosition(QTextCursor::Right);
    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::green));

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::cyan));

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockCharFormat().foreground().color(), QColor(Qt::white));
}

void tst_QTextCursor::anchorInitialized1()
{
    cursor.insertBlock();
    cursor = QTextCursor(cursor.block());
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.anchor(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);
}

void tst_QTextCursor::anchorInitialized2()
{
    cursor.insertBlock();
    cursor = QTextCursorPrivate::fromPosition(cursor.block().docHandle(), 1);
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.anchor(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);
}

void tst_QTextCursor::anchorInitialized3()
{
    QTextFrame *frame = cursor.insertFrame(QTextFrameFormat());
    cursor = QTextCursor(frame);
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.anchor(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);
}

void tst_QTextCursor::selectWord()
{
    cursor.insertText("first second     third");
    cursor.insertBlock();
    cursor.insertText("words in second paragraph");

    cursor.setPosition(9);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 6);
    QCOMPARE(cursor.selectionEnd(), 12);

    cursor.setPosition(5);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 5);

    cursor.setPosition(6);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 6);
    QCOMPARE(cursor.selectionEnd(), 12);

    cursor.setPosition(14);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 6);
    QCOMPARE(cursor.selectionEnd(), 12);

    cursor.movePosition(QTextCursor::Start);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 5);

    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 17);
    QCOMPARE(cursor.selectionEnd(), 22);
}

void tst_QTextCursor::selectWordWithSeparators_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("initialPosition");
    QTest::addColumn<QString>("expectedSelectedText");

    QTest::newRow("dereference") << QString::fromLatin1("foo->bar()") << 1 << QString::fromLatin1("foo");
    QTest::newRow("funcsignature") << QString::fromLatin1("bar(int x);") << 1 << QString::fromLatin1("bar");
    QTest::newRow("def") << QString::fromLatin1("foo *f;") << 1 << QString::fromLatin1("foo");
}

void tst_QTextCursor::selectWordWithSeparators()
{
    QFETCH(QString, text);
    QFETCH(int, initialPosition);
    QFETCH(QString, expectedSelectedText);

    cursor.insertText(text);
    cursor.setPosition(initialPosition);
    cursor.select(QTextCursor::WordUnderCursor);

    QCOMPARE(cursor.selectedText(), expectedSelectedText);
}

void tst_QTextCursor::startOfWord()
{
    cursor.insertText("first     second");
    cursor.setPosition(7);
    cursor.movePosition(QTextCursor::StartOfWord);
    QCOMPARE(cursor.position(), 0);
}

void tst_QTextCursor::selectBlock()
{
    cursor.insertText("foobar");
    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(Qt::AlignHCenter);
    cursor.insertBlock(blockFmt);
    cursor.insertText("blah");
    cursor.insertBlock(QTextBlockFormat());

    cursor.movePosition(QTextCursor::PreviousBlock);
    QCOMPARE(cursor.block().text(), QString("blah"));

    cursor.select(QTextCursor::BlockUnderCursor);
    QVERIFY(cursor.hasSelection());

    QTextDocumentFragment fragment(cursor);
    doc->clear();
    cursor.insertFragment(fragment);
    QCOMPARE(blockCount(), 2);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.blockFormat().alignment(), Qt::AlignHCenter);
    QCOMPARE(cursor.block().text(), QString("blah"));
}

void tst_QTextCursor::selectVisually()
{
    cursor.insertText("Foo\nlong line which is probably going to be cut in two when shown in a widget\nparagraph 3\n");

    cursor.setPosition(6); // somewhere in the long paragraph.
    cursor.select(QTextCursor::LineUnderCursor);
    // since we are not yet laid-out, we expect the whole paragraph to be selected.
    QCOMPARE(cursor.position(), 77);
    QCOMPARE(cursor.anchor(), 4);
}

void tst_QTextCursor::insertText()
{
    QString txt = "Foo\nBar\r\nMeep";
    txt += QChar::LineSeparator;
    txt += "Baz";
    txt += QChar::ParagraphSeparator;
    txt += "yoyodyne";
    cursor.insertText(txt);
    QCOMPARE(blockCount(), 4);
    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.block().text(), QString("Foo"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("Bar"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString(QString("Meep") + QChar(QChar::LineSeparator) + QString("Baz")));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("yoyodyne"));
}

void tst_QTextCursor::insertFragmentShouldUseCurrentCharFormat()
{
    QTextDocumentFragment fragment = QTextDocumentFragment::fromPlainText("Hello World");
    QTextCharFormat fmt;
    fmt.setFontUnderline(true);

    cursor.clearSelection();
    cursor.setCharFormat(fmt);
    cursor.insertFragment(fragment);
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.charFormat(), fmt);
}

int tst_QTextCursor::blockCount()
{
    int cnt = 0;
    for (QTextBlock blk = doc->begin(); blk.isValid(); blk = blk.next())
        ++cnt;
    return cnt;
}

void tst_QTextCursor::endOfLine()
{
    doc->setPageSize(QSizeF(100000, INT_MAX));

    QString text("First Line    \nSecond Line  ");
    text.replace(QLatin1Char('\n'), QChar(QChar::LineSeparator));
    cursor.insertText(text);

    // ensure layouted
    doc->documentLayout()->documentSize();

    cursor.movePosition(QTextCursor::Start);

    QCOMPARE(cursor.block().layout()->lineCount(), 2);

    cursor.movePosition(QTextCursor::EndOfLine);
    QCOMPARE(cursor.position(), 14);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.position(), 15);
    cursor.movePosition(QTextCursor::EndOfLine);
    QCOMPARE(cursor.position(), 28);
}

class CursorListener : public QObject
{
    Q_OBJECT
public:
    CursorListener(QTextCursor *_cursor) : lastRecordedPosition(-1), lastRecordedAnchor(-1), recordingCount(0), cursor(_cursor) {}

    int lastRecordedPosition;
    int lastRecordedAnchor;
    int recordingCount;

public slots:
    void recordCursorPosition()
    {
        lastRecordedPosition = cursor->position();
        lastRecordedAnchor = cursor->anchor();
        ++recordingCount;
    }

    void selectAllContents()
    {
        // Only test the first time
        if (!recordingCount) {
            recordingCount++;
            cursor->select(QTextCursor::Document);
            lastRecordedPosition = cursor->position();
            lastRecordedAnchor = cursor->anchor();
        }
    }

private:
    QTextCursor *cursor;
};

void tst_QTextCursor::editBlocksDuringRemove()
{
    CursorListener listener(&cursor);

    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.selectedText(), QString("Hello World"));

    connect(doc, SIGNAL(contentsChanged()), &listener, SLOT(recordCursorPosition()));
    listener.recordingCount = 0;
    cursor.deleteChar();

    QCOMPARE(listener.recordingCount, 1);
    QCOMPARE(listener.lastRecordedPosition, 0);
    QCOMPARE(listener.lastRecordedAnchor, 0);

    QVERIFY(doc->toPlainText().isEmpty());
}

void tst_QTextCursor::selectAllDuringRemove()
{
    CursorListener listener(&cursor);

    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::End);

    connect(doc, SIGNAL(contentsChanged()), &listener, SLOT(selectAllContents()));
    listener.recordingCount = 0;
    QTextCursor localCursor = cursor;
    localCursor.deletePreviousChar();

    QCOMPARE(listener.lastRecordedPosition, 10);
    QCOMPARE(listener.lastRecordedAnchor, 0);
}

void tst_QTextCursor::update_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<int>("modifyPosition");
    QTest::addColumn<int>("modifyAnchor");
    QTest::addColumn<QString>("insertText");
    QTest::addColumn<int>("expectedPosition");
    QTest::addColumn<int>("expectedAnchor");

    QString text("Hello big world");
    int charsToDelete = 3;
    QTest::newRow("removeInsideSelection")
        << text
        << /*position*/ 0
        << /*anchor*/ text.length()
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ 0
        << /*expectedAnchor*/ text.length() - charsToDelete
        ;

    text = "Hello big world";
    charsToDelete = 3;
    QTest::newRow("removeInsideSelectionWithSwappedAnchorAndPosition")
        << text
        << /*position*/ text.length()
        << /*anchor*/ 0
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ text.length() - charsToDelete
        << /*expectedAnchor*/ 0
        ;


    text = "Hello big world";
    charsToDelete = 3;
    QString textToInsert("small");
    QTest::newRow("replaceInsideSelection")
        << text
        << /*position*/ 0
        << /*anchor*/ text.length()
        // delete 'big' ...
        << 6
        << 6 + charsToDelete
        << textToInsert // ... and replace 'big' with 'small'
        << /*expectedPosition*/ 0
        << /*expectedAnchor*/ text.length() - charsToDelete + textToInsert.length()
        ;

    text = "Hello big world";
    charsToDelete = 3;
    textToInsert = "small";
    QTest::newRow("replaceInsideSelectionWithSwappedAnchorAndPosition")
        << text
        << /*position*/ text.length()
        << /*anchor*/ 0
        // delete 'big' ...
        << 6
        << 6 + charsToDelete
        << textToInsert // ... and replace 'big' with 'small'
        << /*expectedPosition*/ text.length() - charsToDelete + textToInsert.length()
        << /*expectedAnchor*/ 0
        ;


    text = "Hello big world";
    charsToDelete = 3;
    QTest::newRow("removeBeforeSelection")
        << text
        << /*position*/ text.length() - 5
        << /*anchor*/ text.length()
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ text.length() - 5 - charsToDelete
        << /*expectedAnchor*/ text.length() - charsToDelete
        ;

    text = "Hello big world";
    charsToDelete = 3;
    QTest::newRow("removeAfterSelection")
        << text
        << /*position*/ 0
        << /*anchor*/ 4
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ 0
        << /*expectedAnchor*/ 4
        ;

}

void tst_QTextCursor::update()
{
    QFETCH(QString, text);

    doc->setPlainText(text);

    QFETCH(int, position);
    QFETCH(int, anchor);

    cursor.setPosition(anchor);
    cursor.setPosition(position, QTextCursor::KeepAnchor);

    QCOMPARE(cursor.position(), position);
    QCOMPARE(cursor.anchor(), anchor);

    QFETCH(int, modifyPosition);
    QFETCH(int, modifyAnchor);

    QTextCursor modifyCursor = cursor;
    modifyCursor.setPosition(modifyAnchor);
    modifyCursor.setPosition(modifyPosition, QTextCursor::KeepAnchor);

    QCOMPARE(modifyCursor.position(), modifyPosition);
    QCOMPARE(modifyCursor.anchor(), modifyAnchor);

    QFETCH(QString, insertText);
    modifyCursor.insertText(insertText);

    QFETCH(int, expectedPosition);
    QFETCH(int, expectedAnchor);

    QCOMPARE(cursor.position(), expectedPosition);
    QCOMPARE(cursor.anchor(), expectedAnchor);
}

void tst_QTextCursor::disallowSettingObjectIndicesOnCharFormats()
{
    QTextCharFormat fmt;
    fmt.setObjectIndex(42);
    cursor.insertText("Hey", fmt);
    QCOMPARE(cursor.charFormat().objectIndex(), -1);

    cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(fmt);
    QCOMPARE(doc->begin().begin().fragment().charFormat().objectIndex(), -1);

    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(fmt);
    QCOMPARE(doc->begin().begin().fragment().charFormat().objectIndex(), -1);

    cursor.setBlockCharFormat(fmt);
    QCOMPARE(cursor.blockCharFormat().objectIndex(), -1);

    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    QCOMPARE(cursor.blockCharFormat().objectIndex(), -1);

    doc->clear();

    QTextTable *table = cursor.insertTable(1, 1);
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(fmt);

    cursor = table->cellAt(0, 0).firstCursorPosition();
    QVERIFY(!cursor.isNull());
    QCOMPARE(cursor.blockCharFormat().objectIndex(), table->objectIndex());
}

void tst_QTextCursor::blockAndColumnNumber()
{
    QCOMPARE(QTextCursor().columnNumber(), 0);
    QCOMPARE(QTextCursor().blockNumber(), 0);

    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 0);
    cursor.insertText("Hello");
    QCOMPARE(cursor.columnNumber(), 5);
    QCOMPARE(cursor.blockNumber(), 0);

    cursor.insertBlock();
    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 1);
    cursor.insertText("Blah");
    QCOMPARE(cursor.blockNumber(), 1);

    // trigger a layout
    doc->documentLayout();

    cursor.insertBlock();
    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 2);
    cursor.insertText("Test");
    QCOMPARE(cursor.columnNumber(), 4);
    QCOMPARE(cursor.blockNumber(), 2);
    cursor.insertText(QString(QChar(QChar::LineSeparator)));
    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 2);
    cursor.insertText("A");
    QCOMPARE(cursor.columnNumber(), 1);
    QCOMPARE(cursor.blockNumber(), 2);
}

void tst_QTextCursor::movePositionEndOfLine()
{
    cursor.insertText("blah\nblah\n");
    // Select part of the second line ("la")
    cursor.setPosition(6);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);
    QCOMPARE(cursor.selectedText(), QLatin1String("la"));

    // trigger a layout
    doc->documentLayout();

    // Remove "la" and append "something" to the end in one undo operation
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    QTextCursor c2(doc);
    c2.setPosition(7);
    c2.insertText("foo"); // append to doc without touching the cursor.

    QCOMPARE(cursor.position(), 6);
    cursor.movePosition(QTextCursor::EndOfLine); // in an edit block visual movement is moved to the end of the paragraph
    QCOMPARE(cursor.position(), 10);
    cursor.endEditBlock();
}

void tst_QTextCursor::clearCells()
{
    QTextTable *table = cursor.insertTable(3, 5);
    cursor.setPosition(table->cellAt(0,0).firstPosition()); // select cell 1 and cell 2
    cursor.setPosition(table->cellAt(0,1).firstPosition(), QTextCursor::KeepAnchor);
    cursor.deleteChar(); // should clear the cells, and not crash ;)
}

void tst_QTextCursor::task244408_wordUnderCursor_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::newRow("trailingSpace") << QString::fromLatin1("foo ") << QString::fromLatin1("");
    QTest::newRow("noTrailingSpace") << QString::fromLatin1("foo") << QString::fromLatin1("foo");
}

void tst_QTextCursor::task244408_wordUnderCursor()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    cursor.insertText(input);
    cursor.movePosition(QTextCursor::End);
    cursor.select(QTextCursor::WordUnderCursor);
    QCOMPARE(cursor.selectedText(), expected);
}

void tst_QTextCursor::adjustCursorsOnInsert()
{
    cursor.insertText("Some text before ");
    int posBefore = cursor.position();
    cursor.insertText("selected text");
    int posAfter = cursor.position();
    cursor.insertText(" some text afterwards");

    QTextCursor selection = cursor;
    selection.setPosition(posBefore);
    selection.setPosition(posAfter, QTextCursor::KeepAnchor);

    cursor.setPosition(posBefore-1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.anchor(), posBefore+1);
    QCOMPARE(selection.position(), posAfter+1);
    doc->undo();

    cursor.setPosition(posBefore);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.anchor(), posBefore+1);
    QCOMPARE(selection.position(), posAfter+1);
    doc->undo();

    cursor.setPosition(posBefore+1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.anchor(), posBefore);
    QCOMPARE(selection.position(), posAfter+1);
    doc->undo();

    cursor.setPosition(posAfter-1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.anchor(), posBefore);
    QCOMPARE(selection.position(), posAfter+1);
    doc->undo();

    selection.setKeepPositionOnInsert(true);
    cursor.setPosition(posAfter);
    cursor.insertText(QLatin1String("x"));
    selection.setKeepPositionOnInsert(false);
    QCOMPARE(selection.anchor(), posBefore);
    QCOMPARE(selection.position(), posAfter);
    doc->undo();

    cursor.setPosition(posAfter+1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.anchor(), posBefore);
    QCOMPARE(selection.position(), posAfter);
    doc->undo();

    selection.setPosition(posAfter);
    selection.setPosition(posBefore, QTextCursor::KeepAnchor);

    cursor.setPosition(posBefore-1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.position(), posBefore+1);
    QCOMPARE(selection.anchor(), posAfter+1);
    doc->undo();

    cursor.setPosition(posBefore);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.position(), posBefore+1);
    QCOMPARE(selection.anchor(), posAfter+1);
    doc->undo();

    cursor.setPosition(posBefore+1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.position(), posBefore);
    QCOMPARE(selection.anchor(), posAfter+1);
    doc->undo();

    cursor.setPosition(posAfter-1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.position(), posBefore);
    QCOMPARE(selection.anchor(), posAfter+1);
    doc->undo();

    cursor.setPosition(posAfter);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.position(), posBefore);
    QCOMPARE(selection.anchor(), posAfter+1);
    doc->undo();

    cursor.setPosition(posAfter+1);
    cursor.insertText(QLatin1String("x"));
    QCOMPARE(selection.position(), posBefore);
    QCOMPARE(selection.anchor(), posAfter);
    doc->undo();

}
void tst_QTextCursor::cursorPositionWithBlockUndoAndRedo()
{
    cursor.insertText("AAAABBBBCCCCDDDD");
    cursor.setPosition(12);
    int cursorPositionBefore = cursor.position();
    cursor.beginEditBlock();
    cursor.insertText("*");
    cursor.setPosition(8);
    cursor.insertText("*");
    cursor.setPosition(4);
    cursor.insertText("*");
    cursor.setPosition(0);
    cursor.insertText("*");
    int cursorPositionAfter = cursor.position();
    cursor.endEditBlock();

    QCOMPARE(doc->toPlainText(), QLatin1String("*AAAA*BBBB*CCCC*DDDD"));
    QCOMPARE(12, cursorPositionBefore);
    QCOMPARE(1, cursorPositionAfter);

    doc->undo(&cursor);
    QCOMPARE(doc->toPlainText(), QLatin1String("AAAABBBBCCCCDDDD"));
    QCOMPARE(cursor.position(), cursorPositionBefore);
    doc->redo(&cursor);
    QCOMPARE(doc->toPlainText(), QLatin1String("*AAAA*BBBB*CCCC*DDDD"));
    QCOMPARE(cursor.position(), cursorPositionAfter);
}

void tst_QTextCursor::cursorPositionWithBlockUndoAndRedo2()
{
    cursor.insertText("AAAABBBB");
    int cursorPositionBefore = cursor.position();
    cursor.setPosition(0, QTextCursor::KeepAnchor);
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.insertText("AAAABBBBCCCCDDDD");
    cursor.endEditBlock();
    doc->undo(&cursor);
    QCOMPARE(doc->toPlainText(), QLatin1String("AAAABBBB"));
    QCOMPARE(cursor.position(), cursorPositionBefore);

    cursor.insertText("CCCC");
    QCOMPARE(doc->toPlainText(), QLatin1String("AAAABBBBCCCC"));

    cursorPositionBefore = cursor.position();
    cursor.setPosition(0, QTextCursor::KeepAnchor);
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.insertText("AAAABBBBCCCCDDDD");
    cursor.endEditBlock();

    /* this undo now implicitely reinserts two segments, first "CCCCC", then
       "AAAABBBB". The test ensures that the two are combined in order to
       reconstruct the correct cursor position */
    doc->undo(&cursor);


    QCOMPARE(doc->toPlainText(), QLatin1String("AAAABBBBCCCC"));
    QCOMPARE(cursor.position(), cursorPositionBefore);
}

void tst_QTextCursor::cursorPositionWithBlockUndoAndRedo3()
{
    // verify that it's the position of the beginEditBlock that counts, and not the last edit position
    cursor.insertText("AAAABBBB");
    int cursorPositionBefore = cursor.position();
    cursor.beginEditBlock();
    cursor.setPosition(4);
    QVERIFY(cursor.position() != cursorPositionBefore);
    cursor.insertText("*");
    cursor.endEditBlock();
    QCOMPARE(cursor.position(), 5);
    doc->undo(&cursor);
    QCOMPARE(cursor.position(), cursorPositionBefore);
}

void tst_QTextCursor::joinNonEmptyRemovedBlockUserState()
{
    cursor.insertText("Hello");
    cursor.insertBlock();
    cursor.insertText("World");
    cursor.block().setUserState(10);

    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    QCOMPARE(cursor.block().userState(), 10);
}

void tst_QTextCursor::crashOnDetachingDanglingCursor()
{
    QTextDocument *document = new QTextDocument;
    QTextCursor cursor(document);
    QTextCursor cursor2 = cursor;
    delete document;
    cursor2.setPosition(0); // Don't crash here
}

QTEST_MAIN(tst_QTextCursor)
#include "tst_qtextcursor.moc"
