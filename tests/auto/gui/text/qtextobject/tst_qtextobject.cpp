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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qtextobject.h>
#include <qtextdocument.h>
#ifndef QT_NO_WIDGETS
#include <qtextedit.h>
#endif
#include <qtextcursor.h>

class tst_QTextObject : public QObject
{
Q_OBJECT

public:
    tst_QTextObject();
    virtual ~tst_QTextObject();

private slots:
#ifndef QT_NO_WIDGETS
    void getSetCheck();
#endif
    void testStandAloneTextObject();
};

tst_QTextObject::tst_QTextObject()
{
}

tst_QTextObject::~tst_QTextObject()
{
}

#ifndef QT_NO_WIDGETS
// Testing get/set functions
void tst_QTextObject::getSetCheck()
{
    QTextEdit edit;
    QTextFrame obj1(edit.document());
    // QTextFrameLayoutData * QTextFrame::layoutData()
    // void QTextFrame::setLayoutData(QTextFrameLayoutData *)
    QTextFrameLayoutData *var1 = new QTextFrameLayoutData;
    obj1.setLayoutData(var1);
    QCOMPARE(var1, obj1.layoutData());
    obj1.setLayoutData((QTextFrameLayoutData *)0);
    QCOMPARE((QTextFrameLayoutData *)0, obj1.layoutData());
    // delete var1; // No delete, since QTextFrame takes ownership

    QTextBlock obj2 = edit.textCursor().block();
    // QTextBlockUserData * QTextBlock::userData()
    // void QTextBlock::setUserData(QTextBlockUserData *)
    QTextBlockUserData *var2 = new QTextBlockUserData;
    obj2.setUserData(var2);
    QCOMPARE(var2, obj2.userData());
    obj2.setUserData((QTextBlockUserData *)0);
    QCOMPARE((QTextBlockUserData *)0, obj2.userData());

    // int QTextBlock::userState()
    // void QTextBlock::setUserState(int)
    obj2.setUserState(0);
    QCOMPARE(0, obj2.userState());
    obj2.setUserState(INT_MIN);
    QCOMPARE(INT_MIN, obj2.userState());
    obj2.setUserState(INT_MAX);
    QCOMPARE(INT_MAX, obj2.userState());
}
#endif

class TestTextObject : public QTextObject
{
public:
    TestTextObject(QTextDocument *document) : QTextObject(document) {}
};

void tst_QTextObject::testStandAloneTextObject()
{
    QTextDocument document;
    TestTextObject textObject(&document);

    QCOMPARE(textObject.document(), &document);
    // don't crash
    textObject.format();
    textObject.formatIndex();
    QCOMPARE(textObject.objectIndex(), -1);
}

QTEST_MAIN(tst_QTextObject)
#include "tst_qtextobject.moc"
