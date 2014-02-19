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

#define protected public

#include <qtextdocument.h>
#undef protected
#include <private/qtextdocument_p.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextobject.h>
#include <qdebug.h>
#include <stdlib.h>
#include <qtextcursor.h>
#include "../qtextdocument/common.h"

class tst_QTextPieceTable : public QObject
{
    Q_OBJECT

public:
    tst_QTextPieceTable();


public slots:
    void init();
    void cleanup();
private slots:
    void insertion1();
    void insertion2();
    void insertion3();
    void insertion4();
    void insertion5();

    void removal1();
    void removal2();
    void removal3();
    void removal4();

    void undoRedo1();
    void undoRedo2();
    void undoRedo3();
    void undoRedo4();
    void undoRedo5();
    void undoRedo6();
    void undoRedo7();
    void undoRedo8();
    void undoRedo9();
    void undoRedo10();
    void undoRedo11();

    void checkDocumentChanged();
    void checkDocumentChanged2();
    void setBlockFormat();

    void blockInsertion();
    void blockInsertion2();

    void blockRemoval1();
    void blockRemoval2();
    void blockRemoval3();
    void blockRemoval4();
    void blockRemoval5();

    void checkBlockSeparation();

    void checkFrames1();
    void removeFrameDirect();
    void removeWithChildFrame();
    void clearWithFrames();

private:
    QTextDocument *doc;
    QTextDocumentPrivate *table;
    int blockFormatIndex;
    int charFormatIndex;
};

tst_QTextPieceTable::tst_QTextPieceTable()
{ doc = 0; table = 0; }


void tst_QTextPieceTable::init()
{
    doc = new QTextDocument(0);
    table = doc->docHandle();
    blockFormatIndex = table->formatCollection()->indexForFormat(QTextBlockFormat());
    charFormatIndex = table->formatCollection()->indexForFormat(QTextCharFormat());
}

void tst_QTextPieceTable::cleanup()
{
    delete doc;
    doc = 0;
}

void tst_QTextPieceTable::insertion1()
{
    table->insert(0, "aacc", charFormatIndex);
    QCOMPARE(table->plainText(), QString("aacc"));
    table->insert(2, "bb", charFormatIndex);
    QCOMPARE(table->plainText(), QString("aabbcc"));
    table->insert(1, "1", charFormatIndex);
    QCOMPARE(table->plainText(), QString("a1abbcc"));
    table->insert(6, "d", charFormatIndex);
    QCOMPARE(table->plainText(), QString("a1abbcdc"));
    table->insert(8, "z", charFormatIndex);
    QCOMPARE(table->plainText(), QString("a1abbcdcz"));
}

void tst_QTextPieceTable::insertion2()
{
    table->insert(0, "bb", charFormatIndex);
    QCOMPARE(table->plainText(), QString("bb"));
}

void tst_QTextPieceTable::insertion3()
{
    QString compare;
    for (int i = 0; i < 20000; ++i) {
        int pos = rand() % (i+1);
        QChar c((unsigned short)(i & 0xff) + 1);
        QString str;
        str += c;
        table->insert(pos, str, charFormatIndex);
        compare.insert(pos, str);
    }
    QVERIFY(table->plainText() == compare);
}

void tst_QTextPieceTable::insertion4()
{
    QString compare;
    for (int i = 0; i < 20000; ++i) {
        int pos = rand() % (i+1);
        QChar c((unsigned short)((i % 26) + (i>25?'A':'a')));
        QString str;
        str += c;
        str += c;
        table->insert(pos, str, charFormatIndex);
        compare.insert(pos, str);
//        if (table->text() != compare) {
//            qDebug("compare failed: i=%d (current char=%c) insert at %d\nexpected '%s'\ngot      '%s'", i, (i % 26) + (i>25?'A':'a'), pos, compare.latin1(), table->text().latin1());
//            exit(12);
//        }
    }
    QVERIFY(table->plainText() == compare);
}

void tst_QTextPieceTable::insertion5()
{
    QString compare;
    for (int i = 0; i < 20000; ++i) {
        int pos = rand() % (i+1);
        QChar c((unsigned short)((i % 26) + (i>25?'A':'a')));
        QString str;
        str += c;
        str += c;
        if (c == 'a') {
            table->insertBlock(pos, blockFormatIndex, charFormatIndex);
            str = QChar(QChar::ParagraphSeparator);
        } else {
            table->insert(pos, str, charFormatIndex);
        }
        compare.insert(pos, str);
    }
    QVERIFY(table->plainText() == compare);
    for (QTextBlock it = table->blocksBegin(); it != table->blocksEnd(); it = it.next()) {
        QTextDocumentPrivate::FragmentIterator fit = table->find(it.position());
        QVERIFY(fit.position() == it.position());
    }
}

void tst_QTextPieceTable::removal1()
{
    table->insert(0, "abbccc", charFormatIndex);
    QCOMPARE(table->plainText(), QString("abbccc"));
    table->remove(1, 2);
    QCOMPARE(table->plainText(), QString("accc"));
    table->insert(1, "1", charFormatIndex);
    QCOMPARE(table->plainText(), QString("a1ccc"));
    table->remove(4, 1);
    QCOMPARE(table->plainText(), QString("a1cc"));
    table->insert(4, "z", charFormatIndex);
    QCOMPARE(table->plainText(), QString("a1ccz"));
}

void tst_QTextPieceTable::removal2()
{
    table->insert(0, "bb", charFormatIndex);
    QCOMPARE(table->plainText(), QString("bb"));
    table->remove(0, 2);
    QCOMPARE(table->plainText(), QString(""));
    table->insertBlock(0, blockFormatIndex, charFormatIndex);
    QCOMPARE(table->plainText(), QString(QChar(QChar::ParagraphSeparator)));
    table->remove(0, 1);
    QCOMPARE(table->plainText(), QString(""));

    table->insert(0, "bb", charFormatIndex);
    QCOMPARE(table->plainText(), QString("bb"));
    table->insertBlock(1, blockFormatIndex, charFormatIndex);
    QCOMPARE(table->plainText(), QString("b") + QString(QChar(QChar::ParagraphSeparator)) + QString("b"));
    table->remove(1, 1);
    QCOMPARE(table->plainText(), QString("bb"));
}

void tst_QTextPieceTable::removal3()
{
    QString compare;
    int l = 0;
    for (int i = 0; i < 20000; ++i) {
        bool remove = l && (rand() % 2);
        int pos = rand() % (remove ? l : (l+1));
        QChar c((unsigned short)((i % 26) + (i>25?'A':'a')));
        QString str;
        str += c;
        str += c;
        if (remove && pos < table->length()) {
            compare.remove(pos, 1);
            table->remove(pos, 1);
        } else {
            compare.insert(pos, str);
            table->insert(pos, str, charFormatIndex);
        }
        l += remove ? -1 : 2;
//        if (table->text() != compare) {
//            qDebug("compare failed: i=%d (current char=%c) insert at %d\nexpected '%s'\ngot      '%s'", i, (i % 26) + (i>25?'A':'a'), pos, compare.latin1(), table->text().latin1());
//            exit(12);
//        }
    }
    QVERIFY(table->plainText() == compare);
}

void tst_QTextPieceTable::removal4()
{
    QString compare;
    int l = 0;
    for (int i = 0; i < 20000; ++i) {
        bool remove = l && (rand() % 2);
        int pos = (l > 1) ? rand() % (remove ? l-1 : l) : 0;
        QChar c((unsigned short)((i % 26) + (i>25?'A':'a')));
        QString str;
        if (c != 'a') {
            str += c;
            str += c;
        } else {
            str = QChar(QChar::ParagraphSeparator);
        }
        if (remove && pos < table->length() - 1) {
            compare.remove(pos, 1);
            table->remove(pos, 1);
        } else {
            if (str[0] == QChar(QChar::ParagraphSeparator))
                table->insertBlock(pos, blockFormatIndex, charFormatIndex);
            else
                table->insert(pos, str, charFormatIndex);
            compare.insert(pos, str);
        }
        l += remove ? -1 : 2;
//        if (table->plainText() != compare) {
//            qDebug("compare failed: i=%d (current char=%c) insert at %d\nexpected '%s'\ngot      '%s'", i, (i % 26) + (i>25?'A':'a'), pos, compare.latin1(), table->plainText().latin1());
//            exit(12);
//        }
    }
    QVERIFY(table->plainText() == compare);
}

void tst_QTextPieceTable::undoRedo1()
{
    table->insert(0, "01234567", charFormatIndex);
    table->insert(0, "a", charFormatIndex);
    table->insert(1, "b", charFormatIndex);
    QCOMPARE(table->plainText(), QString("ab01234567"));
    table->undo();
    QCOMPARE(table->plainText(), QString("01234567"));
    table->redo();
    QCOMPARE(table->plainText(), QString("ab01234567"));
    table->undo();
    table->insert(1, "c", charFormatIndex);
    QCOMPARE(table->plainText(), QString("0c1234567"));
    table->undo();
    QCOMPARE(table->plainText(), QString("01234567"));
    table->undo();
    QVERIFY(table->plainText().isEmpty());
}

void tst_QTextPieceTable::undoRedo2()
{
    table->insert(0, "01", charFormatIndex);
    table->insert(1, "a", charFormatIndex);
    QCOMPARE(table->plainText(), QString("0a1"));
    table->undo();
    QCOMPARE(table->plainText(), QString("01"));
    table->undo();
    QCOMPARE(table->plainText(), QString(""));
    table->redo();
    QCOMPARE(table->plainText(), QString("01"));
    table->redo();
    QCOMPARE(table->plainText(), QString("0a1"));
}

void tst_QTextPieceTable::undoRedo3()
{
    table->insert(0, "01", charFormatIndex);
    table->insert(2, "ab", charFormatIndex);
    table->remove(2, 1);
    QCOMPARE(table->plainText(), QString("01b"));
    table->undo();
    QCOMPARE(table->plainText(), QString("01ab"));
    table->undo();
    QVERIFY(table->plainText().isEmpty());
    table->redo();
    QCOMPARE(table->plainText(), QString("01ab"));
    table->redo();
    QCOMPARE(table->plainText(), QString("01b"));
}

void tst_QTextPieceTable::undoRedo4()
{
    table->insert(0, "01", charFormatIndex);
    table->insert(0, "ab", charFormatIndex);
    table->remove(0, 1);
    QCOMPARE(table->plainText(), QString("b01"));
    table->undo();
    QCOMPARE(table->plainText(), QString("ab01"));
    table->undo();
    QCOMPARE(table->plainText(), QString("01"));
    table->undo();
    QCOMPARE(table->plainText(), QString(""));
    table->redo();
    QCOMPARE(table->plainText(), QString("01"));
    table->redo();
    QCOMPARE(table->plainText(), QString("ab01"));
    table->redo();
    QCOMPARE(table->plainText(), QString("b01"));
}

void tst_QTextPieceTable::undoRedo5()
{
    table->beginEditBlock();
    table->insert(0, "01", charFormatIndex);
    table->remove(1, 1);
    table->endEditBlock();
    QCOMPARE(table->plainText(), QString("0"));
    table->undo();
    QCOMPARE(table->plainText(), QString(""));
}

void tst_QTextPieceTable::undoRedo6()
{
    // this is essentially a test for the undoStack[undoPosition - 1].block = false in PieceTable::endUndoBlock()
    QTextDocument doc;
    QTextCursor cursor(&doc);
    cursor.insertText("Hello World");

    cursor.insertBlock();
    cursor.insertText("Hello World2");

    cursor.movePosition(QTextCursor::Start);
    QTextBlockFormat bfmt;
    bfmt.setAlignment(Qt::AlignHCenter);
    cursor.setBlockFormat(bfmt);
    QVERIFY(cursor.blockFormat().alignment() == Qt::AlignHCenter);

    QTextCursor range = cursor;
    range.clearSelection();
    range.movePosition(QTextCursor::Start);
    range.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    QTextCharFormat modifier;
    modifier.setFontItalic(true);
    range.mergeCharFormat(modifier);

    cursor.movePosition(QTextCursor::Start);
    QVERIFY(cursor.blockFormat().alignment() == Qt::AlignHCenter);

    doc.undo();

    QVERIFY(cursor.blockFormat().alignment() == Qt::AlignHCenter);
}

void tst_QTextPieceTable::undoRedo7()
{
    table->insert(0, "a", charFormatIndex);
    table->insert(1, "b", charFormatIndex);
    QCOMPARE(table->plainText(), QString("ab"));

    table->undo();
    QVERIFY(table->plainText().isEmpty());
}

void tst_QTextPieceTable::undoRedo8()
{
    table->insert(0, "a", charFormatIndex);
    table->insert(1, "b", charFormatIndex);
    QCOMPARE(table->plainText(), QString("ab"));

    table->remove(0, 1);
    table->remove(0, 1);

    QVERIFY(table->plainText().isEmpty());
    table->undo();
    QCOMPARE(table->plainText(), QString("ab"));
}

void tst_QTextPieceTable::undoRedo9()
{
    table->insert(0, "a", charFormatIndex);
    table->insert(1, "b", charFormatIndex);
    QCOMPARE(table->plainText(), QString("ab"));

    table->remove(1, 1);
    table->remove(0, 1);

    QVERIFY(table->plainText().isEmpty());
    table->undo();
    QCOMPARE(table->plainText(), QString("ab"));
}

void tst_QTextPieceTable::undoRedo10()
{
    // testcase for the beginUndoBlock/endUndoBlock calls being surrounded by an if (undoEnabled)
    QTextCharFormat cf;
    cf.setForeground(Qt::blue);
    int cfIdx = table->formatCollection()->indexForFormat(cf);

    QTextBlockFormat f;
    int idx = table->formatCollection()->indexForFormat(f);

    table->insert(0, "a", cfIdx);
    table->insertBlock(1, idx, cfIdx);
    table->insert(1, "b", cfIdx);

    cf.setForeground(Qt::red);
    int newCfIdx = table->formatCollection()->indexForFormat(cf);

    table->setCharFormat(0, 3, cf, QTextDocumentPrivate::MergeFormat);

    QCOMPARE(table->find(0).value()->format, newCfIdx);

    table->undo();

    QCOMPARE(table->find(0).value()->format, cfIdx);
}

void tst_QTextPieceTable::undoRedo11()
{
    srand(3);
    const int loops = 20;
    QString compare;
    int l = 0;
    for (int i = 0; i < loops; ++i) {
        bool remove = l && (rand() % 2);
        int pos = (l > 1) ? rand() % (remove ? l-1 : l) : 0;
        QChar c((unsigned short)((i % 26) + (i>25?'A':'a')));
        QString str;
        str += c;
        str += c;
        if (remove) {
            compare.remove(pos, 1);
            table->remove(pos, 1);
        } else {
            compare.insert(pos, str);
            table->insert(pos, str, charFormatIndex);
        }
        l += remove ? -1 : 2;
    }
    QVERIFY(table->plainText() == compare);
    for (int i = 0; i < loops; ++i)
        table->undo();
    QVERIFY(table->plainText() == QString(""));
    for (int i = 0; i < loops; ++i)
        table->redo();
    QVERIFY(table->plainText() == compare);
}


void tst_QTextPieceTable::checkDocumentChanged()
{
    table->enableUndoRedo(false);
    QTestDocumentLayout *layout = new QTestDocumentLayout(doc);
    doc->setDocumentLayout(layout);

    // single insert
    layout->expect(0, 0, 15);
    table->insert(0, "012345678901234", charFormatIndex);
    QVERIFY(!layout->error);

    // single remove
    layout->expect(0, 5, 0);
    table->remove(0, 5);
    QVERIFY(!layout->error);

    // symmetric insert/remove
    layout->expect(0, 0, 0);
    table->beginEditBlock();
    table->insert(0, "01234", charFormatIndex);
    table->remove(0, 5);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 5, 5);
    table->beginEditBlock();
    table->remove(0, 5);
    table->insert(0, "01234", charFormatIndex);
    table->endEditBlock();
    QVERIFY(!layout->error);

    // replace
    layout->expect(0, 3, 5);
    table->beginEditBlock();
    table->remove(0, 3);
    table->insert(0, "01234", charFormatIndex);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 0, 2);
    table->beginEditBlock();
    table->insert(0, "01234", charFormatIndex);
    table->remove(0, 3);
    table->endEditBlock();
    QVERIFY(!layout->error);

    // insert + remove inside insert block
    layout->expect(0, 0, 2);
    table->beginEditBlock();
    table->insert(0, "01234", charFormatIndex);
    table->remove(1, 3);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 0, 2);
    table->beginEditBlock();
    table->insert(0, "01234", charFormatIndex);
    table->remove(2, 3);
    table->endEditBlock();
    QVERIFY(!layout->error);

    // insert + remove partly outside
    layout->expect(0, 1, 0);
    table->beginEditBlock();
    table->insert(1, "0", charFormatIndex);
    table->remove(0, 2);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 1, 1);
    table->beginEditBlock();
    table->insert(1, "01", charFormatIndex);
    table->remove(0, 2);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 1, 2);
    table->beginEditBlock();
    table->insert(1, "012", charFormatIndex);
    table->remove(0, 2);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(1, 1, 0);
    table->beginEditBlock();
    table->insert(1, "0", charFormatIndex);
    table->remove(1, 2);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(1, 1, 1);
    table->beginEditBlock();
    table->insert(1, "01", charFormatIndex);
    table->remove(2, 2);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(1, 1, 2);
    table->beginEditBlock();
    table->insert(1, "012", charFormatIndex);
    table->remove(3, 2);
    table->endEditBlock();
    QVERIFY(!layout->error);

    // insert + remove non overlapping
    layout->expect(0, 1, 1);
    table->beginEditBlock();
    table->insert(1, "0", charFormatIndex);
    table->remove(0, 1);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 2, 2);
    table->beginEditBlock();
    table->insert(2, "1", charFormatIndex);
    table->remove(0, 1);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 2, 2);
    table->beginEditBlock();
    table->remove(0, 1);
    table->insert(1, "0", charFormatIndex);
    table->endEditBlock();
    QVERIFY(!layout->error);

    layout->expect(0, 3, 3);
    table->beginEditBlock();
    table->remove(0, 1);
    table->insert(2, "1", charFormatIndex);
    table->endEditBlock();


    layout->expect(0, 3, 3);
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    table->beginEditBlock();
    table->setCharFormat(0, 1, fmt);
    table->setCharFormat(2, 1, fmt);
    table->endEditBlock();
    QVERIFY(!layout->error);
}

void tst_QTextPieceTable::checkDocumentChanged2()
{
    QTestDocumentLayout *layout = new QTestDocumentLayout(doc);
    doc->setDocumentLayout(layout);

    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    int anotherCharFormatIndex = table->formatCollection()->indexForFormat(fmt);

    layout->expect(0, 0, 12);
    table->beginEditBlock();
    table->insert(0, "0123", charFormatIndex);
    table->insert(4, "4567", anotherCharFormatIndex);
    table->insert(8, "8901", charFormatIndex);
    table->endEditBlock();
    QVERIFY(!layout->error);

    fmt.setFontItalic(true);

    layout->expect(1, 10, 10);
    table->beginEditBlock();
    table->setCharFormat(8, 3, fmt);
    table->setCharFormat(4, 4, fmt);
    table->setCharFormat(1, 3, fmt);
    table->endEditBlock();
    QVERIFY(!layout->error);
}

void tst_QTextPieceTable::setBlockFormat()
{
    QTextBlockFormat bfmt;
    int index = table->formatCollection()->indexForFormat(bfmt);

    table->insertBlock(0, index, charFormatIndex);
    table->insertBlock(0, index, charFormatIndex);
    table->insertBlock(0, index, charFormatIndex);

    QTextBlockFormat newbfmt = bfmt;
    newbfmt.setAlignment(Qt::AlignRight);
    index = table->formatCollection()->indexForFormat(bfmt);
    QTextBlock b = table->blocksFind(1);
    table->setBlockFormat(b, b, newbfmt);

    QVERIFY(table->blocksFind(0).blockFormat() == bfmt);
    QVERIFY(table->blocksFind(1).blockFormat() == newbfmt);
    QVERIFY(table->blocksFind(2).blockFormat() == bfmt);
}


void tst_QTextPieceTable::blockInsertion()
{
    QTextBlockFormat fmt;
    fmt.setTopMargin(100);
    int idx = table->formatCollection()->indexForFormat(fmt);
    int charFormat = table->formatCollection()->indexForFormat(QTextCharFormat());
    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());

    table->insertBlock(0, idx, charFormat);
    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(1).blockFormat() == fmt);

    table->undo();
    QVERIFY(table->blockMap().length() == 1);
    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());

    table->redo();
    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(1).blockFormat() == fmt);
}

void tst_QTextPieceTable::blockInsertion2()
{
    // caused evil failing assertion in fragmentmap
    int pos = 0;
    table->insertBlock(pos, blockFormatIndex, charFormatIndex);
    pos += 1;
    table->insert(pos, "a", charFormatIndex);
    pos += 1;

    pos -= 1;
    table->insertBlock(pos, blockFormatIndex, charFormatIndex);
    QCOMPARE(table->blocksFind(0).position(), 0);
    QCOMPARE(table->blocksFind(1).position(), 1);
    QCOMPARE(table->blocksFind(2).position(), 2);
}

/*
  Tests correct removal behaviour when deleting over block boundaries or complete blocks.
*/

void tst_QTextPieceTable::blockRemoval1()
{
    QTextBlockFormat fmt1;
    fmt1.setTopMargin(100);
    QTextBlockFormat fmt2;
    fmt2.setAlignment(Qt::AlignRight);
    int idx1 = table->formatCollection()->indexForFormat(fmt1);
    int idx2 = table->formatCollection()->indexForFormat(fmt2);

    table->insert(0, "0123", charFormatIndex);
    table->insertBlock(4, idx1, charFormatIndex);
    table->insert(5, "5678", charFormatIndex);
    table->insertBlock(9, idx2, charFormatIndex);
    table->insert(10, "0123", charFormatIndex);

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->beginEditBlock();
    table->remove(5, 5);
    table->endEditBlock();
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(4).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 5);

    table->undo();

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->redo();
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(4).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 5);
}

void tst_QTextPieceTable::blockRemoval2()
{
    QTextBlockFormat fmt1;
    fmt1.setTopMargin(100);
    QTextBlockFormat fmt2;
    fmt2.setAlignment(Qt::AlignRight);
    int idx1 = table->formatCollection()->indexForFormat(fmt1);
    int idx2 = table->formatCollection()->indexForFormat(fmt2);

    table->insert(0, "0123", charFormatIndex);
    table->insertBlock(4, idx1, charFormatIndex);
    table->insert(5, "5678", charFormatIndex);
    table->insertBlock(9, idx2, charFormatIndex);
    table->insert(10, "0123", charFormatIndex);

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->remove(4, 1);
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(6).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 0);

    table->undo();

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->redo();
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(6).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 0);
}

void tst_QTextPieceTable::blockRemoval3()
{
    QTextBlockFormat fmt1;
    fmt1.setTopMargin(100);
    QTextBlockFormat fmt2;
    fmt2.setAlignment(Qt::AlignRight);
    int idx1 = table->formatCollection()->indexForFormat(fmt1);
    int idx2 = table->formatCollection()->indexForFormat(fmt2);

    table->insert(0, "0123", charFormatIndex);
    table->insertBlock(4, idx1, charFormatIndex);
    table->insert(5, "5678", charFormatIndex);
    table->insertBlock(9, idx2, charFormatIndex);
    table->insert(10, "0123", charFormatIndex);

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->beginEditBlock();
    table->remove(3, 4);
    table->endEditBlock();

    QVERIFY(table->blocksFind(1).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 0);

    table->undo();

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->redo();
    QVERIFY(table->blocksFind(1).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 0);
}

void tst_QTextPieceTable::blockRemoval4()
{
#if 0
    QTextBlockFormat fmt1;
    fmt1.setTopMargin(100);
    QTextBlockFormat fmt2;
    fmt2.setAlignment(Qt::AlignRight);
    int idx1 = table->formatCollection()->indexForFormat(fmt1);
    int idx2 = table->formatCollection()->indexForFormat(fmt2);

    table->insert(0, "0123", charFormatIndex);
    table->insertBlock(4, idx1, charFormatIndex);
    table->insert(5, "5678", charFormatIndex);
    table->insertBlock(9, idx2, charFormatIndex);
    table->insert(10, "0123", charFormatIndex);

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->remove(3, 7);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 0);
    QVERIFY(table->blocksFind(1).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == QTextBlockFormat());

    table->undo();

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->redo();
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 0);
    QVERIFY(table->blocksFind(1).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == QTextBlockFormat());
#endif
}

void tst_QTextPieceTable::blockRemoval5()
{
    QTextBlockFormat fmt1;
    fmt1.setTopMargin(100);
    QTextBlockFormat fmt2;
    fmt2.setAlignment(Qt::AlignRight);
    int idx1 = table->formatCollection()->indexForFormat(fmt1);
    int idx2 = table->formatCollection()->indexForFormat(fmt2);

    table->insert(0, "0123", charFormatIndex);
    table->insertBlock(4, idx1, charFormatIndex);
    table->insert(5, "5678", charFormatIndex);
    table->insertBlock(9, idx2, charFormatIndex);
    table->insert(10, "0123", charFormatIndex);

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->beginEditBlock();
    table->remove(3, 8);
    table->endEditBlock();

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 0);

    table->undo();

    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(4).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == fmt1);
    QVERIFY(table->blocksFind(10).blockFormat() == fmt2);
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(6).position() == 5);
    QVERIFY(table->blocksFind(11).position() == 10);

    table->redo();
    QVERIFY(table->blocksFind(0).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(5).blockFormat() == QTextBlockFormat());
    QVERIFY(table->blocksFind(1).position() == 0);
    QVERIFY(table->blocksFind(5).position() == 0);
}


void tst_QTextPieceTable::checkBlockSeparation()
{
    table->insertBlock(0, blockFormatIndex, charFormatIndex);
    table->insertBlock(1, blockFormatIndex, charFormatIndex);

    QVERIFY(table->find(0) != table->find(1));
}

void tst_QTextPieceTable::checkFrames1()
{
    QTextFrameFormat ffmt;
    table->insert(0, "Hello", charFormatIndex);
    QPointer<QTextFrame> frame = table->insertFrame(1, 3, ffmt);
    QTextFrame *root = table->rootFrame();

    QVERIFY(root == frame->parentFrame());

    QVERIFY(root);
    QVERIFY(root->parentFrame() == 0);

    QVERIFY(root->childFrames().count() == 1);
    QVERIFY(frame->format() == ffmt);
    QVERIFY(frame->firstPosition() == 2);
    QVERIFY(frame->lastPosition() == 4);


    QPointer<QTextFrame> frame2 = table->insertFrame(2, 3, ffmt);

    QVERIFY(root->childFrames().count() == 1);
    QVERIFY(root->childFrames().at(0) == frame);
    QVERIFY(frame->childFrames().count() == 1);
    QVERIFY(frame2->childFrames().count() == 0);
    QVERIFY(frame2->parentFrame() == frame);
    QVERIFY(frame2->firstPosition() == 3);
    QVERIFY(frame2->lastPosition() == 4);

    QVERIFY(frame->format() == ffmt);
    QVERIFY(frame->firstPosition() == 2);
    QVERIFY(frame->lastPosition() == 6);

    table->removeFrame(frame);

    QVERIFY(root->childFrames().count() == 1);
    QVERIFY(root->childFrames().at(0) == frame2);
    QVERIFY(!frame);
    QVERIFY(frame2->childFrames().count() == 0);
    QVERIFY(frame2->parentFrame() == root);
    QVERIFY(frame2->firstPosition() == 2);
    QVERIFY(frame2->lastPosition() == 3);

    table->undo();

    frame = table->frameAt(2);

    QVERIFY(root->childFrames().count() == 1);
    QVERIFY(root->childFrames().at(0) == frame);
    QVERIFY(frame->childFrames().count() == 1);
    QVERIFY(frame->childFrames().at(0) == frame2);
    QVERIFY(frame2->childFrames().count() == 0);
    QVERIFY(frame2->parentFrame() == frame);
    QVERIFY(frame2->firstPosition() == 3);
    QVERIFY(frame2->lastPosition() == 4);

    QVERIFY(frame->firstPosition() == 2);
    QVERIFY(frame->lastPosition() == 6);

    table->undo();

    QVERIFY(root->childFrames().count() == 1);
    QVERIFY(root->childFrames().at(0) == frame);
    QVERIFY(frame->childFrames().count() == 0);
    QVERIFY(!frame2);

    QVERIFY(frame->firstPosition() == 2);
    QVERIFY(frame->lastPosition() == 4);
}

void tst_QTextPieceTable::removeFrameDirect()
{
    QTextFrameFormat ffmt;
    table->insert(0, "Hello", charFormatIndex);

    QTextFrame *frame = table->insertFrame(1, 5, ffmt);

    QVERIFY(frame->parentFrame() == table->rootFrame());

    const int start = frame->firstPosition() - 1;
    const int end = frame->lastPosition();
    const int length = end - start + 1;

    table->remove(start, length);
}

void tst_QTextPieceTable::removeWithChildFrame()
{
    /*
       The piecetable layout is:

       ...
       1 BeginningOfFrame(first frame)
       2 text
       3 BeginningOfFrame(second frame)
       4 text
       5 text
       6 EndOfFrame(second frame)
       7 text
       8 text
       9 EndOfFrame(first frame)
       ...

       The idea is to remove from [2] until [6], basically some trailing text and the second frame.
       In this case frameAt(2) != frameAt(6), so the assertion in remove() needed an adjustement.
     */
    QTextFrameFormat ffmt;
    table->insert(0, "Hello World", charFormatIndex);

    QTextFrame *frame = table->insertFrame(1, 6, ffmt);
    QTextFrame *childFrame = table->insertFrame(3, 5, ffmt);
    Q_UNUSED(frame);
    Q_UNUSED(childFrame);

    // used to give a failing assertion
    table->remove(2, 5);
    QVERIFY(true);
}

void tst_QTextPieceTable::clearWithFrames()
{
    /*
       The piecetable layout is:

       ...
       1 BeginningOfFrame(first frame)
       2 text
       3 EndOfFrame(first frame)
       4 BeginningOfFrame(second frame)
       5 text
       6 text
       7 EndOfFrame(second frame)
       ...

       The idea is to remove from [1] until [7].
     */
    QTextFrameFormat ffmt;
    table->insert(0, "Hello World", charFormatIndex);

    QTextFrame *firstFrame = table->insertFrame(1, 2, ffmt);
    QTextFrame *secondFrame = table->insertFrame(4, 6, ffmt);

    const int start = firstFrame->firstPosition() - 1;
    const int end = secondFrame->lastPosition();
    const int length = end - start + 1;
    // used to give a failing assertion
    table->remove(start, length);
    QVERIFY(true);
}

QTEST_MAIN(tst_QTextPieceTable)


#include "tst_qtextpiecetable.moc"

