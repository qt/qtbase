// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qguiapplication_p.h>
#include <private/qeventpoint_p.h>

#include <qpa/qplatformintegration.h>

#include "qtestsupport_gui.h"
#include "qwindow.h"

#include <QtCore/qtestsupport_core.h>
#include <QtCore/qthread.h>
#include <QtCore/QDebug>

#if QT_CONFIG(test_gui)
#include <QtCore/qloggingcategory.h>
#include <private/qinputdevicemanager_p.h>
#include <private/qeventpoint_p.h>
#include <private/qhighdpiscaling_p.h>
#endif // #if QT_CONFIG(test_gui)

QT_BEGIN_NAMESPACE

/*!
    \since 5.0
    \overload

    The \a timeout is in milliseconds.
*/
bool QTest::qWaitForWindowActive(QWindow *window, int timeout)
{
    return qWaitForWindowActive(window, QDeadlineTimer{timeout, Qt::TimerType::PreciseTimer});
}

/*!
    \since 6.10

    Returns \c true, if \a window is active within \a timeout. Otherwise returns \c false.

    The method is useful in tests that call QWindow::show() and rely on the window actually being
    active (i.e. being visible and having focus) before proceeding.

    \note  The method will time out and return \c false if another window prevents \a window from
    becoming active.

    \note Since focus is an exclusive property, \a window may loose its focus to another window at
    any time - even after the method has returned \c true.

    \sa qWaitForWindowExposed(), qWaitForWindowFocused(), QWindow::isActive()
*/
bool QTest::qWaitForWindowActive(QWindow *window, QDeadlineTimer timeout)
{
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))) {
        qWarning() << "qWaitForWindowActive was called on a platform that doesn't support window"
                   << "activation. This means there is an error in the test and it should either"
                   << "check for the WindowActivation platform capability before calling"
                   << "qWaitForWindowActivate, use qWaitForWindowExposed instead, or skip the test."
                   << "Falling back to qWaitForWindowExposed.";
        return qWaitForWindowExposed(window, timeout);
    }
    return QTest::qWaitFor([wp = QPointer(window)]() {
        if (QWindow *w = wp.data(); !w)
            return false;
        else
            return w->isActive();
    }, timeout);
}

/*!
    \since 6.10
    \overload

    This function uses the default timeout of 5 seconds.
*/
bool QTest::qWaitForWindowActive(QWindow *window)
{
    return qWaitForWindowActive(window, Internal::defaultTryTimeout);
}

/*!
    \since 6.7

    Returns \c true, if \a window is the focus window within \a timeout. Otherwise returns \c false.

    The method is useful in tests that call QWindow::show() and rely on the window
    having focus (for receiving keyboard events e.g.) before proceeding.

    \note  The method will time out and return \c false if another window prevents \a window from
    becoming focused.

    \note Since focus is an exclusive property, \a window may loose its focus to another window at
    any time - even after the method has returned \c true.

    \sa qWaitForWindowExposed(), qWaitForWindowActive(), QGuiApplication::focusWindow()
*/
Q_GUI_EXPORT bool QTest::qWaitForWindowFocused(QWindow *window, QDeadlineTimer timeout)
{
    return QTest::qWaitFor([wp = QPointer(window)]() {
        if (QWindow *w = wp.data(); !w)
            return false;
        else
            return qGuiApp->focusWindow() == w;
    }, timeout);
}

/*!
    \since 6.10
    \overload

    This function uses the default timeout of 5 seconds.
*/
bool QTest::qWaitForWindowFocused(QWindow *window)
{
    return qWaitForWindowFocused(window, Internal::defaultTryTimeout);
}

/*!
    \since 5.0
    \overload

    The \a timeout is in milliseconds.
*/
bool QTest::qWaitForWindowExposed(QWindow *window, int timeout)
{
    return qWaitForWindowExposed(window, std::chrono::milliseconds(timeout));
}

/*!
    \since 6.10

    Returns \c true, if \a window is exposed within \a timeout. Otherwise returns \c false.

    The method is useful in tests that call QWindow::show() and rely on the window actually being
    being visible before proceeding.

    \note A window mapped to screen may still not be considered exposed, if the window client area is
    not visible, e.g. because it is completely covered by other windows.
    In such cases, the method will time out and return \c false.

    \sa qWaitForWindowActive(), QWindow::isExposed()
*/
bool QTest::qWaitForWindowExposed(QWindow *window, QDeadlineTimer timeout)
{
    return QTest::qWaitFor([wp = QPointer(window)]() {
        if (QWindow *w = wp.data(); !w)
            return false;
        else
            return w->isExposed();
    }, timeout);
}

/*!
    \since 6.10
    \overload

    This function uses the default timeout of 5 seconds.
*/
bool QTest::qWaitForWindowExposed(QWindow *window)
{
    return qWaitForWindowExposed(window, Internal::defaultTryTimeout);
}

namespace QTest {

QTouchEventSequence::~QTouchEventSequence()
{
    if (commitWhenDestroyed)
        QTouchEventSequence::commit();
}
QTouchEventSequence& QTouchEventSequence::press(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(window, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Pressed);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::move(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(window, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Updated);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::release(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(window, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Released);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::stationary(int touchId)
{
    auto &p = pointOrPreviousPoint(touchId);
    QMutableEventPoint::setState(p, QEventPoint::State::Stationary);
    return *this;
}

bool QTouchEventSequence::commit(bool processEvents)
{
    if (points.isEmpty())
        return false;
    QThread::sleep(std::chrono::milliseconds{1});
    bool ret = false;
    if (targetWindow)
        ret = qt_handleTouchEventv2(targetWindow, device, points.values());
    if (processEvents)
        QCoreApplication::processEvents();
    previousPoints = points;
    points.clear();
    return ret;
}

QTouchEventSequence::QTouchEventSequence(QWindow *window, QPointingDevice *aDevice, bool autoCommit)
    : targetWindow(window), device(aDevice), commitWhenDestroyed(autoCommit)
{
}

QPoint QTouchEventSequence::mapToScreen(QWindow *window, const QPoint &pt)
{
    if (window)
        return window->mapToGlobal(pt);
    return targetWindow ? targetWindow->mapToGlobal(pt) : pt;
}

QEventPoint &QTouchEventSequence::point(int touchId)
{
    if (!points.contains(touchId))
        points[touchId] = QEventPoint(touchId);
    return points[touchId];
}

QEventPoint &QTouchEventSequence::pointOrPreviousPoint(int touchId)
{
    if (!points.contains(touchId)) {
        if (previousPoints.contains(touchId))
            points[touchId] = previousPoints.value(touchId);
        else
            points[touchId] = QEventPoint(touchId);
    }
    return points[touchId];
}

} // namespace QTest

//
//  W A R N I N G
//  -------------
//
// The QtGuiTest namespace is not part of the Qt API.  It exists purely as an
// implementation detail.  It may change from version to version without notice,
// or even be removed.
//
// We mean it.
//
#if QT_CONFIG(test_gui)
Q_STATIC_LOGGING_CATEGORY(lcQtGuiTest, "qt.gui.test");
#define deb qCDebug(lcQtGuiTest)

/*!
   \internal
   \return the application's input device manager.
   \return nullptr and log error, if the application hasn't been initialized.
 */
static QInputDeviceManager *inputDeviceManager()
{
    if (auto *idm = QGuiApplicationPrivate::inputDeviceManager())
        return idm;

    deb << "No input device manager present.";
    return nullptr;
}

/*!
   \internal
   Synthesize keyboard modifier action by passing \a modifiers
   to the application's input device manager.
 */
void QtGuiTest::setKeyboardModifiers(Qt::KeyboardModifiers modifiers)
{
    auto *idm = inputDeviceManager();
    if (Q_UNLIKELY(!idm))
        return;

    idm->setKeyboardModifiers(modifiers);
    deb << "Keyboard modifiers synthesized:" << modifiers;
}

/*!
   \internal
   Synthesize user-initiated mouse positioning by passing \a position
   to the application's input device manager.
 */
void QtGuiTest::setCursorPosition(const QPoint &position)
{
    auto *idm = inputDeviceManager();
    if (Q_UNLIKELY(!idm))
        return;

    idm->setCursorPos(position);
    deb << "Mouse curser set to" << position;
}

/*!
   \internal
   Synthesize an extended \a key event of \a type, with \a modifiers, \a nativeScanCode,
   \a nativeVirtualKey and \a text on application level.
   Log whether the synthesizing has been successful.

   \note
   The application is expected to propagate the extended key event to its focus window,
   if one exists.
 */
void QtGuiTest::synthesizeExtendedKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                                  quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                  const QString &text)
{
    Q_ASSERT_X((type == QEvent::KeyPress
                || type == QEvent::KeyRelease),
               Q_FUNC_INFO,
               "called with invalid QEvent type");

    deb << "Synthesizing key event:" << type << Qt::Key(key) << modifiers << text;

    if (QWindowSystemInterface::handleExtendedKeyEvent(nullptr, type, key, modifiers,
                                                       nativeScanCode, nativeVirtualKey,
                                                       modifiers, text, /* autorep = */ false,
                                                       /* count = */ 0)) {

        // If the key event is a shortcut, it may cause other events to be posted.
        // => process those.
        QCoreApplication::sendPostedEvents();
        deb << "(success)";
    } else {
        deb << "(failure)";
    }
}

/*!
   \internal
   Synthesize a key event \a k of type \a t, with modifiers \a mods, \a text,
   \a autorep and \a count on application level.
   Log whether the synthesizing has been successful.

   \note
   The application is expected to propagate the key event to its focus window,
   if one exists.
 */
bool QtGuiTest::synthesizeKeyEvent(QWindow *window, QEvent::Type t, int k, Qt::KeyboardModifiers mods,
                                          const QString & text, bool autorep,
                                          ushort count)
{
    Q_ASSERT_X((t == QEvent::KeyPress
                || t == QEvent::KeyRelease),
               Q_FUNC_INFO,
               "called with invalid QEvent type");

    deb << "Synthesizing key event:" << t << Qt::Key(k) << mods << text;

    bool result = QWindowSystemInterface::handleKeyEvent(window, t, k, mods, text, autorep, count);
    if (result) {
        // If the key event is a shortcut, it may cause other events to be posted.
        // => process those.
        QCoreApplication::sendPostedEvents();
        deb << "(success)";
    } else {
        deb << "(failure)";
    }

    return result;
}

/*!
   \internal
   Synthesize a mouse event of \a type, with \a button at \a position at application level.
   Respect \a state and \a modifiers.

   The application is expected to
   \list
   \li propagate the mouse event to its focus window,
       if one exists.
   \li convert a click/release squence into a double click.
   \endlist

   \note
   QEvent::MouseButtonDoubleClick can't be explicitly synthesized.
 */
void QtGuiTest::synthesizeMouseEvent(const QPointF &position, Qt::MouseButtons state,
                                        Qt::MouseButton button, QEvent::Type type,
                                        Qt::KeyboardModifiers modifiers)
{
    Q_ASSERT_X((type == QEvent::MouseButtonPress
                || type == QEvent::MouseButtonRelease
                || type == QEvent::MouseMove),
               Q_FUNC_INFO,
               "called with invalid QEvent type");

    deb << "Synthesizing mouse event:" << type << position << button << modifiers;

    if (QWindowSystemInterface::handleMouseEvent(nullptr, position, position, state, button,
                                                 type, modifiers, Qt::MouseEventNotSynthesized)) {
        // If the mouse event reacts to a shortcut, it may cause other events to be posted.
        // => process those.
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents();

        deb << "(success)";
    } else {
        deb << "(failure)";
    }
}

/*!
   \internal
   Synthesize a wheel event with \a modifiers and \a rollCount representing the number of
   roll unit on application level.

   \note
   The application is expected to handle the wheel event, or propagate it
   to its focus window, if one exists.
 */
void QtGuiTest::synthesizeWheelEvent(int rollCount, Qt::KeyboardModifiers modifiers)
{
    deb << "Synthesizing wheel event:" << rollCount << modifiers;

    QPoint position = QCursor::pos();
    if (QWindowSystemInterface::handleWheelEvent(nullptr, position, position,
                                                 QPoint(), QPoint(0, -rollCount), modifiers)) {

        // It's unlikely that a shortcut relates to a subsequent wheel event.
        // But it's not harmful, to send posted events here.
        QCoreApplication::sendPostedEvents();
        deb << "(success)";
    } else {
        deb << "(failure)";
    }
}

/*!
   \internal
   \return the number of milliseconds since the QElapsedTimer
   eventTime was last started.
*/
qint64 QtGuiTest::eventTimeElapsed()
{
    return QWindowSystemInterfacePrivate::eventTime.elapsed();
}

/*!
   \internal
   Post fake window activation with \a window representing the
   fake window being activated.
*/
void QtGuiTest::postFakeWindowActivation(QWindow *window)
{
    Q_ASSERT_X(window,
               Q_FUNC_INFO,
               "called with nullptr");

    deb << "Posting fake window activation:" << window;

    QWindowSystemInterfacePrivate::FocusWindowEvent e(window, Qt::OtherFocusReason);
    QGuiApplicationPrivate::processWindowSystemEvent(&e);
    QWindowSystemInterface::handleFocusWindowChanged(window);
}

/*!
   \internal
   \return native \a window position from \a value.
*/
QPoint QtGuiTest::toNativePixels(const QPoint &value, const QWindow *window)
{
    Q_ASSERT_X(window,
               Q_FUNC_INFO,
               "called with nullptr");

    deb << "Calculating native pixels: " << value << window;
    return QHighDpi::toNativePixels<QPoint, QWindow>(value, window);
}

/*!
   \internal
   \return native \a window rectangle from \a value.
*/
QRect QtGuiTest::toNativePixels(const QRect &value, const QWindow *window)
{
    Q_ASSERT_X(window,
               Q_FUNC_INFO,
               "called with nullptr");

    deb << "Calculating native pixels: " << value << window;
    return QHighDpi::toNativePixels<QRect, QWindow>(value, window);
}

/*!
   \internal
   \return scaling factor of \a window relative to Qt.
*/
qreal QtGuiTest::scaleFactor(const QWindow *window)
{
    Q_ASSERT_X(window,
               Q_FUNC_INFO,
               "called with nullptr");

    deb << "Calculating scaling factor: " << window;
    return QHighDpiScaling::factor(window);
}

/*!
   \internal
   Set the id of \a p to \a arg.
*/
void QtGuiTest::setEventPointId(QEventPoint &p, int arg)
{
    QMutableEventPoint::setId(p, arg);
}

/*!
   \internal
   Set the pressure of \a p to \a arg.
*/
void QtGuiTest::setEventPointPressure(QEventPoint &p, qreal arg)
{
    QMutableEventPoint::setPressure(p, arg);
}

/*!
   \internal
   Set the state of \a p to \a arg.
*/
void QtGuiTest::setEventPointState(QEventPoint &p, QEventPoint::State arg)
{
    QMutableEventPoint::setState(p, arg);
}

/*!
   \internal
   Set the position of \a p to \a arg.
*/
void QtGuiTest::setEventPointPosition(QEventPoint &p, QPointF arg)
{
    QMutableEventPoint::setPosition(p, arg);
}

/*!
   \internal
   Set the global position of \a p to \a arg.
*/
void QtGuiTest::setEventPointGlobalPosition(QEventPoint &p, QPointF arg)
{
    QMutableEventPoint::setGlobalPosition(p, arg);
}

/*!
   \internal
   Set the scene position of \a p to \a arg.
*/
void QtGuiTest::setEventPointScenePosition(QEventPoint &p, QPointF arg)
{
    QMutableEventPoint::setScenePosition(p, arg);
}

/*!
   \internal
   Set the ellipse diameters of \a p to \a arg.
*/
void QtGuiTest::setEventPointEllipseDiameters(QEventPoint &p, QSizeF arg)
{
    QMutableEventPoint::setEllipseDiameters(p, arg);
}

#undef deb
#endif // #if QT_CONFIG(test_gui)
QT_END_NAMESPACE
