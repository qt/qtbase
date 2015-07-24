/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtGui/QtGui>
#include <private/qtextengine_p.h>

#include "bidireorderstring.h"

class tst_QComplexText : public QObject
{
Q_OBJECT

public:
    tst_QComplexText();
    virtual ~tst_QComplexText();


public slots:
    void init();
    void cleanup();
private slots:
    void bidiReorderString_data();
    void bidiReorderString();
    void bidiCursor_qtbug2795();
    void bidiCursor_PDF();
    void bidiCursorMovement_data();
    void bidiCursorMovement();
    void bidiCursorLogicalMovement_data();
    void bidiCursorLogicalMovement();
    void bidiInvalidCursorNoMovement_data();
    void bidiInvalidCursorNoMovement();
};

tst_QComplexText::tst_QComplexText()
{
}

tst_QComplexText::~tst_QComplexText()
{

}

void tst_QComplexText::init()
{
// This will be executed immediately before each test is run.
// TODO: Add initialization code here.
}

void tst_QComplexText::cleanup()
{
// This will be executed immediately after each test is run.
// TODO: Add cleanup code here.
}

void tst_QComplexText::bidiReorderString_data()
{
    QTest::addColumn<QString>("logical");
    QTest::addColumn<QString>("VISUAL");
    QTest::addColumn<int>("basicDir");

    const LV *data = logical_visual;
    while ( data->name ) {
        //next we fill it with data
        QTest::newRow( data->name )
            << QString::fromUtf8( data->logical )
            << QString::fromUtf8( data->visual )
            << (int) data->basicDir;
        data++;
    }
}

void tst_QComplexText::bidiReorderString()
{
    QFETCH( QString, logical );
    QFETCH( int,  basicDir );

    // replace \n with Unicode newline. The new algorithm ignores \n
    logical.replace(QChar('\n'), QChar(0x2028));

    QTextEngine e(logical, QFont());
    e.option.setTextDirection((QChar::Direction)basicDir == QChar::DirL ? Qt::LeftToRight : Qt::RightToLeft);
    e.itemize();
    quint8 levels[256];
    int visualOrder[256];
    int nitems = e.layoutData->items.size();
    int i;
    for (i = 0; i < nitems; ++i) {
        //qDebug("item %d bidiLevel=%d", i,  e.items[i].analysis.bidiLevel);
        levels[i] = e.layoutData->items[i].analysis.bidiLevel;
    }
    e.bidiReorder(nitems, levels, visualOrder);

    QString visual;
    for (i = 0; i < nitems; ++i) {
        QScriptItem &si = e.layoutData->items[visualOrder[i]];
        QString sub = logical.mid(si.position, e.length(visualOrder[i]));
        if (si.analysis.bidiLevel % 2) {
            // reverse sub
            QChar *a = (QChar *)sub.unicode();
            QChar *b = a + sub.length() - 1;
            while (a < b) {
                QChar tmp = *a;
                *a = *b;
                *b = tmp;
                ++a;
                --b;
            }
            a = (QChar *)sub.unicode();
            b = a + sub.length();
            while (a<b) {
                *a = a->mirroredChar();
                ++a;
            }
        }
        visual += sub;
    }
    // replace Unicode newline back with  \n to compare.
    visual.replace(QChar(0x2028), QChar('\n'));

    QTEST(visual, "VISUAL");
}

void tst_QComplexText::bidiCursor_qtbug2795()
{
    QString str = QString::fromUtf8("الجزيرة نت");
    QTextLayout l1(str);

    l1.beginLayout();
    l1.setCacheEnabled(true);
    QTextLine line1 = l1.createLine();
    l1.endLayout();

    qreal x1 = line1.cursorToX(0) - line1.cursorToX(str.size());

    str.append("1");
    QTextLayout l2(str);
    l2.setCacheEnabled(true);
    l2.beginLayout();
    QTextLine line2 = l2.createLine();
    l2.endLayout();

    qreal x2 = line2.cursorToX(0) - line2.cursorToX(str.size());

    // The cursor should remain at the same position after a digit is appended
    QCOMPARE(x1, x2);
}

void tst_QComplexText::bidiCursorMovement_data()
{
    QTest::addColumn<QString>("logical");
    QTest::addColumn<int>("basicDir");

    const LV *data = logical_visual;
    while ( data->name ) {
        //next we fill it with data
        QTest::newRow( data->name )
            << QString::fromUtf8( data->logical )
            << (int) data->basicDir;
        data++;
    }
}

void tst_QComplexText::bidiCursorMovement()
{
    QFETCH(QString, logical);
    QFETCH(int,  basicDir);

    QTextLayout layout(logical);
    layout.setCacheEnabled(true);

    QTextOption option = layout.textOption();
    option.setTextDirection(basicDir == QChar::DirL ? Qt::LeftToRight : Qt::RightToLeft);
    layout.setTextOption(option);
    layout.setCursorMoveStyle(Qt::VisualMoveStyle);
    bool moved;
    int oldPos, newPos = 0;
    qreal x, newX;

    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    newX = line.cursorToX(0);
    do {
        oldPos = newPos;
        x = newX;
        newX = line.cursorToX(oldPos);
        if (basicDir == QChar::DirL) {
            QVERIFY(newX >= x);
            newPos = layout.rightCursorPosition(oldPos);
        } else
        {
            QVERIFY(newX <= x);
            newPos = layout.leftCursorPosition(oldPos);
        }
        moved = (oldPos != newPos);
    } while (moved);
}

void tst_QComplexText::bidiCursorLogicalMovement_data()
{
    bidiCursorMovement_data();
}

void tst_QComplexText::bidiCursorLogicalMovement()
{
    QFETCH(QString, logical);
    QFETCH(int,  basicDir);

    QTextLayout layout(logical);

    QTextOption option = layout.textOption();
    option.setTextDirection(basicDir == QChar::DirL ? Qt::LeftToRight : Qt::RightToLeft);
    layout.setTextOption(option);
    bool moved;
    int oldPos, newPos = 0;

    do {
        oldPos = newPos;
        newPos = layout.nextCursorPosition(oldPos);
        QVERIFY(newPos >= oldPos);
        moved = (oldPos != newPos);
    } while (moved);

    do {
        oldPos = newPos;
        newPos = layout.previousCursorPosition(oldPos);
        QVERIFY(newPos <= oldPos);
        moved = (oldPos != newPos);
    } while (moved);
}

void tst_QComplexText::bidiInvalidCursorNoMovement_data()
{
    bidiCursorMovement_data();
}

void tst_QComplexText::bidiInvalidCursorNoMovement()
{
    QFETCH(QString, logical);
    QFETCH(int,  basicDir);

    QTextLayout layout(logical);

    QTextOption option = layout.textOption();
    option.setTextDirection(basicDir == QChar::DirL ? Qt::LeftToRight : Qt::RightToLeft);
    layout.setTextOption(option);

    // visual
    QCOMPARE(layout.rightCursorPosition(-1000), -1000);
    QCOMPARE(layout.rightCursorPosition(1000), 1000);

    QCOMPARE(layout.leftCursorPosition(-1000), -1000);
    QCOMPARE(layout.leftCursorPosition(1000), 1000);

    // logical
    QCOMPARE(layout.nextCursorPosition(-1000), -1000);
    QCOMPARE(layout.nextCursorPosition(1000), 1000);

    QCOMPARE(layout.previousCursorPosition(-1000), -1000);
    QCOMPARE(layout.previousCursorPosition(1000), 1000);
}

void tst_QComplexText::bidiCursor_PDF()
{
    QString str = QString::fromUtf8("\342\200\252hello\342\200\254");
    QTextLayout layout(str);
    layout.setCacheEnabled(true);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    int size = str.size();

    QVERIFY(line.cursorToX(size) == line.cursorToX(size - 1));
}

QTEST_MAIN(tst_QComplexText)
#include "tst_qcomplextext.moc"
