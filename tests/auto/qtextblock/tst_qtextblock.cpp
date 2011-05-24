/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#define private public
#include <qtextdocument.h>
#include <qdebug.h>
#ifndef Q_WS_WIN
#include <private/qtextdocument_p.h>
#endif



#include <qtextobject.h>
#include <qtextcursor.h>


//TESTED_FILES=

QT_FORWARD_DECLARE_CLASS(QTextDocument)

class tst_QTextBlock : public QObject
{
    Q_OBJECT

public:
    tst_QTextBlock();


public slots:
    void init();
    void cleanup();
private slots:
    void fragmentOverBlockBoundaries();
    void excludeParagraphSeparatorFragment();
    void backwardsBlockIterator();
    void previousBlock_qtbug18026();
    void removedBlock_qtbug18500();

private:
    QTextDocument *doc;
    QTextCursor cursor;
};

tst_QTextBlock::tst_QTextBlock()
{}

void tst_QTextBlock::init()
{
    doc = new QTextDocument;
    cursor = QTextCursor(doc);
}

void tst_QTextBlock::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextBlock::fragmentOverBlockBoundaries()
{
    /* this creates two fragments in the piecetable:
     * 1) 'hello<parag separator here>world'
     * 2) '<parag separator>'
     * (they are not united because the former was interested after the latter,
     * hence their position in the pt buffer is the other way around)
     */
    cursor.insertText("Hello");
    cursor.insertBlock();
    cursor.insertText("World");

    cursor.movePosition(QTextCursor::Start);

    const QTextDocument *doc = cursor.block().document();
    QVERIFY(doc);
    // Block separators are always a fragment of their self. Thus:
    // |Hello|\b|World|\b|
#if !defined(Q_WS_WIN) && !defined(Q_WS_S60)
    QVERIFY(doc->d_func()->fragmentMap().numNodes() == 4);
#endif

    QCOMPARE(cursor.block().text(), QString("Hello"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("World"));
}

void tst_QTextBlock::excludeParagraphSeparatorFragment()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertText("Hello", fmt);

    QTextBlock block = doc->begin();
    QVERIFY(block.isValid());

    QTextBlock::Iterator it = block.begin();

    QTextFragment fragment = it.fragment();
    QVERIFY(fragment.isValid());
    QCOMPARE(fragment.text(), QString("Hello"));

    ++it;
    QVERIFY(it.atEnd());
    QVERIFY(it == block.end());
}

void tst_QTextBlock::backwardsBlockIterator()
{
    QTextCharFormat fmt;

    fmt.setForeground(Qt::magenta);
    cursor.insertText("A", fmt);

    fmt.setForeground(Qt::red);
    cursor.insertText("A", fmt);

    fmt.setForeground(Qt::magenta);
    cursor.insertText("A", fmt);

    QTextBlock block = doc->begin();
    QVERIFY(block.isValid());

    QTextBlock::Iterator it = block.begin();
    QCOMPARE(it.fragment().position(), 0);
    ++it;
    QCOMPARE(it.fragment().position(), 1);
    ++it;

    QCOMPARE(it.fragment().position(), 2);

    --it;
    QCOMPARE(it.fragment().position(), 1);
    --it;
    QCOMPARE(it.fragment().position(), 0);
}

void tst_QTextBlock::previousBlock_qtbug18026()
{
    QTextBlock last = doc->end().previous();
    QVERIFY(last.isValid());
}

void tst_QTextBlock::removedBlock_qtbug18500()
{
    cursor.insertText("line 1\nline 2\nline 3 \nline 4\n");
    cursor.setPosition(7);
    QTextBlock block = cursor.block();
    cursor.setPosition(21, QTextCursor::KeepAnchor);

    cursor.removeSelectedText();
    QVERIFY(!block.isValid());
}

QTEST_MAIN(tst_QTextBlock)
#include "tst_qtextblock.moc"
