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
#include <QtWidgets/qgraphicsanchorlayout.h>
#include <private/qgraphicsanchorlayout_p.h>
#include <QtWidgets/qgraphicswidget.h>
#include <QtWidgets/qgraphicsproxywidget.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qstylefactory.h>
#include <QtWidgets/qproxystyle.h>


class tst_QGraphicsAnchorLayout : public QObject {
    Q_OBJECT

public:
    tst_QGraphicsAnchorLayout() : QObject() {
        hasSimplification = qgetenv("QT_ANCHORLAYOUT_NO_SIMPLIFICATION").isEmpty();
    }

private:
    bool hasSimplification;

private slots:
    void simple();
    void simple_center();
    void simple_semifloat();
    void layoutDirection();
    void diagonal();
    void parallel();
    void parallel2();
    void snake();
    void snakeOppositeDirections();
    void fairDistribution();
    void fairDistributionOppositeDirections();
    void proportionalPreferred();
    void example();
    void setSpacing();
    void styleDefaults();
    void hardComplexS60();
    void stability();
    void delete_anchor();
    void conflicts();
    void sizePolicy();
    void floatConflict();
    void infiniteMaxSizes();
    void simplifiableUnfeasible();
    void simplificationVsOrder();
    void parallelSimplificationOfCenter();
    void simplificationVsRedundance();
    void spacingPersistency();
    void snakeParallelWithLayout();
    void parallelToHalfLayout();
    void globalSpacing();
    void graphicsAnchorHandling();
    void invalidHierarchyCheck();
};

class RectWidget : public QGraphicsWidget
{
public:
    RectWidget(QGraphicsItem *parent = 0) : QGraphicsWidget(parent){}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->drawRoundRect(rect());
        painter->drawLine(rect().topLeft(), rect().bottomRight());
        painter->drawLine(rect().bottomLeft(), rect().topRight());
    }
};

static QGraphicsWidget *createItem(const QSizeF &minimum = QSizeF(100.0, 100.0),
                                   const QSizeF &preferred = QSize(150.0, 100.0),
                                   const QSizeF &maximum = QSizeF(200.0, 100.0),
                                   const QString &name = QString())
{
    QGraphicsWidget *w = new RectWidget;
    w->setMinimumSize(minimum);
    w->setPreferredSize(preferred);
    w->setMaximumSize(maximum);
    w->setData(0, name);
    return w;
}

static void setAnchor(QGraphicsAnchorLayout *l,
                      QGraphicsLayoutItem *firstItem,
                      Qt::AnchorPoint firstEdge,
                      QGraphicsLayoutItem *secondItem,
                      Qt::AnchorPoint secondEdge,
                      qreal spacing = 0)
{
    QGraphicsAnchor *anchor = l->addAnchor(firstItem, firstEdge, secondItem, secondEdge);
    anchor->setSpacing(spacing);
}

static bool checkReverseDirection(QGraphicsWidget *widget)
{
    QGraphicsLayout *layout = widget->layout();
    qreal left, top, right, bottom;
    layout->getContentsMargins(&left, &top, &right, &bottom);
    widget->setLayoutDirection(Qt::LeftToRight);
    QApplication::processEvents();
    QMap<QGraphicsLayoutItem *, QRectF> geometries;
    for (int i = 0; i < layout->count(); ++i) {
        QGraphicsLayoutItem *item = layout->itemAt(i);
        geometries.insert(item, item->geometry());
    }
    widget->setLayoutDirection(Qt::RightToLeft);
    QApplication::processEvents();
    const QRectF layoutGeometry = layout->geometry().adjusted(+right, +top, -left, -bottom);
    for (int i = 0; i < layout->count(); ++i) {
        QGraphicsLayoutItem *item = layout->itemAt(i);
        const QRectF rightToLeftGeometry = item->geometry();
        const QRectF leftToRightGeometry = geometries.value(item);
        QRectF expectedGeometry = leftToRightGeometry;
        expectedGeometry.moveRight(layoutGeometry.right() - leftToRightGeometry.left());
        if (expectedGeometry != rightToLeftGeometry) {
            qDebug() << "layout->geometry():" << layoutGeometry
                     << "expected:" << expectedGeometry
                     << "actual:" << rightToLeftGeometry;
            return false;
        }
    }
    return true;
}

static bool layoutHasConflict(QGraphicsAnchorLayout *l)
{
    return QGraphicsAnchorLayoutPrivate::get(l)->hasConflicts();
}

static bool usedSimplex(QGraphicsAnchorLayout *l, Qt::Orientation o)
{
    QGraphicsAnchorLayoutPrivate::Orientation oo = (o == Qt::Horizontal) ?
        QGraphicsAnchorLayoutPrivate::Horizontal :
        QGraphicsAnchorLayoutPrivate::Vertical;

    return QGraphicsAnchorLayoutPrivate::get(l)->lastCalculationUsedSimplex[oo];
}

void tst_QGraphicsAnchorLayout::simple()
{
    QGraphicsWidget *w1 = createItem();
    QGraphicsWidget *w2 = createItem();

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);

    // Horizontal
    l->addAnchor(l, Qt::AnchorLeft, w1, Qt::AnchorLeft);
    l->addAnchor(w1, Qt::AnchorRight, w2, Qt::AnchorLeft);
    l->addAnchor(w2, Qt::AnchorRight, l, Qt::AnchorRight);

    // Vertical
    l->addAnchors(l, w1, Qt::Vertical);
    l->addAnchors(l, w2, Qt::Vertical);

    QCOMPARE(l->count(), 2);

    QGraphicsWidget p;
    p.setLayout(l);
    p.adjustSize();

    if (hasSimplification) {
        QVERIFY(!usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }
}

void tst_QGraphicsAnchorLayout::simple_center()
{
    QSizeF minSize(10, 10);
    QSizeF pref(50, 10);
    QSizeF maxSize(100, 10);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "a");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "b");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "c");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    // horizontal
    setAnchor(l, l, Qt::AnchorLeft, a, Qt::AnchorLeft, 0);
    setAnchor(l, a, Qt::AnchorRight, b, Qt::AnchorLeft, 0);
    setAnchor(l, b, Qt::AnchorRight, l, Qt::AnchorRight, 0);
    setAnchor(l, a, Qt::AnchorHorizontalCenter, c, Qt::AnchorLeft, 0);
    setAnchor(l, c, Qt::AnchorRight, b, Qt::AnchorHorizontalCenter, 0);

    // vertical
    setAnchor(l, l, Qt::AnchorTop, a, Qt::AnchorTop, 0);
    setAnchor(l, l, Qt::AnchorTop, b, Qt::AnchorTop, 0);
    setAnchor(l, a, Qt::AnchorBottom, c, Qt::AnchorTop, 0);
    setAnchor(l, b, Qt::AnchorBottom, c, Qt::AnchorTop, 0);
    setAnchor(l, c, Qt::AnchorBottom, l, Qt::AnchorBottom, 0);

    QCOMPARE(l->count(), 3);

    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);
    p->setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QCOMPARE(layoutMinimumSize, QSizeF(20, 20));

    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QCOMPARE(layoutMaximumSize, QSizeF(200, 20));

    if (hasSimplification) {
        QVERIFY(usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }

    delete p;
}

void tst_QGraphicsAnchorLayout::simple_semifloat()
{
    // Useful for testing simplification between A_left and B_left.
    // Unfortunately the only way to really test that now is to manually inspect the
    // simplified graph.
    QSizeF minSize(10, 10);
    QSizeF pref(50, 10);
    QSizeF maxSize(100, 10);

    QGraphicsWidget *A = createItem(minSize, pref, maxSize, "A");
    QGraphicsWidget *B = createItem(minSize, pref, maxSize, "B");
    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "a");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "b");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);

    // horizontal
    setAnchor(l, l, Qt::AnchorLeft, A, Qt::AnchorLeft, 0);
    setAnchor(l, A, Qt::AnchorRight, B, Qt::AnchorLeft, 0);
    setAnchor(l, B, Qt::AnchorRight, l, Qt::AnchorRight, 0);

    setAnchor(l, A, Qt::AnchorLeft, a, Qt::AnchorLeft, 0);
    setAnchor(l, B, Qt::AnchorLeft, b, Qt::AnchorLeft, 0);

    // vertical
    setAnchor(l, l, Qt::AnchorTop, A, Qt::AnchorTop, 0);
    setAnchor(l, l, Qt::AnchorTop, B, Qt::AnchorTop, 0);
    setAnchor(l, A, Qt::AnchorBottom, a, Qt::AnchorTop, 0);
    setAnchor(l, B, Qt::AnchorBottom, b, Qt::AnchorTop, 0);
    setAnchor(l, a, Qt::AnchorBottom, l, Qt::AnchorBottom, 0);
    setAnchor(l, b, Qt::AnchorBottom, l, Qt::AnchorBottom, 0);

    QCOMPARE(l->count(), 4);

    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);
    p->setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QCOMPARE(layoutMinimumSize, QSizeF(20, 20));

    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(100, 20));

    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QCOMPARE(layoutMaximumSize, QSizeF(200, 20));

    delete p;
}

void tst_QGraphicsAnchorLayout::layoutDirection()
{
    QSizeF minSize(10, 10);
    QSizeF pref(50, 10);
    QSizeF maxSize(100, 10);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "a");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "b");
    QGraphicsWidget *c = createItem(minSize, pref, QSizeF(100, 20), "c");

    a->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    c->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);


    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 5, 10, 15);
    // horizontal
    setAnchor(l, l, Qt::AnchorLeft, a, Qt::AnchorLeft, 0);
    setAnchor(l, a, Qt::AnchorRight, b, Qt::AnchorLeft, 0);
    setAnchor(l, b, Qt::AnchorRight, l, Qt::AnchorRight, 0);
    setAnchor(l, a, Qt::AnchorHorizontalCenter, c, Qt::AnchorLeft, 0);
    setAnchor(l, c, Qt::AnchorRight, b, Qt::AnchorHorizontalCenter, 0);

    // vertical
    setAnchor(l, l, Qt::AnchorTop, a, Qt::AnchorTop, 0);
    setAnchor(l, l, Qt::AnchorTop, b, Qt::AnchorTop, 0);
    setAnchor(l, a, Qt::AnchorBottom, c, Qt::AnchorTop, 0);
    setAnchor(l, b, Qt::AnchorBottom, c, Qt::AnchorTop, 0);
    setAnchor(l, c, Qt::AnchorBottom, l, Qt::AnchorBottom, 0);

    QCOMPARE(l->count(), 3);

    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);
    p->setLayoutDirection(Qt::LeftToRight);
    p->setLayout(l);

    QGraphicsScene scene;
    QGraphicsView *view = new QGraphicsView(&scene);
    scene.addItem(p);
    p->show();
    view->show();

    QVERIFY(p->layout());
    QCOMPARE(checkReverseDirection(p), true);

    if (hasSimplification) {
        QVERIFY(usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }

    delete p;
    delete view;
}

void tst_QGraphicsAnchorLayout::diagonal()
{
    QSizeF minSize(10, 100);
    QSizeF pref(70, 100);
    QSizeF maxSize(100, 100);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "A");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "B");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "C");
    QGraphicsWidget *d = createItem(minSize, pref, maxSize, "D");
    QGraphicsWidget *e = createItem(minSize, pref, maxSize, "E");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    // vertical
    l->addAnchor(a, Qt::AnchorTop, l, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorTop, l, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorTop, a, Qt::AnchorBottom);
    l->addAnchor(c, Qt::AnchorTop, b, Qt::AnchorBottom);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, e, Qt::AnchorTop);
    l->addAnchor(d, Qt::AnchorBottom, l, Qt::AnchorBottom);
    l->addAnchor(e, Qt::AnchorBottom, l, Qt::AnchorBottom);

    // horizontal
    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorLeft, d, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);

    l->addAnchor(a, Qt::AnchorRight, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, e, Qt::AnchorLeft);

    l->addAnchor(b, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(e, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(d, Qt::AnchorRight, e, Qt::AnchorLeft);

    QCOMPARE(l->count(), 5);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, QSizeF(30.0, 300.0));
    QCOMPARE(layoutPreferredSize, QSizeF(170.0, 300.0));
    QCOMPARE(layoutMaximumSize, QSizeF(190.0, 300.0));

    p.resize(layoutMinimumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 10.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(10.0, 0.0, 20.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(10.0, 100.0, 10.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 200.0, 20.0, 100.0));
    QCOMPARE(e->geometry(), QRectF(20.0, 200.0, 10.0, 100.0));
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 70.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(70.0, 0.0, 100.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(70.0, 100.0, 30.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 200.0, 100.0, 100.0));
    QCOMPARE(e->geometry(), QRectF(100.0, 200.0, 70.0, 100.0));
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 90.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(90.0, 0.0, 100.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(90.0, 100.0, 10.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 200.0, 100.0, 100.0));
    QCOMPARE(e->geometry(), QRectF(100.0, 200.0, 90.0, 100.0));
    QCOMPARE(p.size(), layoutMaximumSize);

    QSizeF testA(175.0, 300.0);
    p.resize(testA);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 75.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(75.0, 0.0, 100.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(75.0, 100.0, 25.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 200.0, 100.0, 100.0));
    QCOMPARE(e->geometry(), QRectF(100.0, 200.0, 75.0, 100.0));
    QCOMPARE(p.size(), testA);

    if (hasSimplification) {
        QVERIFY(usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }

    QVERIFY(p.layout());
    QCOMPARE(checkReverseDirection(&p), true);

    c->setMinimumWidth(300);
    QVERIFY(layoutHasConflict(l));
}

void tst_QGraphicsAnchorLayout::parallel()
{
    QGraphicsWidget *a = createItem(QSizeF(100, 100),
                                    QSizeF(150, 100),
                                    QSizeF(200, 100), "A");

    QGraphicsWidget *b = createItem(QSizeF(100, 100),
                                    QSizeF(150, 100),
                                    QSizeF(300, 100), "B");

    QGraphicsWidget *c = createItem(QSizeF(100, 100),
                                    QSizeF(200, 100),
                                    QSizeF(350, 100), "C");

    QGraphicsWidget *d = createItem(QSizeF(100, 100),
                                    QSizeF(170, 100),
                                    QSizeF(200, 100), "D");

    QGraphicsWidget *e = createItem(QSizeF(150, 100),
                                    QSizeF(150, 100),
                                    QSizeF(200, 100), "E");

    QGraphicsWidget *f = createItem(QSizeF(100, 100),
                                    QSizeF(150, 100),
                                    QSizeF(200, 100), "F");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(d, Qt::AnchorBottom, e, Qt::AnchorTop);
    l->addAnchor(e, Qt::AnchorBottom, f, Qt::AnchorTop);
    l->addAnchor(f, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, c, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, d, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, e, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, f, Qt::AnchorLeft);
    l->addAnchor(d, Qt::AnchorRight, f, Qt::AnchorLeft);
    l->addAnchor(e, Qt::AnchorRight, f, Qt::AnchorLeft);
    l->addAnchor(f, Qt::AnchorRight, l, Qt::AnchorRight);

    QCOMPARE(l->count(), 6);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);

    QCOMPARE(layoutMinimumSize, QSizeF(450, 600));
    QCOMPARE(layoutPreferredSize, QSizeF(620, 600));
    QCOMPARE(layoutMaximumSize, QSizeF(750, 600));

    p.resize(layoutMinimumSize);
    QCOMPARE(a->geometry(), QRectF(0, 0, 100, 100));
    QCOMPARE(b->geometry(), QRectF(100, 100, 100, 100));
    QCOMPARE(c->geometry(), QRectF(100, 200, 250, 100));
    QCOMPARE(d->geometry(), QRectF(200, 300, 150, 100));
    QCOMPARE(e->geometry(), QRectF(200, 400, 150, 100));
    QCOMPARE(f->geometry(), QRectF(350, 500, 100, 100));
    QCOMPARE(p.size(), layoutMinimumSize);

    if (!hasSimplification)
        return;

    p.resize(layoutPreferredSize);
    QCOMPARE(a->geometry(), QRectF(0, 0, 150, 100));
    QCOMPARE(b->geometry(), QRectF(150, 100, 150, 100));
    QCOMPARE(c->geometry(), QRectF(150, 200, 320, 100));
    QCOMPARE(d->geometry(), QRectF(300, 300, 170, 100));
    QCOMPARE(e->geometry(), QRectF(300, 400, 170, 100));
    QCOMPARE(f->geometry(), QRectF(470, 500, 150, 100));
    QCOMPARE(p.size(), layoutPreferredSize);

    // Maximum size depends on simplification / fair distribution
    // Without that, test may or may not pass, depending on the
    // solution found by the solver at runtime.
    p.resize(layoutMaximumSize);
    QCOMPARE(a->geometry(), QRectF(0, 0, 200, 100));
    QCOMPARE(b->geometry(), QRectF(200, 100, 175, 100));
    QCOMPARE(c->geometry(), QRectF(200, 200, 350, 100));
    QCOMPARE(d->geometry(), QRectF(375, 300, 175, 100));
    QCOMPARE(e->geometry(), QRectF(375, 400, 175, 100));
    QCOMPARE(f->geometry(), QRectF(550, 500, 200, 100));
    QCOMPARE(p.size(), layoutMaximumSize);

    QVERIFY(!usedSimplex(l, Qt::Horizontal));
    QVERIFY(!usedSimplex(l, Qt::Vertical));
}

void tst_QGraphicsAnchorLayout::parallel2()
{
    QGraphicsWidget *a = createItem(QSizeF(70.0, 100.0),
                                    QSizeF(100.0, 100.0),
                                    QSizeF(200.0, 100.0), "A");

    QGraphicsWidget *b = createItem(QSizeF(100.0, 100.0),
                                    QSizeF(150.0, 100.0),
                                    QSizeF(190.0, 100.0), "B");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchors(l, a, Qt::Horizontal);
    l->addAnchor(l, Qt::AnchorLeft, b, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, a, Qt::AnchorRight);

    QCOMPARE(l->count(), 2);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);

    QCOMPARE(layoutMinimumSize, QSizeF(100.0, 200.0));
    QCOMPARE(layoutPreferredSize, QSizeF(150.0, 200.0));
    QCOMPARE(layoutMaximumSize, QSizeF(190.0, 200.0));

    p.resize(layoutMinimumSize);
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(p.size(), layoutMaximumSize);

    if (hasSimplification) {
        QVERIFY(!usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }
}

void tst_QGraphicsAnchorLayout::snake()
{
    QGraphicsWidget *a = createItem(QSizeF(50.0, 100.0),
                                    QSizeF(70.0, 100.0),
                                    QSizeF(100.0, 100.0), "A");

    QGraphicsWidget *b = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(20.0, 100.0),
                                    QSizeF(40.0, 100.0), "B");

    QGraphicsWidget *c = createItem(QSizeF(50.0, 100.0),
                                    QSizeF(70.0, 100.0),
                                    QSizeF(100.0, 100.0), "C");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorRight);
    l->addAnchor(b, Qt::AnchorLeft, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorRight);

    QCOMPARE(l->count(), 3);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, QSizeF(60.0, 300.0));
    QCOMPARE(layoutPreferredSize, QSizeF(120.0, 300.0));
    QCOMPARE(layoutMaximumSize, QSizeF(190.0, 300.0));

    p.resize(layoutMinimumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 50.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(10.0, 100.0, 40.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(10.0, 200.0, 50.0, 100.0));
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 70.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(50.0, 100.0, 20.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(50.0, 200.0, 70.0, 100.0));
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 100.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(90.0, 100.0, 10.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(90.0, 200.0, 100.0, 100.0));
    QCOMPARE(p.size(), layoutMaximumSize);

    QVERIFY(!layoutHasConflict(l));

    // Test QSizePolicy::ExpandFlag, it shouldn't change the extreme
    // points of the layout...
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QSizeF newLayoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF newLayoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF newLayoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, newLayoutMinimumSize);
    QCOMPARE(layoutMaximumSize, newLayoutMaximumSize);
    QCOMPARE(layoutPreferredSize, newLayoutPreferredSize);
}

void tst_QGraphicsAnchorLayout::snakeOppositeDirections()
{
    QGraphicsWidget *a = createItem(QSizeF(50.0, 100.0),
                                    QSizeF(70.0, 100.0),
                                    QSizeF(100.0, 100.0), "A");

    QGraphicsWidget *b = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(20.0, 100.0),
                                    QSizeF(40.0, 100.0), "B");

    QGraphicsWidget *c = createItem(QSizeF(50.0, 100.0),
                                    QSizeF(70.0, 100.0),
                                    QSizeF(100.0, 100.0), "C");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);

    // Both a and c are 'pointing' to b
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorRight);
    l->addAnchor(c, Qt::AnchorLeft, b, Qt::AnchorLeft);

    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorRight);

    QCOMPARE(l->count(), 3);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, QSizeF(60.0, 300.0));
    QCOMPARE(layoutPreferredSize, QSizeF(120.0, 300.0));
    QCOMPARE(layoutMaximumSize, QSizeF(190.0, 300.0));

    p.resize(layoutMinimumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 50.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(10.0, 100.0, 40.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(10.0, 200.0, 50.0, 100.0));
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 70.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(50.0, 100.0, 20.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(50.0, 200.0, 70.0, 100.0));
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 100.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(90.0, 100.0, 10.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(90.0, 200.0, 100.0, 100.0));
    QCOMPARE(p.size(), layoutMaximumSize);

    QVERIFY(p.layout());
    QCOMPARE(checkReverseDirection(&p), true);
}

void tst_QGraphicsAnchorLayout::fairDistribution()
{
    QGraphicsWidget *a = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "A");

    QGraphicsWidget *b = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "B");

    QGraphicsWidget *c = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "C");

    QGraphicsWidget *d = createItem(QSizeF(60.0, 100.0),
                                    QSizeF(165.0, 100.0),
                                    QSizeF(600.0, 100.0), "D");


    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(d, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(l, Qt::AnchorLeft, d, Qt::AnchorLeft);
    l->addAnchor(d, Qt::AnchorRight, l, Qt::AnchorRight);

    QCOMPARE(l->count(), 4);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, QSizeF(60.0, 400.0));
    QCOMPARE(layoutPreferredSize, QSizeF(165.0, 400.0));
    QCOMPARE(layoutMaximumSize, QSizeF(300.0, 400.0));

    p.resize(layoutMinimumSize);
    if (!hasSimplification)
        QEXPECT_FAIL("", "Without simplification there is no fair distribution.", Abort);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 20.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(20.0, 100.0, 20.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(40.0, 200.0, 20.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 300.0, 60.0, 100.0));
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 55.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(55.0, 100.0, 55.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(110.0, 200.0, 55.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 300.0, 165.0, 100.0));
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(a->geometry(), QRectF(0.0, 0.0, 100.0, 100.0));
    QCOMPARE(b->geometry(), QRectF(100.0, 100.0, 100.0, 100.0));
    QCOMPARE(c->geometry(), QRectF(200.0, 200.0, 100.0, 100.0));
    QCOMPARE(d->geometry(), QRectF(0.0, 300.0, 300.0, 100.0));
    QCOMPARE(p.size(), layoutMaximumSize);

    if (hasSimplification) {
        QVERIFY(!usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }
}

void tst_QGraphicsAnchorLayout::fairDistributionOppositeDirections()
{
    QGraphicsWidget *a = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "A");

    QGraphicsWidget *b = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "B");

    QGraphicsWidget *c = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "C");

    QGraphicsWidget *d = createItem(QSizeF(10.0, 100.0),
                                    QSizeF(50.0, 100.0),
                                    QSizeF(100.0, 100.0), "D");

    QGraphicsWidget *e = createItem(QSizeF(60.0, 100.0),
                                    QSizeF(220.0, 100.0),
                                    QSizeF(600.0, 100.0), "E");


    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(d, Qt::AnchorBottom, e, Qt::AnchorTop);
    l->addAnchor(e, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(a, Qt::AnchorLeft, l, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorLeft, a, Qt::AnchorRight);
    l->addAnchor(c, Qt::AnchorLeft, b, Qt::AnchorRight);
    l->addAnchor(d, Qt::AnchorLeft, c, Qt::AnchorRight);
    l->addAnchor(d, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchors(l, e, Qt::Horizontal);

    QCOMPARE(l->count(), 5);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, QSizeF(60.0, 500.0));
    QCOMPARE(layoutPreferredSize, QSizeF(220.0, 500.0));
    QCOMPARE(layoutMaximumSize, QSizeF(400.0, 500.0));

    if (!hasSimplification)
        return;

    p.resize(layoutMinimumSize);
    QCOMPARE(a->size(), b->size());
    QCOMPARE(a->size(), c->size());
    QCOMPARE(a->size(), d->size());
    QCOMPARE(e->size().width(), 4 * a->size().width());
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(a->size(), b->size());
    QCOMPARE(a->size(), c->size());
    QCOMPARE(a->size(), d->size());
    QCOMPARE(e->size().width(), 4 * a->size().width());
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(a->size(), b->size());
    QCOMPARE(a->size(), c->size());
    QCOMPARE(a->size(), d->size());
    QCOMPARE(e->size().width(), 4 * a->size().width());
    QCOMPARE(p.size(), layoutMaximumSize);

    QVERIFY(!usedSimplex(l, Qt::Horizontal));
    QVERIFY(!usedSimplex(l, Qt::Vertical));
}

void tst_QGraphicsAnchorLayout::proportionalPreferred()
{
    QSizeF minSize(0, 100);

    QGraphicsWidget *a = createItem(minSize, QSizeF(10, 100), QSizeF(20, 100), "A");
    QGraphicsWidget *b = createItem(minSize, QSizeF(20, 100), QSizeF(30, 100), "B");
    QGraphicsWidget *c = createItem(minSize, QSizeF(14, 100), QSizeF(20, 100), "C");
    QGraphicsWidget *d = createItem(minSize, QSizeF(10, 100), QSizeF(20, 100), "D");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(d, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorLeft, b, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, c, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, d, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(d, Qt::AnchorRight, l, Qt::AnchorRight);

    QCOMPARE(l->count(), 4);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);

    QCOMPARE(layoutMinimumSize, QSizeF(0, 400));
    QCOMPARE(layoutPreferredSize, QSizeF(24, 400));
    QCOMPARE(layoutMaximumSize, QSizeF(30, 400));

    p.resize(layoutMinimumSize);
    QCOMPARE(p.size(), layoutMinimumSize);

    p.resize(layoutPreferredSize);
    QCOMPARE(c->size().width(), d->size().width());
    QCOMPARE(p.size(), layoutPreferredSize);

    p.resize(layoutMaximumSize);
    QCOMPARE(p.size(), layoutMaximumSize);

    p.resize(QSizeF(12, 400));

    // Proportionality between size given and preferred size, this
    // should be respected in this graph for (a) and (b)|(c).
    qreal factor = 12.0 / 24.0;

    QCOMPARE(c->size().width(), d->size().width());
    QCOMPARE(a->size().width(), 10 * factor);
    QCOMPARE(c->size().width(), 14 * factor);
    QCOMPARE(p.size(), QSizeF(12, 400));

    if (hasSimplification) {
        QVERIFY(!usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }
}

void tst_QGraphicsAnchorLayout::example()
{
    QSizeF minSize(30, 100);
    QSizeF pref(210, 100);
    QSizeF maxSize(300, 100);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "A");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "B");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "C");
    QGraphicsWidget *d = createItem(minSize, pref, maxSize, "D");
    QGraphicsWidget *e = createItem(minSize, pref, maxSize, "E");
    QGraphicsWidget *f = createItem(QSizeF(30, 50), QSizeF(150, 50), maxSize, "F");
    QGraphicsWidget *g = createItem(QSizeF(30, 50), QSizeF(30, 100), maxSize, "G");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    // vertical
    l->addAnchor(a, Qt::AnchorTop, l, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorTop, l, Qt::AnchorTop);

    l->addAnchor(c, Qt::AnchorTop, a, Qt::AnchorBottom);
    l->addAnchor(c, Qt::AnchorTop, b, Qt::AnchorBottom);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, e, Qt::AnchorTop);

    l->addAnchor(d, Qt::AnchorBottom, l, Qt::AnchorBottom);
    l->addAnchor(e, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(c, Qt::AnchorTop, f, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorVerticalCenter, f, Qt::AnchorBottom);
    l->addAnchor(f, Qt::AnchorBottom, g, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, g, Qt::AnchorBottom);

    // horizontal
    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorLeft, d, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);

    l->addAnchor(a, Qt::AnchorRight, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, e, Qt::AnchorLeft);

    l->addAnchor(b, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(e, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(d, Qt::AnchorRight, e, Qt::AnchorLeft);

    l->addAnchor(l, Qt::AnchorLeft, f, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorLeft, g, Qt::AnchorLeft);
    l->addAnchor(f, Qt::AnchorRight, g, Qt::AnchorRight);

    QCOMPARE(l->count(), 7);

    QGraphicsWidget p;
    p.setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(layoutMinimumSize, QSizeF(90.0, 300.0));
    QCOMPARE(layoutPreferredSize, QSizeF(510.0, 300.0));
    QCOMPARE(layoutMaximumSize, QSizeF(570.0, 300.0));

    p.resize(layoutMinimumSize);
    QCOMPARE(p.size(), layoutMinimumSize);
    QCOMPARE(a->size(), e->size());
    QCOMPARE(b->size(), d->size());
    QCOMPARE(f->size(), g->size());

    p.resize(layoutPreferredSize);
    QCOMPARE(p.size(), layoutPreferredSize);
    QCOMPARE(a->size(), e->size());
    QCOMPARE(b->size(), d->size());
    QCOMPARE(f->size(), g->size());

    p.resize(layoutMaximumSize);
    QCOMPARE(p.size(), layoutMaximumSize);
    QCOMPARE(a->size(), e->size());
    QCOMPARE(b->size(), d->size());
    QCOMPARE(f->size(), g->size());

    if (hasSimplification) {
        QVERIFY(usedSimplex(l, Qt::Horizontal));
        QVERIFY(usedSimplex(l, Qt::Vertical));
    }
}

void tst_QGraphicsAnchorLayout::setSpacing()
{
    QSizeF minSize(10, 10);
    QSizeF pref(20, 20);
    QSizeF maxSize(50, 50);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize);
    QGraphicsWidget *b = createItem(minSize, pref, maxSize);
    QGraphicsWidget *c = createItem(minSize, pref, maxSize);

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->addCornerAnchors(l, Qt::TopLeftCorner, a, Qt::TopLeftCorner);
    l->addCornerAnchors(b, Qt::TopRightCorner, l, Qt::TopRightCorner);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);

    l->addAnchors(l, c, Qt::Horizontal);

    l->addAnchor(a, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, l, Qt::AnchorBottom);

    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);

    p->setLayout(l);
    l->setSpacing(1);

    // don't let the style influence the test.
    l->setContentsMargins(0, 0, 0, 0);

    QGraphicsScene scene;
    scene.addItem(p);
    QGraphicsView *view = new QGraphicsView(&scene);
    view->show();
    p->show();

    QApplication::processEvents();
#ifdef Q_OS_MAC
    QTest::qWait(200);
#endif

    // 21x21
    QCOMPARE(p->size(), QSizeF(41, 41));
    QCOMPARE(a->geometry(), QRectF(0, 0, 20, 20));
    QCOMPARE(b->geometry(), QRectF(21, 0, 20, 20));
    QCOMPARE(c->geometry(), QRectF(0, 21, 41, 20));

    l->setHorizontalSpacing(4);
    QApplication::processEvents();
    p->adjustSize();
    QCOMPARE(a->geometry(), QRectF(0, 0, 20, 20));
    QCOMPARE(b->geometry(), QRectF(24, 0, 20, 20));
    QCOMPARE(c->geometry(), QRectF(0, 21, 44, 20));

    l->setVerticalSpacing(0);
    QApplication::processEvents();
    p->adjustSize();
    QCOMPARE(a->geometry(), QRectF(0, 0, 20, 20));
    QCOMPARE(b->geometry(), QRectF(24, 0, 20, 20));
    QCOMPARE(c->geometry(), QRectF(0, 20, 44, 20));

    delete p;
    delete view;
}

class CustomLayoutStyle : public QProxyStyle
{
    Q_OBJECT
public:
    CustomLayoutStyle() : QProxyStyle(QStyleFactory::create("windows"))
    {
        hspacing = 5;
        vspacing = 10;
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0,
                            const QWidget * widget = 0 ) const;

    int hspacing;
    int vspacing;

    int layoutSpacing(QSizePolicy::ControlType control1,
                      QSizePolicy::ControlType control2,
                      Qt::Orientation orientation,
                      const QStyleOption *option = 0,
                      const QWidget *widget = 0) const;

};

#define CT1(c) CT2(c, c)
#define CT2(c1, c2) ((uint)c1 << 16) | (uint)c2

int CustomLayoutStyle::layoutSpacing(QSizePolicy::ControlType control1,
                                QSizePolicy::ControlType control2,
                                Qt::Orientation orientation,
                                const QStyleOption * /*option = 0*/,
                                const QWidget * /*widget = 0*/) const
{
    if (orientation == Qt::Horizontal) {
        switch (CT2(control1, control2)) {
            case CT1(QSizePolicy::PushButton):
                return 2;
                break;
        }
        return 5;
    } else {
        switch (CT2(control1, control2)) {
            case CT1(QSizePolicy::RadioButton):
                return 2;
                break;

        }
        return 10;
    }
}

int CustomLayoutStyle::pixelMetric(PixelMetric metric, const QStyleOption * option /*= 0*/,
                                   const QWidget * widget /*= 0*/ ) const
{
    switch (metric) {
        case PM_LayoutLeftMargin:
            return 0;
        break;
        case PM_LayoutTopMargin:
            return 3;
        break;
        case PM_LayoutRightMargin:
            return 6;
        break;
        case PM_LayoutBottomMargin:
            return 9;
        break;
        case PM_LayoutHorizontalSpacing:
            return hspacing;
        case PM_LayoutVerticalSpacing:
            return vspacing;
        break;
        default:
            break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void tst_QGraphicsAnchorLayout::styleDefaults()
{
    QSizeF minSize (10, 10);
    QSizeF pref(20, 20);
    QSizeF maxSize (50, 50);

    /*
    create this layout, where a,b have controlType QSizePolicy::RadioButton
    c,d have controlType QSizePolicy::PushButton:
    +-------+
    |a      |
    |  b    |
    |    c  |
    |      d|
    +-------+
    */
    QGraphicsScene scene;
    QGraphicsWidget *a = createItem(minSize, pref, maxSize);
    QSizePolicy spRadioButton = a->sizePolicy();
    spRadioButton.setControlType(QSizePolicy::RadioButton);
    a->setSizePolicy(spRadioButton);

    QGraphicsWidget *b = createItem(minSize, pref, maxSize);
    b->setSizePolicy(spRadioButton);

    QGraphicsWidget *c = createItem(minSize, pref, maxSize);
    QSizePolicy spPushButton = c->sizePolicy();
    spPushButton.setControlType(QSizePolicy::PushButton);
    c->setSizePolicy(spPushButton);

    QGraphicsWidget *d = createItem(minSize, pref, maxSize);
    d->setSizePolicy(spPushButton);

    QGraphicsWidget *window = new QGraphicsWidget(0, Qt::Window);

    // Test layoutSpacing
    CustomLayoutStyle *style = new CustomLayoutStyle;
    style->hspacing = -1;
    style->vspacing = -1;
    window->setStyle(style);
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;

    l->addCornerAnchors(l, Qt::TopLeftCorner, a, Qt::TopLeftCorner);
    l->addCornerAnchors(a, Qt::BottomRightCorner, b, Qt::TopLeftCorner);
    l->addCornerAnchors(b, Qt::BottomRightCorner, c, Qt::TopLeftCorner);
    l->addCornerAnchors(c, Qt::BottomRightCorner, d, Qt::TopLeftCorner);
    l->addCornerAnchors(d, Qt::BottomRightCorner, l, Qt::BottomRightCorner);

    window->setLayout(l);

    scene.addItem(window);

    window->show();
    QGraphicsView view(&scene);
    view.resize(200, 200);
    view.show();

    window->adjustSize();
    QCOMPARE(a->geometry(), QRectF(0,   3, 20, 20));    //radio
    QCOMPARE(b->geometry(), QRectF(25, 25, 20, 20));    //radio
    QCOMPARE(c->geometry(), QRectF(50, 55, 20, 20));    //push
    QCOMPARE(d->geometry(), QRectF(72, 85, 20, 20));    //push
    QCOMPARE(l->geometry(), QRectF(0,   0, 98, 114));


    // Test pixelMetric(PM_Layout{Horizontal|Vertical}Spacing
    window->setStyle(0);

    style->hspacing = 1;
    style->vspacing = 2;

    window->setStyle(style);
    window->adjustSize();
    QCOMPARE(a->geometry(), QRectF(0,   3, 20, 20));
    QCOMPARE(b->geometry(), QRectF(21, 25, 20, 20));
    QCOMPARE(c->geometry(), QRectF(42, 47, 20, 20));
    QCOMPARE(d->geometry(), QRectF(63, 69, 20, 20));
    QCOMPARE(l->geometry(), QRectF(0,   0, 89, 98));

    window->setStyle(0);
    delete style;
}


/*!
    Taken from "hard" complex case, found at
    https://cwiki.nokia.com/S60QTUI/AnchorLayoutComplexCases

    This layout has a special property, since it has two possible solutions for its minimum size.

    For instance, when it is in its minimum size - the layout have two possible solutions:
    1. c.width == 10, e.width == 10 and g.width == 10
       (all others have width 0)
    2. d.width == 10 and g.width == 10
       (all others have width 0)

    It also has several solutions for preferred size.
*/
static QGraphicsAnchorLayout *createAmbiguousS60Layout()
{
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    QSizeF minSize(0, 10);
    QSizeF pref(50, 10);
    QSizeF maxSize(100, 10);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "a");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "b");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "c");
    QGraphicsWidget *d = createItem(minSize, pref, maxSize, "d");
    QGraphicsWidget *e = createItem(minSize, pref, maxSize, "e");
    QGraphicsWidget *f = createItem(minSize, pref, maxSize, "f");
    QGraphicsWidget *g = createItem(minSize, pref, maxSize, "g");

    //<!-- Trunk -->
    setAnchor(l, l, Qt::AnchorLeft, a, Qt::AnchorLeft, 10);
    setAnchor(l, a, Qt::AnchorRight, b, Qt::AnchorLeft, 10);
    setAnchor(l, b, Qt::AnchorRight, c, Qt::AnchorLeft, 10);
    setAnchor(l, c, Qt::AnchorRight, d, Qt::AnchorLeft, 10);
    setAnchor(l, d, Qt::AnchorRight, l, Qt::AnchorRight, 10);

    //<!-- Above trunk -->
    setAnchor(l, b, Qt::AnchorLeft, e, Qt::AnchorLeft, 10);
    setAnchor(l, e, Qt::AnchorRight, d, Qt::AnchorLeft, 10);

    //<!-- Below trunk -->
    setAnchor(l, a, Qt::AnchorHorizontalCenter, g, Qt::AnchorLeft, 10);
    setAnchor(l, g, Qt::AnchorRight, f, Qt::AnchorHorizontalCenter, 10);
    setAnchor(l, c, Qt::AnchorLeft, f, Qt::AnchorLeft, 10);
    setAnchor(l, f, Qt::AnchorRight, d, Qt::AnchorRight, 10);

    //<!-- vertical is simpler -->
    setAnchor(l, l, Qt::AnchorTop, e, Qt::AnchorTop, 0);
    setAnchor(l, e, Qt::AnchorBottom, a, Qt::AnchorTop, 0);
    setAnchor(l, e, Qt::AnchorBottom, b, Qt::AnchorTop, 0);
    setAnchor(l, e, Qt::AnchorBottom, c, Qt::AnchorTop, 0);
    setAnchor(l, e, Qt::AnchorBottom, d, Qt::AnchorTop, 0);
    setAnchor(l, a, Qt::AnchorBottom, f, Qt::AnchorTop, 0);
    setAnchor(l, a, Qt::AnchorBottom, b, Qt::AnchorBottom, 0);
    setAnchor(l, a, Qt::AnchorBottom, c, Qt::AnchorBottom, 0);
    setAnchor(l, a, Qt::AnchorBottom, d, Qt::AnchorBottom, 0);
    setAnchor(l, f, Qt::AnchorBottom, g, Qt::AnchorTop, 0);
    setAnchor(l, g, Qt::AnchorBottom, l, Qt::AnchorBottom, 0);
    return l;
}

void tst_QGraphicsAnchorLayout::hardComplexS60()
{
    QGraphicsAnchorLayout *l = createAmbiguousS60Layout();
    QCOMPARE(l->count(), 7);

    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);
    p->setLayout(l);

    QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
    QCOMPARE(layoutMinimumSize, QSizeF(60, 40));
    // expected preferred might be wrong, (haven't manually verified it)
    QSizeF layoutPreferredSize = l->effectiveSizeHint(Qt::PreferredSize);
    QCOMPARE(layoutPreferredSize, QSizeF(220, 40));
    QSizeF layoutMaximumSize = l->effectiveSizeHint(Qt::MaximumSize);
    QCOMPARE(layoutMaximumSize, QSizeF(240, 40));

    delete p;
}

static inline QByteArray msgStability(const QRectF &actual, const QRectF &expected, int pass, int item)
{
    QString result;
    QDebug(&result)
        << "The layout has several solutions, but which solution it picks is not stable ("
        << actual << "!=" << expected << ", iteration" << pass << ", item" << item << ')';
    return result.toLocal8Bit();
}

void tst_QGraphicsAnchorLayout::stability()
{
    QVector<QRectF> geometries;
    geometries.resize(7);
    QGraphicsWidget p(0, Qt::Window);
    // it usually fails after 3-4 iterations
    for (int pass = 0; pass < 20; ++pass) {
        // In case we need to "scramble" the heap allocator to provoke this bug.
        //static const int primes[] = {2, 3, 5, 13, 89, 233, 1597, 28657, 514229}; // fibo primes
        //const int primeCount = sizeof(primes)/sizeof(int);
        //int alloc = primes[pass % primeCount] + pass;
        //void *mem = malloc(alloc);
        //free(mem);
        QGraphicsAnchorLayout *l = createAmbiguousS60Layout();
        p.setLayout(l);
        QSizeF layoutMinimumSize = l->effectiveSizeHint(Qt::MinimumSize);
        l->setGeometry(QRectF(QPointF(0,0), layoutMinimumSize));
        QApplication::processEvents();
        for (int i = l->count() - 1; i >=0; --i) {
            const QRectF actualGeom = l->itemAt(i)->geometry();
            if (pass != 0) {
                if (actualGeom != geometries[i])
                    QEXPECT_FAIL("", msgStability(actualGeom, geometries[i], pass, i).constData(), Abort);
                QCOMPARE(actualGeom, geometries[i]);
            }
            geometries[i] = actualGeom;
        }
        p.setLayout(0);    // uninstalls and deletes the layout
        QApplication::processEvents();
    }
}

void tst_QGraphicsAnchorLayout::delete_anchor()
{
    QGraphicsScene scene;
    QSizeF minSize(0, 0);
    QSizeF prefSize(50, 50);
    QSizeF maxSize(100, 100);
    QGraphicsWidget *w1 = createItem(minSize, prefSize, maxSize, "w1");
    QGraphicsWidget *w2 = createItem(minSize, prefSize, maxSize, "w2");
    QGraphicsWidget *w3 = createItem(minSize, prefSize, maxSize, "w3");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setSpacing(0);
    l->setContentsMargins(0, 0, 0, 0);

    // Horizontal
    l->addAnchor(l, Qt::AnchorLeft, w1, Qt::AnchorLeft);
    l->addAnchor(w1, Qt::AnchorRight, w2, Qt::AnchorLeft);
    l->addAnchor(w2, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(w1, Qt::AnchorRight, w3, Qt::AnchorLeft);
    l->addAnchor(w3, Qt::AnchorRight, l, Qt::AnchorRight);

    // Vertical
    l->addAnchors(l, w1, Qt::Vertical);
    l->addAnchors(l, w2, Qt::Vertical);
    l->addAnchors(l, w3, Qt::Vertical);

    QGraphicsAnchor *anchor = l->anchor(w3, Qt::AnchorRight, l, Qt::AnchorRight);
    anchor->setSpacing(10);

    QGraphicsWidget *p = new QGraphicsWidget;
    p->setLayout(l);

    QCOMPARE(l->count(), 3);

    scene.addItem(p);
    QGraphicsView *view = new QGraphicsView(&scene);
    QApplication::processEvents();
    // Should now be simplified
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize).width(), qreal(110));
    QGraphicsAnchor *anchor1 = l->anchor(w3, Qt::AnchorRight, l, Qt::AnchorRight);
    QVERIFY(anchor1);
    QGraphicsAnchor *anchor2 = l->anchor(w3, Qt::AnchorRight, l, Qt::AnchorRight);
    QVERIFY(anchor2);
    QGraphicsAnchor *anchor3 = l->anchor(l, Qt::AnchorRight, w3, Qt::AnchorRight);
    QVERIFY(anchor3);
    QGraphicsAnchor *anchor4 = l->anchor(l, Qt::AnchorRight, w3, Qt::AnchorRight);
    QVERIFY(anchor4);

    // should all be the same object
    QCOMPARE(anchor1, anchor2);
    QCOMPARE(anchor2, anchor3);
    QCOMPARE(anchor3, anchor4);

    // check if removal works
    delete anchor1;

    QApplication::processEvents();

    // it should also change the preferred size of the layout
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize).width(), qreal(100));

    delete p;
    delete view;
}

void tst_QGraphicsAnchorLayout::sizePolicy()
{
    QGraphicsScene scene;
    QSizeF minSize(0, 0);
    QSizeF prefSize(50, 50);
    QSizeF maxSize(100, 100);
    QGraphicsWidget *w1 = createItem(minSize, prefSize, maxSize, "w1");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setSpacing(0);
    l->setContentsMargins(0, 0, 0, 0);

    // horizontal and vertical
    l->addAnchors(l, w1);

    QGraphicsWidget *p = new QGraphicsWidget;
    p->setLayout(l);

    scene.addItem(p);
    QGraphicsView *view = new QGraphicsView(&scene);

    // QSizePolicy::Minimum
    w1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QApplication::processEvents();
    w1->adjustSize();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(100, 100));

    // QSizePolicy::Maximum
    w1->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    QApplication::processEvents();
    w1->adjustSize();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(0, 0));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(50, 50));

    // QSizePolicy::Fixed
    w1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QApplication::processEvents();
    w1->adjustSize();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(50, 50));

    // QSizePolicy::Preferred
    w1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QApplication::processEvents();
    w1->adjustSize();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(0, 0));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(100, 100));

    // QSizePolicy::Ignored
    w1->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QApplication::processEvents();
    w1->adjustSize();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(0, 0));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(0, 0));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(100, 100));

    // Anchor size policies
    w1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QGraphicsAnchor *anchor = l->anchor(l, Qt::AnchorLeft, w1, Qt::AnchorLeft);
    anchor->setSpacing(10);

    // QSizePolicy::Minimum
    anchor->setSizePolicy(QSizePolicy::Minimum);
    QApplication::processEvents();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(60, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(60, 50));
    // The layout has a maximum size of QWIDGETSIZE_MAX, so the result won't exceed that value.
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(QWIDGETSIZE_MAX, 50));

    // QSizePolicy::Preferred
    anchor->setSizePolicy(QSizePolicy::Preferred);
    QApplication::processEvents();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(60, 50));
    // The layout has a maximum size of QWIDGETSIZE_MAX, so the result won't exceed that value.
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(QWIDGETSIZE_MAX, 50));

    // QSizePolicy::Maximum
    anchor->setSizePolicy(QSizePolicy::Maximum);
    QApplication::processEvents();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(60, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(60, 50));

    // QSizePolicy::Ignored
    anchor->setSizePolicy(QSizePolicy::Ignored);
    QApplication::processEvents();

    QCOMPARE(l->effectiveSizeHint(Qt::MinimumSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::PreferredSize), QSizeF(50, 50));
    QCOMPARE(l->effectiveSizeHint(Qt::MaximumSize), QSizeF(QWIDGETSIZE_MAX, 50));

    if (hasSimplification) {
        QVERIFY(!usedSimplex(l, Qt::Horizontal));
        QVERIFY(!usedSimplex(l, Qt::Vertical));
    }

    delete p;
    delete view;
}

/*!
    \internal

    Uses private API. (We have decided to pull hasConflicts() out of the API). However, it also
    tests some tight conditions (almost-in-conflict) that we really want to test.
*/
void tst_QGraphicsAnchorLayout::conflicts()
{
    QGraphicsWidget *a = createItem(QSizeF(80,10), QSizeF(90,10), QSizeF(100,10), "a");
    QGraphicsWidget *b = createItem(QSizeF(10,10), QSizeF(20,10), QSizeF(30,10), "b");
    QGraphicsWidget *c = createItem(QSizeF(10,10), QSizeF(20,10), QSizeF(30,10), "c");

    QGraphicsAnchorLayout *l;
    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);

    l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);

    // with the following setup, 'a' cannot be larger than 30 we will first have a Simplex conflict

    // horizontal
    setAnchor(l, l, Qt::AnchorLeft, b, Qt::AnchorLeft);
    setAnchor(l, b, Qt::AnchorRight, c, Qt::AnchorLeft);
    setAnchor(l, c, Qt::AnchorRight, l, Qt::AnchorRight);
    setAnchor(l, b, Qt::AnchorHorizontalCenter, a, Qt::AnchorLeft);
    setAnchor(l, a, Qt::AnchorRight, c, Qt::AnchorHorizontalCenter);

    // vertical
    setAnchor(l, l, Qt::AnchorTop, a, Qt::AnchorTop);
    setAnchor(l, a, Qt::AnchorBottom, b, Qt::AnchorTop);
    setAnchor(l, a, Qt::AnchorBottom, c, Qt::AnchorTop);
    setAnchor(l, b, Qt::AnchorBottom, l, Qt::AnchorBottom);
    setAnchor(l, c, Qt::AnchorBottom, l, Qt::AnchorBottom);

    p->setLayout(l);

    QCOMPARE(layoutHasConflict(l), true);

    a->setMinimumSize(QSizeF(29,10));
    QCOMPARE(layoutHasConflict(l), false);

    a->setMinimumSize(QSizeF(30,10));
    QCOMPARE(layoutHasConflict(l), false);

    delete p;
}

void tst_QGraphicsAnchorLayout::floatConflict()
{
    QGraphicsWidget *a = createItem(QSizeF(80,10), QSizeF(90,10), QSizeF(100,10), "a");
    QGraphicsWidget *b = createItem(QSizeF(80,10), QSizeF(90,10), QSizeF(100,10), "b");

    QGraphicsAnchorLayout *l;
    QGraphicsWidget *p = new QGraphicsWidget(0, Qt::Window);

    l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);

    p->setLayout(l);

    // horizontal
    // with this anchor we have two floating items
    setAnchor(l, a, Qt::AnchorRight, b, Qt::AnchorLeft);

    // Just checking if the layout is handling well the removal of floating items
    delete l->anchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);
    QCOMPARE(l->count(), 0);
    QCOMPARE(layoutHasConflict(l), false);

    // setting back the same anchor
    setAnchor(l, a, Qt::AnchorRight, b, Qt::AnchorLeft);

    // We don't support floating items but they should be counted as if they are in the layout
    QCOMPARE(l->count(), 2);
    // Although, we have an invalid situation
    QCOMPARE(layoutHasConflict(l), true);

    // Semi-floats are supported
    setAnchor(l, a, Qt::AnchorLeft, l, Qt::AnchorLeft);
    QCOMPARE(l->count(), 2);

    // Vertically the layout has floating items. Therefore, we have a conflict
    QCOMPARE(layoutHasConflict(l), true);

    // No more floating items
    setAnchor(l, b, Qt::AnchorRight, l, Qt::AnchorRight);
    setAnchor(l, a, Qt::AnchorTop, l, Qt::AnchorTop);
    setAnchor(l, a, Qt::AnchorBottom, l, Qt::AnchorBottom);
    setAnchor(l, b, Qt::AnchorTop, l, Qt::AnchorTop);
    setAnchor(l, b, Qt::AnchorBottom, l, Qt::AnchorBottom);
    QCOMPARE(layoutHasConflict(l), false);

    delete p;
}

void tst_QGraphicsAnchorLayout::infiniteMaxSizes()
{
    if (sizeof(qreal) <= 4)
        QSKIP("qreal has too little precision, result will be wrong");
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    QSizeF minSize(10, 10);
    QSizeF pref(50, 10);
    QSizeF maxSize(QWIDGETSIZE_MAX, 10);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "a");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "b");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "c");
    QGraphicsWidget *d = createItem(minSize, pref, maxSize, "d");
    QGraphicsWidget *e = createItem(minSize, pref, maxSize, "e");

    //<!-- Trunk -->
    setAnchor(l, l, Qt::AnchorLeft, a, Qt::AnchorLeft, 0);
    setAnchor(l, a, Qt::AnchorRight, b, Qt::AnchorLeft, 0);
    setAnchor(l, b, Qt::AnchorRight, c, Qt::AnchorLeft, 0);
    setAnchor(l, c, Qt::AnchorRight, d, Qt::AnchorLeft, 0);
    setAnchor(l, d, Qt::AnchorRight, l, Qt::AnchorRight, 0);
    setAnchor(l, b, Qt::AnchorHorizontalCenter, e, Qt::AnchorLeft, 0);
    setAnchor(l, e, Qt::AnchorRight, c, Qt::AnchorHorizontalCenter, 0);

    QGraphicsWidget p;
    p.setLayout(l);

    QCOMPARE(int(p.effectiveSizeHint(Qt::MaximumSize).width()),
             QWIDGETSIZE_MAX);

    p.resize(200, 10);
    QCOMPARE(a->geometry(), QRectF(0, 0, 50, 10));
    QCOMPARE(b->geometry(), QRectF(50, 0, 50, 10));
    QCOMPARE(c->geometry(), QRectF(100, 0, 50, 10));
    QCOMPARE(d->geometry(), QRectF(150, 0, 50, 10));

    p.resize(1000, 10);
    QCOMPARE(a->geometry(), QRectF(0, 0, 250, 10));
    QCOMPARE(b->geometry(), QRectF(250, 0, 250, 10));
    QCOMPARE(c->geometry(), QRectF(500, 0, 250, 10));
    QCOMPARE(d->geometry(), QRectF(750, 0, 250, 10));

    p.resize(40000, 10);
    QCOMPARE(a->geometry(), QRectF(0, 0, 10000, 10));
    QCOMPARE(b->geometry(), QRectF(10000, 0, 10000, 10));
    QCOMPARE(c->geometry(), QRectF(20000, 0, 10000, 10));
    QCOMPARE(d->geometry(), QRectF(30000, 0, 10000, 10));
}

void tst_QGraphicsAnchorLayout::simplifiableUnfeasible()
{
    QGraphicsWidget *a = createItem(QSizeF(70.0, 100.0),
                                    QSizeF(100.0, 100.0),
                                    QSizeF(100.0, 100.0), "A");

    QGraphicsWidget *b = createItem(QSizeF(110.0, 100.0),
                                    QSizeF(150.0, 100.0),
                                    QSizeF(190.0, 100.0), "B");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchors(l, a, Qt::Horizontal);
    l->addAnchor(l, Qt::AnchorLeft, b, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, a, Qt::AnchorRight);

    QCOMPARE(l->count(), 2);

    QGraphicsWidget p;
    p.setLayout(l);

    l->invalidate();
    QVERIFY(layoutHasConflict(l));
    if (hasSimplification)
        QVERIFY(!usedSimplex(l, Qt::Horizontal));

    // Now we make it valid
    b->setMinimumWidth(100);

    l->invalidate();
    QVERIFY(!layoutHasConflict(l));
    if (hasSimplification)
        QVERIFY(!usedSimplex(l, Qt::Horizontal));

    // And make it invalid again
    a->setPreferredWidth(70);
    a->setMaximumWidth(70);

    l->invalidate();
    QVERIFY(layoutHasConflict(l));
    if (hasSimplification)
        QVERIFY(!usedSimplex(l, Qt::Horizontal));
}

/*
  Test whether the anchor direction can prevent it from
  being simplificated
*/
void tst_QGraphicsAnchorLayout::simplificationVsOrder()
{
    QSizeF minSize(10, 10);
    QSizeF pref(20, 10);
    QSizeF maxSize(50, 10);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "A");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "B");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "C");

    QGraphicsWidget frame;
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&frame);

    // Bulk anchors
    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorLeft, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorRight);

    // Problematic anchor, direction b->c
    QGraphicsAnchor *anchor = l->addAnchor(b, Qt::AnchorRight, c, Qt::AnchorRight);
    anchor->setSpacing(5);

    l->effectiveSizeHint(Qt::MinimumSize);
    if (hasSimplification) {
        QCOMPARE(usedSimplex(l, Qt::Horizontal), false);
        QCOMPARE(usedSimplex(l, Qt::Vertical), false);
    }

    // Problematic anchor, direction c->b
    delete anchor;
    anchor = l->addAnchor(c, Qt::AnchorRight, b, Qt::AnchorRight);
    anchor->setSpacing(5);

    l->effectiveSizeHint(Qt::MinimumSize);
    if (hasSimplification) {
        QCOMPARE(usedSimplex(l, Qt::Horizontal), false);
        QCOMPARE(usedSimplex(l, Qt::Vertical), false);
    }
}

void tst_QGraphicsAnchorLayout::parallelSimplificationOfCenter()
{
    QSizeF minSize(10, 10);
    QSizeF pref(20, 10);
    QSizeF maxSize(50, 10);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "A");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "B");

    QGraphicsWidget parent;
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&parent);
    l->setContentsMargins(0, 0, 0, 0);

    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorRight, a, Qt::AnchorRight);

    l->addAnchor(a, Qt::AnchorHorizontalCenter, b, Qt::AnchorLeft);
    l->addAnchor(b, Qt::AnchorRight, a, Qt::AnchorRight);

    parent.resize(l->effectiveSizeHint(Qt::PreferredSize));

    QCOMPARE(a->geometry(), QRectF(0, 0, 40, 10));
    QCOMPARE(b->geometry(), QRectF(20, 0, 20, 10));
}

/*
    Test whether redundance of anchors (in this case by using addCornerAnchors), will
    prevent simplification to take place when it should.
*/
void tst_QGraphicsAnchorLayout::simplificationVsRedundance()
{
    QSizeF minSize(10, 10);
    QSizeF pref(20, 10);
    QSizeF maxSize(50, 30);

    QGraphicsWidget *a = createItem(minSize, pref, maxSize, "A");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "B");
    QGraphicsWidget *c = createItem(minSize, pref, maxSize, "C");

    QGraphicsWidget frame;
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&frame);

    l->addCornerAnchors(a, Qt::TopLeftCorner, l, Qt::TopLeftCorner);
    l->addCornerAnchors(a, Qt::BottomLeftCorner, l, Qt::BottomLeftCorner);

    l->addCornerAnchors(b, Qt::TopLeftCorner, a, Qt::TopRightCorner);
    l->addCornerAnchors(b, Qt::TopRightCorner, l, Qt::TopRightCorner);

    l->addCornerAnchors(c, Qt::TopLeftCorner, b, Qt::BottomLeftCorner);
    l->addCornerAnchors(c, Qt::BottomLeftCorner, a, Qt::BottomRightCorner);
    l->addCornerAnchors(c, Qt::TopRightCorner, b, Qt::BottomRightCorner);
    l->addCornerAnchors(c, Qt::BottomRightCorner, l, Qt::BottomRightCorner);

    l->effectiveSizeHint(Qt::MinimumSize);

    QCOMPARE(layoutHasConflict(l), false);

    if (!hasSimplification)
        QEXPECT_FAIL("", "Test depends on simplification.", Abort);

    QCOMPARE(usedSimplex(l, Qt::Horizontal), false);
    QCOMPARE(usedSimplex(l, Qt::Vertical), false);
}

/*
  Avoid regression where the saved prefSize would be lost. This was
  solved by saving the original spacing in the QGraphicsAnchorPrivate class
*/
void tst_QGraphicsAnchorLayout::spacingPersistency()
{
    QGraphicsWidget w;
    QGraphicsWidget *a = createItem();
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&w);

    l->addAnchors(l, a, Qt::Horizontal);
    QGraphicsAnchor *anchor = l->anchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);

    anchor->setSpacing(-30);
    QCOMPARE(anchor->spacing(), -30.0);

    anchor->setSpacing(30);
    QCOMPARE(anchor->spacing(), 30.0);

    anchor->setSizePolicy(QSizePolicy::Ignored);
    w.effectiveSizeHint(Qt::PreferredSize);

    QCOMPARE(anchor->spacing(), 30.0);
}

/*
    Test whether a correct preferred size is set when a "snake" sequence is in parallel with the
    layout or half of the layout. The tricky thing here is that all items on the snake should
    keep their preferred sizes.
*/
void tst_QGraphicsAnchorLayout::snakeParallelWithLayout()
{
    QSizeF minSize(10, 20);
    QSizeF pref(50, 20);
    QSizeF maxSize(100, 20);

    QGraphicsWidget *a = createItem(maxSize, maxSize, maxSize, "A");
    QGraphicsWidget *b = createItem(minSize, pref, maxSize, "B");
    QGraphicsWidget *c = createItem(maxSize, maxSize, maxSize, "C");

    QGraphicsWidget parent;
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&parent);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    // First we'll do the case in parallel with the entire layout...
    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorRight);
    l->addAnchor(b, Qt::AnchorLeft, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorRight);

    l->addAnchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    l->addAnchor(a, Qt::AnchorBottom, b, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, l, Qt::AnchorBottom);

    parent.resize(l->effectiveSizeHint(Qt::PreferredSize));

    // Note that A and C are fixed in the maximum size
    QCOMPARE(l->geometry(), QRectF(QPointF(0, 0), QSizeF(150, 60)));
    QCOMPARE(a->geometry(), QRectF(QPointF(0, 0), maxSize));
    QCOMPARE(b->geometry(), QRectF(QPointF(50, 20), pref));
    QCOMPARE(c->geometry(), QRectF(QPointF(50, 40), maxSize));

    // Then, we change the "snake" to be in parallel with half of the layout
    delete l->anchor(c, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(c, Qt::AnchorRight, l, Qt::AnchorHorizontalCenter);

    parent.resize(l->effectiveSizeHint(Qt::PreferredSize));

    QCOMPARE(l->geometry(), QRectF(QPointF(0, 0), QSizeF(300, 60)));
    QCOMPARE(a->geometry(), QRectF(QPointF(0, 0), maxSize));
    QCOMPARE(b->geometry(), QRectF(QPointF(50, 20), pref));
    QCOMPARE(c->geometry(), QRectF(QPointF(50, 40), maxSize));
}

/*
  Avoid regression where the sizeHint constraints would not be
  created for a parallel anchor that included the first layout half
*/
void tst_QGraphicsAnchorLayout::parallelToHalfLayout()
{
    QGraphicsWidget *a = createItem();

    QGraphicsWidget w;
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&w);
    l->setContentsMargins(10, 10, 10, 10);

    l->addAnchors(l, a, Qt::Vertical);

    QGraphicsAnchor *anchor;
    anchor = l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    anchor->setSpacing(5);
    anchor = l->addAnchor(l, Qt::AnchorHorizontalCenter, a, Qt::AnchorRight);
    anchor->setSpacing(-5);

    const QSizeF minimumSizeHint = w.effectiveSizeHint(Qt::MinimumSize);
    const QSizeF preferredSizeHint = w.effectiveSizeHint(Qt::PreferredSize);
    const QSizeF maximumSizeHint = w.effectiveSizeHint(Qt::MaximumSize);

    const QSizeF overhead = QSizeF(10 + 5 + 5, 10) * 2;

    QCOMPARE(minimumSizeHint, QSizeF(200, 100) + overhead);
    QCOMPARE(preferredSizeHint, QSizeF(300, 100) + overhead);
    QCOMPARE(maximumSizeHint, QSizeF(400, 100) + overhead);
}

void tst_QGraphicsAnchorLayout::globalSpacing()
{
    QGraphicsWidget *a = createItem();
    QGraphicsWidget *b = createItem();

    QGraphicsWidget w;
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout(&w);

    l->addCornerAnchors(l, Qt::TopLeftCorner, a, Qt::TopLeftCorner);
    l->addCornerAnchors(a, Qt::BottomRightCorner, b, Qt::TopLeftCorner);
    l->addCornerAnchors(b, Qt::BottomRightCorner, l, Qt::BottomRightCorner);

    w.resize(w.effectiveSizeHint(Qt::PreferredSize));
    qreal vSpacing = b->geometry().top() - a->geometry().bottom();
    qreal hSpacing = b->geometry().left() - a->geometry().right();

    // Set spacings manually
    l->setVerticalSpacing(vSpacing + 10);
    l->setHorizontalSpacing(hSpacing + 5);

    w.resize(w.effectiveSizeHint(Qt::PreferredSize));
    qreal newVSpacing = b->geometry().top() - a->geometry().bottom();
    qreal newHSpacing = b->geometry().left() - a->geometry().right();

    QCOMPARE(newVSpacing, vSpacing + 10);
    QCOMPARE(newHSpacing, hSpacing + 5);

    // Set a negative spacing. This will unset the previous spacing and
    // bring back the widget-defined spacing.
    l->setSpacing(-1);

    w.resize(w.effectiveSizeHint(Qt::PreferredSize));
    newVSpacing = b->geometry().top() - a->geometry().bottom();
    newHSpacing = b->geometry().left() - a->geometry().right();

    QCOMPARE(newVSpacing, vSpacing);
    QCOMPARE(newHSpacing, hSpacing);
}

void tst_QGraphicsAnchorLayout::graphicsAnchorHandling()
{
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout();
    QGraphicsWidget *a = createItem();

    l->addAnchors(l, a);

    QGraphicsAnchor *layoutAnchor = l->anchor(l, Qt::AnchorTop, l, Qt::AnchorBottom);
    QGraphicsAnchor *itemAnchor = l->anchor(a, Qt::AnchorTop, a, Qt::AnchorBottom);
    QGraphicsAnchor *invalidAnchor = l->anchor(a, Qt::AnchorTop, l, Qt::AnchorBottom);

    // Ensure none of these anchors are accessible.
    QVERIFY(!layoutAnchor);
    QVERIFY(!itemAnchor);
    QVERIFY(!invalidAnchor);

    // Hook the anchors to a QObject
    QObject object;
    QGraphicsAnchor *userAnchor = l->anchor(l, Qt::AnchorTop, a, Qt::AnchorTop);
    userAnchor->setParent(&object);
    userAnchor = l->anchor(l, Qt::AnchorBottom, a, Qt::AnchorBottom);
    userAnchor->setParent(&object);
    userAnchor = l->anchor(l, Qt::AnchorRight, a, Qt::AnchorRight);
    userAnchor->setParent(&object);
    userAnchor = l->anchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    userAnchor->setParent(&object);

    QCOMPARE(object.children().size(), 4);

    // Delete layout, this will cause all anchors to be deleted internally.
    // We expect the public QGraphicsAnchor instances to be deleted too.
    delete l;
    QCOMPARE(object.children().size(), 0);

    delete a;
}

void tst_QGraphicsAnchorLayout::invalidHierarchyCheck()
{
    QGraphicsWidget window(0, Qt::Window);
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    window.setLayout(l);

    QCOMPARE(l->count(), 0);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): "
                         "You cannot add the parent of the layout to the layout.");
    QVERIFY(!l->addAnchor(l, Qt::AnchorLeft, &window, Qt::AnchorLeft));
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): "
                         "You cannot add the parent of the layout to the layout.");
    l->addAnchors(l, &window);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): "
                         "You cannot add the parent of the layout to the layout.");
    l->addCornerAnchors(l, Qt::TopLeftCorner, &window, Qt::TopLeftCorner);
    QCOMPARE(l->count(), 0);
}

QTEST_MAIN(tst_QGraphicsAnchorLayout)
#include "tst_qgraphicsanchorlayout.moc"
