// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

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
#ifndef QT_NO_TEXTHTMLPARSER
    void anchorAt();
    void imageAt();
    void formatAt();
#endif
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

    void draw(QPainter *, const PaintContext &) override {}
    int hitTest(const QPointF &, Qt::HitTestAccuracy) const override { return 0; }
    int pageCount() const override { return 0; }
    QSizeF documentSize() const override { return QSizeF(); }
    QRectF frameBoundingRect(QTextFrame *) const override { return QRectF(); }
    QRectF blockBoundingRect(const QTextBlock &) const override { return QRectF(); }
    void documentChanged(int from, int /* oldLength */, int length) override
    {
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

#ifndef QT_NO_TEXTHTMLPARSER
void tst_QAbstractTextDocumentLayout::anchorAt()
{
    QTextDocument doc;
    doc.setHtml("<a href=\"link\">foo</a>");
    QAbstractTextDocumentLayout *documentLayout = doc.documentLayout();
    QTextBlock firstBlock = doc.begin();
    QTextLayout *layout = firstBlock.layout();
    layout->setPreeditArea(doc.toPlainText().size(), "xxx");

    doc.setPageSize(QSizeF(1000, 1000));
    QFontMetrics metrics(layout->font());
    QPointF blockStart = documentLayout->blockBoundingRect(firstBlock).topLeft();

    // anchorAt on start returns link
    QRect linkBr = metrics.boundingRect("foo");
    QPointF linkPoint((linkBr.width() / 2) + blockStart.x(), (linkBr.height() / 2) + blockStart.y());
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

void tst_QAbstractTextDocumentLayout::imageAt()
{
    QTextDocument doc;
    doc.setHtml("foo<a href=\"link\"><img src=\"image\" width=\"50\" height=\"50\"/></a>");
    QAbstractTextDocumentLayout *documentLayout = doc.documentLayout();
    QTextBlock firstBlock = doc.begin();
    QTextLayout *layout = firstBlock.layout();
    layout->setPreeditArea(doc.toPlainText().size(), "xxx");

    doc.setPageSize(QSizeF(1000, 1000));
    QFontMetrics metrics(layout->font());
    QPointF blockStart = documentLayout->blockBoundingRect(firstBlock).topLeft();

    QRect fooBr = metrics.boundingRect("foo");
    QPointF imagePoint(fooBr.width() + blockStart.x() + 25, blockStart.y() + 25);
    // imageAt on image returns source
    QCOMPARE(documentLayout->imageAt(imagePoint), QString("image"));
    // anchorAt on image returns link
    QCOMPARE(documentLayout->anchorAt(imagePoint), QString("link"));

    // imageAt on start returns nothing (there's the "foo" text)
    QPointF fooPoint(blockStart.x() + (fooBr.width() / 2), (fooBr.height() / 2) + blockStart.y());
    QCOMPARE(documentLayout->imageAt(fooPoint), QString());
}

void tst_QAbstractTextDocumentLayout::formatAt()
{
    QTextDocument doc;
    doc.setHtml("foo<i><a href=\"link\"><img src=\"image\" width=\"50\" height=\"50\"/></a></i>");
    QAbstractTextDocumentLayout *documentLayout = doc.documentLayout();
    QTextBlock firstBlock = doc.begin();
    QTextLayout *layout = firstBlock.layout();
    layout->setPreeditArea(doc.toPlainText().size(), "xxx");

    doc.setPageSize(QSizeF(1000, 1000));
    QFontMetrics metrics(layout->font());
    QPointF blockStart = documentLayout->blockBoundingRect(firstBlock).topLeft();

    QRect fooBr = metrics.boundingRect("foo");
    QPointF imagePoint(fooBr.width() + blockStart.x() + 25, blockStart.y() + 25);

    QTextFormat format = documentLayout->formatAt(imagePoint);
    QVERIFY(format.isCharFormat());
    QVERIFY(format.toCharFormat().isAnchor());
    QVERIFY(format.toCharFormat().fontItalic());
    QVERIFY(format.isImageFormat());

    // move over the unformatted "foo" text)
    QPointF fooPoint(blockStart.x() + (fooBr.width() / 2), (fooBr.height() / 2) + blockStart.y());
    format = documentLayout->formatAt(fooPoint);
    QVERIFY(format.isCharFormat());
    QVERIFY(!format.toCharFormat().isAnchor());
    QVERIFY(!format.toCharFormat().fontItalic());
    QVERIFY(!format.isImageFormat());
}
#endif

QTEST_MAIN(tst_QAbstractTextDocumentLayout)
#include "tst_qabstracttextdocumentlayout.moc"
