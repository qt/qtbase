// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qscrollarea.h>
#include <qlayout.h>
#include <qscrollbar.h>

class tst_QScrollArea : public QObject
{
Q_OBJECT

public:
    tst_QScrollArea();
    virtual ~tst_QScrollArea();

private slots:
    void getSetCheck();
    void ensureMicroFocusVisible_Task_167838();
    void checkHFW_Task_197736();
    void stableHeightForWidth();
};

tst_QScrollArea::tst_QScrollArea()
{
}

tst_QScrollArea::~tst_QScrollArea()
{
}

// Testing get/set functions
void tst_QScrollArea::getSetCheck()
{
    QScrollArea obj1;
    // QWidget * QScrollArea::widget()
    // void QScrollArea::setWidget(QWidget *)
    QWidget *var1 = new QWidget();
    obj1.setWidget(var1);
    QCOMPARE(var1, obj1.widget());
    obj1.setWidget((QWidget *)0);
    QCOMPARE(var1, obj1.widget()); // Cannot set a 0-widget. Old widget returned
    // delete var1; // No delete, since QScrollArea takes ownership

    // bool QScrollArea::widgetResizable()
    // void QScrollArea::setWidgetResizable(bool)
    obj1.setWidgetResizable(false);
    QCOMPARE(false, obj1.widgetResizable());
    obj1.setWidgetResizable(true);
    QCOMPARE(true, obj1.widgetResizable());
}

class WidgetWithMicroFocus : public QWidget
{
public:
    WidgetWithMicroFocus(QWidget *parent = nullptr) : QWidget(parent)
    {
        setBackgroundRole(QPalette::Dark);
    }
protected:
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override
    {
        if (query == Qt::ImCursorRectangle)
            return QRect(width() / 2, height() / 2, 5, 5);
        return QWidget::inputMethodQuery(query);
    }
//     void paintEvent(QPaintEvent *event)
//     {
//         QPainter painter(this);
//         painter.fillRect(rect(), QBrush(Qt::red));
//     }
};

void tst_QScrollArea::ensureMicroFocusVisible_Task_167838()
{
    QScrollArea scrollArea;
    scrollArea.resize(100, 100);
    scrollArea.show();
    QWidget *parent = new QWidget;
    parent->setLayout(new QVBoxLayout);
    QWidget *child = new WidgetWithMicroFocus;
    parent->layout()->addWidget(child);
    parent->resize(300, 300);
    scrollArea.setWidget(parent);
    scrollArea.ensureWidgetVisible(child, 10, 10);
    QRect microFocus = child->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QPoint p = child->mapTo(scrollArea.viewport(), microFocus.topLeft());
    microFocus.translate(p - microFocus.topLeft());
    QVERIFY(scrollArea.viewport()->rect().contains(microFocus));
}

class HFWWidget : public QWidget
{
    public:
        HFWWidget();
        int heightForWidth(int w) const override;
};

HFWWidget::HFWWidget()
    : QWidget()
{
    setMinimumSize(QSize(100,50));
    QSizePolicy s = sizePolicy();
    s.setHeightForWidth(true);
    setSizePolicy(s);
}

int HFWWidget::heightForWidth(int w) const
{
    // Mimic a label - the narrower we are, the taller we have to be
    if (w > 0)
        return 40000 / w;
    else
        return 40000;
}

void tst_QScrollArea::checkHFW_Task_197736()
{
    QScrollArea scrollArea;
    HFWWidget *w = new HFWWidget;
    scrollArea.resize(200,100);
    scrollArea.show();
    scrollArea.setWidgetResizable(true);
    scrollArea.setWidget(w);

    // at 200x100px, we expect HFW to be 200px tall, not 100px
    QVERIFY(w->height() >= 200);

    // at 200x300px, we expect HFW to be 300px tall (the heightForWidth is a min, not prescribed)
    scrollArea.resize(QSize(200, 300));
    QVERIFY(w->height() >= 250); // 50px for a fudge factor (size of frame margins/scrollbars etc)

    // make sure this only happens with widget resizable
    scrollArea.setWidgetResizable(false);
    scrollArea.resize(QSize(100,100));
    w->resize(QSize(200,200));
    QCOMPARE(w->width(), 200);
    QCOMPARE(w->height(), 200);
}


/*
    If the scroll area rides the size where, due to the height-for-width
    implementation of the widget, the vertical scrollbar is needed only
    if the vertical scrollbar is visible, then we don't want it to flip
    back and forth, but rather constrain the width of the widget.
    See QTBUG-92958.
*/
void tst_QScrollArea::stableHeightForWidth()
{
    struct HeightForWidthWidget : public QWidget
    {
        HeightForWidthWidget()
        {
            QSizePolicy policy = sizePolicy();
            policy.setHeightForWidth(true);
            setSizePolicy(policy);
        }
        // Aspect ratio 1:1
        int heightForWidth(int width) const override { return width; }
    };

    class HeightForWidthArea : public QScrollArea
    {
    public:
        HeightForWidthArea()
        {
            this->verticalScrollBar()->installEventFilter(this);
        }
    protected:
        bool eventFilter(QObject *obj, QEvent *e) override
        {
            if (obj == verticalScrollBar() && e->type() == QEvent::Hide)
                ++m_hideCount;
            return QScrollArea::eventFilter(obj,e);
        }
    public:
        int m_hideCount = 0;
    };

    HeightForWidthArea area;
    HeightForWidthWidget equalWHWidget;
    area.setWidget(&equalWHWidget);
    area.setWidgetResizable(true);
    // at this size, the widget wants to be 501 pixels high,
    // requiring a vertical scrollbar in a 499 pixel high area.
    // but the width resulting from showing the scrollbar would
    // be less than 499, so no scrollbars would be needed anymore.
    area.resize(501, 499);
    area.show();
    QTest::qWait(500);
    // if the scrollbar got hidden more than once, then the layout
    // isn't stable.
    QVERIFY(area.m_hideCount <= 1);
}

QTEST_MAIN(tst_QScrollArea)
#include "tst_qscrollarea.moc"
