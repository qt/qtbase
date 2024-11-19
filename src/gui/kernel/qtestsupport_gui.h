// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTSUPPORT_GUI_H
#define QTESTSUPPORT_GUI_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qevent.h>
#include <QtCore/qmap.h>

#if QT_CONFIG(test_gui)
#include <QtCore/qloggingcategory.h>
#endif // #if QT_CONFIG(test_gui)

QT_BEGIN_NAMESPACE

class QWindow;

Q_GUI_EXPORT void qt_handleTouchEvent(QWindow *w, const QPointingDevice *device,
                                const QList<QEventPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier);

Q_GUI_EXPORT bool qt_handleTouchEventv2(QWindow *w, const QPointingDevice *device,
                                const QList<QEventPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier);

namespace QTest {

[[nodiscard]] Q_GUI_EXPORT bool qWaitForWindowActive(QWindow *window, int timeout = 5000);
[[nodiscard]] Q_GUI_EXPORT bool qWaitForWindowFocused(QWindow *window, QDeadlineTimer timeout = std::chrono::seconds{5});
[[nodiscard]] Q_GUI_EXPORT bool qWaitForWindowExposed(QWindow *window, int timeout = 5000);

Q_GUI_EXPORT QPointingDevice * createTouchDevice(QInputDevice::DeviceType devType = QInputDevice::DeviceType::TouchScreen,
                                                 QInputDevice::Capabilities caps = QInputDevice::Capability::Position);

class Q_GUI_EXPORT QTouchEventSequence
{
public:
    virtual ~QTouchEventSequence();
    QTouchEventSequence& press(int touchId, const QPoint &pt, QWindow *window = nullptr);
    QTouchEventSequence& move(int touchId, const QPoint &pt, QWindow *window = nullptr);
    QTouchEventSequence& release(int touchId, const QPoint &pt, QWindow *window = nullptr);
    virtual QTouchEventSequence& stationary(int touchId);

    virtual bool commit(bool processEvents = true);

protected:
    QTouchEventSequence(QWindow *window, QPointingDevice *aDevice, bool autoCommit);

    QPoint mapToScreen(QWindow *window, const QPoint &pt);

    QEventPoint &point(int touchId);

    QEventPoint &pointOrPreviousPoint(int touchId);

    QMap<int, QEventPoint> previousPoints;
    QMap<int, QEventPoint> points;
    QWindow *targetWindow;
    QPointingDevice *device;
    bool commitWhenDestroyed;
    friend QTouchEventSequence touchEvent(QWindow *window, QPointingDevice *device, bool autoCommit);
};

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
Q_DECLARE_LOGGING_CATEGORY(lcQtGuiTest)
namespace QtGuiTest
{
    Q_NAMESPACE_EXPORT(Q_GUI_EXPORT)

    void setKeyboardModifiers(Qt::KeyboardModifiers modifiers);
    void setCursorPosition(const QPoint &position);
    void synthesizeExtendedKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                    const QString &text);
    bool synthesizeKeyEvent(QWindow *window, QEvent::Type t, int k, Qt::KeyboardModifiers mods,
                            const QString & text = QString(), bool autorep = false,
                            ushort count = 1);

    void synthesizeMouseEvent(const QPointF &position, Qt::MouseButtons state,
                              Qt::MouseButton button, QEvent::Type type,
                              Qt::KeyboardModifiers modifiers);

    void synthesizeWheelEvent(int rollCount, Qt::KeyboardModifiers modifiers);

    qint64 eventTimeElapsed();

    void postFakeWindowActivation(QWindow *window);

    QPoint toNativePixels(const QPoint &value, const QWindow *window);
    QRect toNativePixels(const QRect &value, const QWindow *window);
    qreal factor(const QWindow *window);

    void setEventPointId(QEventPoint &p, int arg);
    void setEventPointPressure(QEventPoint &p, qreal arg);
    void setEventPointState(QEventPoint &p, QEventPoint::State arg);
    void setEventPointPosition(QEventPoint &p, QPointF arg);
    void setEventPointGlobalPosition(QEventPoint &p, QPointF arg);
    void setEventPointScenePosition(QEventPoint &p, QPointF arg);
    void setEventPointEllipseDiameters(QEventPoint &p, QSizeF arg);
} // namespace QtGuiTest

#endif // #if QT_CONFIG(test_gui)

QT_END_NAMESPACE

#endif // #ifndef QTESTSUPPORT_GUI_H
