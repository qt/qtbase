// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSCONTEXT_H
#define QWINDOWSCONTEXT_H

#include "qtwindowsglobal.h"
#include <QtCore/qt_windows.h>

#include <QtCore/qscopedpointer.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qloggingcategory.h>

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <shlwapi.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindow)
Q_DECLARE_LOGGING_CATEGORY(lcQpaEvents)
Q_DECLARE_LOGGING_CATEGORY(lcQpaGl)
Q_DECLARE_LOGGING_CATEGORY(lcQpaMime)
Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods)
Q_DECLARE_LOGGING_CATEGORY(lcQpaDialogs)
Q_DECLARE_LOGGING_CATEGORY(lcQpaMenus)
Q_DECLARE_LOGGING_CATEGORY(lcQpaTablet)
Q_DECLARE_LOGGING_CATEGORY(lcQpaAccessibility)
Q_DECLARE_LOGGING_CATEGORY(lcQpaUiAutomation)
Q_DECLARE_LOGGING_CATEGORY(lcQpaTrayIcon)
Q_DECLARE_LOGGING_CATEGORY(lcQpaScreen)

class QWindow;
class QPlatformScreen;
class QPlatformWindow;
class QWindowsMenuBar;
class QWindowsScreenManager;
class QWindowsTabletSupport;
class QWindowsWindow;
class QWindowsMimeRegistry;
struct QWindowCreationContext;
struct QWindowsContextPrivate;
class QPoint;
class QKeyEvent;
class QPointingDevice;

class QWindowsContext
{
    Q_DISABLE_COPY_MOVE(QWindowsContext)
public:
    using HandleBaseWindowHash = QHash<HWND, QWindowsWindow *>;

    enum SystemInfoFlags
    {
        SI_RTL_Extensions = 0x1,
        SI_SupportsTouch = 0x2,
        SI_SupportsPointer = 0x4,
    };

    // Verbose flag set by environment variable QT_QPA_VERBOSE
    static int verbose;

    explicit QWindowsContext();
    ~QWindowsContext();

    bool initTouch();
    bool initTouch(unsigned integrationOptions); // For calls from QWindowsIntegration::QWindowsIntegration() only.
    void registerTouchWindows();
    bool initTablet();
    bool initPointer(unsigned integrationOptions);
    bool disposeTablet();

    bool initPowerNotificationHandler();

    int defaultDPI() const;

    static QString classNamePrefix();
    QString registerWindowClass(const QWindow *w);
    QString registerWindowClass(QString cname, WNDPROC proc,
                                unsigned style = 0, HBRUSH brush = nullptr,
                                bool icon = false);
    HWND createDummyWindow(const QString &classNameIn,
                           const wchar_t *windowName,
                           WNDPROC wndProc = nullptr, DWORD style = WS_OVERLAPPED);

    HDC displayContext() const;
    int screenDepth() const;

    static QWindowsContext *instance();

    void addWindow(HWND, QWindowsWindow *w);
    void removeWindow(HWND);

    QWindowsWindow *findClosestPlatformWindow(HWND) const;
    QWindowsWindow *findPlatformWindow(HWND) const;
    QWindowsWindow *findPlatformWindow(const QWindowsMenuBar *mb) const;
    QWindow *findWindow(HWND) const;
    QWindowsWindow *findPlatformWindowAt(HWND parent, const QPoint &screenPoint,
                                             unsigned cwex_flags) const;

    static bool shouldHaveNonClientDpiScaling(const QWindow *window);

    QWindow *windowUnderMouse() const;
    void clearWindowUnderMouse();

    inline bool windowsProc(HWND hwnd, UINT message,
                            QtWindows::WindowsEventType et,
                            WPARAM wParam, LPARAM lParam, LRESULT *result,
                            QWindowsWindow **platformWindowPtr);

    QWindow *keyGrabber() const;
    void setKeyGrabber(QWindow *hwnd);

    QSharedPointer<QWindowCreationContext> setWindowCreationContext(const QSharedPointer<QWindowCreationContext> &ctx);
    QSharedPointer<QWindowCreationContext> windowCreationContext() const;

    static void setTabletAbsoluteRange(int a);

    static bool setProcessDpiAwareness(QtWindows::DpiAwareness dpiAwareness);
    static QtWindows::DpiAwareness processDpiAwareness();
    static QtWindows::DpiAwareness windowDpiAwareness(HWND hwnd);

    static bool isDarkMode();

    void setDetectAltGrModifier(bool a);

    // Returns a combination of SystemInfoFlags
    unsigned systemInfo() const;

    bool useRTLExtensions() const;
    QList<int> possibleKeys(const QKeyEvent *e) const;

    HandleBaseWindowHash &windows();

    static bool isSessionLocked();

    QWindowsMimeRegistry &mimeConverter() const;
    QWindowsScreenManager &screenManager();
    QWindowsTabletSupport *tabletSupport() const;

    bool asyncExpose() const;
    void setAsyncExpose(bool value);

    static void forceNcCalcSize(HWND hwnd);

    static bool systemParametersInfo(unsigned action, unsigned param, void *out, unsigned dpi = 0);
    static bool systemParametersInfoForScreen(unsigned action, unsigned param, void *out,
                                              const QPlatformScreen *screen = nullptr);
    static bool systemParametersInfoForWindow(unsigned action, unsigned param, void *out,
                                              const QPlatformWindow *win = nullptr);
    static bool nonClientMetrics(NONCLIENTMETRICS *ncm, unsigned dpi = 0);
    static bool nonClientMetricsForScreen(NONCLIENTMETRICS *ncm,
                                          const QPlatformScreen *screen = nullptr);
    static bool nonClientMetricsForWindow(NONCLIENTMETRICS *ncm,
                                          const QPlatformWindow *win = nullptr);

    static DWORD readAdvancedExplorerSettings(const wchar_t *subKey, DWORD defaultValue);

    static bool filterNativeEvent(MSG *msg, LRESULT *result);
    static bool filterNativeEvent(QWindow *window, MSG *msg, LRESULT *result);

private:
    void handleFocusEvent(QtWindows::WindowsEventType et, QWindowsWindow *w);
#ifndef QT_NO_CONTEXTMENU
    bool handleContextMenuEvent(QWindow *window, const MSG &msg);
#endif
    void handleExitSizeMove(QWindow *window);
    void unregisterWindowClasses();

    QScopedPointer<QWindowsContextPrivate> d;
    static QWindowsContext *m_instance;
};

extern "C" LRESULT QT_WIN_CALLBACK qWindowsWndProc(HWND, UINT, WPARAM, LPARAM);

QT_END_NAMESPACE

#endif // QWINDOWSCONTEXT_H
