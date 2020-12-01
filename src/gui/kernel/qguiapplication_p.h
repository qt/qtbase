/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGUIAPPLICATION_P_H
#define QGUIAPPLICATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qguiapplication.h>

#include <QtCore/QPointF>
#include <QtCore/QSharedPointer>
#include <QtCore/private/qcoreapplication_p.h>

#include <QtCore/private/qthread_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#if QT_CONFIG(shortcut)
#  include "private/qshortcutmap_p.h"
#endif

#include <qicon.h>

QT_BEGIN_NAMESPACE

class QColorTrcLut;
class QPlatformIntegration;
class QPlatformTheme;
class QPlatformDragQtResponse;
#if QT_CONFIG(draganddrop)
class QDrag;
#endif // QT_CONFIG(draganddrop)
class QInputDeviceManager;
#ifndef QT_NO_ACTION
class QActionPrivate;
#endif
#if QT_CONFIG(shortcut)
class QShortcutPrivate;
#endif

class Q_GUI_EXPORT QGuiApplicationPrivate : public QCoreApplicationPrivate
{
    Q_DECLARE_PUBLIC(QGuiApplication)
public:
    QGuiApplicationPrivate(int &argc, char **argv, int flags);
    ~QGuiApplicationPrivate();

    void init();

    void createPlatformIntegration();
    void createEventDispatcher() override;
    void eventDispatcherReady() override;

    virtual void notifyLayoutDirectionChange();
    virtual void notifyActiveWindowChange(QWindow *previous);

#if QT_CONFIG(commandlineparser)
    void addQtOptions(QList<QCommandLineOption> *options) override;
#endif
    virtual bool shouldQuit() override;
    void quit() override;

    bool shouldQuitInternal(const QWindowList &processedWindows);

    static void captureGlobalModifierState(QEvent *e);
    static Qt::KeyboardModifiers modifier_buttons;
    static Qt::MouseButtons mouse_buttons;

    static QPlatformIntegration *platform_integration;

    static QPlatformIntegration *platformIntegration()
    { return platform_integration; }

    static QPlatformTheme *platform_theme;

    static QPlatformTheme *platformTheme()
    { return platform_theme; }

    static QAbstractEventDispatcher *qt_qpa_core_dispatcher()
    {
        if (QCoreApplication::instance())
            return QCoreApplication::instance()->d_func()->threadData.loadRelaxed()->eventDispatcher.loadRelaxed();
        else
            return nullptr;
    }

    static void processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e);
    static void processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e);
    static void processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e);
    static void processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e);

    static void processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e);

    static void processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e);

    static void processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e);
    static void processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e);

    static void processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e);
    static void processWindowStateChangedEvent(QWindowSystemInterfacePrivate::WindowStateChangedEvent *e);
    static void processWindowScreenChangedEvent(QWindowSystemInterfacePrivate::WindowScreenChangedEvent *e);

    static void processSafeAreaMarginsChangedEvent(QWindowSystemInterfacePrivate::SafeAreaMarginsChangedEvent *e);

    static void processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

    static void processApplicationTermination(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

    static void updateFilteredScreenOrientation(QScreen *screen);
    static void processScreenOrientationChange(QWindowSystemInterfacePrivate::ScreenOrientationEvent *e);
    static void processScreenGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e);
    static void processScreenLogicalDotsPerInchChange(QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e);
    static void processScreenRefreshRateChange(QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e);
    static void processThemeChanged(QWindowSystemInterfacePrivate::ThemeChangeEvent *tce);

    static void processExposeEvent(QWindowSystemInterfacePrivate::ExposeEvent *e);
    static void processPaintEvent(QWindowSystemInterfacePrivate::PaintEvent *e);

    static void processFileOpenEvent(QWindowSystemInterfacePrivate::FileOpenEvent *e);

    static void processTabletEvent(QWindowSystemInterfacePrivate::TabletEvent *e);
    static void processTabletEnterProximityEvent(QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e);
    static void processTabletLeaveProximityEvent(QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e);

#ifndef QT_NO_GESTURES
    static void processGestureEvent(QWindowSystemInterfacePrivate::GestureEvent *e);
#endif

    static void processPlatformPanelEvent(QWindowSystemInterfacePrivate::PlatformPanelEvent *e);
#ifndef QT_NO_CONTEXTMENU
    static void processContextMenuEvent(QWindowSystemInterfacePrivate::ContextMenuEvent *e);
#endif

#if QT_CONFIG(draganddrop)
    static QPlatformDragQtResponse processDrag(QWindow *w, const QMimeData *dropData,
                                               const QPoint &p, Qt::DropActions supportedActions,
                                               Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    static QPlatformDropQtResponse processDrop(QWindow *w, const QMimeData *dropData,
                                               const QPoint &p, Qt::DropActions supportedActions,
                                               Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    static bool processNativeEvent(QWindow *window, const QByteArray &eventType, void *message, qintptr *result);
#else
    static bool processNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result);
#endif

    static bool sendQWindowEventToQPlatformWindow(QWindow *window, QEvent *event);

    static inline Qt::Alignment visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
    {
        if (!(alignment & Qt::AlignHorizontal_Mask))
            alignment |= Qt::AlignLeft;
        if (!(alignment & Qt::AlignAbsolute) && (alignment & (Qt::AlignLeft | Qt::AlignRight))) {
            if (direction == Qt::RightToLeft)
                alignment ^= (Qt::AlignLeft | Qt::AlignRight);
            alignment |= Qt::AlignAbsolute;
        }
        return alignment;
    }

    static void emitLastWindowClosed();

    QPixmap getPixmapCursor(Qt::CursorShape cshape);

    void _q_updateFocusObject(QObject *object);

    static QGuiApplicationPrivate *instance() { return self; }

    static QIcon *app_icon;
    static QString *platform_name;
    static QString *displayName;
    static QString *desktopFileName;

    QWindowList modalWindowList;
    static void showModalWindow(QWindow *window);
    static void hideModalWindow(QWindow *window);
    static void updateBlockedStatus(QWindow *window);
    virtual bool isWindowBlocked(QWindow *window, QWindow **blockingWindow = nullptr) const;
    virtual bool popupActive() { return false; }

    static Qt::MouseButton mousePressButton;
    static QPointF lastCursorPosition;
    static QWindow *currentMouseWindow;
    static QWindow *currentMousePressWindow;
    static Qt::ApplicationState applicationState;
    static Qt::HighDpiScaleFactorRoundingPolicy highDpiScaleFactorRoundingPolicy;
    static bool highDpiScalingUpdated;
    static QPointer<QWindow> currentDragWindow;

    // TODO remove this: QPointingDevice can store what we need directly
    struct TabletPointData {
        TabletPointData(qint64 devId = 0) : deviceId(devId), state(Qt::NoButton), target(nullptr) {}
        qint64 deviceId;
        Qt::MouseButtons state;
        QWindow *target;
    };
    static QList<TabletPointData> tabletDevicePoints;
    static TabletPointData &tabletDevicePoint(qint64 deviceId);

#ifndef QT_NO_CLIPBOARD
    static QClipboard *qt_clipboard;
#endif

    static QPalette *app_pal;

    static QWindowList window_list;
    static QWindow *focus_window;

#ifndef QT_NO_CURSOR
    QList<QCursor> cursor_list;
#endif
    static QList<QScreen *> screen_list;

    static QFont *app_font;

    static QString styleOverride;
    static QStyleHints *styleHints;
    static bool obey_desktop_settings;
    QInputMethod *inputMethod;

    QString firstWindowTitle;
    QIcon forcedWindowIcon;

    static QList<QObject *> generic_plugin_list;
#if QT_CONFIG(shortcut)
    QShortcutMap shortcutMap;
#endif

#ifndef QT_NO_SESSIONMANAGER
    QSessionManager *session_manager;
    bool is_session_restored;
    bool is_saving_session;
    void commitData();
    void saveState();
#endif

    QEvent::Type lastTouchType;
    struct SynthesizedMouseData {
        SynthesizedMouseData(const QPointF &p, const QPointF &sp, QWindow *w)
            : pos(p), screenPos(sp), window(w) { }
        QPointF pos;
        QPointF screenPos;
        QPointer<QWindow> window;
    };
    QHash<QWindow *, SynthesizedMouseData> synthesizedMousePoints;

    static QInputDeviceManager *inputDeviceManager();

    const QColorTrcLut *colorProfileForA8Text();
    const QColorTrcLut *colorProfileForA32Text();

    // hook reimplemented in QApplication to apply the QStyle function on the QIcon
    virtual QPixmap applyQIconStyleHelper(QIcon::Mode, const QPixmap &basePixmap) const { return basePixmap; }

    virtual void notifyWindowIconChanged();

    static void applyWindowGeometrySpecificationTo(QWindow *window);

    static void setApplicationState(Qt::ApplicationState state, bool forcePropagate = false);

    static void resetCachedDevicePixelRatio();

#ifndef QT_NO_ACTION
    virtual QActionPrivate *createActionPrivate() const;
#endif
#ifndef QT_NO_SHORTCUT
    virtual QShortcutPrivate *createShortcutPrivate() const;
#endif

    static void updatePalette();

protected:
    virtual void notifyThemeChanged();

    static bool setPalette(const QPalette &palette);
    virtual QPalette basePalette() const;
    virtual void handlePaletteChanged(const char *className = nullptr);

#if QT_CONFIG(draganddrop)
    virtual void notifyDragStarted(const QDrag *);
#endif // QT_CONFIG(draganddrop)

private:
    static void clearPalette();

    friend class QDragManager;

    static QGuiApplicationPrivate *self;
    static int m_fakeMouseSourcePointId;
    QSharedPointer<QColorTrcLut> m_a8ColorProfile;
    QSharedPointer<QColorTrcLut> m_a32ColorProfile;

    bool ownGlobalShareContext;

    static QInputDeviceManager *m_inputDeviceManager;

    // Cache the maximum device pixel ratio, to iterate through the screen list
    // only the first time it's required, or when devices are added or removed.
    static qreal m_maxDevicePixelRatio;
};

// ----------------- QNativeInterface -----------------

namespace QNativeInterface::Private {

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)

class QWindowsMime;

struct Q_GUI_EXPORT QWindowsApplication
{
    QT_DECLARE_NATIVE_INTERFACE(QWindowsApplication)

    enum WindowActivationBehavior {
        DefaultActivateWindow,
        AlwaysActivateWindow
    };

    enum TouchWindowTouchType {
        NormalTouch   = 0x00000000,
        FineTouch     = 0x00000001,
        WantPalmTouch = 0x00000002
    };

    Q_DECLARE_FLAGS(TouchWindowTouchTypes, TouchWindowTouchType)

    enum DarkModeHandlingFlag {
        DarkModeWindowFrames = 0x1,
        DarkModeStyle = 0x2
    };

    Q_DECLARE_FLAGS(DarkModeHandling, DarkModeHandlingFlag)

    virtual void setTouchWindowTouchType(TouchWindowTouchTypes type) = 0;
    virtual TouchWindowTouchTypes touchWindowTouchType() const = 0;

    virtual WindowActivationBehavior windowActivationBehavior() const = 0;
    virtual void setWindowActivationBehavior(WindowActivationBehavior behavior) = 0;

    virtual bool isTabletMode() const = 0;

    virtual bool isWinTabEnabled() const = 0;
    virtual bool setWinTabEnabled(bool enabled) = 0;

    virtual bool isDarkMode() const = 0;

    virtual DarkModeHandling darkModeHandling() const = 0;
    virtual void setDarkModeHandling(DarkModeHandling handling) = 0;

    virtual void registerMime(QWindowsMime *mime) = 0;
    virtual void unregisterMime(QWindowsMime *mime) = 0;

    virtual int registerMimeType(const QString &mime) = 0;

    virtual HWND createMessageWindow(const QString &classNameTemplate,
                                     const QString &windowName,
                                     QFunctionPointer eventProc = nullptr) const = 0;

    virtual bool asyncExpose() const = 0; // internal, used by Active Qt
    virtual void setAsyncExpose(bool value) = 0;

    virtual QVariant gpu() const = 0; // internal, used by qtdiag
    virtual QVariant gpuList() const = 0;
};
#endif // Q_OS_WIN

} // QNativeInterface::Private

#if defined(Q_OS_WIN)
Q_DECLARE_OPERATORS_FOR_FLAGS(QNativeInterface::Private::QWindowsApplication::TouchWindowTouchTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QNativeInterface::Private::QWindowsApplication::DarkModeHandling)
#endif

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_P_H
