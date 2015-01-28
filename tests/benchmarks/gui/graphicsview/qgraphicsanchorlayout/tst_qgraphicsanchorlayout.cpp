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
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicswidget.h>
#include <QtWidgets/qgraphicsview.h>

class tst_QGraphicsAnchorLayout : public QObject
{
    Q_OBJECT
public:
    tst_QGraphicsAnchorLayout() {}
    ~tst_QGraphicsAnchorLayout() {}

private slots:
    void hard_complex_data();
    void hard_complex();
    void linearVsAnchorSizeHints_data();
    void linearVsAnchorSizeHints();
    void linearVsAnchorSetGeometry_data();
    void linearVsAnchorSetGeometry();
    void linearVsAnchorNested_data();
    void linearVsAnchorNested();
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
                      qreal spacing)
{
    QGraphicsAnchor *anchor = l->addAnchor(firstItem, firstEdge, secondItem, secondEdge);
    anchor->setSpacing(spacing);
}

void tst_QGraphicsAnchorLayout::hard_complex_data()
{
    QTest::addColumn<int>("whichSizeHint");
    QTest::newRow("minimumSizeHint")
        << int(Qt::MinimumSize);
    QTest::newRow("preferredSizeHint")
        << int(Qt::PreferredSize);
    QTest::newRow("maximumSizeHint")
        << int(Qt::MaximumSize);
    // Add it as a reference to see how much overhead the body of effectiveSizeHint takes.
    QTest::newRow("noSizeHint")
        << -1;
}

void tst_QGraphicsAnchorLayout::hard_complex()
{
    QFETCH(int, whichSizeHint);

    // Test for "hard" complex case, taken from wiki
    // https://cwiki.nokia.com/S60QTUI/AnchorLayoutComplexCases
    QSizeF min(0, 10);
    QSizeF pref(50, 10);
    QSizeF max(100, 10);

    QGraphicsWidget *a = createItem(min, pref, max, "a");
    QGraphicsWidget *b = createItem(min, pref, max, "b");
    QGraphicsWidget *c = createItem(min, pref, max, "c");
    QGraphicsWidget *d = createItem(min, pref, max, "d");
    QGraphicsWidget *e = createItem(min, pref, max, "e");
    QGraphicsWidget *f = createItem(min, pref, max, "f");
    QGraphicsWidget *g = createItem(min, pref, max, "g");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setContentsMargins(0, 0, 0, 0);

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

    // It won't query the size hint if it already has a size set.
    // If only one of the sizes is unset it will query sizeHint only of for that hint type.
    l->setMinimumSize(60,40);
    l->setPreferredSize(220,40);
    l->setMaximumSize(240,40);

    switch (whichSizeHint) {
    case Qt::MinimumSize:
        l->setMinimumSize(-1, -1);
        break;
    case Qt::PreferredSize:
        l->setPreferredSize(-1, -1);
        break;
    case Qt::MaximumSize:
        l->setMaximumSize(-1, -1);
        break;
    default:
        break;
    }

    QSizeF sizeHint;
    // warm up instruction cache
    l->invalidate();
    sizeHint = l->effectiveSizeHint((Qt::SizeHint)whichSizeHint);
    // ...then measure...
    QBENCHMARK {
        l->invalidate();
        sizeHint = l->effectiveSizeHint((Qt::SizeHint)whichSizeHint);
    }
}

static QGraphicsLayout* createLayouts(int whichLayout)
{
    QSizeF min(0, 10);
    QSizeF pref(50, 10);
    QSizeF max(100, 10);

    QGraphicsWidget *a = createItem(min, pref, max, "a");
    QGraphicsWidget *b = createItem(min, pref, max, "b");
    QGraphicsWidget *c = createItem(min, pref, max, "c");
    QGraphicsWidget *d = createItem(min, pref, max, "d");

    QGraphicsLayout *l;
    if (whichLayout == 0) {
        l = new QGraphicsLinearLayout;
        QGraphicsLinearLayout *linear = static_cast<QGraphicsLinearLayout *>(l);
        linear->setContentsMargins(0, 0, 0, 0);

        linear->addItem(a);
        linear->addItem(b);
        linear->addItem(c);
        linear->addItem(d);
    } else {
        l = new QGraphicsAnchorLayout;
        QGraphicsAnchorLayout *anchor = static_cast<QGraphicsAnchorLayout *>(l);
        anchor->setContentsMargins(0, 0, 0, 0);

        // Horizontal
        setAnchor(anchor, anchor, Qt::AnchorLeft, a, Qt::AnchorLeft, 0);
        setAnchor(anchor, a, Qt::AnchorRight, b, Qt::AnchorLeft, 0);
        setAnchor(anchor, b, Qt::AnchorRight, c, Qt::AnchorLeft, 0);
        setAnchor(anchor, c, Qt::AnchorRight, d, Qt::AnchorLeft, 0);
        setAnchor(anchor, d, Qt::AnchorRight, anchor, Qt::AnchorRight, 0);

        // Vertical
        anchor->addAnchors(anchor, a, Qt::Vertical);
        anchor->addAnchors(anchor, b, Qt::Vertical);
        anchor->addAnchors(anchor, c, Qt::Vertical);
        anchor->addAnchors(anchor, d, Qt::Vertical);
    }

    return l;
}

void tst_QGraphicsAnchorLayout::linearVsAnchorSizeHints_data()
{
    QTest::addColumn<int>("whichLayout");
    QTest::addColumn<int>("whichSizeHint");

    QTest::newRow("QGraphicsLinearLayout::minimum")
        << 0 << int(Qt::MinimumSize);
    QTest::newRow("QGraphicsLinearLayout::preferred")
        << 0 << int(Qt::PreferredSize);
    QTest::newRow("QGraphicsLinearLayout::maximum")
        << 0 << int(Qt::MaximumSize);
    QTest::newRow("QGraphicsLinearLayout::noSizeHint")
        << 0 << -1;

    QTest::newRow("QGraphicsAnchorLayout::minimum")
        << 1 << int(Qt::MinimumSize);
    QTest::newRow("QGraphicsAnchorLayout::preferred")
        << 1 << int(Qt::PreferredSize);
    QTest::newRow("QGraphicsAnchorLayout::maximum")
        << 1 << int(Qt::MaximumSize);
    QTest::newRow("QGraphicsAnchorLayout::noSizeHint")
        << 1 << -1;
}

void tst_QGraphicsAnchorLayout::linearVsAnchorSizeHints()
{
    QFETCH(int, whichSizeHint);
    QFETCH(int, whichLayout);

    QGraphicsLayout *l = createLayouts(whichLayout);

    QSizeF sizeHint;
    // warm up instruction cache
    l->invalidate();
    sizeHint = l->effectiveSizeHint((Qt::SizeHint)whichSizeHint);
    // ...then measure...

    QBENCHMARK {
        l->invalidate();
        sizeHint = l->effectiveSizeHint((Qt::SizeHint)whichSizeHint);
    }
}

void tst_QGraphicsAnchorLayout::linearVsAnchorSetGeometry_data()
{
    QTest::addColumn<int>("whichLayout");

    QTest::newRow("QGraphicsLinearLayout")
        << 0;
    QTest::newRow("QGraphicsAnchorLayout")
        << 1;
}

void tst_QGraphicsAnchorLayout::linearVsAnchorSetGeometry()
{
    QFETCH(int, whichLayout);

    QGraphicsLayout *l = createLayouts(whichLayout);

    QRectF sizeHint;
    qreal maxWidth;
    qreal increment;
    // warm up instruction cache
    l->invalidate();
    sizeHint.setSize(l->effectiveSizeHint(Qt::MinimumSize));
    maxWidth = l->effectiveSizeHint(Qt::MaximumSize).width();
    increment = (maxWidth - sizeHint.width()) / 100;
    l->setGeometry(sizeHint);
    // ...then measure...

    QBENCHMARK {
        l->invalidate();
        for (qreal width = sizeHint.width(); width <= maxWidth; width += increment) {
            sizeHint.setWidth(width);
            l->setGeometry(sizeHint);
        }
    }
}

void tst_QGraphicsAnchorLayout::linearVsAnchorNested_data()
{
    QTest::addColumn<int>("whichLayout");
    QTest::newRow("LinearLayout")
        << 0;
    QTest::newRow("AnchorLayout setup with null-anchors knot")
        << 1;
    QTest::newRow("AnchorLayout setup easy to simplificate")
        << 2;
}

void tst_QGraphicsAnchorLayout::linearVsAnchorNested()
{
    QFETCH(int, whichLayout);

    QSizeF min(10, 10);
    QSizeF pref(80, 80);
    QSizeF max(150, 150);

    QGraphicsWidget *a = createItem(min, pref, max, "a");
    QGraphicsWidget *b = createItem(min, pref, max, "b");
    QGraphicsWidget *c = createItem(min, pref, max, "c");
    QGraphicsWidget *d = createItem(min, pref, max, "d");

    QGraphicsLayout *layout;

    if (whichLayout == 0) {
        QGraphicsLinearLayout *linear1 = new QGraphicsLinearLayout;
        QGraphicsLinearLayout *linear2 = new QGraphicsLinearLayout(Qt::Vertical);
        QGraphicsLinearLayout *linear3 = new QGraphicsLinearLayout;

        linear1->addItem(a);
        linear1->addItem(linear2);
        linear2->addItem(b);
        linear2->addItem(linear3);
        linear3->addItem(c);
        linear3->addItem(d);

        layout = linear1;
    } else if (whichLayout == 1) {
        QGraphicsAnchorLayout *anchor = new QGraphicsAnchorLayout;

        // A
        anchor->addCornerAnchors(a, Qt::TopLeftCorner, anchor, Qt::TopLeftCorner);
        anchor->addCornerAnchors(a, Qt::TopRightCorner, b, Qt::TopLeftCorner);
        anchor->addCornerAnchors(a, Qt::BottomLeftCorner, anchor, Qt::BottomLeftCorner);
        anchor->addCornerAnchors(a, Qt::BottomRightCorner, c, Qt::BottomLeftCorner);

        // B
        anchor->addCornerAnchors(b, Qt::TopRightCorner, anchor, Qt::TopRightCorner);
        anchor->addCornerAnchors(b, Qt::BottomLeftCorner, c, Qt::TopLeftCorner);
        anchor->addCornerAnchors(b, Qt::BottomRightCorner, d, Qt::TopRightCorner);

        // C
        anchor->addCornerAnchors(c, Qt::TopRightCorner, d, Qt::TopLeftCorner);
        anchor->addCornerAnchors(c, Qt::BottomRightCorner, d, Qt::BottomLeftCorner);

        // D
        anchor->addCornerAnchors(d, Qt::BottomRightCorner, anchor, Qt::BottomRightCorner);

        layout = anchor;
    } else {
        QGraphicsAnchorLayout *anchor = new QGraphicsAnchorLayout;

        // A
        anchor->addAnchor(a, Qt::AnchorLeft, anchor, Qt::AnchorLeft);
        anchor->addAnchors(a, anchor, Qt::Vertical);
        anchor->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);
        anchor->addAnchor(a, Qt::AnchorRight, c, Qt::AnchorLeft);

        // B
        anchor->addAnchor(b, Qt::AnchorTop, anchor, Qt::AnchorTop);
        anchor->addAnchor(b, Qt::AnchorRight, anchor, Qt::AnchorRight);
        anchor->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);
        anchor->addAnchor(b, Qt::AnchorBottom, d, Qt::AnchorTop);

        // C
        anchor->addAnchor(c, Qt::AnchorRight, d, Qt::AnchorLeft);
        anchor->addAnchor(c, Qt::AnchorBottom, anchor, Qt::AnchorBottom);

        // D
        anchor->addAnchor(d, Qt::AnchorRight, anchor, Qt::AnchorRight);
        anchor->addAnchor(d, Qt::AnchorBottom, anchor, Qt::AnchorBottom);

        layout = anchor;
    }

    QSizeF sizeHint;
    // warm up instruction cache
    layout->invalidate();
    sizeHint = layout->effectiveSizeHint(Qt::PreferredSize);

    // ...then measure...
    QBENCHMARK {
        // To ensure that all sizeHints caches are invalidated in
        // the LinearLayout setup, we must call updateGeometry on the
        // children. If we didn't, only the top level layout would be
        // re-calculated.
        static_cast<QGraphicsLayoutItem *>(a)->updateGeometry();
        static_cast<QGraphicsLayoutItem *>(b)->updateGeometry();
        static_cast<QGraphicsLayoutItem *>(c)->updateGeometry();
        static_cast<QGraphicsLayoutItem *>(d)->updateGeometry();
        layout->invalidate();
        sizeHint = layout->effectiveSizeHint(Qt::PreferredSize);
    }
}

QTEST_MAIN(tst_QGraphicsAnchorLayout)

#include "tst_qgraphicsanchorlayout.moc"
