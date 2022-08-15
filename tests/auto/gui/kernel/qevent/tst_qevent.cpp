// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>
#include <QtCore/private/qfutureinterface_p.h>

#define FOR_EACH_CORE_EVENT(X) \
    /* qcoreevent.h */ \
    X(QEvent, (QEvent::None)) \
    X(QTimerEvent, (42)) \
    X(QChildEvent, (QEvent::ChildAdded, nullptr)) \
    X(QDynamicPropertyChangeEvent, ("size")) \
    X(QDeferredDeleteEvent, ()) \
    /* qfutureinterface_p.h */ \
    X(QFutureCallOutEvent, ()) \
    /* end */

#define FOR_EACH_GUI_EVENT(X) \
    /* qevent.h */ \
    X(QInputEvent, (QEvent::None, nullptr)) \
    X(QPointerEvent, (QEvent::None, nullptr)) \
    /* doesn't work with nullptr: */ \
    X(QSinglePointEvent, (QEvent::None, QPointingDevice::primaryPointingDevice(), {}, {}, {}, {}, {}, {})) \
    X(QEnterEvent, ({}, {}, {})) \
    X(QMouseEvent, (QEvent::None, {}, {}, {}, {}, {}, {}, {}, QPointingDevice::primaryPointingDevice())) \
    X(QHoverEvent, (QEvent::None, {}, {}, QPointF{})) \
    X(QWheelEvent, ({}, {}, {}, {}, {}, {}, {}, {})) \
    X(QTabletEvent, (QEvent::None, QPointingDevice::primaryPointingDevice(), {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})) \
    X(QNativeGestureEvent, ({}, QPointingDevice::primaryPointingDevice(), 0, {}, {}, {}, {}, {})) \
    X(QKeyEvent, (QEvent::None, 0, {})) \
    X(QFocusEvent, (QEvent::None)) \
    X(QPaintEvent, (QRect{0, 0, 100, 100})) \
    X(QMoveEvent, ({}, {})) \
    X(QExposeEvent, ({})) \
    X(QPlatformSurfaceEvent, ({})) \
    X(QResizeEvent, ({}, {})) \
    X(QCloseEvent, ()) \
    X(QIconDragEvent, ()) \
    X(QShowEvent, ()) \
    X(QHideEvent, ()) \
    X(QContextMenuEvent, (QContextMenuEvent::Reason::Keyboard, {}, {})) \
    X(QInputMethodEvent, ()) \
    X(QInputMethodQueryEvent, ({})) \
    X(QDropEvent, ({}, {}, {}, {}, {})) \
    X(QDragMoveEvent, ({}, {}, {}, {}, {})) \
    X(QDragEnterEvent, ({}, {}, {}, {}, {})) \
    X(QDragLeaveEvent, ()) \
    X(QHelpEvent, ({}, {}, {})) \
    X(QStatusTipEvent, ({})) \
    X(QWhatsThisClickedEvent, ({})) \
    X(QActionEvent, (0, nullptr)) \
    X(QFileOpenEvent, (QString{})) \
    X(QToolBarChangeEvent, (false)) \
    X(QShortcutEvent, ({}, 0)) \
    X(QWindowStateChangeEvent, ({})) \
    X(QTouchEvent, (QEvent::None)) \
    X(QScrollPrepareEvent, ({})) \
    X(QScrollEvent, ({}, {}, {})) \
    X(QScreenOrientationChangeEvent, (nullptr, {})) \
    X(QApplicationStateChangeEvent, ({})) \
    /* end */

#define FOR_EACH_EVENT(X) \
    FOR_EACH_CORE_EVENT(X) \
    FOR_EACH_GUI_EVENT(X) \
    /* end */

class tst_QEvent : public QObject
{
    Q_OBJECT
public:
    tst_QEvent();
    ~tst_QEvent();

private slots:
    void clone() const;
    void registerEventType_data();
    void registerEventType();
    void exhaustEventTypeRegistration(); // keep behind registerEventType() test

private:
    bool registerEventTypeSucceeded; // track success of registerEventType for use by exhaustEventTypeRegistration()
};

tst_QEvent::tst_QEvent()
    : registerEventTypeSucceeded(true)
{ }

tst_QEvent::~tst_QEvent()
{ }

void tst_QEvent::clone() const
{
#define ACTION(Type, Init) do { \
        const std::unique_ptr<const Type> e(new Type Init); \
        auto c = e->clone(); \
        static_assert(std::is_same_v<decltype(c), Type *>); \
        delete c; \
    } while (0);

    FOR_EACH_EVENT(ACTION)
}

void tst_QEvent::registerEventType_data()
{
    QTest::addColumn<int>("hint");
    QTest::addColumn<int>("expected");

    // default argument
    QTest::newRow("default") << -1 << int(QEvent::MaxUser);
    // hint not valid
    QTest::newRow("User-1") << int(QEvent::User - 1) << int(QEvent::MaxUser - 1);
    // hint not valid II
    QTest::newRow("MaxUser+1") << int(QEvent::MaxUser + 1) << int(QEvent::MaxUser - 2);
    // hint valid, but already taken
    QTest::newRow("MaxUser-1") << int(QEvent::MaxUser - 1) << int(QEvent::MaxUser - 3);
    // hint valid, but not taken
    QTest::newRow("User + 1000") << int(QEvent::User + 1000) << int(QEvent::User + 1000);
}

void tst_QEvent::registerEventType()
{
    const bool oldRegisterEventTypeSucceeded = registerEventTypeSucceeded;
    registerEventTypeSucceeded = false;
    QFETCH(int, hint);
    QFETCH(int, expected);
    QCOMPARE(QEvent::registerEventType(hint), expected);
    registerEventTypeSucceeded = oldRegisterEventTypeSucceeded;
}

void tst_QEvent::exhaustEventTypeRegistration()
{
    if (!registerEventTypeSucceeded)
        QSKIP("requires the previous test (registerEventType) to have finished successfully");

    int i = QEvent::User;
    int result;
    while ((result = QEvent::registerEventType(i)) == i)
        ++i;
    QCOMPARE(i, int(QEvent::User + 1000));
    QCOMPARE(result, int(QEvent::MaxUser - 4));
    i = QEvent::User + 1001;
    while ((result = QEvent::registerEventType(i)) == i)
        ++i;
    QCOMPARE(result, -1);
    QCOMPARE(i, int(QEvent::MaxUser - 4));
}

QTEST_MAIN(tst_QEvent)
#include "tst_qevent.moc"
