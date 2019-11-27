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
#include <qgraphicsgridlayout.h>
#include <qgraphicswidget.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>

class tst_QGraphicsGridLayout : public QObject
{
    Q_OBJECT

private slots:
    void qgraphicsgridlayout_data();
    void qgraphicsgridlayout();
    void addItem_data();
    void addItem();
    void alignment_data();
    void alignment();
    void alignment2();
    void alignment2_data();
    void columnAlignment_data();
    void columnAlignment();
    void columnCount_data();
    void columnCount();
    void columnMaximumWidth_data();
    void columnMaximumWidth();
    void columnMinimumWidth_data();
    void columnMinimumWidth();
    void columnPreferredWidth_data();
    void columnPreferredWidth();
    void setColumnFixedWidth();
    void columnSpacing();
    void columnStretchFactor();
    void count();
    void contentsMargins();
    void horizontalSpacing_data();
    void horizontalSpacing();
    void itemAt();
    void removeAt();
    void removeItem();
    void rowAlignment_data();
    void rowAlignment();
    void rowCount_data();
    void rowCount();
    void rowMaximumHeight_data();
    void rowMaximumHeight();
    void rowMinimumHeight_data();
    void rowMinimumHeight();
    void rowPreferredHeight_data();
    void rowPreferredHeight();
    void rowSpacing();
    void rowStretchFactor_data();
    void rowStretchFactor();
    void setColumnSpacing_data();
    void setColumnSpacing();
    void setGeometry_data();
    void setGeometry();
    void setRowFixedHeight();
    void setRowSpacing_data();
    void setRowSpacing();
    void setSpacing_data();
    void setSpacing();
    void sizeHint_data();
    void sizeHint();
    void verticalSpacing_data();
    void verticalSpacing();
    void layoutDirection_data();
    void layoutDirection();
    void removeLayout();
    void defaultStretchFactors_data();
    void defaultStretchFactors();
    void geometries_data();
    void geometries();
    void avoidRecursionInInsertItem();
    void styleInfoLeak();
    void task236367_maxSizeHint();
    void spanningItem2x2_data();
    void spanningItem2x2();
    void spanningItem2x3_data();
    void spanningItem2x3();
    void spanningItem();
    void spanAcrossEmptyRow();
    void heightForWidth();
    void widthForHeight();
    void heightForWidthWithSpanning();
    void stretchAndHeightForWidth();
    void testDefaultAlignment();
    void hiddenItems();
};

class RectWidget : public QGraphicsWidget
{
public:
    RectWidget(QGraphicsItem *parent = 0) : QGraphicsWidget(parent), m_fnConstraint(0) {}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->drawRoundedRect(rect(), 25, 25, Qt::RelativeSize);
        painter->drawLine(rect().topLeft(), rect().bottomRight());
        painter->drawLine(rect().bottomLeft(), rect().topRight());
    }

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const
    {
        if (constraint.width() < 0 && constraint.height() < 0 && m_sizeHints[which].isValid()) {
            return m_sizeHints[which];
        }
        if (m_fnConstraint) {
            return m_fnConstraint(which, constraint);
        }
        return QGraphicsWidget::sizeHint(which, constraint);
    }

    void setSizeHint(Qt::SizeHint which, const QSizeF &size) {
        m_sizeHints[which] = size;
        updateGeometry();
    }

    void setConstraintFunction(QSizeF (*fnConstraint)(Qt::SizeHint, const QSizeF &)) {
        m_fnConstraint = fnConstraint;
    }

    // Initializer {} is a workaround for gcc bug 68949
    QSizeF m_sizeHints[Qt::NSizeHints] {};
    QSizeF (*m_fnConstraint)(Qt::SizeHint, const QSizeF &);

};

struct ItemDesc
{
    ItemDesc(int row, int col)
    : m_pos(qMakePair(row, col))
    {
    }

    ItemDesc &rowSpan(int span) {
        m_rowSpan = span;
        return (*this);
    }

    ItemDesc &colSpan(int span) {
        m_colSpan = span;
        return (*this);
    }

    ItemDesc &sizePolicy(const QSizePolicy &sp) {
        m_sizePolicy = sp;
        return (*this);
    }

    ItemDesc &sizePolicy(QSizePolicy::Policy horAndVer) {
        m_sizePolicy = QSizePolicy(horAndVer, horAndVer);
        return (*this);
    }

    ItemDesc &sizePolicyH(QSizePolicy::Policy hor) {
        m_sizePolicy.setHorizontalPolicy(hor);
        return (*this);
    }

    ItemDesc &sizePolicyV(QSizePolicy::Policy ver) {
        m_sizePolicy.setVerticalPolicy(ver);
        return (*this);
    }

    ItemDesc &sizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver) {
        m_sizePolicy = QSizePolicy(hor, ver);
        return (*this);
    }

    ItemDesc &sizeHint(Qt::SizeHint which, const QSizeF &sh) {
        m_sizeHints[which] = sh;
        return (*this);
    }

    ItemDesc &preferredSizeHint(const QSizeF &sh) {
        m_sizeHints[Qt::PreferredSize] = sh;
        return (*this);
    }

    ItemDesc &minSize(const QSizeF &sz) {
        m_sizes[Qt::MinimumSize] = sz;
        return (*this);
    }
    ItemDesc &preferredSize(const QSizeF &sz) {
        m_sizes[Qt::PreferredSize] = sz;
        return (*this);
    }
    ItemDesc &maxSize(const QSizeF &sz) {
        m_sizes[Qt::MaximumSize] = sz;
        return (*this);
    }

    ItemDesc &alignment(Qt::Alignment alignment) {
        m_align = alignment;
        return (*this);
    }

    ItemDesc &dynamicConstraint(QSizeF (*fnConstraint)(Qt::SizeHint, const QSizeF &),
                                Qt::Orientation orientation) {
        m_fnConstraint = fnConstraint;
        m_constraintOrientation = orientation;
        return (*this);
    }

    void apply(QGraphicsGridLayout *layout, QGraphicsWidget *item) {
        QSizePolicy sp = m_sizePolicy;
        if (m_fnConstraint) {
            sp.setHeightForWidth(m_constraintOrientation == Qt::Vertical);
            sp.setWidthForHeight(m_constraintOrientation == Qt::Horizontal);
        }

        item->setSizePolicy(sp);
        for (int i = 0; i < Qt::NSizeHints; ++i) {
            if (!m_sizes[i].isValid())
                continue;
            switch ((Qt::SizeHint)i) {
            case Qt::MinimumSize:
                item->setMinimumSize(m_sizes[i]);
                break;
            case Qt::PreferredSize:
                item->setPreferredSize(m_sizes[i]);
                break;
            case Qt::MaximumSize:
                item->setMaximumSize(m_sizes[i]);
                break;
            default:
                qWarning("not implemented");
                break;
            }
        }

        layout->addItem(item, m_pos.first, m_pos.second, m_rowSpan, m_colSpan);
        layout->setAlignment(item, m_align);
    }

    void apply(QGraphicsGridLayout *layout, RectWidget *item) {
        for (int i = 0; i < Qt::NSizeHints; ++i)
            item->setSizeHint((Qt::SizeHint)i, m_sizeHints[i]);
        item->setConstraintFunction(m_fnConstraint);
        apply(layout, static_cast<QGraphicsWidget*>(item));
    }

//private:
    QPair<int,int> m_pos; // row,col
    int m_rowSpan = 1;
    int m_colSpan = 1;
    QSizePolicy m_sizePolicy{QSizePolicy::Preferred, QSizePolicy::Preferred};

    // Initializer {} is a workaround for gcc bug 68949
    QSizeF m_sizeHints[Qt::NSizeHints] {};
    QSizeF m_sizes[Qt::NSizeHints] {};
    Qt::Alignment m_align;

    Qt::Orientation m_constraintOrientation = Qt::Horizontal;
    QSizeF (*m_fnConstraint)(Qt::SizeHint, const QSizeF &) = nullptr;
};

typedef QList<ItemDesc> ItemList;
Q_DECLARE_METATYPE(ItemList);

typedef QList<QSizeF> SizeList;

void tst_QGraphicsGridLayout::qgraphicsgridlayout_data()
{
}

void tst_QGraphicsGridLayout::qgraphicsgridlayout()
{
    QGraphicsGridLayout layout;
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::addItem: invalid row span/column span: 0");
    layout.addItem(0, 0, 0, 0, 0);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::addItem: cannot add null item");
    layout.addItem(0, 0, 0);
    layout.alignment(0);
    layout.columnAlignment(0);
    layout.columnCount();
    layout.columnMaximumWidth(0);
    layout.columnMinimumWidth(0);
    layout.columnPreferredWidth(0);
    layout.columnSpacing(0);
    layout.columnStretchFactor(0);
    layout.count();
    layout.horizontalSpacing();
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::itemAt: invalid row, column 0, 0");
    layout.itemAt(0, 0);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::itemAt: invalid index 0");
    layout.itemAt(0);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::removeAt: invalid index 0");
    layout.removeAt(0);
    layout.rowAlignment(0);
    layout.rowCount();
    layout.rowMaximumHeight(0);
    layout.rowMinimumHeight(0);
    layout.rowPreferredHeight(0);
    layout.rowSpacing(0);
    layout.rowStretchFactor(0);
    layout.setAlignment(0, Qt::AlignRight);
    layout.setColumnAlignment(0, Qt::AlignRight);
    layout.setColumnFixedWidth(0, 0);
    layout.setColumnMaximumWidth(0, 0);
    layout.setColumnMinimumWidth(0, 0);
    layout.setColumnPreferredWidth(0, 0);
    layout.setColumnSpacing(0, 0);
    layout.setColumnStretchFactor(0, 0);
    layout.setGeometry(QRectF());
    layout.setHorizontalSpacing(0);
    layout.setRowAlignment(0, { });
    layout.setRowFixedHeight(0, 0);
    layout.setRowMaximumHeight(0, 0);
    layout.setRowMinimumHeight(0, 0);
    layout.setRowPreferredHeight(0, 0);
    layout.setRowSpacing(0, 0);
    layout.setRowStretchFactor(0, 0);
    layout.setSpacing(0);
    layout.setVerticalSpacing(0);
    layout.sizeHint(Qt::MinimumSize);
    layout.verticalSpacing();
}

static void populateLayout(QGraphicsGridLayout *gridLayout, int width, int height, bool hasHeightForWidth = false)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QGraphicsWidget *item = new RectWidget();
            item->setMinimumSize(10, 10);
            item->setPreferredSize(25, 25);
            item->setMaximumSize(50, 50);
            gridLayout->addItem(item, y, x);
            QSizePolicy policy = item->sizePolicy();
            policy.setHeightForWidth(hasHeightForWidth);
            item->setSizePolicy(policy);
        }
    }
}


/** populates \a gridLayout with a 3x2 layout:
 * +----+----+----+
 * |+---|---+|xxxx|
 * ||span=2 ||hole|
 * |+---|---+|xxxx|
 * +----+----+----+
 * |xxxx|+---|---+|
 * |hole||span=2 ||
 * |xxxx|+---|---+|
 * +----+----+----+
 */
static void populateLayoutWithSpansAndHoles(QGraphicsGridLayout *gridLayout, bool hasHeightForWidth = false)
{
    QGraphicsWidget *item = new RectWidget();
    item->setMinimumSize(10, 10);
    item->setPreferredSize(25, 25);
    item->setMaximumSize(50, 50);
    QSizePolicy sizepolicy = item->sizePolicy();
    sizepolicy.setHeightForWidth(hasHeightForWidth);
    item->setSizePolicy(sizepolicy);
    gridLayout->addItem(item, 0, 0, 1, 2);

    item = new RectWidget();
    item->setMinimumSize(10, 10);
    item->setPreferredSize(25, 25);
    item->setMaximumSize(50, 50);
    item->setSizePolicy(sizepolicy);
    gridLayout->addItem(item, 1, 1, 1, 2);
}

Q_DECLARE_METATYPE(Qt::Alignment)
void tst_QGraphicsGridLayout::addItem_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("rowSpan");
    QTest::addColumn<int>("columnSpan");
    QTest::addColumn<Qt::Alignment>("alignment");

    for (int a = -1; a < 3; ++a) {
    for (int b = -1; b < 2; ++b) {
    for (int c = -1; c < 2; ++c) {
    for (int d = -1; d < 2; ++d) {
        int row = a;
        int column = b;
        int rowSpan = c;
        int columnSpan = d;
        const QByteArray name = '(' + QByteArray::number(a) + ',' + QByteArray::number(b)
            + ',' + QByteArray::number(c) + ',' + QByteArray::number(d);
        Qt::Alignment alignment = Qt::AlignLeft;
        QTest::newRow(name.constData()) << row << column << rowSpan << columnSpan << alignment;
    }}}}
}

// public void addItem(QGraphicsLayoutItem* item, int row, int column, int rowSpan, int columnSpan, Qt::Alignment alignment = 0)
void tst_QGraphicsGridLayout::addItem()
{
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(int, rowSpan);
    QFETCH(int, columnSpan);
    QFETCH(Qt::Alignment, alignment);

    QGraphicsGridLayout *layout = new QGraphicsGridLayout;

    QGraphicsWidget *wid = new QGraphicsWidget;
    if (row < 0 || column < 0) {
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::addItem: invalid row/column: -1");
    } else if (rowSpan < 1 || columnSpan < 1) {
        char buf[1024];
        ::qsnprintf(buf, sizeof(buf), "QGraphicsGridLayout::addItem: invalid row span/column span: %d",
            rowSpan < 1 ? rowSpan : columnSpan);
        QTest::ignoreMessage(QtWarningMsg, buf);
    }
    layout->addItem(wid, row, column, rowSpan, columnSpan, alignment);

    delete layout;
}

void tst_QGraphicsGridLayout::alignment_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

// public Qt::Alignment alignment(QGraphicsLayoutItem* item) const
void tst_QGraphicsGridLayout::alignment()
{
#ifdef Q_OS_MAC
    QSKIP("Resizing a QGraphicsWidget to effectiveSizeHint(Qt::MaximumSize) is currently not supported on mac");
#endif
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();
    // no alignment (the default)
    QCOMPARE(layout->itemAt(0, 0)->geometry().left(), 0.0);
    QCOMPARE(layout->itemAt(0, 0)->geometry().right(), layout->itemAt(0, 1)->geometry().left());
    QCOMPARE(layout->itemAt(0, 1)->geometry().left(), 25.0);
    QCOMPARE(layout->itemAt(0, 1)->geometry().right(), layout->itemAt(0, 2)->geometry().left());
    QCOMPARE(layout->itemAt(0, 2)->geometry().left(), 50.0);
    QCOMPARE(layout->itemAt(0, 2)->geometry().right(), 75.0);

    QCOMPARE(layout->itemAt(1, 0)->geometry().left(), 0.0);
    QCOMPARE(layout->itemAt(1, 0)->geometry().right(), layout->itemAt(1, 1)->geometry().left());
    QCOMPARE(layout->itemAt(1, 1)->geometry().left(), 25.0);
    QCOMPARE(layout->itemAt(1, 1)->geometry().right(), layout->itemAt(1, 2)->geometry().left());
    QCOMPARE(layout->itemAt(1, 2)->geometry().left(), 50.0);
    QCOMPARE(layout->itemAt(1, 2)->geometry().right(), 75.0);

    QCOMPARE(layout->itemAt(0, 0)->geometry().top(), 0.0);
    QCOMPARE(layout->itemAt(0, 0)->geometry().bottom(), layout->itemAt(1, 0)->geometry().top());
    QCOMPARE(layout->itemAt(1, 0)->geometry().top(), 25.0);
    QCOMPARE(layout->itemAt(1, 0)->geometry().bottom(), 50.0);

    // align first column left, second hcenter, third right
    layout->setColumnMinimumWidth(0, 100);
    layout->setAlignment(layout->itemAt(0,0), Qt::AlignLeft);
    layout->setAlignment(layout->itemAt(1,0), Qt::AlignLeft);
    layout->setColumnMinimumWidth(1, 100);
    layout->setAlignment(layout->itemAt(0,1), Qt::AlignHCenter);
    layout->setAlignment(layout->itemAt(1,1), Qt::AlignHCenter);
    layout->setColumnMinimumWidth(2, 100);
    layout->setAlignment(layout->itemAt(0,2), Qt::AlignRight);
    layout->setAlignment(layout->itemAt(1,2), Qt::AlignRight);

    widget->resize(widget->effectiveSizeHint(Qt::MaximumSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0,    0,  50,  50));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0,   50,  50,  50));
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(125,  0,  50,  50));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(125, 50,  50,  50));
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(250,  0,  50,  50));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(250, 50,  50,  50));

    delete widget;
}

void tst_QGraphicsGridLayout::columnAlignment_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

// public void setColumnAlignment(int column, Qt::Alignment alignment)
// public Qt::Alignment columnAlignment(int column) const
void tst_QGraphicsGridLayout::columnAlignment()
{
#ifdef Q_OS_MAC
    QSKIP("Resizing a QGraphicsWidget to effectiveSizeHint(Qt::MaximumSize) is currently not supported on mac");
#endif
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    widget->setContentsMargins(0, 0, 0, 0);

    layout->setColumnMinimumWidth(0, 100);
    layout->setColumnMinimumWidth(1, 100);
    layout->setColumnMinimumWidth(2, 100);

    view.resize(450,150);
    widget->resize(widget->effectiveSizeHint(Qt::MaximumSize));
    view.show();
    widget->show();
    QApplication::sendPostedEvents(0, 0);
    // Check default
    QCOMPARE(layout->columnAlignment(0), 0);
    QCOMPARE(layout->columnAlignment(1), 0);
    QCOMPARE(layout->columnAlignment(2), 0);

    layout->setColumnAlignment(0, Qt::AlignLeft);
    layout->setColumnAlignment(1, Qt::AlignHCenter);
    layout->setColumnAlignment(2, Qt::AlignRight);

    // see if item alignment takes preference over columnAlignment
    layout->setAlignment(layout->itemAt(1,0), Qt::AlignHCenter);
    layout->setAlignment(layout->itemAt(1,1), Qt::AlignRight);
    layout->setAlignment(layout->itemAt(1,2), Qt::AlignLeft);

    QApplication::sendPostedEvents(0, 0);  // process LayoutRequest
    /*
      +----------+------------+---------+
      | Left     |  HCenter   |  Right  |
      +----------+------------+---------+
      | HCenter  |   Right    |   Left  |
      +---------------------------------+
    */
    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0,   0,   50,  50));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(25,  51,  50,  50));   // item is king
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(126,  0,  50,  50));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(151, 51,  50,  50));   // item is king
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(252,  0,  50,  50));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(202, 51,  50,  50));   // item is king

    delete widget;
}

void tst_QGraphicsGridLayout::columnCount_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}
// public int columnCount() const
void tst_QGraphicsGridLayout::columnCount()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    widget->setContentsMargins(0, 0, 0, 0);

    view.show();
    widget->show();
    QApplication::processEvents();

    QCOMPARE(layout->columnCount(), 0);
    layout->addItem(new RectWidget(widget), 0, 0);
    QCOMPARE(layout->columnCount(), 1);
    layout->addItem(new RectWidget(widget), 1, 1);
    QCOMPARE(layout->columnCount(), 2);
    layout->addItem(new RectWidget(widget), 0, 2);
    QCOMPARE(layout->columnCount(), 3);
    layout->addItem(new RectWidget(widget), 1, 0);
    QCOMPARE(layout->columnCount(), 3);
    layout->addItem(new RectWidget(widget), 0, 1);
    QCOMPARE(layout->columnCount(), 3);
    layout->addItem(new RectWidget(widget), 1, 2);
    QCOMPARE(layout->columnCount(), 3);

    // ### Talk with Jasmin. Not sure if removeAt() should adjust columnCount().
    widget->setLayout(0);
    layout = new QGraphicsGridLayout();
    populateLayout(layout, 3, 2, hasHeightForWidth);
    QCOMPARE(layout->columnCount(), 3);
    layout->removeAt(5);
    layout->removeAt(3);
    QCOMPARE(layout->columnCount(), 3);
    layout->removeAt(1);
    QCOMPARE(layout->columnCount(), 3);
    layout->removeAt(0);
    QCOMPARE(layout->columnCount(), 3);
    layout->removeAt(0);
    QCOMPARE(layout->columnCount(), 2);

    delete widget;
}

void tst_QGraphicsGridLayout::columnMaximumWidth_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}
// public qreal columnMaximumWidth(int column) const
void tst_QGraphicsGridLayout::columnMaximumWidth()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QCOMPARE(layout->minimumSize(), QSizeF(10+10+10, 10+10));
    QCOMPARE(layout->preferredSize(), QSizeF(25+25+25, 25+25));
    QCOMPARE(layout->maximumSize(), QSizeF(50+50+50, 50+50));

    // should at least be a very large number
    QVERIFY(layout->columnMaximumWidth(0) >= 10000);
    QCOMPARE(layout->columnMaximumWidth(0), layout->columnMaximumWidth(1));
    QCOMPARE(layout->columnMaximumWidth(1), layout->columnMaximumWidth(2));
    layout->setColumnMaximumWidth(0, 20);
    layout->setColumnMaximumWidth(2, 60);

    QCOMPARE(layout->minimumSize(), QSizeF(10+10+10, 10+10));
    QCOMPARE(layout->preferredSize(), QSizeF(20+25+25, 25+25));
    QCOMPARE(layout->maximumSize(), QSizeF(20+50+60, 50+50));
    QCOMPARE(layout->maximumSize(), widget->maximumSize());

    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    layout->activate();

    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0, 0, 20, 25));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0, 25, 20, 25));
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(20, 0, 25, 25));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(20, 25, 25, 25));
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(45, 0, 25, 25));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(45, 25, 25, 25));

    layout->setColumnAlignment(2, Qt::AlignCenter);
    widget->resize(widget->effectiveSizeHint(Qt::MaximumSize));
    layout->activate();
    QCOMPARE(layout->geometry(), QRectF(0,0,20+50+60, 50+50));
    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0, 0, 20, 50));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0, 50, 20, 50));
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(20, 0, 50, 50));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(20, 50, 50, 50));
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(75, 0, 50, 50));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(75, 50, 50, 50));

    for (int i = 0; i < layout->count(); i++)
        layout->setAlignment(layout->itemAt(i), Qt::AlignRight | Qt::AlignBottom);
    layout->activate();
    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0, 0, 20, 50));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0, 50, 20, 50));
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(20, 0, 50, 50));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(20, 50, 50, 50));
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(80, 0, 50, 50));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(80, 50, 50, 50));
    for (int i = 0; i < layout->count(); i++)
        layout->setAlignment(layout->itemAt(i), Qt::AlignCenter);

    layout->setMaximumSize(layout->maximumSize() + QSizeF(60,60));
    widget->resize(widget->effectiveSizeHint(Qt::MaximumSize));
    layout->activate();

    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0, 15, 20, 50));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0, 95, 20, 50));
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(20+30, 15, 50, 50));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(20+30, 95, 50, 50));
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(20+60+50+5, 15, 50, 50));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(20+60+50+5, 95, 50, 50));

    layout->setMaximumSize(layout->preferredSize() + QSizeF(20,20));
    widget->resize(widget->effectiveSizeHint(Qt::MaximumSize));
    layout->activate();

    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(0, 0, 20, 35));
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0, 35, 20, 35));
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(20, 0, 35, 35));
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(20, 35, 35, 35));
    QCOMPARE(layout->itemAt(0,2)->geometry(), QRectF(55, 0, 35, 35));
    QCOMPARE(layout->itemAt(1,2)->geometry(), QRectF(55, 35, 35, 35));

    delete widget;
}

void tst_QGraphicsGridLayout::columnMinimumWidth_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}
// public qreal columnMinimumWidth(int column) const
void tst_QGraphicsGridLayout::columnMinimumWidth()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // should at least be a very large number
    QCOMPARE(layout->columnMinimumWidth(0), 0.0);
    QCOMPARE(layout->columnMinimumWidth(0), layout->columnMinimumWidth(1));
    QCOMPARE(layout->columnMinimumWidth(1), layout->columnMinimumWidth(2));
    layout->setColumnMinimumWidth(0, 20);
    layout->setColumnMinimumWidth(2, 40);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(0,2)->geometry().width(), 40.0);
    QCOMPARE(layout->itemAt(1,2)->geometry().width(), 40.0);

    delete widget;
}

void tst_QGraphicsGridLayout::columnPreferredWidth_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}
// public qreal columnPreferredWidth(int column) const
void tst_QGraphicsGridLayout::columnPreferredWidth()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // default preferred width ??
    QCOMPARE(layout->columnPreferredWidth(0), 0.0);
    QCOMPARE(layout->columnPreferredWidth(0), layout->columnPreferredWidth(1));
    QCOMPARE(layout->columnPreferredWidth(1), layout->columnPreferredWidth(2));
    layout->setColumnPreferredWidth(0, 20);
    layout->setColumnPreferredWidth(2, 40);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(0,2)->geometry().width(), 40.0);
    QCOMPARE(layout->itemAt(1,2)->geometry().width(), 40.0);

    delete widget;
}

// public void setColumnFixedWidth(int row, qreal height)
void tst_QGraphicsGridLayout::setColumnFixedWidth()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->setColumnFixedWidth(0, 20);
    layout->setColumnFixedWidth(2, 40);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().width(), 20.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().width(), 20.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().width(), 25.0);
    QCOMPARE(layout->itemAt(0,2)->geometry().width(), 40.0);
    QCOMPARE(layout->itemAt(1,2)->geometry().width(), 40.0);

    delete widget;
}

// public qreal columnSpacing(int column) const
void tst_QGraphicsGridLayout::columnSpacing()
{
    {
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
        QGraphicsGridLayout *layout = new QGraphicsGridLayout();
        scene.addItem(widget);
        widget->setLayout(layout);
        populateLayout(layout, 3, 2);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        QCOMPARE(layout->columnSpacing(0), 0.0);

        layout->setColumnSpacing(0, 20);
        view.show();
        widget->show();
        widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
        QApplication::processEvents();

        QCOMPARE(layout->itemAt(0,0)->geometry().left(),   0.0);
        QCOMPARE(layout->itemAt(0,0)->geometry().right(), 25.0);
        QCOMPARE(layout->itemAt(0,1)->geometry().left(),  45.0);
        QCOMPARE(layout->itemAt(0,1)->geometry().right(), 70.0);
        QCOMPARE(layout->itemAt(0,2)->geometry().left(),  70.0);
        QCOMPARE(layout->itemAt(0,2)->geometry().right(), 95.0);

        delete widget;
    }

    {
        // don't include items and spacings that was previously part of the layout
        // (horizontal)
        QGraphicsGridLayout *layout = new QGraphicsGridLayout;
        populateLayout(layout, 3, 1);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setColumnSpacing(0, 10);
        layout->setColumnSpacing(1, 10);
        layout->setColumnSpacing(2, 10);
        layout->setColumnSpacing(3, 10);
        QCOMPARE(layout->preferredSize(), QSizeF(95, 25));
        layout->removeAt(2);
        QCOMPARE(layout->preferredSize(), QSizeF(60, 25));
        layout->removeAt(1);
        QCOMPARE(layout->preferredSize(), QSizeF(25, 25));
        delete layout;
    }
    {
        // don't include items and spacings that was previously part of the layout
        // (vertical)
        QGraphicsGridLayout *layout = new QGraphicsGridLayout;
        populateLayout(layout, 2, 2);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setColumnSpacing(0, 10);
        layout->setColumnSpacing(1, 10);
        layout->setRowSpacing(0, 10);
        layout->setRowSpacing(1, 10);
        QCOMPARE(layout->preferredSize(), QSizeF(60, 60));
        layout->removeAt(3);
        QCOMPARE(layout->preferredSize(), QSizeF(60, 60));
        layout->removeAt(2);
        QCOMPARE(layout->preferredSize(), QSizeF(60, 25));
        layout->removeAt(1);
        QCOMPARE(layout->preferredSize(), QSizeF(25, 25));
        delete layout;
    }

}

// public int columnStretchFactor(int column) const
void tst_QGraphicsGridLayout::columnStretchFactor()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->setColumnStretchFactor(0, 1);
    layout->setColumnStretchFactor(1, 2);
    layout->setColumnStretchFactor(2, 3);
    view.show();
    widget->show();
    widget->resize(130, 50);
    QApplication::processEvents();

    QVERIFY(layout->itemAt(0,0)->geometry().width() <  layout->itemAt(0,1)->geometry().width());
    QVERIFY(layout->itemAt(0,1)->geometry().width() <  layout->itemAt(0,2)->geometry().width());
    QVERIFY(layout->itemAt(1,0)->geometry().width() <  layout->itemAt(1,1)->geometry().width());
    QVERIFY(layout->itemAt(1,1)->geometry().width() <  layout->itemAt(1,2)->geometry().width());

    delete widget;
}


// public int count() const
void tst_QGraphicsGridLayout::count()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    QCOMPARE(layout->count(), 6);
    layout->removeAt(5);
    layout->removeAt(3);
    QCOMPARE(layout->count(), 4);
    layout->removeAt(1);
    QCOMPARE(layout->count(), 3);
    layout->removeAt(0);
    QCOMPARE(layout->count(), 2);
    layout->removeAt(0);
    QCOMPARE(layout->count(), 1);
    layout->removeAt(0);
    QCOMPARE(layout->count(), 0);

    delete widget;
}

void tst_QGraphicsGridLayout::horizontalSpacing_data()
{
    QTest::addColumn<qreal>("horizontalSpacing");
    QTest::newRow("zero") << qreal(0.0);
    QTest::newRow("10") << qreal(10.0);
}

// public qreal horizontalSpacing() const
void tst_QGraphicsGridLayout::horizontalSpacing()
{
    QFETCH(qreal, horizontalSpacing);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    layout->setContentsMargins(0, 0, 0, 0);
    qreal w = layout->sizeHint(Qt::PreferredSize, QSizeF()).width();
    qreal oldSpacing = layout->horizontalSpacing();

    // The remainder of this test is only applicable if the current style uses uniform layout spacing
    if (oldSpacing != -1) {
        layout->setHorizontalSpacing(horizontalSpacing);
        QApplication::processEvents();
        qreal new_w = layout->sizeHint(Qt::PreferredSize, QSizeF()).width();
        QCOMPARE(new_w, w - (3-1)*(oldSpacing - horizontalSpacing));
    }
    delete widget;
}

void tst_QGraphicsGridLayout::contentsMargins()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    QGraphicsGridLayout *sublayout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    layout->addItem(sublayout,0, 1);

    qreal left, top, right, bottom;
    // sublayouts have 0 margin
    sublayout->getContentsMargins(&left, &top, &right, &bottom);
    QCOMPARE(left, 0.0);
    QCOMPARE(top, 0.0);
    QCOMPARE(right, 0.0);
    QCOMPARE(bottom, 0.0);

    // top level layouts have style dependent margins.
    // we'll just check if its different from 0. (applies to all our styles)
    layout->getContentsMargins(&left, &top, &right, &bottom);
    QVERIFY(left >= 0.0);
    QVERIFY(top >= 0.0);
    QVERIFY(right >= 0.0);
    QVERIFY(bottom >= 0.0);

    delete widget;
}

// public QGraphicsLayoutItem* itemAt(int index) const
void tst_QGraphicsGridLayout::itemAt()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayoutWithSpansAndHoles(layout);

    //itemAt(int row, int column)
    QVERIFY( layout->itemAt(0,0));
    QVERIFY( layout->itemAt(0,1));
    QCOMPARE(layout->itemAt(0,2), static_cast<QGraphicsLayoutItem*>(0));
    QCOMPARE(layout->itemAt(1,0), static_cast<QGraphicsLayoutItem*>(0));
    QVERIFY( layout->itemAt(1,1));
    QVERIFY( layout->itemAt(1,2));


    //itemAt(int index)
    for (int i = -2; i < layout->count() + 2; ++i) {
        if (i >= 0 && i < layout->count()) {
            QVERIFY(layout->itemAt(i));
        } else {
            const QByteArray message = "QGraphicsGridLayout::itemAt: invalid index " + QByteArray::number(i);
            QTest::ignoreMessage(QtWarningMsg, message.constData());
            QCOMPARE(layout->itemAt(i), nullptr);
        }
    }
    delete widget;
}

// public void removeAt(int index)
void tst_QGraphicsGridLayout::removeAt()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    QCOMPARE(layout->count(), 6);
    layout->removeAt(5);
    layout->removeAt(3);
    QCOMPARE(layout->count(), 4);
    layout->removeAt(1);
    QCOMPARE(layout->count(), 3);
    layout->removeAt(0);
    QCOMPARE(layout->count(), 2);
    layout->removeAt(0);
    QCOMPARE(layout->count(), 1);
    QGraphicsLayoutItem *item0 = layout->itemAt(0);
    QCOMPARE(item0->parentLayoutItem(), static_cast<QGraphicsLayoutItem *>(layout));
    layout->removeAt(0);
    QCOMPARE(item0->parentLayoutItem(), nullptr);
    QCOMPARE(layout->count(), 0);
    QTest::ignoreMessage(QtWarningMsg, QString::fromLatin1("QGraphicsGridLayout::removeAt: invalid index 0").toLatin1().constData());
    layout->removeAt(0);
    QCOMPARE(layout->count(), 0);
    delete widget;
}

void tst_QGraphicsGridLayout::removeItem()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    scene.addItem(widget);
    QGraphicsGridLayout *l = new QGraphicsGridLayout();
    widget->setLayout(l);

    populateLayout(l, 3, 2);
    QCOMPARE(l->count(), 6);
    l->removeItem(l->itemAt(5));
    l->removeItem(l->itemAt(4));
    QCOMPARE(l->count(), 4);

    // Avoid crashing. Note that the warning message might change in the future.
    QTest::ignoreMessage(QtWarningMsg, QString::fromLatin1("QGraphicsGridLayout::removeAt: invalid index -1").toLatin1().constData());
    l->removeItem(0);
    QCOMPARE(l->count(), 4);

    QTest::ignoreMessage(QtWarningMsg, QString::fromLatin1("QGraphicsGridLayout::removeAt: invalid index -1").toLatin1().constData());
    l->removeItem(new QGraphicsWidget);
    QCOMPARE(l->count(), 4);
}

void tst_QGraphicsGridLayout::rowAlignment_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

// public Qt::Alignment rowAlignment(int row) const
void tst_QGraphicsGridLayout::rowAlignment()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    widget->setContentsMargins(0, 0, 0, 0);

    view.resize(330,450);
    widget->resize(300, 400);
    view.show();
    widget->show();
    QApplication::sendPostedEvents(0, 0);
    // Check default
    QCOMPARE(layout->rowAlignment(0), 0);
    QCOMPARE(layout->rowAlignment(1), 0);
    QCOMPARE(layout->rowAlignment(2), 0);

    // make the grids larger than the items, so that alignment kicks in
    layout->setRowMinimumHeight(0, 100.0);
    layout->setRowMinimumHeight(1, 100.0);
    layout->setRowMinimumHeight(2, 100.0);
    // expand columns also, so we can test combination of horiz and vertical alignment
    layout->setColumnMinimumWidth(0, 100.0);
    layout->setColumnMinimumWidth(1, 100.0);

    layout->setRowAlignment(0, Qt::AlignBottom);
    layout->setRowAlignment(1, Qt::AlignVCenter);
    layout->setRowAlignment(2, Qt::AlignTop);

    // see if item alignment takes preference over rowAlignment
    layout->setAlignment(layout->itemAt(0,0), Qt::AlignRight);
    layout->setAlignment(layout->itemAt(1,0), Qt::AlignTop);
    layout->setAlignment(layout->itemAt(2,0), Qt::AlignHCenter);

    QApplication::sendPostedEvents(0, 0);   // process LayoutRequest

    QCOMPARE(layout->alignment(layout->itemAt(0,0)), Qt::AlignRight);  //Qt::AlignRight | Qt::AlignBottom
    QCOMPARE(layout->itemAt(0,0)->geometry(), QRectF(50,  50,  50,  50));
    QCOMPARE(layout->rowAlignment(0), Qt::AlignBottom);
    QCOMPARE(layout->itemAt(0,1)->geometry(), QRectF(101, 50,  50,  50));
    QCOMPARE(layout->alignment(layout->itemAt(1,0)), Qt::AlignTop);
    QCOMPARE(layout->itemAt(1,0)->geometry(), QRectF(0,  101,  50,  50));
    QCOMPARE(layout->rowAlignment(1), Qt::AlignVCenter);
    QCOMPARE(layout->itemAt(1,1)->geometry(), QRectF(101, 126, 50,  50));
    QCOMPARE(layout->alignment(layout->itemAt(2,0)), Qt::AlignHCenter);
    QCOMPARE(layout->itemAt(2,0)->geometry(), QRectF(25, 202,  50,  50));
    QCOMPARE(layout->rowAlignment(2), Qt::AlignTop);
    QCOMPARE(layout->itemAt(2,1)->geometry(), QRectF(101,202,  50,  50));

    delete widget;
}

void tst_QGraphicsGridLayout::rowCount_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

// public int rowCount() const
// public int columnCount() const
void tst_QGraphicsGridLayout::rowCount()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    widget->setContentsMargins(0, 0, 0, 0);
    QCOMPARE(layout->rowCount(), 3);
    QCOMPARE(layout->columnCount(), 2);

    // with spans and holes...
    widget->setLayout(0);
    layout = new QGraphicsGridLayout();
    populateLayoutWithSpansAndHoles(layout, hasHeightForWidth);
    QCOMPARE(layout->rowCount(), 2);
    QCOMPARE(layout->columnCount(), 3);

    delete widget;
}

void tst_QGraphicsGridLayout::rowMaximumHeight_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

// public qreal rowMaximumHeight(int row) const
void tst_QGraphicsGridLayout::rowMaximumHeight()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // should at least be a very large number
    QVERIFY(layout->rowMaximumHeight(0) >= 10000);
    QCOMPARE(layout->rowMaximumHeight(0), layout->rowMaximumHeight(1));
    QCOMPARE(layout->rowMaximumHeight(1), layout->rowMaximumHeight(2));
    layout->setRowMaximumHeight(0, 20);
    layout->setRowMaximumHeight(2, 60);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().height(), 20.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().height(), 20.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(2,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(2,1)->geometry().height(), 25.0);

    delete widget;
}

void tst_QGraphicsGridLayout::rowMinimumHeight_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}
// public qreal rowMinimumHeight(int row) const
void tst_QGraphicsGridLayout::rowMinimumHeight()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // should at least be a very large number
    QCOMPARE(layout->rowMinimumHeight(0), 0.0);
    QCOMPARE(layout->rowMinimumHeight(0), layout->rowMinimumHeight(1));
    QCOMPARE(layout->rowMinimumHeight(1), layout->rowMinimumHeight(2));
    layout->setRowMinimumHeight(0, 20);
    layout->setRowMinimumHeight(2, 40);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(2,0)->geometry().height(), 40.0);
    QCOMPARE(layout->itemAt(2,1)->geometry().height(), 40.0);

    delete widget;
}

void tst_QGraphicsGridLayout::rowPreferredHeight_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}
// public qreal rowPreferredHeight(int row) const
void tst_QGraphicsGridLayout::rowPreferredHeight()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // default preferred height ??
    QCOMPARE(layout->rowPreferredHeight(0), 0.0);
    QCOMPARE(layout->rowPreferredHeight(0), layout->rowPreferredHeight(1));
    QCOMPARE(layout->rowPreferredHeight(1), layout->rowPreferredHeight(2));
    layout->setRowPreferredHeight(0, 20);
    layout->setRowPreferredHeight(2, 40);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    // ### Jasmin: Should rowPreferredHeight have precedence over sizeHint(Qt::PreferredSize) ?
    QCOMPARE(layout->itemAt(0,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(2,0)->geometry().height(), 40.0);
    QCOMPARE(layout->itemAt(2,1)->geometry().height(), 40.0);

    delete widget;
}

// public void setRowFixedHeight(int row, qreal height)
void tst_QGraphicsGridLayout::setRowFixedHeight()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->setRowFixedHeight(0, 20.);
    layout->setRowFixedHeight(2, 40.);

    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().height(), 20.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().height(), 20.0);
    QCOMPARE(layout->itemAt(1,0)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(1,1)->geometry().height(), 25.0);
    QCOMPARE(layout->itemAt(2,0)->geometry().height(), 40.0);
    QCOMPARE(layout->itemAt(2,1)->geometry().height(), 40.0);

    delete widget;
}

// public qreal rowSpacing(int row) const
void tst_QGraphicsGridLayout::rowSpacing()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    QCOMPARE(layout->columnSpacing(0), 0.0);

    layout->setColumnSpacing(0, 20);
    view.show();
    widget->show();
    widget->resize(widget->effectiveSizeHint(Qt::PreferredSize));
    QApplication::processEvents();

    QCOMPARE(layout->itemAt(0,0)->geometry().left(),   0.0);
    QCOMPARE(layout->itemAt(0,0)->geometry().right(), 25.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().left(),  45.0);
    QCOMPARE(layout->itemAt(0,1)->geometry().right(), 70.0);
    QCOMPARE(layout->itemAt(0,2)->geometry().left(),  70.0);
    QCOMPARE(layout->itemAt(0,2)->geometry().right(), 95.0);

    delete widget;

}

void tst_QGraphicsGridLayout::rowStretchFactor_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

// public int rowStretchFactor(int row) const
void tst_QGraphicsGridLayout::rowStretchFactor()
{
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 2, 3, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->setRowStretchFactor(0, 1);
    layout->setRowStretchFactor(1, 2);
    layout->setRowStretchFactor(2, 3);
    view.show();
    widget->show();
    widget->resize(50, 130);
    QApplication::processEvents();

    QVERIFY(layout->itemAt(0,0)->geometry().height() <  layout->itemAt(1,0)->geometry().height());
    QVERIFY(layout->itemAt(1,0)->geometry().height() <  layout->itemAt(2,0)->geometry().height());
    QVERIFY(layout->itemAt(0,1)->geometry().height() <  layout->itemAt(1,1)->geometry().height());
    QVERIFY(layout->itemAt(1,1)->geometry().height() <  layout->itemAt(2,1)->geometry().height());

    delete widget;
}

void tst_QGraphicsGridLayout::setColumnSpacing_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<qreal>("spacing");
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("null") << 0 << qreal(0.0) << false;
    QTest::newRow("10") << 0 << qreal(10.0) << false;
    QTest::newRow("null, hasHeightForWidth") << 0 << qreal(0.0) << true;
    QTest::newRow("10, hasHeightForWidth") << 0 << qreal(10.0) << true;
}

// public void setColumnSpacing(int column, qreal spacing)
void tst_QGraphicsGridLayout::setColumnSpacing()
{
    QFETCH(int, column);
    QFETCH(qreal, spacing);
    QFETCH(bool, hasHeightForWidth);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    qreal oldSpacing = layout->columnSpacing(column);
    QCOMPARE(oldSpacing, 0.0);
    qreal w = layout->sizeHint(Qt::PreferredSize, QSizeF()).width();
    layout->setColumnSpacing(column, spacing);
    QApplication::processEvents();
    QCOMPARE(layout->sizeHint(Qt::PreferredSize, QSizeF()).width(), w + spacing);
}

void tst_QGraphicsGridLayout::setGeometry_data()
{
    QTest::addColumn<QRectF>("rect");
    QTest::newRow("null") << QRectF();
    QTest::newRow("normal") << QRectF(0,0, 50, 50);
}

// public void setGeometry(QRectF const& rect)
void tst_QGraphicsGridLayout::setGeometry()
{
    QFETCH(QRectF, rect);

    QGraphicsWidget *window = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    window->setLayout(layout);
    QGraphicsGridLayout *layout2 = new QGraphicsGridLayout();
    layout2->setMaximumSize(100, 100);
    layout->addItem(layout2, 0, 0);
    layout2->setGeometry(rect);
    QCOMPARE(layout2->geometry(), rect);
}

void tst_QGraphicsGridLayout::setRowSpacing_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<qreal>("spacing");
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("null") << 0 << qreal(0.0) << false;
    QTest::newRow("10") << 0 << qreal(10.0) << false;
    QTest::newRow("null, hasHeightForWidth") << 0 << qreal(0.0) << true;
    QTest::newRow("10, hasHeightForWidth") << 0 << qreal(10.0) << true;
}

// public void setRowSpacing(int row, qreal spacing)
void tst_QGraphicsGridLayout::setRowSpacing()
{
    QFETCH(int, row);
    QFETCH(qreal, spacing);
    QFETCH(bool, hasHeightForWidth);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    qreal oldSpacing = layout->rowSpacing(row);
    QCOMPARE(oldSpacing, 0.0);
    qreal h = layout->sizeHint(Qt::PreferredSize, QSizeF()).height();
    layout->setRowSpacing(row, spacing);
    QApplication::processEvents();
    QCOMPARE(layout->sizeHint(Qt::PreferredSize, QSizeF()).height(), h + spacing);
}

void tst_QGraphicsGridLayout::setSpacing_data()
{
    QTest::addColumn<qreal>("spacing");
    QTest::addColumn<bool>("hasHeightForWidth");
    QTest::newRow("zero") << qreal(0.0) << false;
    QTest::newRow("17") << qreal(17.0) << false;
    QTest::newRow("zero, hasHeightForWidth") << qreal(0.0) << true;
    QTest::newRow("17, hasHeightForWidth") << qreal(17.0) << true;
}

// public void setSpacing(qreal spacing)
void tst_QGraphicsGridLayout::setSpacing()
{
    QFETCH(qreal, spacing);
    QFETCH(bool, hasHeightForWidth);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2, hasHeightForWidth);
    layout->setContentsMargins(0, 0, 0, 0);
    QSizeF sh = layout->sizeHint(Qt::PreferredSize, QSizeF());
    qreal oldVSpacing = layout->verticalSpacing();
    qreal oldHSpacing = layout->horizontalSpacing();

    // The remainder of this test is only applicable if the current style uses uniform layout spacing
    if (oldVSpacing != -1) {
        layout->setSpacing(spacing);
        QApplication::processEvents();
        QSizeF newSH = layout->sizeHint(Qt::PreferredSize, QSizeF());
        QCOMPARE(newSH.height(), sh.height() - (2-1)*(oldVSpacing - spacing));
        QCOMPARE(newSH.width(), sh.width() - (3-1)*(oldHSpacing - spacing));
    }
    delete widget;
}

void tst_QGraphicsGridLayout::sizeHint_data()
{
    QTest::addColumn<ItemList>("itemDescriptions");
    QTest::addColumn<QSizeF>("expectedMinimumSizeHint");
    QTest::addColumn<QSizeF>("expectedPreferredSizeHint");
    QTest::addColumn<QSizeF>("expectedMaximumSizeHint");

    QTest::newRow("rowSpan_larger_than_rows") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(50,300))
                                        .maxSize(QSizeF(50,300))
                                        .rowSpan(2)
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(50,0))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSize(50, 1000))
                                    << ItemDesc(1,1)
                                        .minSize(QSizeF(50,0))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSize(50, 1000))
                                )
                            << QSizeF(100, 300)
                            << QSizeF(100, 300)
                            << QSizeF(100, 2000);

    QTest::newRow("rowSpan_smaller_than_rows") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(50, 0))
                                        .preferredSize(QSizeF(50, 50))
                                        .maxSize(QSizeF(50, 300))
                                        .rowSpan(2)
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(50, 50))
                                        .preferredSize(QSizeF(50, 50))
                                        .maxSize(QSize(50, 50))
                                    << ItemDesc(1,1)
                                        .minSize(QSizeF(50, 50))
                                        .preferredSize(QSizeF(50, 50))
                                        .maxSize(QSize(50, 50))
                                )
                            << QSizeF(100, 100)
                            << QSizeF(100, 100)
                            << QSizeF(100, 100);

    QTest::newRow("colSpan_with_ignored_column") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(40,20))
                                        .maxSize(QSizeF(60,20))
                                        .colSpan(2)
                                    << ItemDesc(0,2)
                                        .minSize(QSizeF(20, 20))
                                        .maxSize(QSizeF(30, 20))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(60, 20))
                                        .maxSize(QSizeF(90, 20))
                                        .colSpan(3)
                                )
                            << QSizeF(60, 40)
                            << QSizeF(80, 40)
                            << QSizeF(90, 40);

}

// public QSizeF sizeHint(Qt::SizeHint which, QSizeF const& constraint = QSizeF()) const
void tst_QGraphicsGridLayout::sizeHint()
{
    QFETCH(ItemList, itemDescriptions);
    QFETCH(QSizeF, expectedMinimumSizeHint);
    QFETCH(QSizeF, expectedPreferredSizeHint);
    QFETCH(QSizeF, expectedMaximumSizeHint);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    scene.addItem(widget);
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0.0);
    widget->setContentsMargins(0, 0, 0, 0);

    int i;
    for (i = 0; i < itemDescriptions.count(); ++i) {
        ItemDesc desc = itemDescriptions.at(i);
        RectWidget *item = new RectWidget(widget);
        desc.apply(layout, item);
    }

    QApplication::sendPostedEvents(0, 0);

    widget->show();
    view.show();
    view.resize(400,300);
    QCOMPARE(layout->sizeHint(Qt::MinimumSize), expectedMinimumSizeHint);
    QCOMPARE(layout->sizeHint(Qt::PreferredSize), expectedPreferredSizeHint);
    QCOMPARE(layout->sizeHint(Qt::MaximumSize), expectedMaximumSizeHint);

}

void tst_QGraphicsGridLayout::verticalSpacing_data()
{
    QTest::addColumn<qreal>("verticalSpacing");
    QTest::newRow("zero") << qreal(0.0);
    QTest::newRow("17") << qreal(10.0);
}

// public qreal verticalSpacing() const
void tst_QGraphicsGridLayout::verticalSpacing()
{
    QFETCH(qreal, verticalSpacing);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout();
    scene.addItem(widget);
    widget->setLayout(layout);
    populateLayout(layout, 3, 2);
    layout->setContentsMargins(0, 0, 0, 0);
    qreal h = layout->sizeHint(Qt::PreferredSize, QSizeF()).height();
    qreal oldSpacing = layout->verticalSpacing();

    // The remainder of this test is only applicable if the current style uses uniform layout spacing
    if (oldSpacing != -1) {
        layout->setVerticalSpacing(verticalSpacing);
        QApplication::processEvents();
        qreal new_h = layout->sizeHint(Qt::PreferredSize, QSizeF()).height();
        QCOMPARE(new_h, h - (2-1)*(oldSpacing - verticalSpacing));
    }
    delete widget;
}

void tst_QGraphicsGridLayout::layoutDirection_data()
{
    QTest::addColumn<bool>("hasHeightForWidth");

    QTest::newRow("") << false;
    QTest::newRow("hasHeightForWidth") << true;
}

void tst_QGraphicsGridLayout::layoutDirection()
{
    QFETCH(bool, hasHeightForWidth);

    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWidget *window = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    layout->setContentsMargins(1, 2, 3, 4);
    layout->setSpacing(6);
    RectWidget *w1 = new RectWidget;
    w1->setMinimumSize(30, 20);
    layout->addItem(w1, 0, 0);
    RectWidget *w2 = new RectWidget;
    w2->setMinimumSize(20, 20);
    w2->setMaximumSize(20, 20);
    layout->addItem(w2, 0, 1);
    RectWidget *w3 = new RectWidget;
    w3->setMinimumSize(20, 20);
    w3->setMaximumSize(20, 20);
    layout->addItem(w3, 1, 0);
    RectWidget *w4 = new RectWidget;
    w4->setMinimumSize(30, 20);
    layout->addItem(w4, 1, 1);

    QSizePolicy policy = w1->sizePolicy();
    policy.setHeightForWidth(hasHeightForWidth);
    w1->setSizePolicy(policy);
    w2->setSizePolicy(policy);
    w4->setSizePolicy(policy);

    layout->setAlignment(w2, Qt::AlignRight);
    layout->setAlignment(w3, Qt::AlignLeft);

    scene.addItem(window);
    window->setLayout(layout);
    view.show();
    window->resize(70, 52);
    QApplication::processEvents();
    QCOMPARE(w1->geometry().left(), 1.0);
    QCOMPARE(w1->geometry().right(), 31.0);
    QCOMPARE(w2->geometry().left(), 47.0);
    QCOMPARE(w2->geometry().right(), 67.0);
    QCOMPARE(w3->geometry().left(), 1.0);
    QCOMPARE(w3->geometry().right(), 21.0);
    QCOMPARE(w4->geometry().left(), 37.0);
    QCOMPARE(w4->geometry().right(), 67.0);

    window->setLayoutDirection(Qt::RightToLeft);
    QApplication::processEvents();
    QCOMPARE(w1->geometry().left(),  39.0);
    QCOMPARE(w1->geometry().right(), 69.0);
    QCOMPARE(w2->geometry().left(),   3.0);
    QCOMPARE(w2->geometry().right(), 23.0);
    QCOMPARE(w3->geometry().left(),  49.0);
    QCOMPARE(w3->geometry().right(), 69.0);
    QCOMPARE(w4->geometry().left(),   3.0);
    QCOMPARE(w4->geometry().right(), 33.0);

    delete window;
}

void tst_QGraphicsGridLayout::removeLayout()
{
    QGraphicsScene scene;
    RectWidget *textEdit = new RectWidget;
    RectWidget *pushButton = new RectWidget;
    scene.addItem(textEdit);
    scene.addItem(pushButton);

    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    layout->addItem(textEdit, 0, 0);
    layout->addItem(pushButton, 1, 0);

    QGraphicsWidget *form = new QGraphicsWidget;
    form->setLayout(layout);
    scene.addItem(form);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QRectF r1 = textEdit->geometry();
    QRectF r2 = pushButton->geometry();
    form->setLayout(0);
    //documentation of QGraphicsWidget::setLayout:
    //If layout is 0, the widget is left without a layout. Existing subwidgets' geometries will remain unaffected.
    QCOMPARE(textEdit->geometry(), r1);
    QCOMPARE(pushButton->geometry(), r2);
}

void tst_QGraphicsGridLayout::defaultStretchFactors_data()
{
    QTest::addColumn<ItemList>("itemDescriptions");
    QTest::addColumn<QSizeF>("newSize");
    QTest::addColumn<SizeList>("expectedSizes");

    QTest::newRow("usepreferredsize") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(10,10)
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("preferredsizeIsZero") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(0,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .maxSize(QSizeF(20, 10))
                                )
                            << QSizeF(30, 10)
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(20,10)
                            );

    QTest::newRow("ignoreitem01") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(10,10)
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("ignoreitem01_resize120x40") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(20,10))
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(30,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(20,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(30,10))
                                )
                            << QSizeF(120, 40)
                            << (SizeList()
                                << QSizeF(20,20) << QSizeF(40,20) << QSizeF(60,20)
                                << QSizeF(20,20) << QSizeF(40,20) << QSizeF(60,20)
                            );

    QTest::newRow("ignoreitem11_resize120x40") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(20,10))
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(30,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                    << ItemDesc(1,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(20,20))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(30,20))
                                )
                            << QSizeF(120, 60)
                            << (SizeList()
                                << QSizeF(20,20) << QSizeF(40,20) << QSizeF(60,20)
                                << QSizeF(20,40) << QSizeF(40,40) << QSizeF(60,40)
                            );

    QTest::newRow("ignoreitem01_span01_resize70x60") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(20,10))
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .rowSpan(2)
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(30,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(30,20))
                                )
                            << QSizeF(70, 60)
                            << (SizeList()
                                << QSizeF(20,20) << QSizeF(10,60) << QSizeF(40,20)
                                << QSizeF(20,40) << QSizeF(40,40)
                            );

    QTest::newRow("ignoreitem10_resize40x120") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,20))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,20))
                                    << ItemDesc(2,0)
                                        .preferredSizeHint(QSizeF(10,30))
                                    << ItemDesc(2,1)
                                        .preferredSizeHint(QSizeF(10,30))
                                )
                            << QSizeF(40, 120)
                            << (SizeList()
                                << QSizeF(20,20) << QSizeF(20,20)
                                << QSizeF(20,40) << QSizeF(20,40)
                                << QSizeF(20,60) << QSizeF(20,60)
                            );

    QTest::newRow("ignoreitem01_span02") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .rowSpan(2)
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(0,20) << QSizeF(10,10)
                                << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("ignoreitem02_span02") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,2)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .rowSpan(2)
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(0,20)
                                << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("ignoreitem02_span00_span02") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .rowSpan(2)
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,2)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .rowSpan(2)
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,20) << QSizeF(10,10) << QSizeF(0,20)
                                << QSizeF(10,10)
                            );

    QTest::newRow("ignoreitem00_colspan00") << (ItemList()
                                    << ItemDesc(0,0)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .colSpan(2)
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(20,10) << QSizeF(10,10) << QSizeF(10,10)
                                << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("ignoreitem01_colspan01") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .colSpan(2)
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(20,10) << QSizeF(10,10)
                                << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("ignorecolumn1_resize70x60") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(20,10))
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(30,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                    << ItemDesc(1,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(20,20))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(30,20))
                                )
                            << QSizeF(70, 60)
                            << (SizeList()
                                << QSizeF(20,20) << QSizeF(10,20) << QSizeF(40,20)
                                << QSizeF(20,40) << QSizeF(10,40) << QSizeF(40,40)
                            );

    QTest::newRow("ignorerow0") << (ItemList()
                                    << ItemDesc(0,0)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,2)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,0) << QSizeF(10,0) << QSizeF(10,0)
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(10,10)
                            );

    QTest::newRow("ignorerow1") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,2)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,0)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(1,2)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                )
                            << QSizeF()
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(10,10) << QSizeF(10,10)
                                << QSizeF(10,0) << QSizeF(10,0) << QSizeF(10,0)
                            );

    QTest::newRow("ignorerow0_resize60x50") << (ItemList()
                                    << ItemDesc(0,0)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(10,10))
                                    << ItemDesc(0,1)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(20,10))
                                    << ItemDesc(0,2)
                                        .sizePolicy(QSizePolicy::Ignored)
                                        .preferredSizeHint(QSizeF(30,10))
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,30))
                                    << ItemDesc(1,1)
                                        .preferredSizeHint(QSizeF(20,30))
                                    << ItemDesc(1,2)
                                        .preferredSizeHint(QSizeF(30,30))
                                )
                            << QSizeF(60, 50)
                            << (SizeList()
                                << QSizeF(10,10) << QSizeF(20,10) << QSizeF(30,10)
                                << QSizeF(10,40) << QSizeF(20,40) << QSizeF(30,40)
                            );

}

void tst_QGraphicsGridLayout::defaultStretchFactors()
{
    QFETCH(ItemList, itemDescriptions);
    QFETCH(QSizeF, newSize);
    QFETCH(SizeList, expectedSizes);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    scene.addItem(widget);
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0.0);
    widget->setContentsMargins(0, 0, 0, 0);

    int i;
    for (i = 0; i < itemDescriptions.count(); ++i) {
        ItemDesc desc = itemDescriptions.at(i);
        RectWidget *item = new RectWidget(widget);
        desc.apply(layout, item);
    }

    QApplication::sendPostedEvents(0, 0);

    widget->show();
    view.show();
    view.resize(400,300);
    if (newSize.isValid())
        widget->resize(newSize);

    QApplication::sendPostedEvents(0, 0);
    for (i = 0; i < expectedSizes.count(); ++i) {
        QSizeF itemSize = layout->itemAt(i)->geometry().size();
        QCOMPARE(itemSize, expectedSizes.at(i));
    }

    delete widget;
}

typedef QList<QRectF> RectList;

void tst_QGraphicsGridLayout::alignment2_data()
{
    QTest::addColumn<ItemList>("itemDescriptions");
    QTest::addColumn<QSizeF>("newSize");
    QTest::addColumn<RectList>("expectedGeometries");

    QTest::newRow("hor_sizepolicy_fixed") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 10,20) << QRectF(10, 0, 10,10)
                            );

    QTest::newRow("hor_sizepolicy_fixed_alignvcenter") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                        .alignment(Qt::AlignVCenter)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 10,20) << QRectF(10, 5, 10,10)
                            );

    QTest::newRow("hor_sizepolicy_fixed_aligntop") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                        .alignment(Qt::AlignTop)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 10,20) << QRectF(10, 0, 10,10)
                            );

    QTest::newRow("hor_sizepolicy_fixed_alignbottom") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(10,20))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                    << ItemDesc(0,1)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyV(QSizePolicy::Fixed)
                                        .alignment(Qt::AlignBottom)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 10,20) << QRectF(10, 10, 10,10)
                            );

    QTest::newRow("ver_sizepolicy_fixed") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(20,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 20,10) << QRectF(0, 10, 10,10)
                            );

    QTest::newRow("ver_sizepolicy_fixed_alignhcenter") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(20,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                        .alignment(Qt::AlignHCenter)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 20,10) << QRectF(5, 10, 10,10)
                            );

    QTest::newRow("ver_sizepolicy_fixed_alignleft") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(20,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                        .alignment(Qt::AlignLeft)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 20,10) << QRectF(0, 10, 10,10)
                            );

    QTest::newRow("ver_sizepolicy_fixed_alignright") << (ItemList()
                                    << ItemDesc(0,0)
                                        .preferredSizeHint(QSizeF(20,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                    << ItemDesc(1,0)
                                        .preferredSizeHint(QSizeF(10,10))
                                        .sizePolicyH(QSizePolicy::Fixed)
                                        .alignment(Qt::AlignRight)
                                )
                            << QSizeF()
                            << (RectList()
                                << QRectF(0, 0, 20,10) << QRectF(10, 10, 10,10)
                            );
}

void tst_QGraphicsGridLayout::alignment2()
{
    QFETCH(ItemList, itemDescriptions);
    QFETCH(QSizeF, newSize);
    QFETCH(RectList, expectedGeometries);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    scene.addItem(widget);
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0.0);
    widget->setContentsMargins(0, 0, 0, 0);

    int i;
    for (i = 0; i < itemDescriptions.count(); ++i) {
        ItemDesc desc = itemDescriptions.at(i);
        RectWidget *item = new RectWidget(widget);
        desc.apply(layout, item);
    }

    QApplication::sendPostedEvents(0, 0);

    widget->show();
    view.resize(400,300);
    view.show();
    if (newSize.isValid())
        widget->resize(newSize);

    QApplication::sendPostedEvents(0, 0);
    for (i = 0; i < expectedGeometries.count(); ++i) {
        QRectF itemRect = layout->itemAt(i)->geometry();
        QCOMPARE(itemRect, expectedGeometries.at(i));
    }

    delete widget;
}

static QSizeF hfw1(Qt::SizeHint, const QSizeF &constraint)
{
    QSizeF result(constraint);
    const qreal ch = constraint.height();
    const qreal cw = constraint.width();
    if (cw < 0 && ch < 0) {
        return QSizeF(50, 400);
    } else if (cw > 0) {
        result.setHeight(20000./cw);
    }
    return result;
}

static QSizeF wfh1(Qt::SizeHint, const QSizeF &constraint)
{
    QSizeF result(constraint);
    const qreal ch = constraint.height();
    const qreal cw = constraint.width();
    if (cw < 0 && ch < 0) {
        return QSizeF(400, 50);
    } else if (ch > 0) {
        result.setWidth(20000./ch);
    }
    return result;
}

static QSizeF wfh2(Qt::SizeHint, const QSizeF &constraint)
{
    QSizeF result(constraint);
    const qreal ch = constraint.height();
    const qreal cw = constraint.width();
    if (ch < 0 && cw < 0)
        return QSizeF(50, 50);
    if (ch >= 0)
        result.setWidth(ch);
    return result;
}

static QSizeF hfw3(Qt::SizeHint, const QSizeF &constraint)
{
    QSizeF result(constraint);
    const qreal ch = constraint.height();
    const qreal cw = constraint.width();
    if (cw < 0 && ch < 0) {
        return QSizeF(10, 10);
    } else if (cw > 0) {
        result.setHeight(100./cw);
    }
    return result;
}

static QSizeF hfw2(Qt::SizeHint /*which*/, const QSizeF &constraint)
{
    return QSizeF(constraint.width(), constraint.width());
}

void tst_QGraphicsGridLayout::geometries_data()
{

    QTest::addColumn<ItemList>("itemDescriptions");
    QTest::addColumn<QSizeF>("newSize");
    QTest::addColumn<RectList>("expectedGeometries");

    QTest::newRow("combine_max_sizes") << (ItemList()
                                    << ItemDesc(0,0)
                                        .maxSize(QSizeF(50,10))
                                    << ItemDesc(1,0)
                                        .maxSize(QSizeF(10,10))
                                )
                            << QSizeF(50, 20)
                            << (RectList()
                                << QRectF(0, 0, 50,10) << QRectF(0, 10, 10,10)
                            );

    QTest::newRow("combine_min_sizes") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(50,10))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(10,10))
                                )
                            << QSizeF(60, 20)
                            << (RectList()
                                << QRectF(0, 0, 60,10) << QRectF(0, 10, 60,10)
                            );

    // change layout height and verify
    QTest::newRow("hfw-100x401") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .minSize(QSizeF(40,-1))
                                        .preferredSize(QSizeF(50,-1))
                                        .maxSize(QSizeF(500, -1))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 401)
                            << (RectList()
                                << QRectF(0, 0, 50,  1) << QRectF(50, 0, 50,  1)
                                << QRectF(0, 1, 50,100) << QRectF(50, 1, 50,400)
                            );
    QTest::newRow("hfw-h408") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(500, 500))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 408)
                            << (RectList()
                                << QRectF(0, 0, 50,  8) << QRectF(50,  0, 50,  8)
                                << QRectF(0, 8, 50,100) << QRectF(50,  8, 50,400)
                            );
    QTest::newRow("hfw-h410") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .minSize(QSizeF(40,40))
                                        .preferredSize(QSizeF(50,400))
                                        .maxSize(QSizeF(500, 500))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 410)
                            << (RectList()
                                << QRectF(0, 0, 50,10) << QRectF(50, 0, 50,10)
                                << QRectF(0, 10, 50,100) << QRectF(50, 10, 50,400)
                            );

    QTest::newRow("hfw-100x470") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(500,500))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 470)
                            << (RectList()
                                << QRectF(0, 0, 50,70) << QRectF(50, 0, 50,70)
                                << QRectF(0, 70, 50,100) << QRectF(50, 70, 50,400)
                            );

    // change layout width and verify
    QTest::newRow("hfw-100x401") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .minSize(QSizeF(-1,-1))
                                        .preferredSize(QSizeF(-1,-1))
                                        .maxSize(QSizeF(-1, -1))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 401)
                            << (RectList()
                                << QRectF( 0, 0,  50,   1) << QRectF( 50,  0,  50,   1)
                                << QRectF( 0, 1,  50, 100) << QRectF( 50,  1,  50, 400)
                            );

    QTest::newRow("hfw-160x350") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(5000,5000))
                                       .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(160, 350)
                            << (RectList()
                                << QRectF( 0,   0,  80, 100) << QRectF( 80,   0,  80, 100)
                                << QRectF( 0, 100,  80, 100) << QRectF( 80, 100,  80, 250)
                            );

    QTest::newRow("hfw-160x300") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(5000, 5000))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(160, 300)
                            << (RectList()
                                << QRectF( 0,   0,  80,  50) << QRectF( 80,   0,  80,  50)
                                << QRectF( 0,  50,  80, 100) << QRectF( 80,  50,  80, 250)
                            );

    QTest::newRow("hfw-20x40") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,10))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(1, 1))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50, 50))
                                        .sizeHint(Qt::MaximumSize, QSizeF(100, 100))
                                        .dynamicConstraint(hfw3, Qt::Vertical)
                                )
                            << QSizeF(20, 40)
                            << (RectList()
                                << QRectF(0,  0, 10, 20) << QRectF(10,  0, 10, 20)
                                << QRectF(0, 20, 10, 20) << QRectF(10, 20, 10, 10)
                            );

    QTest::newRow("wfh-300x160") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(10,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(10,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(10,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(10,10))
                                        .sizeHint(Qt::PreferredSize, QSizeF(400,50))
                                        .sizeHint(Qt::MaximumSize, QSizeF(5000, 5000))
                                        .dynamicConstraint(wfh1, Qt::Horizontal)
                                )
                            << QSizeF(300, 160)
                            << (RectList()
                                << QRectF( 0,   0,  50,  80) << QRectF( 50,   0, 100,  80)
                                << QRectF( 0,  80,  50,  80) << QRectF( 50,  80, 250,  80)
                            );

    QTest::newRow("wfh-40x20") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                    // Note, must be 10 in order to match stretching  of wfh item
                                    // below (the same stretch makes it easier to test)
                                        .minSize(QSizeF(10,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(1,1))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,50))
                                        .sizeHint(Qt::MaximumSize, QSizeF(100, 100))
                                        .dynamicConstraint(wfh2, Qt::Horizontal)
                                )
                            << QSizeF(40, 20)
                            << (RectList()
                                << QRectF(0,  0, 20, 10) << QRectF(20,  0, 20, 10)
                                << QRectF(0, 10, 20, 10) << QRectF(20, 10, 10, 10)
                            );

    QTest::newRow("wfh-400x160") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(1,1))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,50))
                                        .sizeHint(Qt::MaximumSize, QSizeF(100, 100))
                                        .dynamicConstraint(wfh2, Qt::Horizontal)
                                )

                            << QSizeF(400, 160)
                            << (RectList()
                                << QRectF(0,  0, 100, 80) << QRectF(100,  0, 100, 80)
                                << QRectF(0, 80, 100, 80) << QRectF(100, 80,  80, 80)
                            );

    QTest::newRow("wfh-160x100") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        // Note, preferred width must be 50 in order to match
                                        // preferred width of wfh item below.
                                        // (The same preferred size makes the stretch the same, and
                                        // makes it easier to test) (The stretch algorithm is a
                                        // blackbox)
                                        .preferredSize(QSizeF(50,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(10,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(10,50))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(1,1))
                                        .sizeHint(Qt::PreferredSize, QSizeF(10,50))
                                        .sizeHint(Qt::MaximumSize, QSizeF(500, 500))
                                        .dynamicConstraint(wfh2, Qt::Horizontal)
                                )
                            << QSizeF(160, 100)
                            << (RectList()
                                << QRectF(0,  0,  80,  50) << QRectF( 80,  0,  80,  50)
                                << QRectF(0, 50,  80,  50) << QRectF( 80, 50,  50,  50)
                            );

    QTest::newRow("hfw-h470") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(500,500))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 470)
                            << (RectList()
                                << QRectF(0, 0, 50,70) << QRectF(50, 0, 50,70)
                                << QRectF(0, 70, 50,100) << QRectF(50, 70, 50,400)
                            );

    // change layout width and verify
    QTest::newRow("hfw-w100") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(5000,5000))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(100, 401)
                            << (RectList()
                                << QRectF( 0, 0,  50,   1) << QRectF( 50,  0,  50,   1)
                                << QRectF( 0, 1,  50, 100) << QRectF( 50,  1,  50, 400)
                            );

    QTest::newRow("hfw-w160") << (ItemList()
                                     << ItemDesc(0,0)
                                         .minSize(QSizeF(1,1))
                                         .preferredSize(QSizeF(50,10))
                                         .maxSize(QSizeF(100, 100))
                                     << ItemDesc(0,1)
                                         .minSize(QSizeF(1,1))
                                         .preferredSize(QSizeF(50,10))
                                         .maxSize(QSizeF(100, 100))
                                     << ItemDesc(1,0)
                                         .minSize(QSizeF(1,1))
                                         .preferredSize(QSizeF(50,10))
                                         .maxSize(QSizeF(100, 100))
                                     << ItemDesc(1,1)
                                         .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                         .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                         .sizeHint(Qt::MaximumSize, QSizeF(5000,5000))
                                         .dynamicConstraint(hfw1, Qt::Vertical)
                                 )
                             << QSizeF(160, 350)
                             << (RectList()
                                 << QRectF( 0,   0,  80, 100) << QRectF( 80,   0,  80, 100)
                                 << QRectF( 0, 100,  80, 100) << QRectF( 80, 100,  80, 250)
                             );

    QTest::newRow("hfw-w500") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(0,1)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(1,1))
                                        .preferredSize(QSizeF(50,10))
                                        .maxSize(QSizeF(100, 100))
                                    << ItemDesc(1,1)
                                        .sizeHint(Qt::MinimumSize, QSizeF(40,40))
                                        .sizeHint(Qt::PreferredSize, QSizeF(50,400))
                                        .sizeHint(Qt::MaximumSize, QSizeF(5000,5000))
                                        .dynamicConstraint(hfw1, Qt::Vertical)
                                )
                            << QSizeF(500, 200)
                            << (RectList()
                                << QRectF( 0,   0, 100, 100) << QRectF(100,   0, 100, 100)
                                << QRectF( 0, 100, 100, 100) << QRectF(100, 100, 400,  50)
                            );

    QTest::newRow("hfw-alignment-defaults") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(100, 100))
                                        .maxSize(QSizeF(100, 100))
                                        .dynamicConstraint(hfw2, Qt::Vertical)
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(200, 200))
                                        .maxSize(QSizeF(200, 200))
                                        .dynamicConstraint(hfw2, Qt::Vertical)
                                    << ItemDesc(2,0)
                                        .minSize(QSizeF(300, 300))
                                        .maxSize(QSizeF(300, 300))
                                )
                            << QSizeF(300, 600)
                            << (RectList()
                                << QRectF(0, 0,   100, 100)
                                << QRectF(0, 100, 200, 200)
                                << QRectF(0, 300, 300, 300)
                            );

    QTest::newRow("hfw-alignment2") << (ItemList()
                                    << ItemDesc(0,0)
                                        .minSize(QSizeF(100, 100))
                                        .maxSize(QSizeF(100, 100))
                                        .dynamicConstraint(hfw2, Qt::Vertical)
                                        .alignment(Qt::AlignRight)
                                    << ItemDesc(1,0)
                                        .minSize(QSizeF(200, 200))
                                        .maxSize(QSizeF(200, 200))
                                        .dynamicConstraint(hfw2, Qt::Vertical)
                                        .alignment(Qt::AlignHCenter)
                                    << ItemDesc(2,0)
                                        .minSize(QSizeF(300, 300))
                                        .maxSize(QSizeF(300, 300))
                                )
                            << QSizeF(300, 600)
                            << (RectList()
                                << QRectF(200, 0,   100, 100)
                                << QRectF( 50, 100, 200, 200)
                                << QRectF(  0, 300, 300, 300)
                            );

}

void tst_QGraphicsGridLayout::geometries()
{
    QFETCH(ItemList, itemDescriptions);
    QFETCH(QSizeF, newSize);
    QFETCH(RectList, expectedGeometries);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    scene.addItem(widget);
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0.0);
    widget->setContentsMargins(0, 0, 0, 0);

    int i;
    for (i = 0; i < itemDescriptions.count(); ++i) {
        ItemDesc desc = itemDescriptions.at(i);
        RectWidget *item = new RectWidget(widget);
        desc.apply(layout, item);
    }

    QApplication::processEvents();

    widget->show();
    view.resize(400,300);
    view.show();
    if (newSize.isValid())
        widget->resize(newSize);

    QApplication::processEvents();
    for (i = 0; i < expectedGeometries.count(); ++i) {
        QRectF itemRect = layout->itemAt(i)->geometry();
        QCOMPARE(itemRect, expectedGeometries.at(i));
    }

    delete widget;
}

void tst_QGraphicsGridLayout::avoidRecursionInInsertItem()
{
    QGraphicsWidget window(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout(&window);
    QCOMPARE(layout->count(), 0);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsGridLayout::addItem: cannot insert itself");
    layout->addItem(layout, 0, 0);
    QCOMPARE(layout->count(), 0);
}

void tst_QGraphicsGridLayout::styleInfoLeak()
{
    QGraphicsGridLayout grid;
    grid.horizontalSpacing();
}

void tst_QGraphicsGridLayout::task236367_maxSizeHint()
{
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    int w = 203;
    int h = 204;
    widget->resize(w, h);
    QCOMPARE(widget->size(), QSizeF(w, h));
}

static QSizeF hfw(Qt::SizeHint /*which*/, const QSizeF &constraint)
{
    QSizeF result(constraint);
    const qreal cw = constraint.width();
    const qreal ch = constraint.height();
    if (cw < 0 && ch < 0) {
        return QSizeF(200, 100);
    } else if (cw >= 0) {
        result.setHeight(20000./cw);
    } else if (cw == 0) {
        result.setHeight(20000);
    } else if (ch >= 0) {
        result.setWidth(20000./ch);
    } else if (ch == 0) {
        result.setWidth(20000);
    }
    return result;
}

static QSizeF wfh(Qt::SizeHint /*which*/, const QSizeF &constraint)
{
    QSizeF result(constraint);
    const qreal ch = constraint.height();
    if (ch >= 0) {
        result.setWidth(ch);
    }
    return result;
}

bool qFuzzyCompare(const QSizeF &a, const QSizeF &b)
{
    return qFuzzyCompare(a.width(), b.width()) && qFuzzyCompare(a.height(), b.height());
}

void tst_QGraphicsGridLayout::heightForWidth()
{
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    RectWidget *w00 = new RectWidget;
    w00->setSizeHint(Qt::MinimumSize, QSizeF(1,1));
    w00->setSizeHint(Qt::PreferredSize, QSizeF(10,10));
    w00->setSizeHint(Qt::MaximumSize, QSizeF(100,100));
    layout->addItem(w00, 0, 0);

    RectWidget *w01 = new RectWidget;
    w01->setSizeHint(Qt::MinimumSize, QSizeF(1,1));
    w01->setSizeHint(Qt::PreferredSize, QSizeF(10,10));
    w01->setSizeHint(Qt::MaximumSize, QSizeF(100,100));
    layout->addItem(w01, 0, 1);

    RectWidget *w10 = new RectWidget;
    w10->setSizeHint(Qt::MinimumSize, QSizeF(1,1));
    w10->setSizeHint(Qt::PreferredSize, QSizeF(10,10));
    w10->setSizeHint(Qt::MaximumSize, QSizeF(100,100));
    layout->addItem(w10, 1, 0);

    RectWidget *w11 = new RectWidget;
    w11->setSizeHint(Qt::MinimumSize, QSizeF(1,1));
    w11->setSizeHint(Qt::MaximumSize, QSizeF(30000,30000));
    w11->setConstraintFunction(hfw);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    w11->setSizePolicy(sp);
    layout->addItem(w11, 1, 1);

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(-1, -1)), QSizeF(2, 2));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, -1)), QSizeF(210, 110));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(-1, -1)), QSizeF(30100, 30100));

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(2, -1)), QSizeF(2, 20001));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(2, -1)), QSizeF(2, 20010));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(2, -1)), QSizeF(2, 20100));

    // Since 20 is somewhere between "minimum width hint" (2) and
    // "preferred width hint" (210), it will try to do distribution by
    // stretching them with different factors.
    // Since column 1 has a "preferred width" of 200 it means that
    // column 1 will be a bit wider than column 0. Thus it will also be a bit
    // shorter than 2001, (the expected height if all columns had width=10)
    QSizeF sh = layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(20, -1));
    // column 1 cannot be wider than 19, which means that it must be taller than 20000/19~=1052
    QVERIFY(sh.height() < 2000 + 1 && sh.height() > 1052 + 1);

    sh = layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(20, -1));
    QVERIFY(sh.height() < 2000 + 10 && sh.height() > 1052 + 10);

    sh = layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(20, -1));
    QVERIFY(sh.height() < 2000 + 100 && sh.height() > 1052 + 100);

    // the height of the hfw widget is shorter than the one to the left, which is 100, so
    // the total height of the last row is 100 (which leaves the layout height to be 200)
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(500, -1)), QSizeF(500, 100 + 100));

}

void tst_QGraphicsGridLayout::widthForHeight()
{
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    RectWidget *w00 = new RectWidget;
    w00->setMinimumSize(1, 1);
    w00->setPreferredSize(50, 50);
    w00->setMaximumSize(100, 100);

    layout->addItem(w00, 0, 0);

    RectWidget *w01 = new RectWidget;
    w01->setMinimumSize(1,1);
    w01->setPreferredSize(50,50);
    w01->setMaximumSize(100,100);
    layout->addItem(w01, 0, 1);

    RectWidget *w10 = new RectWidget;
    w10->setMinimumSize(1,1);
    w10->setPreferredSize(50,50);
    w10->setMaximumSize(100,100);
    layout->addItem(w10, 1, 0);

    RectWidget *w11 = new RectWidget;
    w11->setSizeHint(Qt::MinimumSize, QSizeF(1,1));
    w11->setSizeHint(Qt::PreferredSize, QSizeF(50,50));
    w11->setSizeHint(Qt::MaximumSize, QSizeF(30000,30000));

    // This will make sure its always square.
    w11->setConstraintFunction(wfh);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setWidthForHeight(true);
    w11->setSizePolicy(sp);
    layout->addItem(w11, 1, 1);

    /*
         | 1, 50, 100   | 1, 50, 100   |
    -----+--------------+--------------+
        1|              |              |
       50|              |              |
      100|              |              |
    -----|--------------+--------------+
        1|              |              |
       50|              |  WFH         |
      100|              |              |
    -----------------------------------+
    */


    QSizeF prefSize = layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, -1));
    QCOMPARE(prefSize, QSizeF(50+50, 50+50));

    // wfh(1):  = 1
    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(-1, 2)), QSizeF(1 + 1, 2));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, 2)), QSizeF(50 + 50, 2));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(-1, 2)), QSizeF(100 + 100, 2));

    // wfh(40) = 40
    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(-1, 80)), QSizeF(1 + 40, 80));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, 80)), QSizeF(50 + 50, 80));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(-1, 80)), QSizeF(100 + 100, 80));

    // wfh(80) = 80
    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(-1, 160)), QSizeF(1 + 80, 160));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, 160)), QSizeF(50 + 80, 160));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(-1, 160)), QSizeF(100 + 100, 160));

    // wfh(200) = 200
    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(-1, 300)), QSizeF(1 + 200, 300));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, 300)), QSizeF(50 + 200, 300));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(-1, 300)), QSizeF(100 + 200, 300));
}

void tst_QGraphicsGridLayout::heightForWidthWithSpanning()
{
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    RectWidget *w = new RectWidget;
    w->setSizeHint(Qt::MinimumSize, QSizeF(1,1));
    w->setSizeHint(Qt::MaximumSize, QSizeF(30000,30000));
    w->setConstraintFunction(hfw);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    w->setSizePolicy(sp);
    layout->addItem(w, 0,0,2,2);

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(-1, -1)), QSizeF(1, 1));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(-1, -1)), QSizeF(200, 100));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(-1, -1)), QSizeF(30000, 30000));

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(200, -1)), QSizeF(200, 100));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(200, -1)), QSizeF(200, 100));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(200, -1)), QSizeF(200, 100));

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(2, -1)), QSizeF(2, 10000));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(2, -1)), QSizeF(2, 10000));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(2, -1)), QSizeF(2, 10000));

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize, QSizeF(200, -1)), QSizeF(200, 100));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize, QSizeF(200, -1)), QSizeF(200, 100));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize, QSizeF(200, -1)), QSizeF(200, 100));
}

Q_DECLARE_METATYPE(QSizePolicy::Policy)
void tst_QGraphicsGridLayout::spanningItem2x2_data()
{
    QTest::addColumn<QSizePolicy::Policy>("sizePolicy");
    QTest::addColumn<int>("itemHeight");
    QTest::addColumn<int>("expectedHeight");

    QTest::newRow("A larger spanning item with 2 widgets with fixed policy") << QSizePolicy::Fixed << 39 << 80;
    QTest::newRow("A larger spanning item with 2 widgets with preferred policy") << QSizePolicy::Preferred << 39 << 80;
    QTest::newRow("An equally-sized spanning item with 2 widgets with fixed policy") << QSizePolicy::Fixed << 40 << 80;
    QTest::newRow("An equally-sized spanning item with 2 widgets with preferred policy") << QSizePolicy::Preferred << 40 << 80;
    QTest::newRow("A smaller spanning item with 2 widgets with fixed policy") << QSizePolicy::Fixed << 41 << 82;
    QTest::newRow("A smaller spanning item with 2 widgets with preferred policy") << QSizePolicy::Preferred << 41 << 82;
}

void tst_QGraphicsGridLayout::spanningItem2x2()
{
    QFETCH(QSizePolicy::Policy, sizePolicy);
    QFETCH(int, itemHeight);
    QFETCH(int, expectedHeight);
    QGraphicsWidget *form = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout(form);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QGraphicsWidget *w1 = new QGraphicsWidget;
    w1->setMinimumSize(80,80);
    w1->setMaximumSize(80,80);

    QGraphicsWidget *w2 = new QGraphicsWidget;
    w2->setMinimumSize(80,itemHeight);
    w2->setPreferredSize(80,itemHeight);
    w2->setSizePolicy(QSizePolicy::Fixed, sizePolicy);

    QGraphicsWidget *w3 = new QGraphicsWidget;
    w3->setMinimumSize(80,itemHeight);
    w3->setPreferredSize(80,itemHeight);
    w3->setSizePolicy(QSizePolicy::Fixed, sizePolicy);

    layout->addItem(w1, 0, 0, 2, 1);
    layout->addItem(w2, 0, 1);
    layout->addItem(w3, 1, 1);

    QCOMPARE(layout->minimumSize(), QSizeF(160,expectedHeight));
    if(sizePolicy == QSizePolicy::Fixed)
        QCOMPARE(layout->maximumSize(), QSizeF(160,expectedHeight));
    else
        QCOMPARE(layout->maximumSize(), QSizeF(160,QWIDGETSIZE_MAX));
}

void tst_QGraphicsGridLayout::spanningItem2x3_data()
{
    QTest::addColumn<bool>("w1_fixed");
    QTest::addColumn<bool>("w2_fixed");
    QTest::addColumn<bool>("w3_fixed");
    QTest::addColumn<bool>("w4_fixed");
    QTest::addColumn<bool>("w5_fixed");

    for(int w1 = 0; w1 < 2; w1++)
        for(int w2 = 0; w2 < 2; w2++)
            for(int w3 = 0; w3 < 2; w3++)
                for(int w4 = 0; w4 < 2; w4++)
                    for(int w5 = 0; w5 < 2; w5++) {
                        QString description = QString("Fixed sizes:") + (w1?" w1":"") + (w2?" w2":"") + (w3?" w3":"") + (w4?" w4":"") + (w5?" w5":"");
                        QTest::newRow(description.toLatin1()) << (bool)w1 << (bool)w2 << (bool)w3 << (bool)w4 << (bool)w5;
                    }
}

void tst_QGraphicsGridLayout::spanningItem2x3()
{
    QFETCH(bool, w1_fixed);
    QFETCH(bool, w2_fixed);
    QFETCH(bool, w3_fixed);
    QFETCH(bool, w4_fixed);
    QFETCH(bool, w5_fixed);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QGraphicsWidget *w1 = new QGraphicsWidget;
    w1->setMinimumSize(80,80);
    w1->setMaximumSize(80,80);
    if (w1_fixed)
        w1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGraphicsWidget *w2 = new QGraphicsWidget;
    w2->setMinimumSize(80,48);
    w2->setPreferredSize(80,48);
    if (w2_fixed)
        w2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGraphicsWidget *w3 = new QGraphicsWidget;
    w3->setMinimumSize(80,30);
    w3->setPreferredSize(80,30);
    if (w3_fixed)
        w3->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGraphicsWidget *w4 = new QGraphicsWidget;
    w4->setMinimumSize(80,30);
    w4->setMaximumSize(80,30);
    if (w4_fixed)
        w4->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGraphicsWidget *w5 = new QGraphicsWidget;
    w5->setMinimumSize(40,24);
    w5->setMaximumSize(40,24);
    if (w5_fixed)
        w5->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    layout->addItem(w1, 0, 0, 2, 1);
    layout->addItem(w2, 0, 1);
    layout->addItem(w3, 1, 1);
    layout->addItem(w4, 0, 2);
    layout->addItem(w5, 1, 2);

    QCOMPARE(layout->minimumSize(), QSizeF(240,80));
    // Only w2 and w3 grow vertically, so when they have a fixed vertical size policy,
    // the whole layout cannot grow vertically.
    if (w2_fixed && w3_fixed)
        QCOMPARE(layout->maximumSize(), QSizeF(QWIDGETSIZE_MAX,80));
    else
        QCOMPARE(layout->maximumSize(), QSizeF(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
}

void tst_QGraphicsGridLayout::spanningItem()
{
    QGraphicsWidget *form = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout(form);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QGraphicsWidget *w1 = new QGraphicsWidget;
    w1->setMinimumSize(80,80);
    w1->setMaximumSize(80,80);

    QGraphicsWidget *w2 = new QGraphicsWidget;
    w2->setMinimumSize(80,38);
    w2->setPreferredSize(80,38);
    w2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QGraphicsWidget *w3 = new QGraphicsWidget;
    w3->setMinimumSize(80,38);
    w3->setPreferredSize(80,38);
    w3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layout->addItem(w1, 0, 0, 2, 1);
    layout->addItem(w2, 0, 1);
    layout->addItem(w3, 1, 1);

    QCOMPARE(layout->minimumSize(), QSizeF(160,80));
    QCOMPARE(layout->maximumSize(), QSizeF(160,80));
}

void tst_QGraphicsGridLayout::spanAcrossEmptyRow()
{
    QGraphicsWidget *form = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout(form);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    RectWidget *w1 = new RectWidget;
    RectWidget *w2 = new RectWidget;
    RectWidget *w3 = new RectWidget;

    QSizeF size(10, 10);
    for (int i = 0; i < 3; ++i) {
        w1->setSizeHint((Qt::SizeHint)i, size);
        w2->setSizeHint((Qt::SizeHint)i, size);
        w3->setSizeHint((Qt::SizeHint)i, size);
        size+=size;                 //[(10,10), (20,20), (40,40)]
    }
    layout->addItem(w1, 0, 0, 1, 1);
    layout->addItem(w2, 0, 1, 1, 2);
    layout->addItem(w3, 0, 99, 1, 1);

    form->resize(60,20);
    QCOMPARE(w1->geometry(), QRectF( 0, 0, 20, 20));
    QCOMPARE(w2->geometry(), QRectF(20, 0, 20, 20));
    QCOMPARE(w3->geometry(), QRectF(40, 0, 20, 20));

    QCOMPARE(layout->effectiveSizeHint(Qt::MinimumSize), QSizeF(30, 10));
    QCOMPARE(layout->effectiveSizeHint(Qt::PreferredSize), QSizeF(60, 20));
    QCOMPARE(layout->effectiveSizeHint(Qt::MaximumSize), QSizeF(120, 40));
}

void tst_QGraphicsGridLayout::stretchAndHeightForWidth()
{
    QGraphicsWidget *widget = new QGraphicsWidget(0, Qt::Window);
    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    RectWidget *w1 = new RectWidget;
    w1->setSizeHint(Qt::MinimumSize, QSizeF(10, 10));
    w1->setSizeHint(Qt::PreferredSize, QSizeF(100, 100));
    w1->setSizeHint(Qt::MaximumSize, QSizeF(500, 500));
    layout->addItem(w1, 0,0,1,1);

    RectWidget *w2 = new RectWidget;
    w2->setSizeHint(Qt::MinimumSize, QSizeF(10, 10));
    w2->setSizeHint(Qt::PreferredSize, QSizeF(100, 100));
    w2->setSizeHint(Qt::MaximumSize, QSizeF(500, 500));
    layout->addItem(w2, 0,1,1,1);
    layout->setColumnStretchFactor(1, 2);

    QApplication::sendPostedEvents();
    QGraphicsScene scene;
    QGraphicsView *view = new QGraphicsView(&scene);

    scene.addItem(widget);

    view->show();

    widget->resize(500, 100);
    // w1 should stay at its preferred size
    QCOMPARE(w1->geometry(), QRectF(0, 0, 100, 100));
    QCOMPARE(w2->geometry(), QRectF(100, 0, 400, 100));


    // only w1 has hfw
    w1->setConstraintFunction(hfw);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    w1->setSizePolicy(sp);
    QApplication::sendPostedEvents();

    QCOMPARE(w1->geometry(), QRectF(0, 0, 100, 200));
    QCOMPARE(w2->geometry(), QRectF(100, 0, 400, 200));

    // only w2 has hfw
    w2->setConstraintFunction(hfw);
    w2->setSizePolicy(sp);

    w1->setConstraintFunction(0);
    sp.setHeightForWidth(false);
    w1->setSizePolicy(sp);
    QApplication::sendPostedEvents();

    QCOMPARE(w1->geometry(), QRectF(0, 0, 100, 100));
    QCOMPARE(w2->geometry(), QRectF(100, 0, 400, 50));

}

void tst_QGraphicsGridLayout::testDefaultAlignment()
{
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QGraphicsWidget *w = new QGraphicsWidget;
    w->setMinimumSize(50,50);
    w->setMaximumSize(50,50);
    layout->addItem(w,0,0);

    //Default alignment should be to the top-left

    //First, check by forcing the layout to be bigger
    layout->setMinimumSize(100,100);
    layout->activate();
    QCOMPARE(layout->geometry(), QRectF(0,0,100,100));
    QCOMPARE(w->geometry(), QRectF(0,0,50,50));
    layout->setMinimumSize(-1,-1);

    //Second, check by forcing the column and row to be bigger instead
    layout->setColumnMinimumWidth(0, 100);
    layout->setRowMinimumHeight(0, 100);
    layout->activate();
    QCOMPARE(layout->geometry(), QRectF(0,0,100,100));
    QCOMPARE(w->geometry(), QRectF(0,0,50,50));
    layout->setMinimumSize(-1,-1);
    layout->setColumnMinimumWidth(0, 0);
    layout->setRowMinimumHeight(0, 0);


    //Third, check by adding a larger item in the column
    QGraphicsWidget *w2 = new QGraphicsWidget;
    w2->setMinimumSize(100,100);
    w2->setMaximumSize(100,100);
    layout->addItem(w2,1,0);
    layout->activate();
    QCOMPARE(layout->geometry(), QRectF(0,0,100,150));
    QCOMPARE(w->geometry(), QRectF(0,0,50,50));
    QCOMPARE(w2->geometry(), QRectF(0,50,100,100));
}

static RectWidget *addWidget(QGraphicsGridLayout *grid, int row, int column)
{
    RectWidget *w = new RectWidget;
    w->setPreferredSize(20, 20);
    grid->addItem(w, row, column);
    return w;
}

static void setVisible(bool visible, QGraphicsWidget **widgets)
{
    for (int i = 0; i < 3; ++i)
        if (widgets[i]) widgets[i]->setVisible(visible);
}

static void setRetainSizeWhenHidden(bool retainSize, QGraphicsWidget **widgets)
{
    QSizePolicy sp = widgets[0]->sizePolicy();
    sp.setRetainSizeWhenHidden(retainSize);
    for (int i = 0; i < 3; ++i)
        if (widgets[i]) widgets[i]->setSizePolicy(sp);
}

void tst_QGraphicsGridLayout::hiddenItems()
{
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsGridLayout *layout = new QGraphicsGridLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // Create a 3x3 layout
    addWidget(layout, 0, 0);
    RectWidget *w01 = addWidget(layout, 0, 1);
    addWidget(layout, 0, 2);
    RectWidget *w10 = addWidget(layout, 1, 0);
    RectWidget *w11 = addWidget(layout, 1, 1);
    RectWidget *w12 = addWidget(layout, 1, 2);
    addWidget(layout, 2, 0);
    RectWidget *w21 = addWidget(layout, 2, 1);
    addWidget(layout, 2, 2);

    QGraphicsWidget *middleColumn[] = {w01, w11, w21 };
    QGraphicsWidget *topTwoOfMiddleColumn[] = {w01, w11, 0 };

    // hide and show middle column
    QCOMPARE(layout->preferredWidth(), qreal(64));
    setVisible(false, middleColumn);   // hide middle column
    QCOMPARE(layout->preferredWidth(), qreal(42));
    setVisible(true, middleColumn);    // show middle column
    QCOMPARE(layout->preferredWidth(), qreal(64));
    setRetainSizeWhenHidden(true, middleColumn);
    QCOMPARE(layout->preferredWidth(), qreal(64));
    setVisible(false, middleColumn);   // hide middle column
    QCOMPARE(layout->preferredWidth(), qreal(64));
    setRetainSizeWhenHidden(false, middleColumn);
    QCOMPARE(layout->preferredWidth(), qreal(42));
    setVisible(true, middleColumn);
    QCOMPARE(layout->preferredWidth(), qreal(64));

    // Hide only two items, => column should not collapse
    setVisible(false, topTwoOfMiddleColumn);
    QCOMPARE(layout->preferredWidth(), qreal(64));


    QGraphicsWidget *middleRow[] = {w10, w11, w12 };
    QGraphicsWidget *leftMostTwoOfMiddleRow[] = {w10, w11, 0 };

    // hide and show middle row
    QCOMPARE(layout->preferredHeight(), qreal(64));
    setVisible(false, middleRow);
    QCOMPARE(layout->preferredHeight(), qreal(42));
    setVisible(true, middleRow);
    QCOMPARE(layout->preferredHeight(), qreal(64));
    setRetainSizeWhenHidden(true, middleColumn);
    QCOMPARE(layout->preferredHeight(), qreal(64));
    setVisible(false, middleRow);
    QCOMPARE(layout->preferredHeight(), qreal(64));
    setRetainSizeWhenHidden(false, middleRow);
    QCOMPARE(layout->preferredHeight(), qreal(42));
    setVisible(true, middleRow);
    QCOMPARE(layout->preferredHeight(), qreal(64));

    // Hide only two items => row should not collapse
    setVisible(false, leftMostTwoOfMiddleRow);
    QCOMPARE(layout->preferredHeight(), qreal(64));

}

QTEST_MAIN(tst_QGraphicsGridLayout)
#include "tst_qgraphicsgridlayout.moc"

