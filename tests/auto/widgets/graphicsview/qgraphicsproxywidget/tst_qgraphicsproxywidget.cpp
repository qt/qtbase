// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtWidgets/qabstractitemview.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qdial.h>
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qfontcombobox.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicsproxywidget.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qstylefactory.h>

#include <QtGui/qevent.h>

#include <QtCore/qmimedata.h>
#include <QtCore/qtimer.h>

#include <private/qapplication_p.h>
#include <private/qgraphicsproxywidget_p.h>
#include <private/qlayoutengine_p.h>    // qSmartMin functions...

#include <memory>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcTests, "qt.widgets.tests")

static QStringList weekDays()
{
    return {"monday"_L1, "tuesday"_L1, "wednesday"_L1, "thursday"_L1, "saturday"_L1, "sunday"_L1};
}

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
    bool eventFilter(QObject *, QEvent *event) override
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
    void keyPressEvent_data();
    void keyPressEvent();
    void keyReleaseEvent_data();
    void keyReleaseEvent();
    void mouseDoubleClickEvent();
    void mousePressReleaseEvent_data();
    void mousePressReleaseEvent();
    void resizeEvent_data();
    void resizeEvent();
    void paintEvent();
#if QT_CONFIG(wheelevent)
    void wheelEvent();
#endif
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
    void mapToGlobalIgnoreTranformation();
    void QTBUG_43780_visibility();
#if QT_CONFIG(wheelevent)
    void wheelEventPropagation();
#endif
    void forwardTouchEvent();
    void touchEventPropagation();
};

// Subclass that exposes the protected functions.
class SubQGraphicsProxyWidget : public QGraphicsProxyWidget
{
    friend tst_QGraphicsProxyWidget;
public:
    using QGraphicsProxyWidget::QGraphicsProxyWidget;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override {
        paintCount++;
        QGraphicsProxyWidget::paint(painter, option, widget);
    }

    void focusOutEvent(QFocusEvent *event) override
    {
        focusOut++;
        QGraphicsProxyWidget::focusOutEvent(event);
    }

    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::KeyPress && object == widget())
            keyPress++;
        return QGraphicsProxyWidget::eventFilter(object, event);
    }
    int paintCount = 0;
    int keyPress = 0;
    int focusOut = 0;
};

#if QT_CONFIG(wheelevent)
class WheelWidget : public QWidget
{
public:
    WheelWidget() { setFocusPolicy(Qt::WheelFocus); }

    void wheelEvent(QWheelEvent *event) override { event->accept(); wheelEventCalled = true; }

    bool wheelEventCalled = false;
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

void tst_QGraphicsProxyWidget::qgraphicsproxywidget()
{
    SubQGraphicsProxyWidget proxy;
    proxy.paint(nullptr, nullptr, nullptr);
    proxy.setWidget(nullptr);
    QCOMPARE(proxy.type(), int(QGraphicsProxyWidget::Type));
    QVERIFY(!proxy.widget());
    QEvent event(QEvent::None);
    proxy.eventFilter(nullptr, &event);
    QFocusEvent focusEvent(QEvent::FocusIn);
    focusEvent.ignore();
    proxy.focusInEvent(&focusEvent);
    QVERIFY(!focusEvent.isAccepted());
    QVERIFY(!proxy.focusNextPrevChild(false));
    QVERIFY(!proxy.focusNextPrevChild(true));
    proxy.focusOutEvent(&focusEvent);
    QHideEvent hideEvent;
    proxy.hideEvent(&hideEvent);
    QGraphicsSceneHoverEvent hoverEvent;
    proxy.hoverEnterEvent(&hoverEvent);
    proxy.hoverLeaveEvent(&hoverEvent);
    proxy.hoverMoveEvent(&hoverEvent);
    QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier);
    proxy.keyPressEvent(&keyEvent);
    proxy.keyReleaseEvent(&keyEvent);
    QGraphicsSceneMouseEvent mouseEvent;
    proxy.mouseDoubleClickEvent(&mouseEvent);
    proxy.mouseMoveEvent(&mouseEvent);
    proxy.mousePressEvent(&mouseEvent);
    proxy.mouseReleaseEvent(&mouseEvent);
    QGraphicsSceneResizeEvent resizeEvent;
    proxy.resizeEvent(&resizeEvent);
    QShowEvent showEvent;
    proxy.showEvent(&showEvent);
    proxy.sizeHint(Qt::PreferredSize, QSizeF());
}

// public void paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget)
void tst_QGraphicsProxyWidget::paint()
{
    SubQGraphicsProxyWidget proxy;

    proxy.paint(nullptr, nullptr, nullptr);
}

class MyProxyWidget : public QGraphicsProxyWidget
{
public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
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
    auto *proxyWidget = new MyProxyWidget;
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
    QScopedPointer<QStyle> style(QStyleFactory::create("Fusion"_L1));
    if (style.isNull())
        QSKIP("This test requires the Fusion style");
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

    auto *widget = new QWidget;
#ifndef QT_NO_CURSOR
    widget->setCursor(Qt::IBeamCursor);
#endif
    widget->setPalette(QPalette(Qt::magenta));
    widget->setLayoutDirection(Qt::RightToLeft);
    widget->setStyle(style.data());
    widget->setFont(QFont("Times"_L1));
    widget->setVisible(true);
    QApplicationPrivate::setActiveWindow(widget);
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

    QWidget *subWidget = insertWidget ? widget : nullptr;
    bool shouldBeInsertable = !hasParent && subWidget;
    if (shouldBeInsertable)
        subWidget->setAttribute(Qt::WA_QuitOnClose, true);

    if (hasParent) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
            "QGraphicsProxyWidget::setWidget: cannot embed widget .* which is not a toplevel widget, and is not a child of an embedded widget"_L1));
    }
    proxy->setWidget(subWidget);

    if (shouldBeInsertable) {
        QCOMPARE(proxy->widget(), subWidget);
        QVERIFY(subWidget->testAttribute(Qt::WA_DontShowOnScreen));
        QVERIFY(!subWidget->testAttribute(Qt::WA_QuitOnClose));
        QVERIFY(proxy->acceptHoverEvents());
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
        QVERIFY(proxy->acceptHoverEvents()); // to get widget enter events
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
        QVERIFY(!proxy->acceptHoverEvents());
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
        widget->setParent(nullptr);

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
    QCoreApplication::sendEvent(&scene, &windowActivate);

    auto *proxy = new SubQGraphicsProxyWidget;
    scene.addItem(proxy);

    auto *widget = new QWidget(nullptr, Qt::FramelessWindowHint);
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
        proxy->eventFilter(widget, &event);
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
            proxy->eventFilter(widget, &event);
        }
        QVERIFY(!proxy->isVisible());
        break;
                         }
    case QEvent::Show: {
        // A show event can either come from a widget or somewhere else
        widget->hide();
        if (fromObject) {
            widget->show();
        } else {
            QShowEvent event;
            proxy->eventFilter(widget, &event);
        }
        QVERIFY(proxy->isVisible());
        break;
                         }
    case QEvent::EnabledChange: {
        widget->setEnabled(false);
        proxy->setEnabled(false);
        if (fromObject) {
            widget->setEnabled(true);
            QVERIFY(proxy->isEnabled());
            widget->setEnabled(false);
            QVERIFY(!proxy->isEnabled());
        } else {
            QEvent event(QEvent::EnabledChange);
            proxy->eventFilter(widget, &event);
            // match the widget not the event
            QVERIFY(!proxy->isEnabled());
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
            proxy->eventFilter(widget, &event);
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
    QCoreApplication::sendEvent(&scene, &windowActivate);

    auto *proxy = new SubQGraphicsProxyWidget;
    proxy->setEnabled(true);
    scene.addItem(proxy);

    auto *widget = new QWidget;
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
    proxy->focusInEvent(&event);
    QTRY_COMPARE(widget->hasFocus(), widgetCanHaveFocus);
}

void tst_QGraphicsProxyWidget::focusInEventNoWidget()
{
    QGraphicsView view;
    QGraphicsScene scene(&view);
    auto *proxy = new SubQGraphicsProxyWidget;
    proxy->setEnabled(true);
    scene.addItem(proxy);
    view.show();

    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // <- shouldn't need to do this
    QFocusEvent event(QEvent::FocusIn, Qt::TabFocusReason);
    event.ignore();
    //should not crash
    proxy->focusInEvent(&event);
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

    std::unique_ptr<SubQGraphicsProxyWidget> proxyGuard(new SubQGraphicsProxyWidget);
    auto *proxy = proxyGuard.get();

    if (hasWidget) {
        auto *widget = new QLabel;
        widget->setText(R"(
            <html>
            <head>
                <meta name=\"qrichtext\" content=\"1\" />
                <style type=\"text/css\">
                    p, li { white-space: pre-wrap; }
                </style>
            </head>
            <body>
            <p>
                <a href=\"http://www.slashdot.org\">
                    <span style=\" text-decoration: underline; color:#0000ff;\">old</span>
                </a> foo
                <a href=\"http://www.reddit.org\">
                    <span style=\" text-decoration: underline; color:#0000ff;\">new</span>
                </a>
            </p>
            </body>
            </html>)"_L1);
        widget->setTextInteractionFlags(Qt::TextBrowserInteraction);
        proxy->setWidget(widget);
    }

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowFocused(&view));
    if (hasScene) {
        scene.addItem(proxyGuard.release());
        proxy->show();

        // widget should take precedence over scene so make scene.focusNextPrevChild return false
        // so we know that a true can only come from the widget
        if (!(hasWidget && hasScene)) {
            auto *item = new QGraphicsTextItem("Foo"_L1);
            item->setTextInteractionFlags(Qt::TextBrowserInteraction);
            scene.addItem(item);
            item->setPos(50, 40);
        }
        scene.setFocusItem(proxy);
        QVERIFY(proxy->hasFocus());
    }

    QCOMPARE(proxy->focusNextPrevChild(next), focusNextPrevChild);
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
    auto *proxy = new SubQGraphicsProxyWidget;
    scene.addItem(proxy);
    view.show();
    view.activateWindow();
    view.setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.isVisible());
    QTRY_COMPARE(QApplication::activeWindow(), (QWidget*)&view);

    std::unique_ptr<QWidget> widgetGuard(new QWidget);
    QWidget *widget = widgetGuard.get();
    widgetGuard->setFocusPolicy(Qt::WheelFocus);
    if (hasWidget)
        proxy->setWidget(widgetGuard.release());
    proxy->show();
    proxy->setFocus();
    QVERIFY(proxy->hasFocus());
    QEXPECT_FAIL("widget, focus to other widget", "Widget should have focus but doesn't", Continue);
    QEXPECT_FAIL("widget, focusOutCalled", "Widget should have focus but doesn't", Continue);
    QCOMPARE(widget->hasFocus(), hasWidget);

    if (!call) {
        QWidget *other = new QLineEdit(&view);
        other->show();
        QTRY_VERIFY(other->isVisible());
        other->setFocus();
        QTRY_VERIFY(other->hasFocus());
        QTRY_VERIFY(!proxy->hasFocus());
        QVERIFY(proxy->focusOut);
        QVERIFY(!widget->hasFocus());
    }
}

void tst_QGraphicsProxyWidget::focusProxy_QTBUG_51856()
{
#ifdef ANDROID
    QSKIP("This test leads to failures on subsequent test cases, QTBUG-119574");
#endif
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

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    auto *proxy = new SubQGraphicsProxyWidget;
    scene.addItem(proxy);
    view.show();
    view.raise();
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *spinBox = new FocusedSpinBox;

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

        auto *edit = new QLineEdit(&view);
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
    EventLogger() { installEventFilter(this); }

    void enterEvent(QEnterEvent *event) override
    {
        enterCount++;
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event ) override
    {
        leaveCount++;
        QWidget::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        event->setAccepted(true);
        moveCount++;
        QWidget::mouseMoveEvent(event);
    }

    int enterCount = 0;
    int leaveCount = 0;
    int moveCount = 0;

    int hoverEnter = 0;
    int hoverLeave = 0;
    int hoverMove = 0;

protected:
    bool eventFilter(QObject *object, QEvent *event) override
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
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *proxy = new SubQGraphicsProxyWidget;
    std::unique_ptr<EventLogger> widgetGuard(new EventLogger);
    EventLogger *widget = widgetGuard.get();
    widget->resize(50, 50);
    widget->setAttribute(Qt::WA_Hover, hoverEnabled);
    widget->setMouseTracking(true);
    view.resize(100, 100);
    if (hasWidget)
        proxy->setWidget(widgetGuard.release());
    proxy->setPos(50, 0);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    QTRY_VERIFY(!sceneChangedSpy.isEmpty());

    // outside graphics item
    QTest::mouseMove(&view, QPoint(10, 10));

    QVERIFY(!widget->testAttribute(Qt::WA_UnderMouse));
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
    QTRY_VERIFY(!widget->testAttribute(Qt::WA_UnderMouse));
    QTRY_COMPARE(widget->leaveCount, hasWidget ? 1 : 0);
    QTRY_COMPARE(widget->hoverLeave, (hasWidget && hoverEnabled) ? 1 : 0);
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
    QVERIFY(QTest::qWaitForWindowFocused(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget*)&view);

    auto *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!

    std::unique_ptr<QLineEdit> widgetGuard(new QLineEdit);
    QLineEdit *widget = widgetGuard.get();
    widget->resize(50, 50);
    view.resize(100, 100);
    if (hasWidget)
        proxy->setWidget(widgetGuard.release());

    scene.addItem(proxy);
    proxy->show();
    proxy->setPos(50, 0);
    proxy->setFocus();

    QTest::keyPress(view.viewport(), 'x');

    const auto expected = hasWidget ? QLatin1StringView("x") : QLatin1StringView{};
    QTRY_COMPARE(widget->text(), expected);
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
    QVERIFY(QTest::qWaitForWindowFocused(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget*)&view);


    auto *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    std::unique_ptr<QPushButton> widgetGuard(new QPushButton);
    QSignalSpy spy(widgetGuard.get(), SIGNAL(clicked()));
    widgetGuard->resize(50, 50);
    view.resize(100, 100);
    if (hasWidget) {
        proxy->setWidget(widgetGuard.release());
        proxy->show();
    }
    proxy->setPos(50, 0);
    scene.addItem(proxy);
    proxy->setFocus();

    QTest::keyPress(view.viewport(), Qt::Key_Space);
    QTRY_COMPARE(spy.size(), 0);
    QTest::keyRelease(view.viewport(), Qt::Key_Space);
    QTRY_COMPARE(spy.size(), hasWidget ? 1 : 0);
}

// protected void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
void tst_QGraphicsProxyWidget::mouseDoubleClickEvent()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    auto *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    auto *widget = new QLineEdit("foo"_L1);
    widget->resize(50, 50);
    proxy->setWidget(widget);

    proxy->setPos(50, 0);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    proxy->setFocus();

    view.resize(100, 100);
    view.show();

    QVERIFY(QTest::qWaitForWindowFocused(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget*)&view);
    // wait for scene to be updated before doing any coordinate mappings on it
    QTRY_VERIFY(!sceneChangedSpy.isEmpty());

    QPoint pointInLineEdit = view.mapFromScene(proxy->mapToScene(15, proxy->boundingRect().center().y()));
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, pointInLineEdit);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, {}, pointInLineEdit);
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, {}, pointInLineEdit);

    QTRY_COMPARE(widget->selectedText(), "foo"_L1);
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

    auto *proxy = new SubQGraphicsProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true); // ### remove me!!!
    std::unique_ptr<QPushButton> widgetGuard(new QPushButton);
    QSignalSpy spy(widgetGuard.get(), SIGNAL(clicked()));
    widgetGuard->resize(50, 50);
    if (hasWidget)
        proxy->setWidget(widgetGuard.release());
    proxy->setPos(50, 0);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    proxy->setFocus();

    // wait for scene to be updated before doing any coordinate mappings on it
    QTRY_VERIFY(!sceneChangedSpy.isEmpty());

    QPoint buttonCenter = view.mapFromScene(proxy->mapToScene(proxy->boundingRect().center()));
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, buttonCenter);
    QTRY_COMPARE(spy.size(), 0);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, {}, buttonCenter);
    QTRY_COMPARE(spy.size(), hasWidget ? 1 : 0);
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

    if (hasWidget)
        proxy.setWidget(new QWidget);

    QSize newSize(100, 100);
    QGraphicsSceneResizeEvent event;
    event.setOldSize(QSize(10, 10));
    event.setNewSize(newSize);
    proxy.resizeEvent(&event);
    if (hasWidget)
        QCOMPARE(proxy.widget()->size(), newSize);
}

void tst_QGraphicsProxyWidget::paintEvent()
{
    //we test that calling update on a widget inside a QGraphicsView is triggering a repaint
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowFocused(&view));
    QVERIFY(view.isActiveWindow());

    SubQGraphicsProxyWidget proxy;

    auto *w = new QWidget;
    //showing the widget here seems to create a bug in Graphics View
    //this bug prevents the widget from being updated

    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));
    proxy.setWidget(w);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(&proxy);

    QTRY_VERIFY(!sceneChangedSpy.isEmpty()); // make sure the scene is ready

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

    auto *wheelWidget = new WheelWidget();
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

void tst_QGraphicsProxyWidget::sizePolicy()
{
    for (int p = 0; p < 2; ++p) {
        bool hasWidget = (p == 0);
        QGraphicsProxyWidget proxy;
        std::unique_ptr<QWidget> widgetGuard(new QWidget);
        QWidget *widget = widgetGuard.get();
        QSizePolicy proxyPol(QSizePolicy::Maximum, QSizePolicy::Expanding);
        proxy.setSizePolicy(proxyPol);
        QSizePolicy widgetPol(QSizePolicy::Fixed, QSizePolicy::Minimum);
        widget->setSizePolicy(widgetPol);

        QCOMPARE(proxy.sizePolicy(), proxyPol);
        QCOMPARE(widget->sizePolicy(), widgetPol);
        if (hasWidget) {
            proxy.setWidget(widgetGuard.release());
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
    }
}

void tst_QGraphicsProxyWidget::minimumSize()
{
    QGraphicsProxyWidget proxy;
    std::unique_ptr<QWidget> widgetGuard(new QWidget);
    QSize minSize(50, 50);
    widgetGuard->setMinimumSize(minSize);
    proxy.resize(30, 30);
    widgetGuard->resize(30,30);
    QCOMPARE(proxy.size(), QSizeF(30, 30));
    proxy.setWidget(widgetGuard.release());
    QCOMPARE(proxy.size().toSize(), minSize);
    QCOMPARE(proxy.minimumSize().toSize(), minSize);
    proxy.widget()->setMinimumSize(70, 70);
    QCOMPARE(proxy.minimumSize(), QSizeF(70, 70));
    QCOMPARE(proxy.size(), QSizeF(70, 70));
}

void tst_QGraphicsProxyWidget::maximumSize()
{
    QGraphicsProxyWidget proxy;
    std::unique_ptr<QWidget> widgetGuard(new QWidget);
    QSize maxSize(150, 150);
    widgetGuard->setMaximumSize(maxSize);
    proxy.resize(200, 200);
    widgetGuard->resize(200,200);
    QCOMPARE(proxy.size(), QSizeF(200, 200));
    proxy.setWidget(widgetGuard.release());
    QCOMPARE(proxy.size().toSize(), maxSize);
    QCOMPARE(proxy.maximumSize().toSize(), maxSize);
    proxy.widget()->setMaximumSize(70, 70);
    QCOMPARE(proxy.maximumSize(), QSizeF(70, 70));
    QCOMPARE(proxy.size(), QSizeF(70, 70));
}

class View : public QGraphicsView
{
public:
    using QGraphicsView::QGraphicsView;

    QRegion paintEventRegion;
    int npaints = 0;

protected:
    void paintEvent(QPaintEvent *event) override
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
    ScrollWidget()
    {
        resize(200, 200);
    }
    QRegion paintEventRegion;
    int npaints = 0;

public slots:
    void updateScroll()
    {
        update(0, 0, 200, 10);
        scroll(0, 10, QRect(0, 0, 100, 20));
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        ++npaints;
        paintEventRegion += event->region();
        QPainter painter(this);
        painter.fillRect(event->rect(), Qt::blue);
    }
};

// ### work around missing QList ctor from iterator pair:
static QList<QRect> rects(const QRegion &region)
{
    QList<QRect> result;
    for (QRect r : region)
        result.push_back(r);
    return result;
}

void tst_QGraphicsProxyWidget::scrollUpdate()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    auto *widget = new ScrollWidget;

    QGraphicsScene scene;
    scene.addWidget(widget);

    View view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.npaints >= 1);
    QTest::qWait(150);
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
             QList<QRect>() << QRect(0, 0, 200, 12) << QRect(0, 12, 102, 10));
    QCOMPARE(widget->npaints, 2);
    QCOMPARE(rects(widget->paintEventRegion),
             QList<QRect>() << QRect(0, 0, 200, 12) << QRect(0, 12, 102, 10));
}

void tst_QGraphicsProxyWidget::setWidget_simple()
{
    QGraphicsProxyWidget proxy;
    auto *lineEdit = new QLineEdit;
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
        proxy.setWidget(nullptr);
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
    auto *widget = new QWidget;
    widget->setGeometry(QRect{ QPoint{}, size.toSize() });
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
    auto *lineEdit = new QLineEdit;
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
    auto *lineEdit = new QLineEdit;
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
    auto *lineEdit = new QLineEdit;
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
    auto *lineEdit = new QLineEdit;
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
    auto *edit = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));

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
    auto *edit = new QLineEdit;
    auto *edit2 = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();
    QGraphicsProxyWidget *editProxy2 = scene.addWidget(edit2);
    editProxy2->show();
    editProxy2->setPos(0, editProxy->rect().height() * 1.1);

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));

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

    auto *box = new QGroupBox("QGroupBox"_L1);
    box->setCheckable(true);
    box->setChecked(true);
    auto *vlayout = new QVBoxLayout(box);
    auto *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1"_L1);
    auto *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2"_L1);
    vlayout->addWidget(edit1);
    vlayout->addWidget(edit2);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));

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

    auto *box = new QGroupBox("QGroupBox"_L1);
    box->setCheckable(true);
    box->setChecked(true);
    auto *vlayout = new QVBoxLayout(box);
    auto *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1"_L1);
    auto *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2"_L1);
    auto *fontComboBox = new QFontComboBox;
    vlayout->addWidget(edit1);
    vlayout->addWidget(fontComboBox);
    vlayout->addWidget(edit2);

    auto *box_2 = new QGroupBox("QGroupBox 2"_L1);
    box_2->setCheckable(true);
    box_2->setChecked(true);
    vlayout = new QVBoxLayout(box_2);
    auto *edit1_2 = new QLineEdit;
    edit1_2->setText("QLineEdit 1_2"_L1);
    auto *edit2_2 = new QLineEdit;
    edit2_2->setText("QLineEdit 2_2"_L1);
    auto *fontComboBox2 = new QFontComboBox;
    vlayout->addWidget(edit1_2);
    vlayout->addWidget(fontComboBox2);
    vlayout->addWidget(edit2_2);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();

    QGraphicsProxyWidget *proxy_2 = scene.addWidget(box_2);
    proxy_2->setPos(proxy->boundingRect().width() * 1.2, 0);
    proxy_2->show();

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->rotate(45);
    view->scale(0.5, 0.5);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));

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
    auto *edit = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));
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
    auto *edit = new QLineEdit;
    QGraphicsProxyWidget *editProxy = scene.addWidget(edit);
    editProxy->show();
    auto *edit2 = new QLineEdit;
    auto *edit2Proxy = scene.addWidget(edit2);
    edit2Proxy->show();
    edit2Proxy->setPos(editProxy->boundingRect().width() * 1.01, 0);

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));
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

    auto *box = new QGroupBox("QGroupBox"_L1);
    box->setCheckable(true);
    box->setChecked(true);
    auto *vlayout = new QVBoxLayout(box);
    auto *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1"_L1);
    auto *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2"_L1);
    vlayout->addWidget(edit1);
    vlayout->addWidget(edit2);

    auto *box_2 = new QGroupBox("QGroupBox 2"_L1);
    box_2->setCheckable(true);
    box_2->setChecked(true);
    vlayout = new QVBoxLayout(box_2);
    auto *edit1_2 = new QLineEdit;
    edit1_2->setText("QLineEdit 1_2"_L1);
    auto *edit2_2 = new QLineEdit;
    edit2_2->setText("QLineEdit 2_2"_L1);
    vlayout->addWidget(edit1_2);
    vlayout->addWidget(edit2_2);

    QGraphicsProxyWidget *proxy = scene.addWidget(box);
    proxy->show();

    QGraphicsProxyWidget *proxy_2 = scene.addWidget(box_2);
    proxy_2->setPos(proxy->boundingRect().width() * 1.2, 0);
    proxy_2->show();

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;
    auto *view = new QGraphicsView(&scene);

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(view);
    layout->addWidget(rightDial);

    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowFocused(&window));
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
    std::unique_ptr<QComboBox> boxGuard(new QComboBox);
    QStyleOptionComboBox opt;
    opt.initFrom(boxGuard.get());
    opt.editable = boxGuard->isEditable();
    if (boxGuard->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt))
        QSKIP("Does not work due to SH_Combobox_Popup");

    // ProxyWidget should automatically create proxy's when the widget creates a child
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view.setGeometry(0, 100, 480, 500);
    view.show();

    QComboBox *box = boxGuard.get();
    auto *proxy = new QGraphicsProxyWidget;
    box->setGeometry(0, 0, 320, 40);
    box->addItems(weekDays());
    QCOMPARE(proxy->childItems().size(), 0);
    proxy->setWidget(boxGuard.release());
    proxy->show();
    scene.addItem(proxy);

    QCOMPARE(box->pos(), QPoint());
    QCOMPARE(proxy->pos(), QPointF());

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(125);
    QApplication::processEvents();

    QTest::mousePress(view.viewport(), Qt::LeftButton, {},
                      view.mapFromScene(proxy->mapToScene(proxy->boundingRect().center())));

    QTRY_COMPARE(box->pos(), QPoint());

    QCOMPARE(proxy->childItems().size(), 1);
    auto *child = static_cast<QGraphicsProxyWidget*>(proxy->childItems().at(0));
    QVERIFY(child->isWidget());
    QVERIFY(child->widget());
    QCOMPARE(child->widget()->parent(), box);

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
    auto *groupBox = new QGroupBox;
    groupBox->setTitle("GroupBox"_L1);
    groupBox->setCheckable(true);

    auto *box = new QComboBox;
    box->addItems(weekDays());

    auto *layout = new QVBoxLayout(groupBox);
    layout->addWidget(new QLineEdit("QLineEdit"_L1));
    layout->addWidget(box);
    layout->addWidget(new QLineEdit("QLineEdit"_L1));

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
    auto *child = static_cast<QGraphicsProxyWidget*>(groupBoxProxy->childItems().at(0));
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

    auto *proxy = new QGraphicsProxyWidget;
    auto *widget = new QLineEdit;
    proxy->setWidget(widget);
    QSignalSpy sceneChangedSpy(&scene, &QGraphicsScene::changed);
    scene.addItem(proxy);
    QTRY_VERIFY(!sceneChangedSpy.isEmpty()); // make sure the scene is ready

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
    QString toolTip = "Qt rocks!"_L1;
    QString toolTip2 = "Qt rocks even more!"_L1;

    auto *button = new QPushButton("button"_L1);
    auto *proxy = new QGraphicsProxyWidget;
    auto *proxyd = static_cast<QGraphicsProxyWidgetPrivate *>(QGraphicsItemPrivate::get(proxy));
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
    QVERIFY(QTest::qWaitForWindowActive(&view));
    {
        QHelpEvent helpEvent(QEvent::ToolTip, view.viewport()->rect().topLeft(),
                             view.viewport()->mapToGlobal(view.viewport()->rect().topLeft()));
        QApplication::sendEvent(view.viewport(), &helpEvent);
        QTest::qWait(350);

        bool foundView = false;
        bool foundTipLabel = false;
        const auto widgets = QApplication::topLevelWidgets();
        for (QWidget *widget : widgets) {
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

    QComboBox box;
    box.addItem("Item 1"_L1);
    box.addItem("Item 2"_L1);
    box.addItem("Item 3"_L1);
    box.addItem("Item 4"_L1);

    if (moveCombo)
        box.move(comboPos);

    QGraphicsProxyWidget *proxy = scene.addWidget(&box);
    proxy->show();
    QVERIFY(proxy->isVisible());
    QVERIFY(box.isVisible());

    if (!moveCombo)
        proxy->setPos(proxyPos);

    QCOMPARE(proxy->pos(), proxyPos);
    QCOMPARE(box.pos(), comboPos);

    for (int i = 0; i < 2; ++i) {
        box.showPopup();
        QWidget *menu = box.findChild<QWidget *>();
        QVERIFY(menu);
        QTRY_VERIFY(menu->isVisible());
        QVERIFY(menu->testAttribute(Qt::WA_DontShowOnScreen));

        QCOMPARE(proxy->childItems().size(), 1);
        auto *proxyChild = qobject_cast<QGraphicsProxyWidget *>(
            static_cast<QGraphicsWidget *>(proxy->childItems().constFirst()));
        QVERIFY(proxyChild);
        QVERIFY(proxyChild->isVisible());
        qreal expectedXPosition = 0.0;

        // The Mac style wants the popup to show up at QPoint(4 - 11, 1).
        // See QMacStyle::subControlRect SC_ComboBoxListBoxPopup.
        if (QApplication::style()->inherits("QMacStyle"))
            expectedXPosition = qreal(4 - 11);

        QTRY_COMPARE(proxyChild->pos().x(), expectedXPosition);
        menu->hide();
    }
}

void tst_QGraphicsProxyWidget::autoShow()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsProxyWidget *proxy1 = scene.addWidget(new QPushButton("Button1"_L1));

    auto *button2 = new QPushButton("Button2"_L1);
    button2->hide();
    auto *proxy2 = new QGraphicsProxyWidget();
    proxy2->setWidget(button2);
    scene.addItem(proxy2);

    view.show();
    QApplication::processEvents();

    QVERIFY(proxy1->isVisible());
    QVERIFY(!proxy2->isVisible());
}

void tst_QGraphicsProxyWidget::windowOpacity()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    auto *widget = new QWidget;
    widget->resize(100, 100);
    QGraphicsProxyWidget *proxy = scene.addWidget(widget);
    proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QApplicationPrivate::setActiveWindow(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(view.isActiveWindow());

    qRegisterMetaType<QList<QRectF> >("QList<QRectF>");
    QSignalSpy signalSpy(&scene, SIGNAL(changed(QList<QRectF>)));

    EventSpy eventSpy(widget);
    QVERIFY(widget->isVisible());

    widget->setWindowOpacity(0.5);

    // Make sure setWindowOpacity triggers an update on the scene,
    // and not on the widget or the proxy itself. The entire proxy needs an update
    // in case it has a window decoration. Update: QGraphicsItem::CacheMode is
    // disabled on platforms without alpha channel support in QPixmap (e.g.,
    // X11 without XRender).
    int paints = 0;
    QTRY_COMPARE(eventSpy.counts[QEvent::UpdateRequest], 0);
    QTRY_COMPARE(eventSpy.counts[QEvent::Paint], paints);

    QTRY_COMPARE(signalSpy.size(), 1);
    const QList<QVariant> arguments = signalSpy.takeFirst();
    const QList<QRectF> updateRects = qvariant_cast<QList<QRectF> >(arguments.at(0));
    QCOMPARE(updateRects.size(), 1);
    QCOMPARE(updateRects.at(0), proxy->sceneBoundingRect());
}

void tst_QGraphicsProxyWidget::stylePropagation()
{
    QPointer<QStyle> windowsStyle = QStyleFactory::create("windows"_L1);

    auto *edit = new QLineEdit;
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
    edit->setStyle(nullptr);
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
    proxy.setStyle(nullptr);
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

    auto *edit = new QLineEdit;
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
    if (edit->palette() != palette)
        QEXPECT_FAIL("", "Test case fails unless run alone", Abort);
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

    auto *edit = new QLineEdit;
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
    auto *edit2 = new QLineEdit;
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
      button = new QPushButton("Push me"_L1);
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
    auto *w = new MainWidget();
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
    auto *parent = new QGraphicsProxyWidget;
    auto *child = new QGraphicsProxyWidget;
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

    auto *box = new QGroupBox("QGroupBox"_L1);
    box->setCheckable(true);
    box->setChecked(true);
    auto *edit1 = new QLineEdit;
    edit1->setText("QLineEdit 1"_L1);
    auto *edit2 = new QLineEdit;
    edit2->setText("QLineEdit 2"_L1);
    auto *checkbox = new QCheckBox("QCheckBox"_L1);
    auto *vlayout = new QVBoxLayout(box);
    vlayout->addWidget(edit1);
    vlayout->addWidget(edit2);
    vlayout->addWidget(checkbox);
    vlayout->insertStretch(-1);

    auto *leftDial = new QDial;
    auto *rightDial = new QDial;

    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    layout->addWidget(leftDial);
    layout->addWidget(box);
    layout->addWidget(rightDial);

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

    QTest::mousePress(view.viewport(), Qt::LeftButton, {},
                      view.mapFromScene(checkboxProxy->mapToScene(QPointF(8,8))));
    QTRY_COMPARE(spy.size(), 0);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, {},
                        view.mapFromScene(checkboxProxy->mapToScene(QPointF(8,8))));
    QTRY_COMPARE(spy.size(), 1);



    boxProxy->setWidget(nullptr);

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
    ContextMenuWidget() : QLabel(QStringLiteral("ContextMenuWidget")) { }

    bool embeddedPopup = false;
    bool gotContextMenuEvent = false;

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ContextMenu) {
            if (!m_timer) {
                m_timer = new QTimer(this);
                m_timer->setInterval(10);
                connect(m_timer, SIGNAL(timeout()), this, SLOT(checkMenu()));
                m_timer->start();
            }
        }
        return QLabel::event(event);
    }
    void contextMenuEvent(QContextMenuEvent *) override
    {
        gotContextMenuEvent = true;
    }

private slots:
    void checkMenu()
    {
        auto *menu = findChild<QMenu *>();
        if (!m_embeddedPopupSet) {
            m_embeddedPopupSet = true;
            embeddedPopup = menu != nullptr;
        }
        if (menu && menu->isVisible())
            menu->hide();
        hide();
    }

private:
    bool m_embeddedPopupSet = false;
    QTimer *m_timer{};
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

    auto *widget = new ContextMenuWidget;
    if (actionsContextMenu) {
        widget->addAction(new QAction("item 1"_L1, widget));
        widget->addAction(new QAction("item 2"_L1, widget));
        widget->addAction(new QAction("item 3"_L1, widget));
        widget->setContextMenuPolicy(Qt::ActionsContextMenu);
    }
    QGraphicsScene scene;
    QGraphicsProxyWidget *proxyWidget = scene.addWidget(widget);
    QGraphicsView view(&scene);
    view.setWindowTitle(QStringLiteral("actionsContextMenu"));
    view.resize(200, 200);
    view.move(QGuiApplication::primaryScreen()->geometry().center() - QPoint(100, 100));
    view.show();
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
    QCoreApplication::sendEvent(view.viewport(), &contextMenuEvent);

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
    auto *combo = new QComboBox;
    combo->addItems({ "red"_L1, "green"_L1, "blue"_L1, "white"_L1, "black"_L1,
                      "yellow"_L1, "cyan"_L1, "magenta"_L1 });
    dialog.setLayout(new QVBoxLayout);
    dialog.layout()->addWidget(combo);

    QGraphicsProxyWidget *proxy = scene.addWidget(&dialog);
    view.show();

    proxy->setWidget(nullptr);
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
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QFETCH(bool, bypass);

    std::unique_ptr<QWidget> widgetGuard(new QWidget);
    QWidget *widget = widgetGuard.get();
    widget->resize(100, 100);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QGraphicsProxyWidget *proxy = scene.addWidget(widgetGuard.release());

    QCOMPARE(proxy->widget(), widget);
    QVERIFY(proxy->childItems().isEmpty());

    Qt::WindowFlags flags;
    flags |= Qt::Dialog;
    if (bypass)
        flags |= Qt::BypassGraphicsProxyWidget;
    QFileDialog dialog(widget, flags);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowActive(&dialog));

    QCOMPARE(proxy->childItems().size(), bypass ? 0 : 1);
    if (!bypass)
        QCOMPARE(static_cast<QGraphicsProxyWidget *>(proxy->childItems().constFirst())->widget(), &dialog);

    dialog.hide();
    QApplication::processEvents();
}

static void makeDndEvent(QGraphicsSceneDragDropEvent *event, QGraphicsView *view, const QPointF &pos)
{
    event->setScenePos(pos);
    event->setScreenPos(view->mapToGlobal(view->mapFromScene(pos)));
    event->setButtons(Qt::LeftButton);
    event->setModifiers({});
    event->setPossibleActions(Qt::CopyAction);
    event->setProposedAction(Qt::CopyAction);
    event->setDropAction(Qt::CopyAction);
    event->setWidget(view->viewport());
    event->setSource(view->viewport());
    event->ignore();
}

void tst_QGraphicsProxyWidget::dragDrop()
{
    auto *button = new QPushButton; // acceptDrops(false)
    auto *edit = new QLineEdit; // acceptDrops(true)

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
    data.setText("hei"_L1);
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragEnter);
        makeDndEvent(&event, &view, QPointF(50, 25));
        event.setMimeData(&data);
        QCoreApplication::sendEvent(&scene, &event);
        QVERIFY(event.isAccepted());
    }
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragMove);
        makeDndEvent(&event, &view, QPointF(50, 25));
        event.setMimeData(&data);
        QCoreApplication::sendEvent(&scene, &event);
        QVERIFY(!event.isAccepted());
    }
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragMove);
        makeDndEvent(&event, &view, QPointF(50, 75));
        event.setMimeData(&data);
        QCoreApplication::sendEvent(&scene, &event);
        QVERIFY(event.isAccepted());
    }
    {
        QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDrop);
        makeDndEvent(&event, &view, QPointF(50, 75));
        event.setMimeData(&data);
        QCoreApplication::sendEvent(&scene, &event);
        QVERIFY(event.isAccepted());
    }
    QCOMPARE(edit->text(), "hei"_L1);
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
    const Qt::WindowFlags proxyWFlags(proxyFlags);
    const Qt::WindowFlags widgetWFlags(widgetFlags);
    const Qt::WindowFlags resultingProxyWFlags(resultingProxyFlags);
    const Qt::WindowFlags resultingWidgetWFlags(resultingWidgetFlags);

    QGraphicsProxyWidget proxy(nullptr, proxyWFlags);
    QVERIFY((proxy.windowFlags() & proxyWFlags) == proxyWFlags);

    auto *widget = new QWidget(nullptr, widgetWFlags);
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
    auto *comboBox = new QComboBox;
    comboBox->addItem("Item 1"_L1);
    comboBox->addItem("Item 2"_L1);
    comboBox->addItem("Item 3"_L1);
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
    QGraphicsProxyWidget *proxy = scene.addWidget(new QPushButton("Hello World"_L1));
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
    bool event(QEvent *e) override
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
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QGraphicsScene scene;

    // check that the proxy is initialized with the correct input method sensitivity
    for (int i = 0; i < 2; ++i)
    {
        auto *lineEdit = new QLineEdit;
        lineEdit->setAttribute(Qt::WA_InputMethodEnabled, !!i);
        QGraphicsProxyWidget *proxy = scene.addWidget(lineEdit);
        QCOMPARE(!!(proxy->flags() & QGraphicsItem::ItemAcceptsInputMethod), !!i);
    }

    // check that input method events are only forwarded to widgets with focus
    for (int i = 0; i < 2; ++i)
    {
        auto *lineEdit = new InputMethod_LineEdit;
        lineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
        QGraphicsProxyWidget *proxy = scene.addWidget(lineEdit);

        if (i)
            lineEdit->setFocus();

        lineEdit->inputMethodEvents = 0;
        QInputMethodEvent event;
        QCoreApplication::sendEvent(proxy, &event);
        QCOMPARE(lineEdit->inputMethodEvents, i);
    }

    scene.clear();
    QGraphicsView view(&scene);
    auto *w = new QWidget;
    auto *vLayout = new QVBoxLayout(w);
    auto *lineEdit = new QLineEdit;
    lineEdit->setEchoMode(QLineEdit::Password);
    vLayout->addWidget(lineEdit);
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
    auto *le1 = new QLineEdit;
    QGraphicsProxyWidget *proxy = scene.addWidget(le1);

    QGraphicsView view(&scene);

    {
        EventSpy proxySpy(proxy);
        EventSpy widgetSpy(proxy->widget());

        view.setFrameStyle(0);
        view.resize(300, 300);
        view.show();
        QVERIFY(QTest::qWaitForWindowFocused(&view));

        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());

        QCOMPARE(proxySpy.counts[QEvent::FocusIn], 0);
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 0);

        QPointF lineEditCenter = proxy->mapToScene(proxy->boundingRect().center());
        // Spontaneous mouse click sets focus on a clickable widget.
        for (int retry = 0; retry < 50 && !proxy->hasFocus(); retry++)
            QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, view.mapFromScene(lineEditCenter));
        QVERIFY(proxy->hasFocus());
        QVERIFY(proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusIn], 1);
        QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 1);

        scene.setFocusItem(nullptr);
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 1);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 1);

        // Non-spontaneous mouse click sets focus if the widget has been clicked before
        {
            QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
            event.setScenePos(lineEditCenter);
            event.setButton(Qt::LeftButton);
            QCoreApplication::sendEvent(&scene, &event);
            QVERIFY(proxy->hasFocus());
            QVERIFY(proxy->widget()->hasFocus());
            QCOMPARE(proxySpy.counts[QEvent::FocusIn], 2);
            QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 2);
        }
    }

    scene.setFocusItem(nullptr);
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
            QCoreApplication::sendEvent(&scene, &event);
            QVERIFY(!proxy->hasFocus());
            QVERIFY(!proxy->widget()->hasFocus());
            QCOMPARE(proxySpy.counts[QEvent::FocusIn], 0);
            QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 0);
        }

        scene.setFocusItem(nullptr);
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QCOMPARE(proxySpy.counts[QEvent::FocusOut], 0);
        QCOMPARE(widgetSpy.counts[QEvent::FocusOut], 0);

        // Spontaneous click on non-clickable widget does not give focus.
        proxy->widget()->setFocusPolicy(Qt::NoFocus);
        QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, view.mapFromScene(lineEditCenter));
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());

        // Multiple clicks should only result in one FocusIn.
        proxy->widget()->setFocusPolicy(Qt::StrongFocus);
        scene.setFocusItem(nullptr);
        QVERIFY(!proxy->hasFocus());
        QVERIFY(!proxy->widget()->hasFocus());
        QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, view.mapFromScene(lineEditCenter));
        QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, view.mapFromScene(lineEditCenter));
        QVERIFY(proxy->hasFocus());
        QVERIFY(proxy->widget()->hasFocus());
        QCOMPARE(widgetSpy.counts[QEvent::FocusIn], 1);
        QCOMPARE(proxySpy.counts[QEvent::FocusIn], 1);
    }
}

void tst_QGraphicsProxyWidget::windowFrameMargins()
{
    // Make sure the top margin is non-zero when passing Qt::Window.
    auto *proxy = new QGraphicsProxyWidget(nullptr, Qt::Window);

    qreal left, top, right, bottom;
    proxy->getWindowFrameMargins(&left, &top, &right, &bottom);
    QVERIFY(top > 0);

    proxy->setWidget(new QPushButton("testtest"_L1));
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

void tst_QGraphicsProxyWidget::QTBUG_6986_sendMouseEventToAlienWidget()
{
    struct HoverButton : public QPushButton
    {
        using QPushButton::QPushButton;
        bool hoverLeaveReceived = false;

        bool event(QEvent* e) override
        {
            if (QEvent::HoverLeave == e->type())
                hoverLeaveReceived = true;
            return QPushButton::event(e);
        }
    };

    QGraphicsScene scene;
    auto *background = new QWidget;
    background->setGeometry(0, 0, 500, 500);
    auto *hoverButton = new HoverButton(background);
    hoverButton->setText("Second button"_L1);
    hoverButton->setGeometry(10, 10, 200, 50);
    scene.addWidget(background);

    auto *hideButton = new QPushButton("I'm a button with a very very long text"_L1);
    hideButton->setGeometry(10, 10, 400, 50);
    QGraphicsProxyWidget *topButton = scene.addWidget(hideButton);
    connect(hideButton, &QPushButton::clicked, &scene, [&]() { topButton->hide(); });
    topButton->setFocus();

    QGraphicsView view(&scene);
    view.resize(600, 600);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QPoint topButtonTopLeftCorner = view.mapFromScene(topButton->scenePos());
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, topButtonTopLeftCorner);
    // move to the bottom right corner (buttons are placed in the top left corner)
    QVERIFY(!hoverButton->hoverLeaveReceived);
    QTest::mouseMove(view.viewport(), view.viewport()->rect().bottomRight());
    if (QGuiApplication::platformName() == "cocoa"_L1) {
        // The "Second button" does not receive QEvent::HoverLeave
        QEXPECT_FAIL("", "This test fails only on Cocoa. Investigate why. See QTBUG-69219", Continue);
    }
    QTRY_VERIFY(hoverButton->hoverLeaveReceived);
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
    view.setWindowTitle(QLatin1StringView(QTest::currentTestFunction()));
    view.resize(size);
    view.move(availableGeometry.bottomRight() - QPoint(size.width(), size.height()) - QPoint(100, 100));
    auto *embeddedWidget = new QGroupBox("Embedded"_L1);
    embeddedWidget->setStyleSheet("background-color: \"yellow\"; "_L1);
    embeddedWidget->setFixedSize((size - QSize(10, 10)) / 2);
    auto *childWidget = new QGroupBox("Child"_L1, embeddedWidget);
    childWidget->setStyleSheet("background-color: \"red\"; "_L1);
    childWidget->resize(embeddedWidget->size() / 2);
    childWidget->move(embeddedWidget->width() / 4, embeddedWidget->height() / 4); // center in embeddedWidget
    scene.addWidget(embeddedWidget);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QPoint embeddedCenter = embeddedWidget->rect().center();
    const QPoint embeddedCenterGlobal = embeddedWidget->mapToGlobal(embeddedCenter);
    QCOMPARE(embeddedWidget->mapFromGlobal(embeddedCenterGlobal), embeddedCenter);
    // This should be equivalent to the view center give or take rounding
    // errors due to odd window margins
    const int Tolerance = qCeil(4 * view.devicePixelRatio());
    const QPoint viewCenter = view.geometry().center();
    QVERIFY2((viewCenter - embeddedCenterGlobal).manhattanLength() <= Tolerance,
             msgPointMismatch(embeddedCenterGlobal, viewCenter).constData());

    // Same test with child centered on embeddedWidget. Also make sure
    // the roundtrip maptoGlobal()/mapFromGlobal() returns the same
    // point since that is important for mouse event handling (QTBUG-50030,
    // QTBUG-50136).
    const QPoint childCenter = childWidget->rect().center();
    const QPoint childCenterGlobal = childWidget->mapToGlobal(childCenter);
    QCOMPARE(childWidget->mapFromGlobal(childCenterGlobal), childCenter);
    QVERIFY2((viewCenter - childCenterGlobal).manhattanLength() <= Tolerance,
             msgPointMismatch(childCenterGlobal, viewCenter).constData());
}

void tst_QGraphicsProxyWidget::mapToGlobalWithoutScene() // QTBUG-44509
{
    QGraphicsProxyWidget proxyWidget;
    auto *embeddedWidget = new QWidget;
    proxyWidget.setWidget(embeddedWidget);
    const QPoint localPos(0, 0);
    const QPoint globalPos = embeddedWidget->mapToGlobal(localPos);
    QCOMPARE(embeddedWidget->mapFromGlobal(globalPos), localPos);
}

// QTBUG-128913,  QGraphicsProxyWidget with ItemIgnoresTransformations
void tst_QGraphicsProxyWidget::mapToGlobalIgnoreTranformation()
{
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 2;
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowTitle(QLatin1StringView(QTest::currentTestFunction()));
    view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setTransform(QTransform::fromScale(2, 2));
    view.resize(size);
    view.move(availableGeometry.bottomRight() - QPoint(size.width(), size.height())
              - QPoint(100, 100));

    static constexpr int labelWidth = 200;
    static constexpr int labelHeight = 50;
    auto *transforming = new QGraphicsProxyWidget();
    auto *transformingLabel = new QLabel("Transforming"_L1);
    transformingLabel->resize(labelWidth, labelHeight);
    transforming->setWidget(transformingLabel);
    transforming->setPos(0, 0);
    scene.addItem(transforming);

    auto *nonTransforming = new QGraphicsProxyWidget();
    nonTransforming->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    auto *nonTransformingLabel = new QLabel("NonTransforming"_L1);
    nonTransformingLabel->resize(labelWidth, labelHeight);
    nonTransforming->setWidget(nonTransformingLabel);
    nonTransforming->setPos(labelWidth, 0);
    scene.addItem(nonTransforming);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QPoint labelCenter{ labelWidth / 2, labelHeight / 2 };
    const QPoint topPos = view.geometry().topLeft() + view.viewport()->geometry().topLeft();
    const QPoint transformingGlobal = transformingLabel->mapToGlobal(labelCenter);
    const QPoint nonTransformingGlobal = nonTransformingLabel->mapToGlobal(labelCenter);

    // Center of label at 0,0 scaled by 2 should match size
    QCOMPARE(transformingGlobal - topPos, QPoint(labelWidth, labelHeight));

    // Center of non-transforming label at 200 (scaled by 2), 0
    QCOMPARE(nonTransformingGlobal - topPos,
             QPoint(labelWidth * 2 + labelWidth / 2, labelHeight / 2));
}

// QTBUG_43780: Embedded widgets have isWindow()==true but showing them should not
// trigger the top-level widget code path of show() that closes all popups
// (for example combo popups).
void tst_QGraphicsProxyWidget::QTBUG_43780_visibility()
{
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 4;
    QWidget mainWindow;
    auto *layout = new QVBoxLayout(&mainWindow);
    auto *combo = new QComboBox(&mainWindow);
    combo->addItems({ "i1"_L1, "i2"_L1, "i3"_L1 });
    layout->addWidget(combo);
    auto *scene = new QGraphicsScene(&mainWindow);
    auto *view = new QGraphicsView(scene, &mainWindow);
    layout->addWidget(view);
    mainWindow.setWindowTitle(QLatin1StringView(QTest::currentTestFunction()));
    mainWindow.resize(size);
    mainWindow.move(availableGeometry.topLeft()
                    + QPoint(availableGeometry.width() - size.width(),
                             availableGeometry.height() - size.height()) / 2);
    auto *label = new QLabel(QLatin1StringView(QTest::currentTestFunction()));
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
    TouchWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    bool event(QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            event->accept();
            return true;
        default:
            break;
        }

        return QWidget::event(event);
    }
};

#if QT_CONFIG(wheelevent)
/*!
    QGraphicsProxyWidget receives wheel events from QGraphicsScene, and then
    generates a new event that is sent spontaneously in order to enable event
    propagation. This requires extra handling of the wheel grabbing we do for
    high-precision wheel event streams.

    Test that this doesn't trigger infinite recursion, while still resulting in
    event propagation within the embedded widget hierarchy, and back to the
    QGraphicsView if the event is not accepted.

    See tst_QApplication::wheelEventPropagation for a similar test.
*/
void tst_QGraphicsProxyWidget::wheelEventPropagation()
{
    QGraphicsScene scene(0, 0, 600, 600);

    auto *label = new QLabel("Direct"_L1);
    label->setFixedSize(300, 30);
    QGraphicsProxyWidget *labelProxy = scene.addWidget(label);
    labelProxy->setPos(0, 50);
    labelProxy->show();

    class NestedWidget : public QWidget
    {
    public:
        NestedWidget(const QString &text)
        {
            setObjectName("Nested Label");
            QLabel *nested = new QLabel(text);
            auto *hbox = new QHBoxLayout(this);
            hbox->addWidget(nested);
        }

        int wheelEventCount = 0;
    protected:
        void wheelEvent(QWheelEvent *) override
        {
            ++wheelEventCount;
        }
    };
    auto *nestedWidget = new NestedWidget("Nested"_L1);
    nestedWidget->setFixedSize(300, 60);
    QGraphicsProxyWidget *nestedProxy = scene.addWidget(nestedWidget);
    nestedProxy->setPos(0, 120);
    nestedProxy->show();

    QGraphicsView view(&scene);
    view.setFixedHeight(200);
    view.show();

    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(view.verticalScrollBar()->isVisible());

    view.verticalScrollBar()->setValue(0);
    QSignalSpy scrollSpy(view.verticalScrollBar(), &QScrollBar::valueChanged);

    const QPoint wheelPosition(50, 25);
    auto wheelUp = [&view, wheelPosition](Qt::ScrollPhase phase) {
        const QPoint global = view.mapToGlobal(wheelPosition);
        const QPoint pixelDelta(0, -25);
        const QPoint angleDelta(0, -120);
        QWindowSystemInterface::handleWheelEvent(view.windowHandle(), wheelPosition, global,
                                                pixelDelta, angleDelta, Qt::NoModifier,
                                                phase);
        QCoreApplication::processEvents();
    };

    qsizetype scrollCount = 0;
    // test non-kinetic events; they are not grabbed, and should scroll the view unless
    // accepted by the embedded widget
    QCOMPARE(view.itemAt(wheelPosition), nullptr);
    wheelUp(Qt::NoScrollPhase);
    QCOMPARE(scrollSpy.size(), ++scrollCount);

    // wheeling on the label, which ignores the event, should scroll the view
    QCOMPARE(view.itemAt(wheelPosition), labelProxy);
    wheelUp(Qt::NoScrollPhase);
    QCOMPARE(scrollSpy.size(), ++scrollCount);
    QCOMPARE(view.itemAt(wheelPosition), labelProxy);
    wheelUp(Qt::NoScrollPhase);
    QCOMPARE(scrollSpy.size(), ++scrollCount);

    // left the widget
    QCOMPARE(view.itemAt(wheelPosition), nullptr);
    wheelUp(Qt::NoScrollPhase);
    QCOMPARE(scrollSpy.size(), ++scrollCount);

    // reached the nested widget, which accepts the wheel event, so no more scrolling
    QCOMPARE(view.itemAt(wheelPosition), nestedProxy);
    // remember this position for later
    const int scrollBarValueOnNestedProxy = view.verticalScrollBar()->value();
    wheelUp(Qt::NoScrollPhase);
    QCOMPARE(scrollSpy.size(), scrollCount);
    QCOMPARE(nestedWidget->wheelEventCount, 1);

    // reset, try with kinetic events
    view.verticalScrollBar()->setValue(0);
    ++scrollCount;

    // starting a scroll outside any widget and scrolling through the widgets should work,
    // no matter if the widget accepts wheel events - the view has the grab
    QCOMPARE(view.itemAt(wheelPosition), nullptr);
    wheelUp(Qt::ScrollBegin);
    QCOMPARE(scrollSpy.size(), ++scrollCount);
    for (int i = 0; i < 5; ++i) {
        wheelUp(Qt::ScrollUpdate);
        QCOMPARE(scrollSpy.size(), ++scrollCount);
    }
    wheelUp(Qt::ScrollEnd);
    QCOMPARE(scrollSpy.size(), ++scrollCount);

    // reset
    view.verticalScrollBar()->setValue(0);

    // starting a scroll on a widget that doesn't accept wheel events
    // should also scroll the view, which still gets the grab
    wheelUp(Qt::NoScrollPhase);
    scrollCount = scrollSpy.size();

    QCOMPARE(view.itemAt(wheelPosition), labelProxy);
    wheelUp(Qt::ScrollBegin);
    QCOMPARE(scrollSpy.size(), ++scrollCount);
    for (int i = 0; i < 5; ++i) {
        wheelUp(Qt::ScrollUpdate);
        QCOMPARE(scrollSpy.size(), ++scrollCount);
    }
    wheelUp(Qt::ScrollEnd);
    QCOMPARE(scrollSpy.size(), ++scrollCount);

    // starting a scroll on a widget that does accept wheel events
    // should not scroll the view
    view.verticalScrollBar()->setValue(scrollBarValueOnNestedProxy);
    scrollCount = scrollSpy.size();

    QCOMPARE(view.itemAt(wheelPosition), nestedProxy);
    wheelUp(Qt::ScrollBegin);
    QCOMPARE(scrollSpy.size(), scrollCount);
}
#endif // QT_CONFIG(wheelevent)

// QTBUG_45737
void tst_QGraphicsProxyWidget::forwardTouchEvent()
{
    QGraphicsScene scene;

    auto *widget = new TouchWidget;
    widget->setAttribute(Qt::WA_AcceptTouchEvents);

    QGraphicsProxyWidget *proxy = scene.addWidget(widget);
    proxy->setAcceptTouchEvents(true);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    EventSpy eventSpy(widget);

    const std::unique_ptr<QPointingDevice> device{QTest::createTouchDevice()};

    QVERIFY(device);
    QCOMPARE(eventSpy.counts[QEvent::TouchBegin], 0);
    QCOMPARE(eventSpy.counts[QEvent::TouchUpdate], 0);
    QCOMPARE(eventSpy.counts[QEvent::TouchEnd], 0);

    QTest::touchEvent(&view, device.get()).press(0, QPoint(10, 10), &view);
    QTest::touchEvent(&view, device.get()).move(0, QPoint(15, 15), &view);
    QTest::touchEvent(&view, device.get()).move(0, QPoint(16, 16), &view);
    QTest::touchEvent(&view, device.get()).release(0, QPoint(15, 15), &view);

    QApplication::processEvents();

    QCOMPARE(eventSpy.counts[QEvent::TouchBegin], 1);
    QCOMPARE(eventSpy.counts[QEvent::TouchUpdate], 2);
    QCOMPARE(eventSpy.counts[QEvent::TouchEnd], 1);
}

void tst_QGraphicsProxyWidget::touchEventPropagation()
{
    QGraphicsScene scene(0, 0, 300, 200);
    auto *simpleWidget = new QWidget;
    simpleWidget->setObjectName("simpleWidget");
    simpleWidget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    QGraphicsProxyWidget *simpleProxy = scene.addWidget(simpleWidget);
    simpleProxy->setAcceptTouchEvents(true);
    simpleProxy->setGeometry(QRectF(0, 0, 30, 30));

    auto *formWidget = new QWidget;
    formWidget->setObjectName("formWidget");
    formWidget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    auto *pushButton1 = new QPushButton("One"_L1);
    pushButton1->setObjectName("pushButton1");
    pushButton1->setAttribute(Qt::WA_AcceptTouchEvents, true);
    auto *pushButton2 = new QPushButton("Two"_L1);
    pushButton2->setObjectName("pushButton2");
    pushButton2->setAttribute(Qt::WA_AcceptTouchEvents, true);
    auto *touchWidget1 = new TouchWidget;
    touchWidget1->setObjectName("touchWidget1");
    touchWidget1->setAttribute(Qt::WA_AcceptTouchEvents, true);
    touchWidget1->setFixedSize(pushButton1->sizeHint());
    auto *touchWidget2 = new TouchWidget;
    touchWidget2->setObjectName("touchWidget2");
    touchWidget2->setAttribute(Qt::WA_AcceptTouchEvents, true);
    touchWidget2->setFixedSize(pushButton2->sizeHint());
    auto *vbox = new QVBoxLayout(formWidget);
    vbox->addWidget(pushButton1);
    vbox->addWidget(pushButton2);
    vbox->addWidget(touchWidget1);
    vbox->addWidget(touchWidget2);
    QGraphicsProxyWidget *formProxy = scene.addWidget(formWidget);
    formProxy->setAcceptTouchEvents(true);
    formProxy->setGeometry(QRectF(50, 50, 200, 160));

    QGraphicsView view(&scene);
    view.setFixedSize(scene.width(), scene.height());
    view.verticalScrollBar()->setValue(0);
    view.horizontalScrollBar()->setValue(0);
    view.viewport()->setObjectName("GraphicsView's Viewport");
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    class TouchEventSpy : public QObject
    {
    public:
        using QObject::QObject;

        struct TouchRecord {
            QObject *receiver;
            QEvent::Type eventType;
            QPointF position;
        };
        QHash<int, QList<TouchRecord>> records;
        QWidget *mousePressReceiver = nullptr;

        int count(int id = 0) const { return records.value(id).size(); }
        TouchRecord at(int i, int id = 0) const { return records.value(id).at(i); }
        void clear()
        {
            records.clear();
            mousePressReceiver = nullptr;
        }
    protected:
        bool eventFilter(QObject *receiver, QEvent *event) override
        {
            switch (event->type()) {
            case QEvent::TouchBegin:
            case QEvent::TouchUpdate:
            case QEvent::TouchCancel:
            case QEvent::TouchEnd: {
                auto *touchEvent = static_cast<QTouchEvent *>(event);
                // instead of detaching each QEventPoint, just store the relative positions
                for (const auto &touchPoint : touchEvent->points())
                    records[touchPoint.id()] << TouchRecord{receiver, event->type(), touchPoint.position()};
                qCDebug(lcTests) << "Recording" << event << receiver;
                break;
            }
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
                mousePressReceiver = qobject_cast<QWidget*>(receiver);
                break;
            default:
                break;
            }
            return QObject::eventFilter(receiver, event);
        }
    } eventSpy;
    qApp->installEventFilter(&eventSpy);

    const std::unique_ptr<QPointingDevice> touchDevice{QTest::createTouchDevice()};
    const QPointF simpleCenter = simpleProxy->geometry().center();

    // On systems without double conversion we might get different rounding behavior.
    // One pixel off in any direction is acceptable for this test.
    constexpr auto closeEnough = [](QPointF exp, QPointF act) -> bool {
        const QRectF expArea(exp - QPointF(1., 1.), exp + QPointF(1., 1.));
        const bool contains = expArea.contains(act);
        if (!contains)
            qWarning() << act << "not in" << exp;
        return contains;
    };

    // verify that the embedded widget gets the correctly translated event
    QTest::touchEvent(&view, touchDevice.get()).press(0, simpleCenter.toPoint());
    // window, viewport, scene, simpleProxy, simpleWidget
    QCOMPARE(eventSpy.count(), 5);
    QCOMPARE(eventSpy.at(0).receiver, view.windowHandle());
    QCOMPARE(eventSpy.at(1).receiver, view.viewport());
    QCOMPARE(eventSpy.at(2).receiver, &scene);
    QCOMPARE(eventSpy.at(3).receiver, simpleProxy);
    auto record = eventSpy.at(4);
    QCOMPARE(record.receiver, simpleWidget);
    QCOMPARE(record.eventType, QEvent::TouchBegin);
    QVERIFY(closeEnough(record.position, simpleCenter));
    eventSpy.clear();

    // verify that the layout of formWidget is how we expect it to be
    QCOMPARE(formWidget->childAt(QPoint(5, 5)), nullptr);
    const QPoint pb1Center = pushButton1->rect().center();
    QCOMPARE(formWidget->childAt(pushButton1->pos() + pb1Center), pushButton1);
    const QPoint pb2Center = pushButton2->rect().center();
    QCOMPARE(formWidget->childAt(pushButton2->pos() + pb2Center), pushButton2);
    const QPoint tw1Center = touchWidget1->rect().center();
    QCOMPARE(formWidget->childAt(touchWidget1->pos() + tw1Center), touchWidget1);
    const QPoint tw2Center = touchWidget2->rect().center();
    QCOMPARE(formWidget->childAt(touchWidget2->pos() + tw2Center), touchWidget2);

    // touch events are sent to the view, in view coordinates
    const QPoint formProxyPox = view.mapFromScene(formProxy->pos().toPoint());
    const QPoint pb1TouchPos = pushButton1->pos() + pb1Center + formProxyPox;
    const QPoint pb2TouchPos = pushButton2->pos() + pb2Center + formProxyPox;
    const QPoint tw1TouchPos = touchWidget1->pos() + tw1Center + formProxyPox;
    const QPoint tw2TouchPos = touchWidget2->pos() + tw2Center + formProxyPox;

    QSignalSpy clickedSpy(pushButton1, &QPushButton::clicked);
    // Single touch point to nested widget not accepting event.
    // Event should bubble up and translate correctly, TouchUpdate and TouchEnd events
    // stop at the window since nobody accepted the TouchBegin. A mouse event will be generated.
    QTest::touchEvent(&view, touchDevice.get()).press(0, pb1TouchPos);
    QTest::touchEvent(&view, touchDevice.get()).move(0, pb1TouchPos + QPoint(1, 1));
    QTest::touchEvent(&view, touchDevice.get()).release(0, pb1TouchPos + QPoint(1, 1));
    // ..., formProxy, pushButton1, formWidget, window, window
    QCOMPARE(eventSpy.count(), 8);
    QCOMPARE(eventSpy.at(3).receiver, formProxy); // formProxy dispatches to the right subwidget
    record = eventSpy.at(4);
    QCOMPARE(record.receiver, pushButton1);
    QVERIFY(closeEnough(record.position, pb1Center));
    QCOMPARE(record.eventType, QEvent::TouchBegin);
    // pushButton doesn't accept the point, so the TouchBegin propagates to parent
    record = eventSpy.at(5);
    QCOMPARE(record.receiver, formWidget);
    QVERIFY(closeEnough(record.position, pushButton1->pos() + pb1Center));
    QCOMPARE(record.eventType, QEvent::TouchBegin);
    record = eventSpy.at(6);
    QCOMPARE(record.receiver, view.windowHandle());
    QCOMPARE(record.eventType, QEvent::TouchUpdate);
    record = eventSpy.at(7);
    QCOMPARE(record.receiver, view.windowHandle());
    QCOMPARE(record.eventType, QEvent::TouchEnd);
    QCOMPARE(eventSpy.mousePressReceiver, pushButton1);
    QCOMPARE(clickedSpy.size(), 1);
    eventSpy.clear();
    clickedSpy.clear();

    // Single touch point to nested widget accepting event.
    QTest::touchEvent(&view, touchDevice.get()).press(0, tw1TouchPos);
    QTest::touchEvent(&view, touchDevice.get()).move(0, tw1TouchPos + QPoint(5, 5));
    QTest::touchEvent(&view, touchDevice.get()).release(0, tw1TouchPos + QPoint(5, 5));
    // Press: ..., formProxy, touchWidget1 (5)
    // Move: window, touchWidget1 (2)
    // Release: window, touchWidget1 (2)
    QCOMPARE(eventSpy.count(), 9);
    QCOMPARE(eventSpy.at(3).receiver, formProxy); // form proxy dispatches TouchBegin to the right widget
    record = eventSpy.at(4);
    QCOMPARE(record.receiver, touchWidget1);
    QVERIFY(closeEnough(record.position, tw1Center));
    QCOMPARE(record.eventType, QEvent::TouchBegin);
    QCOMPARE(eventSpy.at(5).receiver, view.windowHandle()); // QWidgetWindow dispatches TouchUpdate
    record = eventSpy.at(6);
    QCOMPARE(record.receiver, touchWidget1);
    QVERIFY(closeEnough(record.position, tw1Center + QPoint(5, 5)));
    QCOMPARE(record.eventType, QEvent::TouchUpdate);
    QCOMPARE(eventSpy.at(7).receiver, view.windowHandle()); // QWidgetWindow dispatches TouchEnd
    record = eventSpy.at(8);
    QCOMPARE(record.receiver, touchWidget1);
    QVERIFY(closeEnough(record.position, tw1Center + QPoint(5, 5)));
    QCOMPARE(record.eventType, QEvent::TouchEnd);
    eventSpy.clear();

    // to simplify the remaining test, install the event spy explicitly on the target widgets
    qApp->removeEventFilter(&eventSpy);
    formWidget->installEventFilter(&eventSpy);
    pushButton1->installEventFilter(&eventSpy);
    pushButton2->installEventFilter(&eventSpy);
    touchWidget1->installEventFilter(&eventSpy);
    touchWidget2->installEventFilter(&eventSpy);

    // multi-touch to different widgets, some do and some don't accept the event
    QTest::touchEvent(&view, touchDevice.get())
        .press(0, pb1TouchPos)
        .press(1, tw1TouchPos)
        .press(2, pb2TouchPos)
        .press(3, tw2TouchPos);
    QTest::touchEvent(&view, touchDevice.get())
        .move(0, pb1TouchPos + QPoint(1, 1))
        .move(1, tw1TouchPos + QPoint(1, 1))
        .move(2, pb2TouchPos + QPoint(1, 1))
        .move(3, tw2TouchPos + QPoint(1, 1));
    QTest::touchEvent(&view, touchDevice.get())
        .release(0, pb1TouchPos + QPoint(1, 1))
        .release(1, tw1TouchPos + QPoint(1, 1))
        .release(2, pb2TouchPos + QPoint(1, 1))
        .release(3, tw2TouchPos + QPoint(1, 1));

    QCOMPARE(eventSpy.count(0), 2); // Begin never accepted, so move up and then stop
    QCOMPARE(eventSpy.count(1), 3); // Begin accepted, so not propagated and update/end received
    QCOMPARE(eventSpy.count(2), 2); // Begin never accepted
    QCOMPARE(eventSpy.count(3), 3); // Begin accepted
    QCOMPARE(eventSpy.at(0, 0).receiver, pushButton1);
    QCOMPARE(eventSpy.at(1, 0).receiver, formWidget);
    QCOMPARE(eventSpy.at(0, 1).receiver, touchWidget1);
    QCOMPARE(eventSpy.at(1, 1).receiver, touchWidget1);
    QCOMPARE(eventSpy.at(2, 1).receiver, touchWidget1);
    QCOMPARE(eventSpy.at(0, 2).receiver, pushButton2);
    QCOMPARE(eventSpy.at(1, 2).receiver, formWidget);
    QCOMPARE(eventSpy.at(0, 3).receiver, touchWidget2);
    QCOMPARE(eventSpy.at(1, 3).receiver, touchWidget2);
    QCOMPARE(eventSpy.at(2, 3).receiver, touchWidget2);
    QCOMPARE(clickedSpy.size(), 0); // multi-touch event does not synthesize a mouse event
}

QTEST_MAIN(tst_QGraphicsProxyWidget)
#include "tst_qgraphicsproxywidget.moc"
