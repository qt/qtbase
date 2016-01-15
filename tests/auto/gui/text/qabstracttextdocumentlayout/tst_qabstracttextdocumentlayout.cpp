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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstracttextdocumentlayout.h>
#include <qimage.h>
#include <qtextobject.h>
#include <qfontmetrics.h>

class tst_QAbstractTextDocumentLayout : public QObject
{
Q_OBJECT

public:
    tst_QAbstractTextDocumentLayout();
    virtual ~tst_QAbstractTextDocumentLayout();

private slots:
    void getSetCheck();
    void maximumBlockCount();
    void anchorAt();
};

tst_QAbstractTextDocumentLayout::tst_QAbstractTextDocumentLayout()
{
}

tst_QAbstractTextDocumentLayout::~tst_QAbstractTextDocumentLayout()
{
}

class MyAbstractTextDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_OBJECT
public:
    MyAbstractTextDocumentLayout(QTextDocument *doc)
        : QAbstractTextDocumentLayout(doc)
        , gotFullLayout(false)
        , blockCount(0)
        , changeEvents(0)
    {
    }

    void draw(QPainter *, const PaintContext &) {}
    int hitTest(const QPointF &, Qt::HitTestAccuracy) const { return 0; }
    int pageCount() const { return 0; }
    QSizeF documentSize() const { return QSizeF(); }
    QRectF frameBoundingRect(QTextFrame *) const { return QRectF(); }
    QRectF blockBoundingRect(const QTextBlock &) const { return QRectF(); }
    void documentChanged(int from, int /* oldLength */, int length) {
        ++changeEvents;

        QTextBlock last = document()->lastBlock();
        int lastPos = last.position() + last.length() - 1;
        if (from == 0 && length == lastPos)
            gotFullLayout = true;
    }

    bool gotFullLayout;
    int blockCount;
    int changeEvents;

public slots:
    void blockCountChanged(int bc) { blockCount = bc; }
};

// Testing get/set functions
void tst_QAbstractTextDocumentLayout::getSetCheck()
{
    QTextDocument doc;
    MyAbstractTextDocumentLayout obj1(&doc);
    // QPaintDevice * QAbstractTextDocumentLayout::paintDevice()
    // void QAbstractTextDocumentLayout::setPaintDevice(QPaintDevice *)
    QImage *var1 = new QImage(QSize(10,10), QImage::Format_ARGB32_Premultiplied);
    obj1.setPaintDevice(var1);
    QCOMPARE(static_cast<QPaintDevice *>(var1), obj1.paintDevice());
    obj1.setPaintDevice((QPaintDevice *)0);
    QCOMPARE(static_cast<QPaintDevice *>(0), obj1.paintDevice());
    delete var1;
}

void tst_QAbstractTextDocumentLayout::maximumBlockCount()
{
    QTextDocument doc;
    doc.setMaximumBlockCount(10);

    MyAbstractTextDocumentLayout layout(&doc);
    doc.setDocumentLayout(&layout);
    QObject::connect(&doc, SIGNAL(blockCountChanged(int)), &layout, SLOT(blockCountChanged(int)));

    QTextCursor cursor(&doc);
    for (int i = 0; i < 10; ++i) {
        cursor.insertBlock();
        cursor.insertText("bla");
    }

    QCOMPARE(layout.blockCount, 10);

    layout.gotFullLayout = false;
    layout.changeEvents = 0;
    cursor.insertBlock();
    QCOMPARE(layout.changeEvents, 2);
    cursor.insertText("foo");
    QCOMPARE(layout.changeEvents, 3);
    cursor.insertBlock();
    QCOMPARE(layout.changeEvents, 5);
    cursor.insertText("foo");
    QCOMPARE(layout.changeEvents, 6);

    QVERIFY(!layout.gotFullLayout);

    QCOMPARE(layout.blockCount, 10);
}

void tst_QAbstractTextDocumentLayout::anchorAt()
{
    QTextDocument doc;
    doc.setHtml("<a href=\"link\">foo</a>");
    QAbstractTextDocumentLayout *documentLayout = doc.documentLayout();
    QTextBlock firstBlock = doc.begin();
    QTextLayout *layout = firstBlock.layout();
    layout->setPreeditArea(doc.toPlainText().length(), "xxx");

    doc.setPageSize(QSizeF(1000, 1000));
    QFontMetrics metrics(layout->font());
    QPointF blockStart = documentLayout->blockBoundingRect(firstBlock).topLeft();

    // anchorAt on start returns link
    QRect linkBr = metrics.boundingRect("foo");
    QPointF linkPoint(linkBr.width() + blockStart.x(), (linkBr.height() / 2) + blockStart.y());
    QCOMPARE(documentLayout->anchorAt(linkPoint), QString("link"));

    // anchorAt() on top of preedit at end should not assert
    QRect preeditBr = metrics.boundingRect(doc.toPlainText() + "xx");
    QPointF preeditPoint(preeditBr.width() + blockStart.x(), (preeditBr.height() / 2) + blockStart.y());
    QCOMPARE(documentLayout->anchorAt(preeditPoint), QString());

    // preedit at start should not return link
    layout->setPreeditArea(0, "xxx");
    preeditBr = metrics.boundingRect("xx");
    preeditPoint = QPointF(preeditBr.width() + blockStart.x(), (preeditBr.height() / 2) + blockStart.y());
    QCOMPARE(documentLayout->anchorAt(preeditPoint), QString());
}

QTEST_MAIN(tst_QAbstractTextDocumentLayout)
#include "tst_qabstracttextdocumentlayout.moc"
