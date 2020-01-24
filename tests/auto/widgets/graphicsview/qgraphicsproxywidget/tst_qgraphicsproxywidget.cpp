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
#include <QtGui>
#include <QtWidgets>
#include <private/qgraphicsproxywidget_p.h>
#include <private/qlayoutengine_p.h>    // qSmartMin functions...

/*
    Notes:

    1) The proxy and the widget geometries are linked.
       proxy resize => widget resize => stop (no livelock)
       widget resize => proxy resize => stop (no livelock)

    2) As far as possible, the properties are linked.
       proxy enable => widget enable => stop
       widget disabled => proxy disabled => stop

    3) Windowed state is linked
       Windowed proxy state => windowed widget state => stop
       Windowed widget state => windowed proxy state => stop
*/

class EventSpy : public QObject
{
public:
    EventSpy(QObject *receiver)
    {
        receiver->installEventFilter(this);
    }

    QMap<QEvent::Type, int> counts;

protected:
    bool eventFilter(QObject *, QEvent *event)
    {
        ++counts[event->type()];
        return false;
    }
};

class tst_QGraphicsProxyWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void qgraphicsproxywidget_data();
    void qgraphicsproxywidget();
    void paint();
    void paint_2();
    void setWidget_data();
    void setWidget();
    void testEventFilter_data();
    void testEventFilter();
    void focusInEvent_data();
    void focusInEvent();
    void focusInEventNoWidget();
    void focusNextPrevChild_data();
    void focusNextPrevChild();
    void focusOutEvent_data();
    void focusOutEvent();
    void focusProxy_QTBUG_51856();
    void hoverEnterLeaveEvent_data();
    void hoverEnterLeaveEvent();
    void hoverMoveEvent_data();
    void hoverMoveEvent();
    void keyPressEvent_data();
    void keyPressEvent();
    void keyReleaseEvent_data();
    void keyReleaseEvent();
    void mouseDoubleClickEvent_data();
    void mouseDoubleClickEvent();
    void mousePressReleaseEvent_data();
    void mousePressReleaseEvent();
    void resizeEvent_data();
    void resizeEvent();
    void paintEvent();
#if QT_CONFIG(wheelevent)
    void wheelEvent();
#endif
    void sizeHint_data();
    void sizeHint();
    void sizePolicy();
    void minimumSize();
    void maximumSize();
    void scrollUpdate();
    void setWidget_simple();
    void setWidget_ownership();
    void resize_simple_data();
    void resize_simple();
    void symmetricMove();
    void symmetricResize();
    void symmetricEnabled();
    void symmetricVisible();
    void tabFocus_simpleWidget();
    void tabFocus_simpleTwoWidgets();
    void tabFocus_complexWidget();
    void tabFocus_complexTwoWidgets();
    void setFocus_simpleWidget();
    void setFocus_simpleTwoWidgets();
    void setFocus_complexTwoWidgets();
    void popup_basic();
    void popup_subwidget();
    void changingCursor_basic();
    void tooltip_basic();
    void childPos_data();
    void childPos();
    void autoShow();
    void windowOpacity();
    void stylePropagation();
    void palettePropagation();
    void fontPropagation();
    void dontCrashWhenDie();
    void dontCrashNoParent();
    void createProxyForChildWidget();
#ifndef QT_NO_CONTEXTMENU
    void actionsContextMenu();
#endif // QT_NO_CONTEXTMENU
    void actionsContextMenu_data();
    void deleteProxyForChildWidget();
    void bypassGraphicsProxyWidget_data();
    void bypassGraphicsProxyWidget();
    void dragDrop();
    void windowFlags_data();
    void windowFlags();
    void comboboxWindowFlags();
    void updateAndDelete();
    void inputMethod();
    void clickFocus();
    void windowFrameMargins();
    void QTBUG_6986_sendMouseEventToAlienWidget();
    void mapToGlobal();
    void mapToGlobalWithoutScene();
    void QTBUG_43780_visibility();
    void forwardTouchEvent();
};

// Subclass that exposes the protected functions.
class SubQGraphicsProxyWidget : public QGraphicsProxyWidget
{

public:
    SubQGraphicsProxyWidget(QGraphicsItem *parent = 0) : QGraphicsProxyWidget(parent),
    paintCount(0), keyPress(0), focusOut(0)
        {}

    bool call_eventFilter(QObject* object, QEvent* event)
        { return SubQGraphicsProxyWidget::eventFilter(object, event); }

    void call_focusInEvent(QFocusEvent* event)
        { return SubQGraphicsProxyWidget::focusInEvent(event); }

    bool call_focusNextPrevChild(bool next)
        { return SubQGraphicsProxyWidget::focusNextPrevChild(next); }

    void call_focusOutEvent(QFocusEvent* event)
        { return SubQGraphicsProxyWidget::focusOutEvent(event); }

    void call_hideEvent(QHideEvent* event)
        { return SubQGraphicsProxyWidget::hideEvent(event); }

    void call_hoverEnterEvent(QGraphicsSceneHoverEvent* event)
        { return SubQGraphicsProxyWidget::hoverEnterEvent(event); }

    void call_hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
        { return SubQGraphicsProxyWidget::hoverLeaveEvent(event); }

    void call_hoverMoveEvent(QGraphicsSceneHoverEvent* event)
        { return SubQGraphicsProxyWidget::hoverMoveEvent(event); }

    void call_keyPressEvent(QKeyEvent* event)
        { return SubQGraphicsProxyWidget::keyPressEvent(event); }

    void call_keyReleaseEvent(QKeyEvent* event)
        { return SubQGraphicsProxyWidget::keyReleaseEvent(event); }

    void call_mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
        { return SubQGraphicsProxyWidget::mouseDoubleClickEvent(event); }

    void call_mouseMoveEvent(QGraphicsSceneMouseEvent* event)
        { return SubQGraphicsProxyWidget::mouseMoveEvent(event); }

    void call_mousePressEvent(QGraphicsSceneMouseEvent* event)
        { return SubQGraphicsProxyWidget::mousePressEvent(event); }

    void call_mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
        { return SubQGraphicsProxyWidget::mouseReleaseEvent(event); }

    void call_resizeEvent(QGraphicsSceneResizeEvent* event)
        { return SubQGraphicsProxyWidget::resizeEvent(event); }

    QSizeF call_sizeHint(Qt::SizeHint which, QSizeF const& constraint = QSizeF()) const
        { return SubQGraphicsProxyWidget::sizeHint(which, constraint); }

    void call_showEvent(QShowEvent* event)
        { return SubQGraphicsProxyWidget::showEvent(event); }

    void paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0)    {
        paintCount++;
        QGraphicsProxyWidget::paint(painter, option, widget);
    }

    void focusOutEvent(QFocusEvent *event)
    {
        focusOut++;
        QGraphicsProxyWidget::focusOutEvent(event);
    }

    bool eventFilter(QObject *object, QEvent *event) {
        if (event->type() == QEvent::KeyPress && object == widget())
            keyPress++;
        return QGraphicsProxyWidget::eventFilter(object, event);
    }
    int paintCount;
    int keyPress;
    int focusOut;
};

#if QT_CONFIG(wheelevent)
class WheelWidget : public QWidget
{
public:
    WheelWidget() : wheelEventCalled(false) { setFocusPolicy(Qt::WheelFocus); }

    virtual void wheelEvent(QWheelEvent *event) { event->accept(); wheelEventCalled = true; }

    bool wheelEventCalled;
};
#endif // QT_CONFIG(wheelevent)

// This will be called before the first test function is executed.
// It is only called once.
void tst_QGraphicsProxyWidget::initTestCase()
{
    // Disable menu animations to prevent the alpha widget from getting in the way
    // in actionsContextMenu().
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    // Disable combo for QTBUG_43780_visibility()/Windows Vista.
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
}

// This will be called after every test function.
void tst_QGraphicsProxyWidget::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QGraphicsProxyWidget::qgraphicsproxywidget_data()
{
}

void tst_QGraphicsProxyWidget::qgraphicsproxywidget()
{
    SubQGraphicsProxyWidget proxy;
    proxy.paint(0, 0, 0);
    proxy.setWidget(0);
    QCOMPARE(proxy.type(), int(QGraphicsProxyWidget::Type));
    QVERIFY(!proxy.widget());
    QEvent event(QEvent::None);
    proxy.call_eventFilter(0, &event);
    QFocusEvent focusEvent(QEvent::FocusIn);
    focusEvent.ignore();
    proxy.call_focusInEvent(&focusEvent);
    QCOMPARE(focusEvent.isAccepted(), false);
    QCOMPARE(proxy.call_focusNextPrevChild(false), false);
    QCOMPARE(proxy.call_focusNextPrevChild(true), false);
    proxy.call_focusOutEvent(&focusEvent);
    QHideEvent hideEvent;
    proxy.call_hideEvent(&hideEvent);
    QGraphicsSceneHoverEvent hoverEvent;
    proxy.call_hoverEnterEvent(&hoverEvent);
    proxy.call_hoverLeaveEvent(&hoverEvent);
    proxy.call_hoverMoveEvent(&hoverEvent);
    QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier);
    proxy.call_keyPressEvent(&keyEvent);
    proxy.call_keyReleaseEvent(&keyEvent);
    QGraphicsSceneMouseEvent mouseEvent;
    proxy.call_mouseDoubleClickEvent(&mouseEvent);
    proxy.call_mouseMoveEvent(&mouseEvent);
    proxy.call_mousePressEvent(&mouseEvent);
    proxy.call_mouseReleaseEvent(&mouseEvent);
    QGraphicsSceneResizeEvent resizeEvent;
    proxy.call_resizeEvent(&resizeEvent);
    QShowEvent showEvent;
    proxy.call_showEvent(&showEvent);
    proxy.call_sizeHint(Qt::PreferredSize, QSizeF());
}

// public void paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget)
void tst_QGraphicsProxyWidget::paint()
{
    SubQGraphicsProxyWidget proxy;
    proxy.paint(0, 0, 0);
}

class MyProxyWidget : public QGraphicsProxyWidget
{
public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        // Make sure QGraphicsProxyWidget::paint does not modify the render hints set on the painter.
        painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform
                                | QPainter::TextAntialiasing);
        const QPainter::RenderHints oldRenderHints = painter->renderHints();
        QGraphicsProxyWidget::paint(painter, option, widget);
        QCOMPARE(painter->renderHints(), oldRenderHints);
    }
};

void tst_QGraphicsProxyWidget::paint_2()
{
    MyProxyWidget *proxyWidget = new MyProxyWidget;
    proxyWidget->setWidget(new QLineEdit);

    QGraphicsScene scene;
    scene.addItem(proxyWidget);
    scene.setSceneRect(scene.itemsBoundingRect());

    // Trigger repaint.
    QPixmap pixmap(scene.sceneRect().toRect().size());
    QPainter painter(&pixmap);
    scene.render(&painter);
}

void tst_QGraphicsProxyWidget::setWidget_data()
{
    QTest::addColumn<bool>("widgetExists");
    QTest::addColumn<bool>("insertWidget");
    QTest::addColumn<bool>("hasParent");
    QTest::addColumn<bool>("proxyHasParent");

    QTest::newRow("setWidget(0)") << false << false << false << false;
    QTest::newRow("setWidget(widget)") << false << true << false << false;
    QTest::newRow("setWidget(widgetWParent)") << false << true << true << false;
    QTest::newRow("setWidget(1), setWidget(0)") << true << false << false << false;
    QTest::newRow("setWidget(1), setWidget(widget)") << true << true << false << false;
    QTest::newRow("setWidget(1), setWidget(widgetWParent)") << true << true << true << false;
    QTest::newRow("p setWidget(0)") << false << false << false << true;
    QTest::newRow("p setWidget(widget)") << false << true << false << true;
    QTest::newRow("p setWidget(widgetWParent)") << false << true << true << true;
    QTest::newRow("p setWidget(1), setWidget(0)") << true << false << false << true;
    QTest::newRow("p setWidget(1), setWidget(widget)") << true << true << false << true;
    QTest::newRow("p setWidget(1), setWidget(widgetWParent)") << true << true << true << true;
}

// public void setWidget(QWidget* widget)
void tst_QGraphicsProxyWidget::setWidget()
{
    QFETCH(bool, widgetExists);
    QFETCH(bool, insertWidget);
    QFETCH(bool, hasParent);
    QFETCH(bool, proxyHasParent);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QScopedPointer<QStyle> style(QStyleFactory::create(QLatin1String("Fusion")));
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QPointer<SubQGraphicsProxyWidget> proxy = new SubQGraphicsProxyWidget;
    SubQGraphicsProxyWidget parentProxy;
    scene.addItem(proxy);
    scene.addItem(&parentProxy);
    if (proxyHasParent)
        proxy->setParent(&parentProxy);
    QPointer<QWidget> existingSubWidget = new QWidget;
    proxy->setVisible(false);
    proxy->setEnabled(false);

    if (widgetExists) {
        existingSubWidget->setAttribute(Qt::WA_QuitOnClose, true);
        proxy->setWidget(existingSubWidget);
    }

    QWidget *widget = new QWidget;
#ifndef QT_NO_CURSOR
    widget->setCursor(Qt::IBeamCursor);
#endif
    widget->setPalette(QPalette(Qt::magenta));
    widget->setLayoutDirection(Qt::RightToLeft);
    widget->setStyle(style.data());
    widget->setFont(QFont("Times"));
    widget->setVisible(true);
    QApplication::setActiveWindow(widget);
    widget->activateWindow();
    widget->setEnabled(true);
    widget->resize(325, 241);
    widget->setMinimumSize(100, 200);
    widget->setMaximumSize(1000, 2000);
    widget->setFocusPolicy(Qt::TabFocus);
    widget->setContentsMargins(10, 29, 19, 81);
    widget->setFocus(Qt::TabFocusReason);
    widget->setAcceptDrops(true);
    QTRY_VERIFY(widget->hasFocus());
    QVERIFY(widget->isActiveWindow());

    QWidget parentWidget;
    if (hasParent)
        widget->setParent(&parentWidget);

    QWidget *subWidget = insertWidget ? widget : 0;
    bool shouldBeInsertable = !hasParent && subWidget;
    if (shouldBeInsertable)
        subWidget->setAttribute(Qt::WA_QuitOnClose, true);

    proxy->setWidget(subWidget);

    if (shouldBeInsertable) {
        QCOMPARE(proxy->widget(), subWidget);
        QVERIFY(subWidget->testAttribute(Qt::WA_DontShowOnScreen));
        QVERIFY(!subWidget->testAttribute(Qt::WA_QuitOnClose));
        QCOMPARE(proxy->acceptHoverEvents(), true);
#ifndef QT_NO_CURSOR
        QVERIFY(proxy->hasCursor());

        // These should match
        QCOMPARE(proxy->cursor().shape(), widget->cursor().shape());
#endif
        //###QCOMPARE(proxy->palette(), widget->palette());
        QCOMPARE(proxy->layoutDirection(), widget->layoutDirection());
        QCOMPARE(proxy->style(), widget->style());
        QCOMPARE(proxy->isVisible(), widget->isVisible());
        QCOMPARE(proxy->isEnabled(), widget->isEnabled());
        QVERIFY(proxy->flags() & QGraphicsItem::ItemIsFocusable);
        QCOMPARE(proxy->effectiveSizeHint(Qt::MinimumSize).toSize(),
            qSmartMinSize(widget));
        QCOMPARE(proxy->minimumSize().toSize(),
                 qSmartMinSize(widget) );
        QCOMPARE(proxy->maximumSize().toSize(), QSize(1000, 2000));
        QCOMPARE(proxy->effectiveSizeHint(Qt::PreferredSize).toSize(),
                qSmartMinSize(widget));
        QCOMPARE(proxy->size().toSize(), widget->size());
        QCOMPARE(proxy->rect().toRect(), widget->rect());
        QCOMPARE(proxy->focusPolicy(), Qt::WheelFocus);
        QVERIFY(proxy->acceptDrops());
        QCOMPARE(proxy->acceptHoverEvents(), true); // to get widget enter events
        const QMarginsF margins = QMarginsF{widget->contentsMargins()};
        qreal rleft, rtop, rright, rbottom;
        proxy->getContentsMargins(&rleft, &rtop, &rright, &rbottom);
        QCOMPARE(margins.left(), rleft);
        QCOMPARE(margins.top(), rtop);
        QCOMPARE(margins.right(), rright);
        QCOMPARE(margins.bottom(), rbottom);
    } else {
        // proxy shouldn't mess with the widget if it can't insert it.
        QCOMPARE(proxy->widget(), nullptr);
        QCOMPARE(proxy->acceptHoverEvents(), false);
        if (subWidget) {
            QVERIFY(!subWidget->testAttribute(Qt::WA_DontShowOnScreen));
            QVERIFY(subWidget->testAttribute(Qt::WA_QuitOnClose));
            // reset
            subWidget->setAttribute(Qt::WA_QuitOnClose, false);
        }
    }

    if (widgetExists) {
        QCOMPARE(existingSubWidget->parent(), nullptr);
        QVERIFY(!existingSubWidget->testAttribute(Qt::WA_DontShowOnScreen));
        QVERIFY(!existingSubWidget->testAttribute(Qt::WA_QuitOnClose));
    }

    if (hasParent)
        widget->setParent(0);

    delete widget;
    if (shouldBeInsertable)
        QVERIFY(!proxy);
    delete existingSubWidget;
    if (!shouldBeInsertable) {
        QVERIFY(proxy);
        delete proxy;
    }
    QVERIFY(!proxy);
}

Q_DECLARE_METATYPE(QEvent::Type)
void tst_QGraphicsProxyWidget::testEventFilter_data()
{
    QTest::addColumn<QEvent::Type>("eventType");
    QTest::addColumn<bool>("fromObject"); // big grin evil
    QTest::newRow("none") << QEvent::None << false;
    for (int i = 0; i < 2; ++i) {
        bool fromObject = (i == 0);
        const char fromObjectC = fromObject ? '1' : '0';
        QTest::newRow((QByteArrayLiteral("resize ") + fromObjectC).constData()) << QEvent::Resize << fromObject;
        QTest::newRow((QByteArrayLiteral("move ") + fromObjectC).constData()) << QEvent::Move << fromObject;
        QTest::newRow((QByteArrayLiteral("hide ") + fromObjectC).constData()) << QEvent::Hide << fromObject;
        QTest::newRow((QByteArrayLiteral("show ") + fromObjectC).constData()) << QEvent::Show << fromObject;
        QTest::newRow((QByteArrayLiteral("enabled ") + fromObjectC).constData()) << QEvent::EnabledChange << fromObject;
        QTest::newRow((QByteArrayLiteral("focusIn ") + fromObjectC).constData()) << QEvent::FocusIn << fromObject;
        QTest::newRow((QByteArrayLiteral("focusOut ") + fromObjectC).constData()) << QEvent::FocusOut << fromObject;
        QTest::newRow((QByteArrayLiteral("keyPress ") + fromObjectC).constData()) << QEvent::KeyPress << fromObject;
    }
}

// protected bool eventFilter(QObject* object, QEvent* event)
void tst_QGraphicsProxyWidget::testEventFilter()
{
    QFETCH(QEvent::Type, eventType);
    QFETCH(bool, fromObject);

    QGraphicsScene scene;
    QEvent windowActivate(QEvent::WindowActivate);
    qApp->sendEvent(&scene, &windowActivate);

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    scene.addItem(proxy);

    QWidget *widget = new QWidget(0, Qt::FramelessWindowHint);
    widget->setFocusPolicy(Qt::TabFocus);
    widget->resize(10, 10);
    widget->show();
    proxy->setWidget(widget);
    proxy->show();

    // mirror whatever is happening to the widget
    // don't get in a loop
    switch (eventType) {
    case QEvent::None: {
        QEvent event(QEvent::None);
        proxy->call_eventFilter(widget, &event);
        break;
                       }
    case QEvent::Resize: {
        QSize newSize = QSize(100, 100);
        if (fromObject) {
            widget->resize(newSize);
        } else {
            proxy->resize(newSize);
        }
        QCOMPARE(proxy->size().toSize(), newSize);
        QCOMPARE(widget->size(), newSize);
        break;
                         }
    case QEvent::Move: {
        QPoint newPoint = QPoint(100, 100);
        if (fromObject) {
            widget->move(newPoint);
        } else {
            proxy->setPos(newPoint);
        }
        QCOMPARE(proxy->pos().toPoint(), newPoint);
        QCOMPARE(widget->pos(), newPoint);
        break;
                         }
    case QEvent::Hide: {
        // A hide event can only come from a widget
        if (fromObject) {
            widget->setFocus(Qt::TabFocusReason);
            widget->hide();
        } else {
            QHideEvent event;
            proxy->call_eventFilter(widget, &event);
        }
        QCOMPARE(proxy->isVisible(), false);
        break;
                         }
    case QEvent::Show: {
        // A show event can either come from a widget or somewhere else
        widget->hide();
        if (fromObject) {
            widget->show();
        } else {
            QShowEvent event;
            proxy->call_eventFilter(widget, &event);
        }
        QCOMPARE(proxy->isVisible(), true);
        break;
                         }
    case QEvent::EnabledChange: {
        widget->setEnabled(false);
        proxy->setEnabled(false);
        if (fromObject) {
            widget->setEnabled(true);
            QCOMPARE(proxy->isEnabled(), true);
            widget->setEnabled(false);
            QCOMPARE(proxy->isEnabled(), false);
        } else {
            QEvent event(QEvent::EnabledChange);
            proxy->call_eventFilter(widget, &event);
            // match the widget not the event
            QCOMPARE(proxy->isEnabled(), false);
        }
        break;
                         }
    case QEvent::FocusIn: {
        if (fromObject) {
            widget->setFocus(Qt::TabFocusReason);
            QVERIFY(proxy->hasFocus());
        }
        break;
                         }
    case QEvent::FocusOut: {
        widget->setFocus(Qt::TabFocusReason);
        QVERIFY(proxy->hasFocus());
        QVERIFY(widget->hasFocus());
        if (fromObject) {
            widget->clearFocus();
            QVERIFY(!proxy->hasFocus());
        }
        break;
                         }
    case QEvent::KeyPress: {
        if (fromObject) {
            QTest::keyPress(widget, Qt::Key_A, Qt::NoModifier);
        } else {
            QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
            proxy->call_eventFilter(widget, &event);
        }
        QCOMPARE(proxy->keyPress, 1);
        break;
                         }
    default:
        break;
    }
}

void tst_QGraphicsProxyWidget::focusInEvent_data()
{
    QTest::addColumn<bool>("widgetHasFocus");
    QTest::addColumn<bool>("widgetCanHaveFocus");
    QTest::newRow("no focus, can't get") << false << false;
    QTest::newRow("no focus, can get") << false << true;
    QTest::newRow("has focus, can't get") << true << false;
    QTest::newRow("has focus, can get") << true << true;
    // ### add test for widget having a focusNextPrevChild
}

// protected void focusInEvent(QFocusEvent* event)
void tst_QGraphicsProxyWidget::focusInEvent()
{
#ifdef Q_OS_WIN
    // Fails on Windows due QPlatformWindow::isActive() check required for embedded native widgets.
    // Since the test is apparently broken anyway, just skip it.
    QSKIP("Broken test.");
#endif

    // ### This test is just plain old broken
    QFETCH(bool, widgetHasFocus);
    QFETCH(bool, widgetCanHaveFocus);

    QGraphicsScene scene;
    QEvent windowActivate(QEvent::WindowActivate);
    qApp->sendEvent(&scene, &windowActivate);

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setEnabled(true);
    scene.addItem(proxy);

    QWidget *widget = new QWidget;
    widget->resize(100, 100);
    if (widgetCanHaveFocus)
        widget->setFocusPolicy(Qt::WheelFocus);
    widget->show();

    if (widgetHasFocus)
        widget->setFocus(Qt::TabFocusReason);

    proxy->setWidget(widget);
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // <- shouldn't need to do this

    // ### This test is just plain old broken - sending a focus in event
    // does not cause items to gain input focus. The widget has focus
    // because the proxy has focus, not because it got this event.

    QFocusEvent event(QEvent::FocusIn, Qt::TabFocusReason);
    event.ignore();
    proxy->call_focusInEvent(&event);
    QTRY_COMPARE(widget->hasFocus(), widgetCanHaveFocus);
}

void tst_QGraphicsProxyWidget::focusInEventNoWidget()
{
    QGraphicsView view;
    QGraphicsScene scene(&view);
    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setEnabled(true);
    scene.addItem(proxy);
    view.show();

    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // <- shouldn't need to do this
    QFocusEvent event(QEvent::FocusIn, Qt::TabFocusReason);
    event.ignore();
    //should not crash
    proxy->call_focusInEvent(&event);
}

void tst_QGraphicsProxyWidget::focusNextPrevChild_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::addColumn<bool>("hasScene");
    QTest::addColumn<bool>("next");
    QTest::addColumn<bool>("focusNextPrevChild");

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                bool next = (i == 0);
                bool hasWidget = (j == 0);
                bool hasScene = (k == 0);
                bool result = hasScene && hasWidget;
                QByteArray name = QByteArrayLiteral("Forward: ") + (next ? '1' : '0')
                    + ", hasWidget: " + (hasWidget ? '1' : '0') + ", hasScene: "
                    + (hasScene ? '1' : '0') + ", result: " + (result ? '1' : '0');
                QTest::newRow(name.constData()) << hasWidget << hasScene << next << result;
            }
        }
    }
}

// protected bool focusNextPrevChild(bool next)
void tst_QGraphicsProxyWidget::focusNextPrevChild()
{
    QFETCH(bool, next);
    QFETCH(bool, focusNextPrevChild);
    QFETCH(bool, hasWidget);
    QFETCH(bool, hasScene);

    // If a widget has its own focusNextPrevChild we need to respect it
    // otherwise respect the scene
    // Respect the widget over the scene!

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;

    QLabel *widget = new QLabel;
    // I can't believe designer adds this much junk!
    widget->setText("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"> p, li { white-space: pre-wrap; } </style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\"> <p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://www.slashdot.org\"><span style=\" text-decoration: underline; color:#0000ff;\">old</span></a> foo <a href=\"http://www.reddit.org\"><span style=\" text-decoration: underline; color:#0000ff;\">new</span></a></p></body></html>");
    widget->setTextInteractionFlags(Qt::TextBrowserInteraction);

    if (hasWidget)
        proxy->setWidget(widget);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    if (hasScene) {
        scene.addItem(proxy);
        proxy->show();

        // widget should take precedence over scene so make scene.focusNextPrevChild return false
        // so we know that a true can only come from the widget
        if (!(hasWidget && hasScene)) {
            QGraphicsTextItem *item = new QGraphicsTextItem("Foo");
            item->setTextInteractionFlags(Qt::TextBrowserInteraction);
            scene.addItem(item);
            item->setPos(50, 40);
        }
        scene.setFocusItem(proxy);
        QVERIFY(proxy->hasFocus());
    }

    QCOMPARE(proxy->call_focusNextPrevChild(next), focusNextPrevChild);

    if (!hasScene)
        delete proxy;
    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::focusOutEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::addColumn<bool>("call");
    QTest::newRow("no widget, focus to other widget") << false << false;
    QTest::newRow("no widget, focusOutCalled") << false << true;
    QTest::newRow("widget, focus to other widget") << true << false;
    QTest::newRow("widget, focusOutCalled") << true << true;
}

// protected void focusOutEvent(QFocusEvent* event)
void tst_QGraphicsProxyWidget::focusOutEvent()
{
    QFETCH(bool, hasWidget);
    QFETCH(bool, call);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    scene.addItem(proxy);
    view.show();
    QApplication::setActiveWindow(&view);
    view.activateWindow();
    view.setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.isVisible());
    QTRY_COMPARE(QApplication::activeWindow(), (QWidget*)&view);

    QScopedPointer<QWidget> widgetGuard(new QWidget);
    QWidget *widget = widgetGuard.data();
    widget->setFocusPolicy(Qt::WheelFocus);
    if (hasWidget)
        proxy->setWidget(widgetGuard.take());
    proxy->show();
    proxy->setFocus();
    QVERIFY(proxy->hasFocus());
    QEXPECT_FAIL("widget, focus to other widget", "Widget should have focus but doesn't", Continue);
    QEXPECT_FAIL("widget, focusOutCalled", "Widget should have focus but doesn't", Continue);
    QCOMPARE(widget->hasFocus(), hasWidget);

    if (!call) {
        QWidget *other = new QLineEdit(&view);
        other->show();
        QApplication::processEvents();
        QTRY_VERIFY(other->isVisible());
        other->setFocus();
        QTRY_VERIFY(other->hasFocus());
        qApp->processEvents();
        QTRY_COMPARE(proxy->hasFocus(), false);
        QVERIFY(proxy->focusOut);
        QCOMPARE(widget->hasFocus(), false);
    } else {
        {
            /*
            ### Test doesn't make sense
            QFocusEvent focusEvent(QEvent::FocusOut);
            proxy->call_focusOutEvent(&focusEvent);
            QCOMPARE(focusEvent.isAccepted(), hasWidget);
            qApp->processEvents();
            QCOMPARE(proxy->paintCount, hasWidget ? 1 : 0);
            */
        }
        {
            /*
            ### Test doesn't make sense
            proxy->setFlag(QGraphicsItem::ItemIsFocusable, false);
            QFocusEvent focusEvent(QEvent::FocusOut);
            proxy->call_focusOutEvent(&focusEvent);
            QCOMPARE(focusEvent.isAccepted(), hasWidget);
            qApp->processEvents();
            QCOMPARE(proxy->paintCount, 0);
            */
        }
    }
}

void tst_QGraphicsProxyWidget::focusProxy_QTBUG_51856()
{
    // QSpinBox has an internal QLineEdit; this QLineEdit has the spinbox
    // as its focus proxy.
    struct FocusedSpinBox : QSpinBox
    {
        int focusCount = 0;

        bool event(QEvent *event) override
        {
            switch (event->type()) {
            case QEvent::FocusIn:
                ++focusCount;
                break;
            case QEvent::FocusOut:
                --focusCount;
                break;
            default:
                break;
            }
            return QSpinBox::event(event);
        }
    };

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    scene.addItem(proxy);
    view.show();
    view.raise();
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    FocusedSpinBox *spinBox = new FocusedSpinBox;

    proxy->setWidget(spinBox);
    proxy->show();
    proxy->setFocus();
    QVERIFY(proxy->hasFocus());
    QEXPECT_FAIL("", "Widget should have focus but doesn't", Continue);
    QVERIFY(spinBox->hasFocus());
    QEXPECT_FAIL("", "Widget should have focus but doesn't", Continue);
    QCOMPARE(spinBox->focusCount, 1);

    enum { Count = 10 };

    for (int i = 0; i < Count; ++i) {
        for (int clickCount = 0; clickCount < Count; ++clickCount) {
            auto proxyCenter = proxy->boundingRect().center();
            auto proxyCenterInScene = proxy->mapToScene(proxyCenter);
            auto proxyCenterInView = view.mapFromScene(proxyCenterInScene);

            QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, proxyCenterInView);
            QTRY_COMPARE(spinBox->focusCount, 1);
        }

        QLineEdit *edit = new QLineEdit(&view);
        edit->show();
        QTRY_VERIFY(edit->isVisible());
        edit->setFocus();
        QTRY_VERIFY(edit->hasFocus());
        QTRY_VERIFY(!proxy->hasFocus());
        QTRY_COMPARE(proxy->focusOut, i + 1);
        QTRY_VERIFY(!spinBox->hasFocus());
        QTRY_COMPARE(spinBox->focusCount, 0);
        delete edit;
    }
}

class EventLogger : public QWidget
{
public:
    EventLogger() : QWidget(), enterCount(0), leaveCount(0), moveCount(0),
        hoverEnter(0), hoverLeave(0), hoverMove(0)
    {
        installEventFilter(this);
    }

    void enterEvent(QEvent *event)
    {
        enterCount++;
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event )
    {
        leaveCount++;
        QWidget::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        event->setAccepted(true);
        moveCount++;
        QWidget::mouseMoveEvent(event);
    }

    int enterCount;
    int leaveCount;
    int moveCount;

    int hoverEnter;
    int hoverLeave;
    int hoverMove;
protected:
    bool eventFilter(QObject *object, QEvent *event)
    {
        switch (event->type()) {
        case QEvent::HoverEnter:
             hoverEnter++;
             break;
        case QEvent::HoverLeave:
             hoverLeave++;
             break;
        case QEvent::HoverMove:
             hoverMove++;
             break;
        default:
             break;
        }
        return QWidget::eventFilter(object, event);
    }
};

void tst_QGraphicsProxyWidget::hoverEnterLeaveEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::addColumn<bool>("hoverEnabled");
    QTest::newRow("widget, no hover") << true << false;
    QTest::newRow("no widget, no hover") << false << false;
    QTest::newRow("widget, hover") << true << true;
    QTest::newRow("no widget, hover") << false << true;
}

// protected void hoverEnterEvent(QGraphicsSceneHoverEvent* event)
void tst_QGraphicsProxyWidget::hoverEnterLeaveEvent()
{
    QFETCH(bool, hasWidget);
    QFETCH(bool, hoverEnabled);

    // proxy should translate this into events that the widget would expect

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    QScopedPointer<EventLogger> widgetGuard(new EventLogger);
    EventLogger *widget = widgetGuard.data();
    widget->resize(50, 50);
    widget->setAttribute(Qt::WA_Hover, hoverEnabled);
    widget->setMouseTracking(true);
    view.resize(100, 100);
    if (hasWidget)
        proxy->setWidget(widgetGuard.take());
    proxy->setPos(50, 0);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    QTRY_VERIFY(sceneChangedSpy.count() > 0);

    // outside graphics item
    QTest::mouseMove(&view, QPoint(10, 10));

    QCOMPARE(widget->testAttribute(Qt::WA_UnderMouse), false);
    QCOMPARE(widget->enterCount, 0);
    QCOMPARE(widget->hoverEnter, 0);
    // over graphics item
    QTest::mouseMove(&view, QPoint(50, 50));
    QTRY_COMPARE(widget->testAttribute(Qt::WA_UnderMouse), hasWidget);
    QCOMPARE(widget->enterCount, hasWidget ? 1 : 0);
    QCOMPARE(widget->hoverEnter, (hasWidget && hoverEnabled) ? 1 : 0);

    QTRY_COMPARE(widget->leaveCount, 0);
    QTRY_COMPARE(widget->hoverLeave, 0);
    // outside graphics item
    QTest::mouseMove(&view, QPoint(10, 10));
    QTRY_COMPARE(widget->testAttribute(Qt::WA_UnderMouse), false);
    QTRY_COMPARE(widget->leaveCount, hasWidget ? 1 : 0);
    QTRY_COMPARE(widget->hoverLeave, (hasWidget && hoverEnabled) ? 1 : 0);
}

void tst_QGraphicsProxyWidget::hoverMoveEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::addColumn<bool>("hoverEnabled");
    QTest::addColumn<bool>("mouseTracking");
    QTest::addColumn<bool>("mouseDown");
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                for (int l = 0; l < 2; ++l) {
                    bool hasWidget = (i == 0);
                    bool hoverEnabled = (j == 0);
                    bool mouseTracking = (k == 0);
                    bool mouseDown = (l == 0);
                    QByteArray name = QByteArrayLiteral("hasWidget:") + (hasWidget ? '1' : '0') + ", hover:"
                        + (hoverEnabled ? '1' : '0') + ", mouseTracking:"
                        + (mouseTracking ? '1' : '0') + ", mouseDown: " + (mouseDown ? '1' : '0');
                    QTest::newRow(name.constData()) << hasWidget << hoverEnabled << mouseTracking << mouseDown;
                }
            }
        }
    }
}

// protected void hoverMoveEvent(QGraphicsSceneHoverEvent* event)
void tst_QGraphicsProxyWidget::hoverMoveEvent()
{
    QFETCH(bool, hasWidget);
    QFETCH(bool, hoverEnabled);
    QFETCH(bool, mouseTracking);
    QFETCH(bool, mouseDown);

    QSKIP("Ambiguous test...");

    // proxy should translate the move events to what the widget would expect

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    EventLogger *widget = new EventLogger;
    widget->resize(50, 50);
    widget->setAttribute(Qt::WA_Hover, hoverEnabled);
    if (mouseTracking)
        widget->setMouseTracking(true);
    view.resize(100, 100);
    if (hasWidget)
        proxy->setWidget(widget);
    proxy->setPos(50, 0);
    scene.addItem(proxy);

    // in
    QTest::mouseMove(&view, QPoint(50, 50));
    QTest::qWait(12);

    if (mouseDown)
        QTest::mousePress(view.viewport(), Qt::LeftButton);

    // move a little bit
    QTest::mouseMove(&view, QPoint(60, 60));
    QTRY_COMPARE(widget->hoverEnter, (hasWidget && hoverEnabled) ? 1 : 0);
    QCOMPARE(widget->moveCount, (hasWidget && mouseTracking) || (hasWidget && mouseDown) ? 1 : 0);

    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::keyPressEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::newRow("widget") << true;
    QTest::newRow("no widget") << false;
}

// protected void keyPressEvent(QKeyEvent* event)
void tst_QGraphicsProxyWidget::keyPressEvent()
{
    QFETCH(bool, hasWidget);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    view.viewport()->setFocus();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget*)&view);

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!

    QLineEdit *widget = new QLineEdit;
    widget->resize(50, 50);
    view.resize(100, 100);
    if (hasWidget) {
        proxy->setWidget(widget);
        proxy->show();
    }
    proxy->setPos(50, 0);
    scene.addItem(proxy);
    proxy->setFocus();

    QTest::keyPress(view.viewport(), 'x');

    QTRY_COMPARE(widget->text(), hasWidget ? QString("x") : QString());

    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::keyReleaseEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::newRow("widget") << true;
    QTest::newRow("no widget") << false;
}

// protected void keyReleaseEvent(QKeyEvent* event)
void tst_QGraphicsProxyWidget::keyReleaseEvent()
{
    QFETCH(bool, hasWidget);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget*)&view);


    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    QPushButton *widget = new QPushButton;
    QSignalSpy spy(widget, SIGNAL(clicked()));
    widget->resize(50, 50);
    view.resize(100, 100);
    if (hasWidget) {
        proxy->setWidget(widget);
        proxy->show();
    }
    proxy->setPos(50, 0);
    scene.addItem(proxy);
    proxy->setFocus();

    QTest::keyPress(view.viewport(), Qt::Key_Space);
    QTRY_COMPARE(spy.count(), 0);
    QTest::keyRelease(view.viewport(), Qt::Key_Space);
    QTRY_COMPARE(spy.count(), (hasWidget) ? 1 : 0);

    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::mouseDoubleClickEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::newRow("widget") << true;
    QTest::newRow("no widget") << false;
}

// protected void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
void tst_QGraphicsProxyWidget::mouseDoubleClickEvent()
{
    QFETCH(bool, hasWidget);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();

    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget*)&view);

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    QLineEdit *widget = new QLineEdit;
    widget->setText("foo");
    widget->resize(50, 50);
    view.resize(100, 100);
    if (hasWidget)
        proxy->setWidget(widget);
    proxy->setPos(50, 0);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    proxy->setFocus();

    // wait for scene to be updated before doing any coordinate mappings on it
    QTRY_VERIFY(sceneChangedSpy.count() > 0);

    QPoint pointInLineEdit = view.mapFromScene(proxy->mapToScene(15, proxy->boundingRect().center().y()));
    QTest::mousePress(view.viewport(), Qt::LeftButton, 0, pointInLineEdit);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, 0, pointInLineEdit);
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, 0, pointInLineEdit);

    QTRY_COMPARE(widget->selectedText(), hasWidget ? QString("foo") : QString());

    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::mousePressReleaseEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::newRow("widget") << true;
    QTest::newRow("no widget") << false;
}

// protected void mousePressEvent(QGraphicsSceneMouseEvent* event)
void tst_QGraphicsProxyWidget::mousePressReleaseEvent()
{
    QFETCH(bool, hasWidget);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.resize(500, 500);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    QPushButton *widget = new QPushButton;
    QSignalSpy spy(widget, SIGNAL(clicked()));
    widget->resize(50, 50);
    if (hasWidget)
        proxy->setWidget(widget);
    proxy->setPos(50, 0);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    proxy->setFocus();

    // wait for scene to be updated before doing any coordinate mappings on it
    QTRY_VERIFY(sceneChangedSpy.count() > 0);

    QPoint buttonCenter = view.mapFromScene(proxy->mapToScene(proxy->boundingRect().center()));
    QTest::mousePress(view.viewport(), Qt::LeftButton, 0, buttonCenter);
    QTRY_COMPARE(spy.count(), 0);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, 0, buttonCenter);
    QTRY_COMPARE(spy.count(), (hasWidget) ? 1 : 0);

    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::resizeEvent_data()
{
    QTest::addColumn<bool>("hasWidget");
    QTest::newRow("widget") << true;
    QTest::newRow("no widget") << false;
}

// protected void resizeEvent(QGraphicsSceneResizeEvent* event)
void tst_QGraphicsProxyWidget::resizeEvent()
{
    QFETCH(bool, hasWidget);

    SubQGraphicsProxyWidget proxy;
    QWidget *widget = new QWidget;
    if (hasWidget)
        proxy.setWidget(widget);

    QSize newSize(100, 100);
    QGraphicsSceneResizeEvent event;
    event.setOldSize(QSize(10, 10));
    event.setNewSize(newSize);
    proxy.call_resizeEvent(&event);
    if (hasWidget)
        QCOMPARE(widget->size(), newSize);
    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::paintEvent()
{
    //we test that calling update on a widget inside a QGraphicsView is triggering a repaint
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(view.isActiveWindow());

    SubQGraphicsProxyWidget proxy;

    QWidget *w = new QWidget;
    //showing the widget here seems to create a bug in Graphics View
    //this bug prevents the widget from being updated

    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));
    proxy.setWidget(w);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(&proxy);

    QTRY_VERIFY(sceneChangedSpy.count() > 0); // make sure the scene is ready

    proxy.paintCount = 0;
    w->update();
    QTRY_VERIFY(proxy.paintCount >= 1); //the widget should have been painted now
}


#if QT_CONFIG(wheelevent)
void tst_QGraphicsProxyWidget::wheelEvent()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    WheelWidget *wheelWidget = new WheelWidget();
    wheelWidget->setFixedSize(400, 400);

    QGraphicsProxyWidget *proxy = scene.addWidget(wheelWidget);
    proxy->setVisible(true);

    QGraphicsSceneWheelEvent event(QEvent::GraphicsSceneWheel);
    event.setScenePos(QPoint(50, 50));
    event.setAccepted(false);
    wheelWidget->wheelEventCalled = false;

    QApplication::sendEvent(&scene, &event);

    QVERIFY(event.isAccepted());
    QVERIFY(wheelWidget->wheelEventCalled);
}
#endif // QT_CONFIG(wheelevent)

Q_DECLARE_METATYPE(Qt::SizeHint)
void tst_QGraphicsProxyWidget::sizeHint_data()
{
    QTest::addColumn<Qt::SizeHint>("which");
    QTest::addColumn<QSizeF>("constraint");
    QTest::addColumn<QSizeF>("sizeHint");
    QTest::addColumn<bool>("hasWidget");

    for (int i = 0; i < 2; ++i) {
        bool hasWidget = (i == 0);
        // ### What should these do?
        QTest::newRow("min") << Qt::MinimumSize << QSizeF() << QSizeF() << hasWidget;
        QTest::newRow("pre") << Qt::PreferredSize << QSizeF() << QSizeF() << hasWidget;
        QTest::newRow("max") << Qt::MaximumSize << QSizeF() << QSizeF() << hasWidget;
        QTest::newRow("mindes") << Qt::MinimumDescent << QSizeF() << QSizeF() << hasWidget;
        QTest::newRow("nsize") << Qt::NSizeHints << QSizeF() << QSizeF() << hasWidget;
    }
}

// protected QSizeF sizeHint(Qt::SizeHint which, QSizeF const& constraint = QSizeF()) const
void tst_QGraphicsProxyWidget::sizeHint()
{
    QFETCH(Qt::SizeHint, which);
    QFETCH(QSizeF, constraint);
    QFETCH(QSizeF, sizeHint);
    QFETCH(bool, hasWidget);
    QSKIP("Broken test");
    SubQGraphicsProxyWidget proxy;
    QWidget *widget = new QWidget;
    if (hasWidget)
        proxy.setWidget(widget);
    QCOMPARE(proxy.call_sizeHint(which, constraint), sizeHint);
    if (!hasWidget)
        delete widget;
}

void tst_QGraphicsProxyWidget::sizePolicy()
{
    for (int p = 0; p < 2; ++p) {
        bool hasWidget = (p == 0);
        SubQGraphicsProxyWidget proxy;
        QWidget *widget = new QWidget;
        QSizePolicy proxyPol(QSizePolicy::Maximum, QSizePolicy::Expanding);
        proxy.setSizePolicy(proxyPol);
        QSizePolicy widgetPol(QSizePolicy::Fixed, QSizePolicy::Minimum);
        widget->setSizePolicy(widgetPol);

        QCOMPARE(proxy.sizePolicy(), proxyPol);
        QCOMPARE(widget->sizePolicy(), widgetPol);
        if (hasWidget) {
            proxy.setWidget(widget);
            QCOMPARE(proxy.sizePolicy(), widgetPol);
        } else {
            QCOMPARE(proxy.sizePolicy(), proxyPol);
        }
        QCOMPARE(widget->sizePolicy(), widgetPol);

        proxy.setSizePolicy(widgetPol);
        widget->setSizePolicy(proxyPol);
        if (hasWidget)
            QCOMPARE(proxy.sizePolicy(), proxyPol);
        else
            QCOMPARE(proxy.sizePolicy(), widgetPol);

        if (!hasWidget)
            delete widget;
    }
}

void tst_QGraphicsProxyWidget::minimumSize()
{
    SubQGraphicsProxyWidget proxy;
    QWidget *widget = new QWidget;
    QSize minSize(50, 50);
    widget->setMinimumSize(minSize);
    proxy.resize(30, 30);
    widget->resize(30,30);
    QCOMPARE(proxy.size(), QSizeF(30, 30));
    proxy.setWidget(widget);
    QCOMPARE(proxy.size().toSize(), minSize);
    QCOMPARE(proxy.minimumSize().toSize(), minSize);
    widget->setMinimumSize(70, 70);
    QCOMPARE(proxy.minimumSize(), QSizeF(70, 70));
    QCOMPARE(proxy.size(), QSizeF(70, 70));
}

void tst_QGraphicsProxyWidget::maximumSize()
{
    SubQGraphicsProxyWidget proxy;
    QWidget *widget = new QWidget;
    QSize maxSize(150, 150);
    widget->setMaximumSize(maxSize);
    proxy.resize(200, 200);
    widget->resize(200,200);
    QCOMPARE(proxy.size(), QSizeF(200, 200));
    proxy.setWidget(widget);
    QCOMPARE(proxy.size().toSize(), maxSize);
    QCOMPARE(proxy.maximumSize().toSize(), maxSize);
    widget->setMaximumSize(70, 70);
    QCOMPARE(proxy.maximumSize(), QSizeF(70, 70));
    QCOMPARE(proxy.size(), QSizeF(70, 70));
}

class View : public QGraphicsView
{
public:
    View(QGraphicsScene *scene, QWidget *parent = 0)
        : QGraphicsView(scene, parent), npaints(0)
    { }
    QRegion paintEventRegion;
    int npaints;
protected:
    void paintEvent(QPaintEvent *event)
    {
        ++npaints;
        paintEventRegion += event->region();
        QGraphicsView::paintEvent(event);
    }
};

class ScrollWidget : public QWidget
{
    Q_OBJECT
public:
    ScrollWidget() : npaints(0)
    {
        resize(200, 200);
    }
    QRegion paintEventRegion;
    int npaints;

public slots:
    void updateScroll()
    {
        update(0, 0, 200, 10);
        scroll(0, 10, QRect(0, 0, 100, 20));
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        ++npaints;
        paintEventRegion += event->region();
        QPainter painter(this);
        painter.fillRect(event->rect(), Qt::blue);
    }
};

// ### work around missing QVector ctor from iterator pair:
static QVector<QRect> rects(const QRegion &region)
{
    QVector<QRect> result;
    for (QRect r : region)
        result.push_back(r);
    return result;
}

void tst_QGraphicsProxyWidget::scrollUpdate()
{
    ScrollWidget *widget = new ScrollWidget;

    QGraphicsScene scene;
    scene.addWidget(widget);

    View view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.npaints >= 1);
    QTest::qWait(20);
    widget->paintEventRegion = QRegion();
    widget->npaints = 0;
    view.paintEventRegion = QRegion();
    view.npaints = 0;
    QTimer::singleShot(0, widget, SLOT(updateScroll()));
    QTRY_COMPARE(view.npaints, 2);
    // QRect(0, 0, 200, 12) is the first update, expanded (-2, -2, 2, 2)
    // QRect(0, 12, 102, 10) is the scroll update, expanded (-2, -2, 2, 2),
    // intersected with the above update.
    QCOMPARE(rects(view.paintEventRegion),
             QVector<QRect>() << QRect(0, 0, 200, 12) << QRect(0, 12, 102, 10));
    QCOMPARE(widget->npaints, 2);
    QCOMPARE(rects(widget->paintEventRegion),
             QVector<QRect>() << QRect(0, 0, 200, 12) << QRect(0, 12, 102, 10));
}

void tst_QGraphicsProxyWidget::setWidget_simple()
{
    QGraphicsProxyWidget proxy;
    QLineEdit *lineEdit = new QLineEdit;
    proxy.setWidget(lineEdit);

    QVERIFY(lineEdit->testAttribute(Qt::WA_DontShowOnScreen));
    // Size hints
    // ### size hints are tested in a different test
    // QCOMPARE(proxy.effectiveSizeHint(Qt::MinimumSize).toSize(), lineEdit->minimumSizeHint());
    // QCOMPARE(proxy.effectiveSizeHint(Qt::MaximumSize).toSize(), lineEdit->maximumSize());
    // QCOMPARE(proxy.effectiveSizeHint(Qt::PreferredSize).toSize(), lineEdit->sizeHint());
    QCOMPARE(proxy.size().toSize(), lineEdit->minimumSizeHint().expandedTo(lineEdit->size()));
    QRect rect = lineEdit->rect();
    rect.setSize(rect.size().expandedTo(lineEdit->minimumSizeHint()));
    QCOMPARE(proxy.rect().toRect(), rect);

    // Properties
    // QCOMPARE(proxy.focusPolicy(), lineEdit->focusPolicy());
    // QCOMPARE(proxy.palette(), lineEdit->palette());
#ifndef QT_NO_CURSOR
    QCOMPARE(proxy.cursor().shape(), lineEdit->cursor().shape());
#endif
    QCOMPARE(proxy.layoutDirection(), lineEdit->layoutDirection());
    QCOMPARE(proxy.style(), lineEdit->style());
    QCOMPARE(proxy.font(), lineEdit->font());
    QCOMPARE(proxy.isEnabled(), lineEdit->isEnabled());
    QCOMPARE(proxy.isVisible(), lineEdit->isVisible());
}

void tst_QGraphicsProxyWidget::setWidget_ownership()
{
    QPointer<QLineEdit> lineEdit = new QLineEdit;
    QPointer<QLineEdit> lineEdit2 = new QLineEdit;
    QVERIFY(lineEdit);
    {
        // Create a proxy and transfer ownership to it
        QGraphicsProxyWidget proxy;
        proxy.setWidget(lineEdit);
        QCOMPARE(proxy.widget(), (QWidget *)lineEdit);

        // Remove the widget without destroying it.
        proxy.setWidget(0);
        QVERIFY(!proxy.widget());
        QVERIFY(lineEdit);

        // Assign the widget again and switch to another widget.
        proxy.setWidget(lineEdit);
        proxy.setWidget(lineEdit2);
        QCOMPARE(proxy.widget(), (QWidget *)lineEdit2);

        // Assign the first widget, and destroy the proxy.
        proxy.setWidget(lineEdit);
    }
    QVERIFY(!lineEdit);
    QVERIFY(lineEdit2);

    QGraphicsScene scene;
    QPointer<QGraphicsProxyWidget> proxy = scene.addWidget(lineEdit2);

    delete lineEdit2;
    QVERIFY(!proxy);
}

void tst_QGraphicsProxyWidget::resize_simple_data()
{
    QTest::addColumn<QSizeF>("size");

    QTest::newRow("200, 200") << QSizeF(200, 200);
#if !defined(Q_PROCESSOR_ARM)
    QTest::newRow("1000, 1000") << QSizeF(1000, 1000);
    // Since 4.5, 10000x10000 runs out of memory.
    // QTest::newRow("10000, 10000") << QSizeF(10000, 10000);
#endif
}

void tst_QGraphicsProxyWidget::resize_simple()
{
    QFETCH(QSizeF, size);

    QGraphicsProxyWidget proxy;
    QWidget *widget = new QWidget;
    widget->setGeometry(0, 0, (int)size.width(), (int)size.height());
    proxy.setWidget(widget);
    widget->show();
    QCOMPARE(widget->pos(), QPoint());

    // The proxy resizes itself, the line edit follows
    proxy.resize(size);
    QCOMPARE(proxy.size(), size);
    QCOMPARE(proxy.size().toSize(), widget->size());

    // The line edit resizes itself, the proxy follows (no loopback/live lock)
    QSize doubleSize = size.toSize() * 2;
    widget->resize(doubleSize);
    QCOMPARE(widget->size(), doubleSize);
    QCOMPARE(widget->size(), proxy.size().toSize());
}

void tst_QGraphicsProxyWidget::symmetricMove()
{
    QGraphicsProxyWidget proxy;
    QLineEdit *lineEdit = new QLineEdit;
    proxy.setWidget(lineEdit);
    lineEdit->show();

    proxy.setPos(10, 10);
    QCOMPARE(proxy.pos(), QPointF(10, 10));
    QCOMPARE(lineEdit->pos(), QPoint(10, 10));

    lineEdit->move(5, 5);
    QCOMPARE(proxy.pos(), QPointF(5, 5));
    QCOMPARE(lineEdit->pos(), QPoint(5, 5));
}

void tst_QGraphicsProxyWidget::symmetricResize()
{
    QGraphicsProxyWidget proxy;
    QLineEdit *lineEdit = new QLineEdit;
    proxy.setWidget(lineEdit);
    lineEdit->show();

    proxy.resize(256, 256);
    QCOMPARE(proxy.size(), QSizeF(256, 256));
    QCOMPARE(lineEdit->size(), QSize(256, 256));

    lineEdit->resize(512, 512);
    QCOMPARE(proxy.size(), QSizeF(512, 512));
    QCOMPARE(lineEdit->size(), QSize(512, 512));
}

void tst_QGraphicsProxyWidget::symmetricVisible()
{
    QGraphicsProxyWidget proxy;
    QLineEdit *lineEdit = new QLineEdit;
    proxy.setWidget(lineEdit);
    lineEdit->show();

    QCOMPARE(proxy.isVisible(), lineEdit->isVisible());

    proxy.hide();
    QCOMPARE(proxy.isVisible(), lineEdit->isVisible());
    proxy.show();
    QCOMPARE(proxy.isVisible(), lineEdit->isVisible());
    lineEdit->hide();
    QCOMPARE(proxy.isVisible(), lineEdit->isVisible());
    lineEdit->show();
    QCOMPARE(proxy.isVisible(), lineEdit->isVisible());
 }

void tst_QGraphicsProxyWidget::symmetricEnabled()
{
    QGraphicsProxyWidget proxy;
    QLineEdit *lineEdit = new QLineEdit;
    proxy.setWidget(lineEdit);
    lineEdit->show();

    QCOMPARE(proxy.isEnabled(), lineEdit->isEnabled());
    proxy.setEnabled(false);
    QCOMPARE(proxy.isEnabled(), lineEdit->isEnabled());
    proxy.setEnabled(true);
    QCOMPARE(proxy.isEnabled(), lineEdit->isEnabled());
    lineEdit->setEnabled(false);
    QCOMPARE(proxy.isEnabled(), lineEdit->isEnabled());
    lineEdit->setEnabled(true);
    QCOMPARE(proxy.isEnabled(), lineEdit->isEnabled());
}

void tst_QGraphicsProxyWidget::tabFocus_simpleWidget()
{
    QGraphicsScene scene;
    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit);

    // Tab into line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(!leftDial->hasFocus());
    QTRY_VERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(editProxy->hasFocus());
    QVERIFY(edit->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 0);

    // Tab into right dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QVERIFY(!scene.hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(!edit->hasFocus());
    QTRY_VERIFY(rightDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);

    // Backtab into line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QTRY_VERIFY(scene.hasFocus());
    QVERIFY(editProxy->hasFocus());
    QVERIFY(edit->hasFocus());
    QVERIFY(!rightDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);

    // Backtab into left dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QVERIFY(!scene.hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(!edit->hasFocus());
    QTRY_VERIFY(leftDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 2);

    delete view;
}

void tst_QGraphicsProxyWidget::tabFocus_simpleTwoWidgets()
{
    QGraphicsScene scene;
    QLineEdit *edit = new QLineEdit;
    QLineEdit *edit2 = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();
    QGraphicsProxyWidget *editProxy2 = scene.addWidget(edit2);
    editProxy2->show();
    editProxy2->setPos(0, editProxy->rect().height() * 1.1);

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit);
    EventSpy eventSpy2(edit2);

    // Tab into line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!leftDial->hasFocus());
    QVERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(editProxy->hasFocus());
    QVERIFY(edit->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 0);

    // Tab into second line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(!edit->hasFocus());
    QVERIFY(editProxy2->hasFocus());
    QVERIFY(edit2->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 0);

    // Tab into right dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QVERIFY(!scene.hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(!edit->hasFocus());
    QVERIFY(!editProxy2->hasFocus());
    QVERIFY(!edit2->hasFocus());
    QVERIFY(rightDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 1);

    // Backtab into line edit 2
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(!edit->hasFocus());
    QVERIFY(editProxy2->hasFocus());
    QVERIFY(edit2->hasFocus());
    QVERIFY(!rightDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 1);

    // Backtab into line edit 1
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(editProxy->hasFocus());
    QVERIFY(edit->hasFocus());
    QVERIFY(!editProxy2->hasFocus());
    QVERIFY(!edit2->hasFocus());
    QVERIFY(!rightDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 2);

    // Backtab into left dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QVERIFY(!scene.hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(!edit->hasFocus());
    QVERIFY(leftDial->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 2);

    delete view;
}

void tst_QGraphicsProxyWidget::tabFocus_complexWidget()
{
    QGraphicsScene scene;

    QLineEdit *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1");
    QLineEdit *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2");
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(edit1);
    vlayout->addWidget(edit2);

    QGroupBox *box = new QGroupBox("QGroupBox");
    box->setCheckable(true);
    box->setChecked(true);
    box->setLayout(vlayout);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit1);
    EventSpy eventSpy2(edit2);
    EventSpy eventSpyBox(box);

    // Tab into group box
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!leftDial->hasFocus());
    QVERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(proxy->hasFocus());
    QVERIFY(box->hasFocus());

    // Tab into line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    edit1->hasFocus();
    QVERIFY(!box->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 0);

    // Tab into line edit 2
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    edit2->hasFocus();
    QVERIFY(!edit1->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 0);

    // Tab into right dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!edit2->hasFocus());
    rightDial->hasFocus();
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 1);

    // Backtab into line edit 2
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!rightDial->hasFocus());
    edit2->hasFocus();
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 1);

    // Backtab into line edit 1
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit2->hasFocus());
    edit1->hasFocus();
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 2);
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 2);

    // Backtab into line box
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit1->hasFocus());
    box->hasFocus();
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 2);

    // Backtab into left dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!box->hasFocus());
    leftDial->hasFocus();

    delete view;
}

void tst_QGraphicsProxyWidget::tabFocus_complexTwoWidgets()
{
    // ### add event spies to this test.
    QGraphicsScene scene;

    QLineEdit *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1");
    QLineEdit *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2");
    QFontComboBox *fontComboBox = new QFontComboBox;
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(edit1);
    vlayout->addWidget(fontComboBox);
    vlayout->addWidget(edit2);

    QGroupBox *box = new QGroupBox("QGroupBox");
    box->setCheckable(true);
    box->setChecked(true);
    box->setLayout(vlayout);

    QLineEdit *edit1_2 = new QLineEdit;
    edit1_2->setText("QLineEdit 1_2");
    QLineEdit *edit2_2 = new QLineEdit;
    edit2_2->setText("QLineEdit 2_2");
    QFontComboBox *fontComboBox2 = new QFontComboBox;
    vlayout = new QVBoxLayout;
    vlayout->addWidget(edit1_2);
    vlayout->addWidget(fontComboBox2);
    vlayout->addWidget(edit2_2);

    QGroupBox *box_2 = new QGroupBox("QGroupBox 2");
    box_2->setCheckable(true);
    box_2->setChecked(true);
    box_2->setLayout(vlayout);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();

    QGraphicsProxyWidget *proxy_2 = scene.addWidget(box_2);
    proxy_2->setPos(proxy->boundingRect().width() * 1.2, 0);
    proxy_2->show();

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->rotate(45);
    view->scale(0.5, 0.5);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit1);
    EventSpy eventSpy2(edit2);
    EventSpy eventSpy3(fontComboBox);
    EventSpy eventSpy1_2(edit1_2);
    EventSpy eventSpy2_2(edit2_2);
    EventSpy eventSpy2_3(fontComboBox2);
    EventSpy eventSpyBox(box);

    // Tab into group box
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!leftDial->hasFocus());
    QVERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(proxy->hasFocus());
    QVERIFY(box->hasFocus());

    // Tab into line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    edit1->hasFocus();
    QVERIFY(!box->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 0);

    // Tab to the font combobox
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    fontComboBox->hasFocus();
    QVERIFY(!edit2->hasFocus());
    QCOMPARE(eventSpy3.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy3.counts[QEvent::FocusOut], 0);
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);

    // Tab into line edit 2
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    edit2->hasFocus();
    QVERIFY(!edit1->hasFocus());
    QCOMPARE(eventSpy2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2.counts[QEvent::FocusOut], 0);
    QCOMPARE(eventSpy3.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);

    // Tab into right box
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!edit2->hasFocus());
    box_2->hasFocus();

    // Tab into right top line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!box_2->hasFocus());
    edit1_2->hasFocus();
    QCOMPARE(eventSpy1_2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy1_2.counts[QEvent::FocusOut], 0);

    // Tab into right font combobox
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!edit1_2->hasFocus());
    fontComboBox2->hasFocus();
    QCOMPARE(eventSpy1_2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy1_2.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2_3.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2_3.counts[QEvent::FocusOut], 0);

    // Tab into right bottom line edit
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!edit1_2->hasFocus());
    edit2_2->hasFocus();
    QCOMPARE(eventSpy1_2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy1_2.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2_3.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2_3.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusOut], 0);

    // Tab into right dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QVERIFY(!edit2->hasFocus());
    rightDial->hasFocus();
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusOut], 1);

    // Backtab into line edit 2
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!rightDial->hasFocus());
    edit2_2->hasFocus();

    // Backtab into the right font combobox
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit2_2->hasFocus());
    fontComboBox2->hasFocus();

    // Backtab into line edit 1
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit2_2->hasFocus());
    edit1_2->hasFocus();

    // Backtab into line box
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit1_2->hasFocus());
    box_2->hasFocus();

    // Backtab into line edit 2
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!rightDial->hasFocus());
    edit2->hasFocus();

    // Backtab into the font combobox
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit2->hasFocus());
    fontComboBox->hasFocus();

    // Backtab into line edit 1
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!fontComboBox->hasFocus());
    edit1->hasFocus();

    // Backtab into line box
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!edit1->hasFocus());
    box->hasFocus();

    // Backtab into left dial
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QVERIFY(!box->hasFocus());
    leftDial->hasFocus();

    delete view;
}

void tst_QGraphicsProxyWidget::setFocus_simpleWidget()
{
    QGraphicsScene scene;
    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QApplication::activeWindow(), &window);

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit);

    // Calling setFocus for the line edit when the view doesn't have input
    // focus does not steal focus from the dial. The edit, proxy and scene
    // have focus but only in their own little space.
    edit->setFocus();
    QVERIFY(scene.hasFocus());
    QVERIFY(edit->hasFocus());
    QVERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QVERIFY(leftDial->hasFocus());

    // Clearing focus is a local operation.
    edit->clearFocus();
    QVERIFY(scene.hasFocus());
    QVERIFY(!edit->hasFocus());
    QVERIFY(!view->hasFocus());
    QVERIFY(leftDial->hasFocus());

    view->setFocus();
    QVERIFY(scene.hasFocus());
    QVERIFY(view->hasFocus());
    QVERIFY(!leftDial->hasFocus());
    QVERIFY(!edit->hasFocus());

    scene.clearFocus();
    QVERIFY(!scene.hasFocus());

    edit->setFocus();
    QVERIFY(scene.hasFocus());
    QVERIFY(edit->hasFocus());
    QVERIFY(editProxy->hasFocus());

    // Symmetry
    editProxy->clearFocus();
    QVERIFY(!edit->hasFocus());

    delete view;
}

void tst_QGraphicsProxyWidget::setFocus_simpleTwoWidgets()
{
    QGraphicsScene scene;
    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();
    QLineEdit *edit2 = new QLineEdit;
    QGraphicsProxyWidget *edit2Proxy = scene.addWidget(edit2);
    edit2Proxy->show();
    edit2Proxy->setPos(editProxy->boundingRect().width() * 1.01, 0);

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QApplication::activeWindow(), &window);

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit);

    view->setFocus();
    QVERIFY(!edit->hasFocus());

    edit->setFocus();
    QVERIFY(scene.hasFocus());
    QVERIFY(edit->hasFocus());
    QVERIFY(editProxy->hasFocus());

    edit2->setFocus();
    QVERIFY(scene.hasFocus());
    QVERIFY(!edit->hasFocus());
    QVERIFY(!editProxy->hasFocus());
    QVERIFY(edit2->hasFocus());
    QVERIFY(edit2Proxy->hasFocus());

    delete view;
}

void tst_QGraphicsProxyWidget::setFocus_complexTwoWidgets()
{
    // ### add event spies to this test.
    QGraphicsScene scene;

    QLineEdit *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1");
    QLineEdit *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2");
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(edit1);
    vlayout->addWidget(edit2);

    QGroupBox *box = new QGroupBox("QGroupBox");
    box->setCheckable(true);
    box->setChecked(true);
    box->setLayout(vlayout);

    QLineEdit *edit1_2 = new QLineEdit;
    edit1_2->setText("QLineEdit 1_2");
    QLineEdit *edit2_2 = new QLineEdit;
    edit2_2->setText("QLineEdit 2_2");
    vlayout = new QVBoxLayout;
    vlayout->addWidget(edit1_2);
    vlayout->addWidget(edit2_2);

    QGroupBox *box_2 = new QGroupBox("QGroupBox 2");
    box_2->setCheckable(true);
    box_2->setChecked(true);
    box_2->setLayout(vlayout);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();

    QGraphicsProxyWidget *proxy_2 = scene.addWidget(box_2);
    proxy_2->setPos(proxy->boundingRect().width() * 1.2, 0);
    proxy_2->show();

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    window.show();
    QApplication::setActiveWindow(&window);
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QApplication::activeWindow(), &window);

    leftDial->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(leftDial->hasFocus());

    EventSpy eventSpy(edit1);
    EventSpy eventSpy2(edit2);
    EventSpy eventSpyBox(box);
    EventSpy eventSpy_2(edit1_2);
    EventSpy eventSpy2_2(edit2_2);
    EventSpy eventSpyBox_2(box_2);

    view->setFocus();

    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 0);

    edit1->setFocus();
    QApplication::processEvents();
    QVERIFY(scene.hasFocus());
    QVERIFY(edit1->hasFocus());
    QVERIFY(!box->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpyBox.counts[QEvent::FocusIn], 0);

    edit2_2->setFocus();
    QApplication::processEvents();
    QVERIFY(!edit1->hasFocus());
    QVERIFY(!box_2->hasFocus());
    QVERIFY(edit2_2->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpyBox.counts[QEvent::FocusIn], 0);
    QCOMPARE(eventSpyBox_2.counts[QEvent::FocusIn], 0);

    box->setFocus();
    QApplication::processEvents();
    QVERIFY(!edit2_2->hasFocus());
    QVERIFY(!edit1->hasFocus());
    QVERIFY(box->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpyBox.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpyBox.counts[QEvent::FocusOut], 0);
    QCOMPARE(eventSpyBox_2.counts[QEvent::FocusIn], 0);
    QCOMPARE(eventSpyBox_2.counts[QEvent::FocusOut], 0);

    edit2_2->setFocus();
    QApplication::processEvents();
    QVERIFY(edit2_2->hasFocus());
    QVERIFY(!edit1->hasFocus());
    QVERIFY(!box->hasFocus());
    QVERIFY(!box_2->hasFocus());
    QCOMPARE(eventSpy.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpy.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusIn], 2);
    QCOMPARE(eventSpy2_2.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpyBox.counts[QEvent::FocusIn], 1);
    QCOMPARE(eventSpyBox.counts[QEvent::FocusOut], 1);
    QCOMPARE(eventSpyBox_2.counts[QEvent::FocusIn], 0);
    QCOMPARE(eventSpyBox_2.counts[QEvent::FocusOut], 0);

    delete view;
}

void tst_QGraphicsProxyWidget::popup_basic()
{
    QScopedPointer<QComboBox> box(new QComboBox);
    QStyleOptionComboBox opt;
    opt.initFrom(box.data());
    opt.editable = box->isEditable();
    if (box->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt))
        QSKIP("Does not work due to SH_Combobox_Popup");

    // ProxyWidget should automatically create proxy's when the widget creates a child
    QGraphicsScene *scene = new QGraphicsScene;
    QGraphicsView view(scene);
    view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view.setGeometry(0, 100, 480, 500);
    view.show();

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    box->setGeometry(0, 0, 320, 40);
    box->addItems(QStringList() << "monday" << "tuesday" << "wednesday"
                  << "thursday" << "saturday" << "sunday");
    QCOMPARE(proxy->childItems().count(), 0);
    proxy->setWidget(box.data());
    proxy->show();
    scene->addItem(proxy);

    QCOMPARE(box->pos(), QPoint());
    QCOMPARE(proxy->pos(), QPointF());

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(125);
    QApplication::processEvents();

    QTest::mousePress(view.viewport(), Qt::LeftButton, 0,
                      view.mapFromScene(proxy->mapToScene(proxy->boundingRect().center())));

    QTRY_COMPARE(box->pos(), QPoint());

    QCOMPARE(proxy->childItems().count(), 1);
    QGraphicsProxyWidget *child = (QGraphicsProxyWidget*)(proxy->childItems())[0];
    QVERIFY(child->isWidget());
    QVERIFY(child->widget());
    QCOMPARE(child->widget()->parent(), static_cast<QObject*>(box.data()));

    QTRY_COMPARE(proxy->pos(), QPointF(box->pos()));
    QCOMPARE(child->x(), qreal(box->x()));
    QCOMPARE(child->y(), qreal(box->rect().bottom()));
#ifndef Q_OS_WIN
    // The popup's coordinates on Windows are in global space,
    // regardless.
    QCOMPARE(child->widget()->x(), box->x());
    QCOMPARE(child->widget()->y(), box->rect().bottom());
    QCOMPARE(child->geometry().toRect(), child->widget()->geometry());
#endif
    QTest::qWait(12);
}

void tst_QGraphicsProxyWidget::popup_subwidget()
{
    QGroupBox *groupBox = new QGroupBox;
    groupBox->setTitle("GroupBox");
    groupBox->setCheckable(true);

    QComboBox *box = new QComboBox;
    box->addItems(QStringList() << "monday" << "tuesday" << "wednesday"
                  << "thursday" << "saturday" << "sunday");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLineEdit("QLineEdit"));
    layout->addWidget(box);
    layout->addWidget(new QLineEdit("QLineEdit"));
    groupBox->setLayout(layout);

    QGraphicsScene scene;
    QGraphicsProxyWidget *groupBoxProxy = scene.addWidget(groupBox);
    groupBox->show();
    groupBox->move(10000, 10000);
    QCOMPARE(groupBox->pos(), QPoint(10000, 10000));
    QCOMPARE(groupBoxProxy->pos(), QPointF(10000, 10000));

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    box->showPopup();

    QVERIFY(!groupBoxProxy->childItems().isEmpty());

    QStyleOptionComboBox opt;
    opt.initFrom(box);
    opt.editable = box->isEditable();
    if (box->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt))
        QSKIP("Does not work due to SH_Combobox_Popup");
    QGraphicsProxyWidget *child = (QGraphicsProxyWidget*)(groupBoxProxy->childItems())[0];
    QWidget *popup = child->widget();
    QCOMPARE(popup->parent(), static_cast<QObject*>(box));
    QCOMPARE(popup->x(), box->mapToGlobal(QPoint()).x());
    QCOMPARE(popup->y(), QRect(box->mapToGlobal(QPoint()), box->size()).bottom());
    QCOMPARE(popup->size(), child->size().toSize());
}

void tst_QGraphicsProxyWidget::changingCursor_basic()
{
#if !QT_CONFIG(cursor)
    QSKIP("This test requires the QCursor API");
#else
    // Confirm that mouse events are working properly by checking that
    // when moving the mouse over a line edit it will change the cursor into the I
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    SubQGraphicsProxyWidget *proxy = new SubQGraphicsProxyWidget;
    QLineEdit *widget = new QLineEdit;
    proxy->setWidget(widget);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    QTRY_VERIFY(sceneChangedSpy.count() > 0); // make sure the scene is ready

    // in
    QTest::mouseMove(view.viewport(), view.mapFromScene(proxy->mapToScene(proxy->boundingRect().center())));
    QTRY_COMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);

    // out
    QTest::mouseMove(view.viewport(), QPoint(1, 1));
    QTRY_COMPARE(view.viewport()->cursor().shape(), Qt::ArrowCursor);
#endif // !QT_CONFIG(cursor)
}

static bool findViewAndTipLabel(const QWidget *view)
{
    bool foundView = false;
    bool foundTipLabel = false;
    const QWidgetList &topLevels = QApplication::topLevelWidgets();
    for (const QWidget *widget : topLevels) {
        if (widget == view)
            foundView = true;
        if (widget->inherits("QTipLabel"))
            foundTipLabel = true;
        if (foundView && foundTipLabel)
            return true;
    }
    return false;
}

void tst_QGraphicsProxyWidget::tooltip_basic()
{
    QString toolTip = "Qt rocks!";
    QString toolTip2 = "Qt rocks even more!";

    QPushButton *button = new QPushButton("button");
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget;
    QGraphicsProxyWidgetPrivate *proxyd = static_cast<QGraphicsProxyWidgetPrivate *>(QGraphicsItemPrivate::get(proxy));
    proxy->setWidget(button);
    proxyd->lastWidgetUnderMouse = button; // force widget under mouse

    QVERIFY(button->toolTip().isEmpty());
    QVERIFY(proxy->toolTip().isEmpty());
    // Check that setting the tooltip on the proxy also set it on the widget
    proxy->setToolTip(toolTip);
    QCOMPARE(proxy->toolTip(), toolTip);
    QCOMPARE(button->toolTip(), toolTip);
    // Check that setting the tooltip on the widget also set it on the proxy
    button->setToolTip(toolTip2);
    QCOMPARE(proxy->toolTip(), toolTip2);
    QCOMPARE(button->toolTip(), toolTip2);

    QGraphicsScene scene;
    scene.addItem(proxy);

    QGraphicsView view(&scene);
    view.setFixedSize(200, 200);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    {
        QHelpEvent helpEvent(QEvent::ToolTip, view.viewport()->rect().topLeft(),
                             view.viewport()->mapToGlobal(view.viewport()->rect().topLeft()));
        QApplication::sendEvent(view.viewport(), &helpEvent);
        QTest::qWait(350);

        bool foundView = false;
        bool foundTipLabel = false;
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            if (widget == &view)
                foundView = true;
            if (widget->inherits("QTipLabel"))
                foundTipLabel = true;
        }
        QVERIFY(foundView);
        QVERIFY(!foundTipLabel);
    }

    {
        QHelpEvent helpEvent(QEvent::ToolTip, view.mapFromScene(proxy->boundingRect().center()),
                             view.viewport()->mapToGlobal(view.mapFromScene(proxy->boundingRect().center())));
        QApplication::sendEvent(view.viewport(), &helpEvent);
        QTRY_VERIFY(findViewAndTipLabel(&view));
    }
}

void tst_QGraphicsProxyWidget::childPos_data()
{
    QTest::addColumn<bool>("moveCombo");
    QTest::addColumn<QPoint>("comboPos");
    QTest::addColumn<QPointF>("proxyPos");

    QTest::newRow("0") << true << QPoint() << QPointF();
    QTest::newRow("1") << true << QPoint(10, 0) << QPointF(10, 0);
    QTest::newRow("2") << true << QPoint(100, 0) << QPointF(100, 0);
    QTest::newRow("3") << true << QPoint(1000, 0) << QPointF(1000, 0);
    QTest::newRow("4") << true << QPoint(10000, 0) << QPointF(10000, 0);
    QTest::newRow("5") << true << QPoint(-10000, 0) << QPointF(-10000, 0);
    QTest::newRow("6") << true << QPoint(-1000, 0) << QPointF(-1000, 0);
    QTest::newRow("7") << true << QPoint(-100, 0) << QPointF(-100, 0);
    QTest::newRow("8") << true << QPoint(-10, 0) << QPointF(-10, 0);
    QTest::newRow("0-") << false << QPoint() << QPointF();
    QTest::newRow("1-") << false << QPoint(10, 0) << QPointF(10, 0);
    QTest::newRow("2-") << false << QPoint(100, 0) << QPointF(100, 0);
    QTest::newRow("3-") << false << QPoint(1000, 0) << QPointF(1000, 0);
    QTest::newRow("4-") << false << QPoint(10000, 0) << QPointF(10000, 0);
    QTest::newRow("5-") << false << QPoint(-10000, 0) << QPointF(-10000, 0);
    QTest::newRow("6-") << false << QPoint(-1000, 0) << QPointF(-1000, 0);
    QTest::newRow("7-") << false << QPoint(-100, 0) << QPointF(-100, 0);
    QTest::newRow("8-") << false << QPoint(-10, 0) << QPointF(-10, 0);
}

void tst_QGraphicsProxyWidget::childPos()
{
    QFETCH(bool, moveCombo);
    QFETCH(QPoint, comboPos);
    QFETCH(QPointF, proxyPos);

    QGraphicsScene scene;

    QComboBox *box = new QComboBox;
    box->addItem("Item 1");
    box->addItem("Item 2");
    box->addItem("Item 3");
    box->addItem("Item 4");

    if (moveCombo)
        box->move(comboPos);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();
    QVERIFY(proxy->isVisible());
    QVERIFY(box->isVisible());

    if (!moveCombo)
        proxy->setPos(proxyPos);

    QCOMPARE(proxy->pos(), proxyPos);
    QCOMPARE(box->pos(), comboPos);

    for (int i = 0; i < 2; ++i) {
        box->showPopup();
        QApplication::processEvents();
        QApplication::processEvents();

        QWidget *menu = 0;
        foreach (QObject *child, box->children()) {
            if ((menu = qobject_cast<QWidget *>(child)))
                break;
        }
        QVERIFY(menu);
        QVERIFY(menu->isVisible());
        QVERIFY(menu->testAttribute(Qt::WA_DontShowOnScreen));

        QCOMPARE(proxy->childItems().size(), 1);
        QGraphicsProxyWidget *proxyChild = 0;
        foreach (QGraphicsItem *child, proxy->childItems()) {
            if (child->isWidget() && (proxyChild = qobject_cast<QGraphicsProxyWidget *>(static_cast<QGraphicsWidget *>(child))))
                break;
        }
        QVERIFY(proxyChild);
        QVERIFY(proxyChild->isVisible());
        qreal expectedXPosition = 0.0;
#if defined(Q_OS_MAC) && !defined(QT_NO_STYLE_MAC)
        // The Mac style wants the popup to show up at QPoint(4 - 11, 1).
        // See QMacStyle::subControlRect SC_ComboBoxListBoxPopup.
        if (QApplication::style()->inherits("QMacStyle"))
            expectedXPosition = qreal(4 - 11);
#endif
        QCOMPARE(proxyChild->pos().x(), expectedXPosition);
        menu->hide();
    }
}

void tst_QGraphicsProxyWidget::autoShow()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsProxyWidget *proxy1 = scene.addWidget(new QPushButton("Button1"));

    QPushButton *button2 = new QPushButton("Button2");
    button2->hide();
    QGraphicsProxyWidget *proxy2 = new QGraphicsProxyWidget();
    proxy2->setWidget(button2);
    scene.addItem(proxy2);

    view.show();
    QApplication::processEvents();

    QCOMPARE(proxy1->isVisible(), true);
    QCOMPARE(proxy2->isVisible(), false);

}

void tst_QGraphicsProxyWidget::windowOpacity()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QWidget *widget = new QWidget;
    widget->resize(100, 100);
    QGraphicsProxyWidget *proxy = scene.addWidget(widget);
    proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QApplication::setActiveWindow(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(view.isActiveWindow());

    qRegisterMetaType<QList<QRectF> >("QList<QRectF>");
    QSignalSpy signalSpy(&scene, SIGNAL(changed(QList<QRectF>)));

    EventSpy eventSpy(widget);
    QVERIFY(widget->isVisible());

    widget->setWindowOpacity(0.5);
    QApplication::processEvents();

    // Make sure setWindowOpacity triggers an update on the scene,
    // and not on the widget or the proxy itself. The entire proxy needs an update
    // in case it has a window decoration. Update: QGraphicsItem::CacheMode is
    // disabled on platforms without alpha channel support in QPixmap (e.g.,
    // X11 without XRender).
    int paints = 0;
    QTRY_COMPARE(eventSpy.counts[QEvent::UpdateRequest], 0);
    QTRY_COMPARE(eventSpy.counts[QEvent::Paint], paints);

    QCOMPARE(signalSpy.count(), 1);
    const QList<QVariant> arguments = signalSpy.takeFirst();
    const QList<QRectF> updateRects = qvariant_cast<QList<QRectF> >(arguments.at(0));
    QCOMPARE(updateRects.size(), 1);
    QCOMPARE(updateRects.at(0), proxy->sceneBoundingRect());
}

void tst_QGraphicsProxyWidget::stylePropagation()
{
    QPointer<QStyle> windowsStyle = QStyleFactory::create("windows");

    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget proxy;
    proxy.setWidget(edit);

    EventSpy editSpy(edit);
    EventSpy proxySpy(&proxy);

    // Widget to proxy
    QCOMPARE(proxy.style(), QApplication::style());
    edit->setStyle(windowsStyle);
    QCOMPARE(editSpy.counts[QEvent::StyleChange], 1);
    QCOMPARE(proxySpy.counts[QEvent::StyleChange], 1);
    QCOMPARE(proxy.style(), (QStyle *)windowsStyle);
    edit->setStyle(0);
    QCOMPARE(editSpy.counts[QEvent::StyleChange], 2);
    QCOMPARE(proxySpy.counts[QEvent::StyleChange], 2);
    QCOMPARE(proxy.style(), QApplication::style());
    QCOMPARE(edit->style(), QApplication::style());
    QVERIFY(windowsStyle); // not deleted

    // Proxy to widget
    proxy.setStyle(windowsStyle);
    QCOMPARE(editSpy.counts[QEvent::StyleChange], 3);
    QCOMPARE(proxySpy.counts[QEvent::StyleChange], 3);
    QCOMPARE(edit->style(), (QStyle *)windowsStyle);
    proxy.setStyle(0);
    QCOMPARE(editSpy.counts[QEvent::StyleChange], 4);
    QCOMPARE(proxySpy.counts[QEvent::StyleChange], 4);
    QCOMPARE(proxy.style(), QApplication::style());
    QCOMPARE(edit->style(), QApplication::style());
    QVERIFY(windowsStyle); // not deleted

    delete windowsStyle;
}

void tst_QGraphicsProxyWidget::palettePropagation()
{
    // Construct a palette with an unlikely setup
    QPalette lineEditPalette = QApplication::palette("QLineEdit");
    QPalette palette = lineEditPalette;
    palette.setBrush(QPalette::Text, Qt::red);

    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget proxy;
    proxy.setWidget(edit);

    EventSpy editSpy(edit);
    EventSpy proxySpy(&proxy);

    // Widget to proxy (no change)
    QVERIFY(!edit->testAttribute(Qt::WA_SetPalette));
    edit->setPalette(palette);
    QCOMPARE(editSpy.counts[QEvent::PaletteChange], 1);
    QCOMPARE(proxySpy.counts[QEvent::PaletteChange], 0);
    QVERIFY(edit->testAttribute(Qt::WA_SetPalette));
    QVERIFY(!proxy.testAttribute(Qt::WA_SetPalette));
    QCOMPARE(proxy.palette(), QPalette());
    edit->setPalette(QPalette());
    QCOMPARE(editSpy.counts[QEvent::PaletteChange], 2);
    QCOMPARE(proxySpy.counts[QEvent::PaletteChange], 0);
    QVERIFY(!edit->testAttribute(Qt::WA_SetPalette));
    QVERIFY(!proxy.testAttribute(Qt::WA_SetPalette));
    QCOMPARE(proxy.palette(), QPalette());

    // Proxy to widget
    proxy.setPalette(palette);
    QVERIFY(proxy.testAttribute(Qt::WA_SetPalette));
    QCOMPARE(editSpy.counts[QEvent::PaletteChange], 3);
    QCOMPARE(proxySpy.counts[QEvent::PaletteChange], 1);
    QVERIFY(!edit->testAttribute(Qt::WA_SetPalette));
    QCOMPARE(edit->palette(), palette);
    QCOMPARE(edit->palette(), proxy.palette());
    QCOMPARE(edit->palette().color(QPalette::Text), QColor(Qt::red));
    proxy.setPalette(QPalette());
    QVERIFY(!proxy.testAttribute(Qt::WA_SetPalette));
    QVERIFY(!edit->testAttribute(Qt::WA_SetPalette));
    QCOMPARE(editSpy.counts[QEvent::PaletteChange], 4);
    QCOMPARE(proxySpy.counts[QEvent::PaletteChange], 2);
    QCOMPARE(edit->palette(), lineEditPalette);
}

void tst_QGraphicsProxyWidget::fontPropagation()
{
    // Construct a font with an unlikely setup
    QGraphicsScene scene;
    QFont lineEditFont = QApplication::font("QLineEdit");
    QFont font = lineEditFont;
    font.setPointSize(43);

    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget proxy;
    proxy.setWidget(edit);

    scene.addItem(&proxy);
    EventSpy editSpy(edit);
    EventSpy proxySpy(&proxy);

    // Widget to proxy (no change)
    QVERIFY(!edit->testAttribute(Qt::WA_SetFont));
    edit->setFont(font);
    QCOMPARE(editSpy.counts[QEvent::FontChange], 1);
    QCOMPARE(proxySpy.counts[QEvent::FontChange], 0);
    QVERIFY(edit->testAttribute(Qt::WA_SetFont));
    QVERIFY(!proxy.testAttribute(Qt::WA_SetFont));
    QCOMPARE(proxy.font(), lineEditFont);
    edit->setFont(QFont());
    QCOMPARE(editSpy.counts[QEvent::FontChange], 2);
    QCOMPARE(proxySpy.counts[QEvent::FontChange], 0);
    QVERIFY(!edit->testAttribute(Qt::WA_SetFont));
    QVERIFY(!proxy.testAttribute(Qt::WA_SetFont));
    QCOMPARE(proxy.font(), lineEditFont);

    // Proxy to widget
    proxy.setFont(font);
    QApplication::processEvents();  // wait for QEvent::Polish
    QVERIFY(proxy.testAttribute(Qt::WA_SetFont));
    QCOMPARE(editSpy.counts[QEvent::FontChange], 3);
    QCOMPARE(proxySpy.counts[QEvent::FontChange], 1);
    QVERIFY(!edit->testAttribute(Qt::WA_SetFont));
    QCOMPARE(edit->font(), font);
    QCOMPARE(edit->font(), proxy.font());
    QCOMPARE(edit->font().pointSize(), 43);
    proxy.setFont(QFont());
    QVERIFY(!proxy.testAttribute(Qt::WA_SetFont));
    QVERIFY(!edit->testAttribute(Qt::WA_SetFont));
    QCOMPARE(editSpy.counts[QEvent::FontChange], 4);
    QCOMPARE(proxySpy.counts[QEvent::FontChange], 2);
    QCOMPARE(edit->font(), lineEditFont);

    proxy.setFont(font);
    QLineEdit *edit2 = new QLineEdit;
    proxy.setWidget(edit2);
    delete edit;
    QCOMPARE(edit2->font().pointSize(), 43);
}

class MainWidget : public QMainWindow
{
Q_OBJECT
public:
    MainWidget() : QMainWindow() {
      view = new QGraphicsView(this);
      scene = new QGraphicsScene(this);
      item = new QGraphicsWidget();
      widget = new QGraphicsProxyWidget(item);
      layout = new QGraphicsLinearLayout(item);
      layout->addItem(widget);
      item->setLayout(layout);
      button = new QPushButton("Push me");
      widget->setWidget(button);
      setCentralWidget(view);
      scene->addItem(item);
      view->setScene(scene);
      scene->setFocusItem(item);
      layout->activate();
    }
    QGraphicsView* view;
    QGraphicsScene* scene;
    QGraphicsWidget * item;
    QGraphicsLinearLayout * layout;
    QGraphicsProxyWidget * widget;
    QPushButton * button;
};

void tst_QGraphicsProxyWidget::dontCrashWhenDie()
{
    MainWidget *w = new MainWidget();
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    QTest::mouseMove(w->view->viewport(), w->view->mapFromScene(w->widget->mapToScene(w->widget->boundingRect().center())));
    delete w->item;

    QApplication::processEvents();
    delete w;
    // This leaves an invisible proxy widget behind.
    qDeleteAll(QApplication::topLevelWidgets());
}

void tst_QGraphicsProxyWidget::dontCrashNoParent()  // QTBUG-15442
{
    QGraphicsProxyWidget *parent(new QGraphicsProxyWidget);
    QGraphicsProxyWidget *child(new QGraphicsProxyWidget);
    QScopedPointer<QLabel> label0(new QLabel);
    QScopedPointer<QLabel> label1(new QLabel);

    child->setParentItem(parent);
    // Set the first label as the proxied widget.
    parent->setWidget(label0.data());
    // If we attempt to change the proxied widget we get a crash.
    parent->setWidget(label1.data());
}

void tst_QGraphicsProxyWidget::createProxyForChildWidget()
{
    QGraphicsScene scene;

    QLineEdit *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1");
    QLineEdit *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2");
    QCheckBox *checkbox = new QCheckBox("QCheckBox");
    QVBoxLayout *vlayout = new QVBoxLayout;

    vlayout->addWidget(edit1);
    vlayout->addWidget(edit2);
    vlayout->addWidget(checkbox);
    vlayout->insertStretch(-1);

    QGroupBox *box = new QGroupBox("QGroupBox");
    box->setCheckable(true);
    box->setChecked(true);
    box->setLayout(vlayout);

    QDial *leftDial = new QDial;
    QDial *rightDial = new QDial;

    QWidget window;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(leftDial);
    layout->addWidget(box);
    layout->addWidget(rightDial);
    window.setLayout(layout);

    QVERIFY(!window.graphicsProxyWidget());
    QVERIFY(!checkbox->graphicsProxyWidget());

    QGraphicsProxyWidget *windowProxy = scene.addWidget(&window);
    QGraphicsView view(&scene);
    view.show();
    view.resize(500,500);

    QCOMPARE(window.graphicsProxyWidget(), windowProxy);
    QVERIFY(!box->graphicsProxyWidget());
    QVERIFY(!checkbox->graphicsProxyWidget());

    QPointer<QGraphicsProxyWidget> checkboxProxy = windowProxy->createProxyForChildWidget(checkbox);

    QGraphicsProxyWidget *boxProxy = box->graphicsProxyWidget();

    QVERIFY(boxProxy);
    QCOMPARE(checkbox->graphicsProxyWidget(), checkboxProxy.data());
    QCOMPARE(checkboxProxy->parentItem(), boxProxy);
    QCOMPARE(boxProxy->parentItem(), windowProxy);

    QVERIFY(checkboxProxy->mapToScene(QPointF()) == checkbox->mapTo(&window, QPoint()));
    QCOMPARE(checkboxProxy->size().toSize(), checkbox->size());
    QCOMPARE(boxProxy->size().toSize(), box->size());

    window.resize(500,500);
    QCOMPARE(windowProxy->size().toSize(), QSize(500,500));
    QVERIFY(checkboxProxy->mapToScene(QPointF()) == checkbox->mapTo(&window, QPoint()));
    QCOMPARE(checkboxProxy->size().toSize(), checkbox->size());
    QCOMPARE(boxProxy->size().toSize(), box->size());

    QTest::qWait(10);


    QSignalSpy spy(checkbox, SIGNAL(clicked()));

    QTest::mousePress(view.viewport(), Qt::LeftButton, 0,
                      view.mapFromScene(checkboxProxy->mapToScene(QPointF(8,8))));
    QTRY_COMPARE(spy.count(), 0);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, 0,
                        view.mapFromScene(checkboxProxy->mapToScene(QPointF(8,8))));
    QTRY_COMPARE(spy.count(), 1);



    boxProxy->setWidget(0);

    QVERIFY(!checkbox->graphicsProxyWidget());
    QVERIFY(!box->graphicsProxyWidget());
    QVERIFY(checkboxProxy.isNull());

    delete boxProxy;
}

#ifndef QT_NO_CONTEXTMENU
class ContextMenuWidget : public QLabel
{
    Q_OBJECT
public:
    ContextMenuWidget()
        : QLabel(QStringLiteral("ContextMenuWidget"))
        , embeddedPopup(false)
        , gotContextMenuEvent(false)
        , m_embeddedPopupSet(false)
        , m_timer(0)
    { }
    bool embeddedPopup;
    bool gotContextMenuEvent;
protected:
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::ContextMenu) {
            if (!m_timer) {
                m_timer = new QTimer(this);
                m_timer->setInterval(10);
                connect(m_timer, SIGNAL(timeout()), this, SLOT(checkMenu()));
                m_timer->start();
            }
        }
        return QWidget::event(event);
    }
    void contextMenuEvent(QContextMenuEvent *)
    {
        gotContextMenuEvent = true;
    }

private slots:
    void checkMenu()
    {
        QMenu *menu = findChild<QMenu *>();
        if (!m_embeddedPopupSet) {
            m_embeddedPopupSet = true;
            embeddedPopup = menu != 0;
        }
        if (menu && menu->isVisible())
            menu->hide();
        hide();
    }

private:
    bool m_embeddedPopupSet;
    QTimer *m_timer;
};
#endif // QT_NO_CONTEXTMENU

void tst_QGraphicsProxyWidget::actionsContextMenu_data()
{
    QTest::addColumn<bool>("actionsContextMenu");
    QTest::addColumn<bool>("hasFocus");

    QTest::newRow("without actionsContextMenu and with focus") << false << true;
    QTest::newRow("without actionsContextMenu and without focus") << false << false;
    QTest::newRow("with actionsContextMenu and focus") << true << true;
    QTest::newRow("with actionsContextMenu without focus") << true << false;
}

#ifndef QT_NO_CONTEXTMENU
void tst_QGraphicsProxyWidget::actionsContextMenu()
{
    QFETCH(bool, hasFocus);
    QFETCH(bool, actionsContextMenu);

    ContextMenuWidget *widget = new ContextMenuWidget;
    if (actionsContextMenu) {
        widget->addAction(new QAction("item 1", widget));
        widget->addAction(new QAction("item 2", widget));
        widget->addAction(new QAction("item 3", widget));
        widget->setContextMenuPolicy(Qt::ActionsContextMenu);
    }
    QGraphicsScene scene;
    QGraphicsProxyWidget *proxyWidget = scene.addWidget(widget);
    QGraphicsView view(&scene);
    view.setWindowTitle(QStringLiteral("actionsContextMenu"));
    view.resize(200, 200);
    view.move(QGuiApplication::primaryScreen()->geometry().center() - QPoint(100, 100));
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    view.setFocus();
    QTRY_VERIFY(view.hasFocus());

    if (hasFocus)
        proxyWidget->setFocus();
    else
        proxyWidget->clearFocus();

    QApplication::processEvents();

    QContextMenuEvent contextMenuEvent(QContextMenuEvent::Mouse,
                                       view.viewport()->rect().center(),
                                       view.viewport()->mapToGlobal(view.viewport()->rect().center()));
    contextMenuEvent.accept();
    qApp->sendEvent(view.viewport(), &contextMenuEvent);

    if (hasFocus) {
        if (actionsContextMenu) {
            //actionsContextMenu embedded popup but no contextMenuEvent (widget has focus)
            QVERIFY(widget->embeddedPopup);
            QVERIFY(!widget->gotContextMenuEvent);
        } else {
            //no embedded popup but contextMenuEvent (widget has focus)
            QVERIFY(!widget->embeddedPopup);
            QVERIFY(widget->gotContextMenuEvent);
        }
    } else {
        //qgraphicsproxywidget doesn't have the focus, the widget must not receive any contextMenuEvent and must not create any QMenu
        QVERIFY(!widget->embeddedPopup);
        QVERIFY(!widget->gotContextMenuEvent);
    }

}
#endif // QT_NO_CONTEXTMENU

void tst_QGraphicsProxyWidget::deleteProxyForChildWidget()
{
    QDialog dialog;
    dialog.resize(320, 120);
    dialog.move(80, 40);

    QGraphicsScene scene;
    scene.setSceneRect(QRectF(0, 0, 640, 480));
    QGraphicsView view(&scene);
    QComboBox *combo = new QComboBox;
    combo->addItems(QStringList() << "red" << "green" << "blue" << "white" << "black" << "yellow" << "cyan" << "magenta");
    dialog.setLayout(new QVBoxLayout);
    dialog.layout()->addWidget(combo);

    QGraphicsProxyWidget *proxy = scene.addWidget(&dialog);
    view.show();

    proxy->setWidget(0);
    //just don't crash
    QApplication::processEvents();
    delete combo;
}

void tst_QGraphicsProxyWidget::bypassGraphicsProxyWidget_data()
{
    QTest::addColumn<bool>("bypass");

    QTest::newRow("autoembed") << false;
    QTest::newRow("bypass") << true;
}

void tst_QGraphicsProxyWidget::bypassGraphicsProxyWidget()
{
    QFETCH(bool, bypass);

    QWidget *widget = new QWidget;
    widget->resize(100, 100);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QGraphicsProxyWidget *proxy = scene.addWidget(widget);

    QCOMPARE(proxy->widget(), widget);
    QVERIFY(proxy->childItems().isEmpty());

    Qt::WindowFlags flags;
    flags |= Qt::Dialog;
    if (bypass)
        flags |= Qt::BypassGraphicsProxyWidget;
    QFileDialog *dialog = new QFileDialog(widget, flags);
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);
    dialog->show();
    QVERIFY(QTest::qWaitForWindowActive(dialog));

    QCOMPARE(proxy->childItems().size(), bypass ? 0 : 1);
    if (!bypass)
        QCOMPARE(((QGraphicsProxyWidget *)proxy->childItems().first())->widget(), (QWidget *)dialog);

    dialog->hide();
    QApplication::processEvents();
    delete dialog;
    delete widget;
}

static void makeDndEvent(QGraphicsSceneDragDropEvent *event, QGraphicsView *view, const QPointF &pos)
{
    event->setScenePos(pos);
    event->setScreenPos(view->mapToGlobal(view->mapFromScene(pos)));
    event->setButtons(Qt::LeftButton);
    event->setModifiers(0);
    event->setPossibleActions(Qt::CopyAction);
    event->setProposedAction(Qt::CopyAction);
    event->setDropAction(Qt::CopyAction);
    event->setWidget(view->viewport());
    event->setSource(view->viewport());
    event->ignore();
}

void tst_QGraphicsProxyWidget::dragDrop()
{
    QPushButton *button = new QPushButton; // acceptDrops(false)
    QLineEdit *edit = new QLineEdit; // acceptDrops(true)

    QGraphicsScene scene;
    QGraphicsProxyWidget *buttonProxy = scene.addWidget(button);
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    QVERIFY(buttonProxy->acceptDrops());
    QVERIFY(editProxy->acceptDrops());

    button->setGeometry(0, 0, 100, 50);
    edit->setGeometry(0, 60, 100, 50);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QMimeData data;
    data.setText("hei");
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragEnter);
        makeDndEvent(&event, &view, QPointF(50, 25));
        event.setMimeData(&data);
        qApp->sendEvent(&scene, &event);
        QVERIFY(event.isAccepted());
    }
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragMove);
        makeDndEvent(&event, &view, QPointF(50, 25));
        event.setMimeData(&data);
        qApp->sendEvent(&scene, &event);
        QVERIFY(!event.isAccepted());
    }
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragMove);
        makeDndEvent(&event, &view, QPointF(50, 75));
        event.setMimeData(&data);
        qApp->sendEvent(&scene, &event);
        QVERIFY(event.isAccepted());
    }
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDrop);
        makeDndEvent(&event, &view, QPointF(50, 75));
        event.setMimeData(&data);
        qApp->sendEvent(&scene, &event);
        QVERIFY(event.isAccepted());
    }
    QCOMPARE(edit->text(), QString("hei"));
}

void tst_QGraphicsProxyWidget::windowFlags_data()
{
    QTest::addColumn<int>("proxyFlags");
    QTest::addColumn<int>("widgetFlags");
    QTest::addColumn<int>("resultingProxyFlags");
    QTest::addColumn<int>("resultingWidgetFlags");

    QTest::newRow("proxy(0) widget(0)") << 0 << 0 << 0 << int(Qt::Window);
    QTest::newRow("proxy(window)") << int(Qt::Window) << 0 << int(Qt::Window) << int(Qt::Window);
    QTest::newRow("proxy(window) widget(window)") << int(Qt::Window) << int(Qt::Window) << int(Qt::Window) << int(Qt::Window);
    QTest::newRow("proxy(0) widget(window)") << int(0) << int(Qt::Window) << int(0) << int(Qt::Window);
}

void tst_QGraphicsProxyWidget::windowFlags()
{
    QFETCH(int, proxyFlags);
    QFETCH(int, widgetFlags);
    QFETCH(int, resultingProxyFlags);
    QFETCH(int, resultingWidgetFlags);
    Qt::WindowFlags proxyWFlags = Qt::WindowFlags(proxyFlags);
    Qt::WindowFlags widgetWFlags = Qt::WindowFlags(widgetFlags);
    Qt::WindowFlags resultingProxyWFlags = Qt::WindowFlags(resultingProxyFlags);
    Qt::WindowFlags resultingWidgetWFlags = Qt::WindowFlags(resultingWidgetFlags);

    QGraphicsProxyWidget proxy(0, proxyWFlags);
    QVERIFY((proxy.windowFlags() & proxyWFlags) == proxyWFlags);

    QWidget *widget = new QWidget(0, widgetWFlags);
    QVERIFY((widget->windowFlags() & widgetWFlags) == widgetWFlags);

    proxy.setWidget(widget);

    if (resultingProxyFlags == 0)
        QVERIFY(!proxy.windowFlags());
    else
        QVERIFY((proxy.windowFlags() & resultingProxyWFlags) == resultingProxyWFlags);
    QVERIFY((widget->windowFlags() & resultingWidgetWFlags) == resultingWidgetWFlags);
}

void tst_QGraphicsProxyWidget::comboboxWindowFlags()
{
    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("Item 1");
    comboBox->addItem("Item 2");
    comboBox->addItem("Item 3");
    QWidget *embedWidget = comboBox;

    QGraphicsScene scene;
    QGraphicsProxyWidget *proxy = scene.addWidget(embedWidget);
    proxy->setWindowFlags(Qt::Window);
    QVERIFY(embedWidget->isWindow());
    QVERIFY(proxy->isWindow());

    comboBox->showPopup();

    QCOMPARE(proxy->childItems().size(), 1);
    QGraphicsItem *popupProxy = proxy->childItems().first();
    QVERIFY(popupProxy->isWindow());
    QVERIFY((static_cast<QGraphicsWidget *>(popupProxy)->windowFlags() & Qt::Popup) == Qt::Popup);
}

void tst_QGraphicsProxyWidget::updateAndDelete()
{
#ifdef Q_OS_MAC
    QSKIP("Test case unstable on this platform, QTBUG-23700");
#endif
    QGraphicsScene scene;
    QGraphicsProxyWidget *proxy = scene.addWidget(new QPushButton("Hello World"));
    View view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.npaints > 0);
    // Wait a bit to clear all pending paint events
    QTest::qWait(10);

    const QRect itemDeviceBoundingRect = proxy->deviceTransform(view.viewportTransform())
                                         .mapRect(proxy->boundingRect()).toRect();
    const QRegion expectedRegion = itemDeviceBoundingRect.adjusted(-2, -2, 2, 2);

    view.npaints = 0;
    view.paintEventRegion = QRegion();

    // Update and hide.
    proxy->update();
    proxy->hide();
    QTRY_COMPARE(view.npaints, 1);
    QCOMPARE(view.paintEventRegion, expectedRegion);

    proxy->show();
    QTest::qWait(50);
    view.npaints = 0;
    view.paintEventRegion = QRegion();

    // Update and delete.
    proxy->update();
    delete proxy;
    QTRY_COMPARE(view.npaints, 1);
    QCOMPARE(view.paintEventRegion, expectedRegion);
}

class InputMethod_LineEdit : public QLineEdit
{
    bool event(QEvent *e)
    {
        if (e->type() == QEvent::InputMethod)
            ++inputMethodEvents;
        return QLineEdit::event(e);
    }
public:
    int inputMethodEvents;
};

void tst_QGraphicsProxyWidget::inputMethod()
{
    QGraphicsScene scene;

    // check that the proxy is initialized with the correct input method sensitivity
    for (int i = 0; i < 2; ++i)
    {
        QLineEdit *lineEdit = new QLineEdit;
        lineEdit->setAttribute(Qt::WA_InputMethodEnabled, !!i);
        QGraphicsProxyWidget *proxy = scene.addWidget(lineEdit);
        QCOMPARE(!!(proxy->flags() & QGraphicsItem::ItemAcceptsInputMethod), !!i);
    }

    // check that input method events are only forwarded to widgets with focus
    for (int i = 0; i < 2; ++i)
    {
        InputMethod_LineEdit *lineEdit = new InputMethod_LineEdit;
        lineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
        QGraphicsProxyWidget *proxy = scene.addWidget(lineEdit);

        if (i)
            lineEdit->setFocus();

        lineEdit->inputMethodEvents = 0;
        QInputMethodEvent event;
        qApp->sendEvent(proxy, &event);
        QCOMPARE(lineEdit->inputMethodEvents, i);
    }

    scene.clear();
    QGraphicsView view(&scene);
    QWidget *w = new QWidget;
    w->setLayout(new QVBoxLayout(w));
    QLineEdit *lineEdit = new QLineEdit;
    lineEdit->setEchoMode(QLineEdit::Password);
    w->layout()->addWidget(lineEdit);
    lineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
    QGraphicsProxyWidget *proxy = scene.addWidget(w);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(!(proxy->flags() & QGraphicsItem::ItemAcceptsInputMethod));
    lineEdit->setFocus();
    QVERIFY((proxy->flags() & QGraphicsItem::ItemAcceptsInputMethod));
}

void tst_QGraphicsProxyWidget::clickFocus()
{
    QGraphicsScene scene;
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    QLineEdit *le1 = new QLineEdit;
    QGraphicsProxyWidget *proxy = scene.addWidget(le1);

    QGraphicsView view(&scene);

    {
        EventSpy proxySpy(proxy);
        EventSpy widgetSpy(proxy->widget());

        view.setFrameStyle(0);
        view.resize(300, 300);
        view.show();
        QApplication::setActiveWindow(&view);
        QVERIFY(QTest::qWaitForWindowActive(&view));

        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());

        QCOMPARE(proxySpy.counts[QEvent::FocusIn], 0);
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 0);

        QPointF lineEditCenter = proxy->mapToScene(proxy->boundingRect().center());
        // Spontaneous mouse click sets focus on a clickable widget.
        for (int retry = 0; retry < 50 && !proxy->hasFocus(); retry++)
            QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(lineEditCenter));
        QVERIFY(proxy->hasFocus());
        QVERIFY(proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusIn], 1);
        QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 1);

        scene.setFocusItem(0);
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 1);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 1);

        // Non-spontaneous mouse click sets focus if the widget has been clicked before
        {
            QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
            event.setScenePos(lineEditCenter);
            event.setButton(Qt::LeftButton);
            qApp->sendEvent(&scene, &event);
            QVERIFY(proxy->hasFocus());
            QVERIFY(proxy->widget()->hasFocus());
            QCOMPARE(proxySpy.counts[QEvent::FocusIn], 2);
            QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 2);
        }
    }

    scene.setFocusItem(0);
    proxy->setWidget(new QLineEdit); // resets focusWidget
    delete le1;

    {
        QPointF lineEditCenter = proxy->mapToScene(proxy->boundingRect().center());
        EventSpy proxySpy(proxy);
        EventSpy widgetSpy(proxy->widget());
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 0);

        // Non-spontaneous mouse click does not set focus on the embedded widget.
        {
            QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
            event.setScenePos(lineEditCenter);
            event.setButton(Qt::LeftButton);
            qApp->sendEvent(&scene, &event);
            QVERIFY(!proxy->hasFocus());
            QVERIFY(!proxy->widget()->hasFocus());
            QCOMPARE(proxySpy.counts[QEvent::FocusIn], 0);
            QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 0);
        }

        scene.setFocusItem(0);
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 0);

        // Spontaneous click on non-clickable widget does not give focus.
        proxy->widget()->setFocusPolicy(Qt::NoFocus);
        QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(lineEditCenter));
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());

        // Multiple clicks should only result in one FocusIn.
        proxy->widget()->setFocusPolicy(Qt::StrongFocus);
        scene.setFocusItem(0);
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(lineEditCenter));
        QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(lineEditCenter));
        QVERIFY(proxy->hasFocus());
        QVERIFY(proxy->widget()->hasFocus());
        QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 1);
        QCOMPARE(proxySpy.counts[QEvent::FocusIn], 1);
    }
}

void tst_QGraphicsProxyWidget::windowFrameMargins()
{
    // Make sure the top margin is non-zero when passing Qt::Window.
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(0, Qt::Window);

    qreal left, top, right, bottom;
    proxy->getWindowFrameMargins(&left, &top, &right, &bottom);
    QVERIFY(top > 0);

    proxy->setWidget(new QPushButton("testtest"));
    proxy->getWindowFrameMargins(&left, &top, &right, &bottom);
    QVERIFY(top > 0);

    QGraphicsScene scene;
    scene.addItem(proxy);
    proxy->getWindowFrameMargins(&left, &top, &right, &bottom);
    QVERIFY(top > 0);

    proxy->unsetWindowFrameMargins();
    proxy->getWindowFrameMargins(&left, &top, &right, &bottom);
    QVERIFY(top > 0);
}

class HoverButton : public QPushButton
{
public:
    HoverButton(QWidget *parent = 0) : QPushButton(parent), hoverLeaveReceived(false)
    {}

    bool hoverLeaveReceived;

    bool event(QEvent* e)
    {
        if(QEvent::HoverLeave == e->type())
            hoverLeaveReceived = true;
        return QPushButton::event(e);
    }
};

class Scene : public QGraphicsScene
{
Q_OBJECT
public:
    Scene() {
        QWidget *background = new QWidget;
        background->setGeometry(0, 0, 500, 500);
        hoverButton = new HoverButton;
        hoverButton->setParent(background);
        hoverButton->setText("Second button");
        hoverButton->setGeometry(10, 10, 200, 50);
        addWidget(background);

        QPushButton *hideButton = new QPushButton("I'm a button with a very very long text");
        hideButton->setGeometry(10, 10, 400, 50);
        topButton = addWidget(hideButton);
        connect(hideButton, &QPushButton::clicked, this, [&]() { topButton->hide(); });
        topButton->setFocus();
    }

    QGraphicsProxyWidget *topButton;
    HoverButton *hoverButton;
};

void tst_QGraphicsProxyWidget::QTBUG_6986_sendMouseEventToAlienWidget()
{
    if (QGuiApplication::platformName() == QLatin1String("cocoa")) {
        // The "Second button" does not receive QEvent::HoverLeave
        QSKIP("This test fails only on Cocoa. Investigate why. See QTBUG-69219");
    }

    QGraphicsView view;
    Scene scene;
    view.setScene(&scene);
    view.resize(600, 600);
    QApplication::setActiveWindow(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QPoint topButtonTopLeftCorner = view.mapFromScene(scene.topButton->scenePos());
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, topButtonTopLeftCorner);
    // move to the bottom right corner (buttons are placed in the top left corner)
    QCOMPARE(scene.hoverButton->hoverLeaveReceived, false);
    QTest::mouseMove(view.viewport(), view.viewport()->rect().bottomRight());
    QTRY_COMPARE(scene.hoverButton->hoverLeaveReceived, true);
}

static QByteArray msgPointMismatch(const QPoint &actual, const QPoint &expected)
{
    QString result;
    QDebug(&result) << actual << " != " << expected << " manhattanLength="
        << (expected - actual).manhattanLength();
    return result.toLocal8Bit();
}

void tst_QGraphicsProxyWidget::mapToGlobal() // QTBUG-41135
{
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 4;
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setTransform(QTransform::fromScale(2, 2));  // QTBUG-50136, use transform.
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(size);
    view.move(availableGeometry.bottomRight() - QPoint(size.width(), size.height()) - QPoint(100, 100));
    QWidget *embeddedWidget = new QGroupBox(QLatin1String("Embedded"));
    embeddedWidget->setStyleSheet(QLatin1String("background-color: \"yellow\"; "));
    embeddedWidget->setFixedSize((size - QSize(10, 10)) / 2);
    QWidget *childWidget = new QGroupBox(QLatin1String("Child"), embeddedWidget);
    childWidget->setStyleSheet(QLatin1String("background-color: \"red\"; "));
    childWidget->resize(embeddedWidget->size() / 2);
    childWidget->move(embeddedWidget->width() / 4, embeddedWidget->height() / 4); // center in embeddedWidget
    scene.addWidget(embeddedWidget);
    QApplication::setActiveWindow(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QPoint embeddedCenter = embeddedWidget->rect().center();
    const QPoint embeddedCenterGlobal = embeddedWidget->mapToGlobal(embeddedCenter);
    QCOMPARE(embeddedWidget->mapFromGlobal(embeddedCenterGlobal), embeddedCenter);
    // This should be equivalent to the view center give or take rounding
    // errors due to odd window margins
    const QPoint viewCenter = view.geometry().center();
    QVERIFY2((viewCenter - embeddedCenterGlobal).manhattanLength() <= 3,
             msgPointMismatch(embeddedCenterGlobal, viewCenter).constData());

    // Same test with child centered on embeddedWidget. Also make sure
    // the roundtrip maptoGlobal()/mapFromGlobal() returns the same
    // point since that is important for mouse event handling (QTBUG-50030,
    // QTBUG-50136).
    const QPoint childCenter = childWidget->rect().center();
    const QPoint childCenterGlobal = childWidget->mapToGlobal(childCenter);
    QCOMPARE(childWidget->mapFromGlobal(childCenterGlobal), childCenter);
    QVERIFY2((viewCenter - childCenterGlobal).manhattanLength() <= 4,
             msgPointMismatch(childCenterGlobal, viewCenter).constData());
}

void tst_QGraphicsProxyWidget::mapToGlobalWithoutScene() // QTBUG-44509
{
    QGraphicsProxyWidget proxyWidget;
    QWidget *embeddedWidget = new QWidget;
    proxyWidget.setWidget(embeddedWidget);
    const QPoint localPos(0, 0);
    const QPoint globalPos = embeddedWidget->mapToGlobal(localPos);
    QCOMPARE(embeddedWidget->mapFromGlobal(globalPos), localPos);
}

// QTBUG_43780: Embedded widgets have isWindow()==true but showing them should not
// trigger the top-level widget code path of show() that closes all popups
// (for example combo popups).
void tst_QGraphicsProxyWidget::QTBUG_43780_visibility()
{
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 4;
    QWidget mainWindow;
    QVBoxLayout *layout = new QVBoxLayout(&mainWindow);
    QComboBox *combo = new QComboBox(&mainWindow);
    combo->addItems(QStringList() << "i1" << "i2" << "i3");
    layout->addWidget(combo);
    QGraphicsScene *scene = new QGraphicsScene(&mainWindow);
    QGraphicsView *view = new QGraphicsView(scene, &mainWindow);
    layout->addWidget(view);
    mainWindow.setWindowTitle(QTest::currentTestFunction());
    mainWindow.resize(size);
    mainWindow.move(availableGeometry.topLeft()
                    + QPoint(availableGeometry.width() - size.width(),
                             availableGeometry.height() - size.height()) / 2);
    QLabel *label = new QLabel(QTest::currentTestFunction());
    scene->addWidget(label);
    label->hide();
    mainWindow.show();
    combo->setFocus();
    mainWindow.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mainWindow));
    combo->showPopup();
    QWidget *comboPopup = combo->view()->window();
    QVERIFY(comboPopup);
    QVERIFY(QTest::qWaitForWindowExposed(comboPopup));
    label->show();
    QTRY_VERIFY(label->isVisible());
    QVERIFY(comboPopup->isVisible());
}

class TouchWidget : public QWidget
{
public:
    TouchWidget(QWidget *parent = 0) : QWidget(parent) {}

    bool event(QEvent *event)
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            event->accept();
            return true;
            break;
        default:
            break;
        }

        return QWidget::event(event);
    }
};

// QTBUG_45737
void tst_QGraphicsProxyWidget::forwardTouchEvent()
{
    QGraphicsScene *scene = new QGraphicsScene;

    TouchWidget *widget = new TouchWidget;

    widget->setAttribute(Qt::WA_AcceptTouchEvents);

    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget;

    proxy->setAcceptTouchEvents(true);
    proxy->setWidget(widget);

    scene->addItem(proxy);

    QGraphicsView *view = new QGraphicsView(scene);

    view->show();

    EventSpy eventSpy(widget);

    QTouchDevice *device = QTest::createTouchDevice();

    QCOMPARE(eventSpy.counts[QEvent::TouchBegin], 0);
    QCOMPARE(eventSpy.counts[QEvent::TouchUpdate], 0);
    QCOMPARE(eventSpy.counts[QEvent::TouchEnd], 0);

    QTest::touchEvent(view, device).press(0, QPoint(10, 10), view);
    QTest::touchEvent(view, device).move(0, QPoint(15, 15), view);
    QTest::touchEvent(view, device).move(0, QPoint(16, 16), view);
    QTest::touchEvent(view, device).release(0, QPoint(15, 15), view);

    QApplication::processEvents();

    QCOMPARE(eventSpy.counts[QEvent::TouchBegin], 1);
    QCOMPARE(eventSpy.counts[QEvent::TouchUpdate], 2);
    QCOMPARE(eventSpy.counts[QEvent::TouchEnd], 1);

    delete view;
    delete proxy;
    delete scene;
}

QTEST_MAIN(tst_QGraphicsProxyWidget)
#include "tst_qgraphicsproxywidget.moc"
