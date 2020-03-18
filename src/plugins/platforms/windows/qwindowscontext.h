/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

struct IBindCtx;
struct _SHSTOCKICONINFO;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindows)
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

class QWindow;
class QPlatformScreen;
class QPlatformWindow;
class QWindowsMenuBar;
class QWindowsScreenManager;
class QWindowsTabletSupport;
class QWindowsWindow;
class QWindowsMimeConverter;
struct QWindowCreationContext;
struct QWindowsContextPrivate;
class QPoint;
class QKeyEvent;
class QTouchDevice;

struct QWindowsUser32DLL
{
    inline void init();
    inline bool supportsPointerApi();

    typedef BOOL (WINAPI *EnableMouseInPointer)(BOOL);
    typedef BOOL (WINAPI *GetPointerType)(UINT32, PVOID);
    typedef BOOL (WINAPI *GetPointerInfo)(UINT32, PVOID);
    typedef BOOL (WINAPI *GetPointerDeviceRects)(HANDLE, RECT *, RECT *);
    typedef BOOL (WINAPI *GetPointerTouchInfo)(UINT32, PVOID);
    typedef BOOL (WINAPI *GetPointerFrameTouchInfo)(UINT32, UINT32 *, PVOID);
    typedef BOOL (WINAPI *GetPointerFrameTouchInfoHistory)(UINT32, UINT32 *, UINT32 *, PVOID);
    typedef BOOL (WINAPI *GetPointerPenInfo)(UINT32, PVOID);
    typedef BOOL (WINAPI *GetPointerPenInfoHistory)(UINT32, UINT32 *, PVOID);
    typedef BOOL (WINAPI *SkipPointerFrameMessages)(UINT32);
    typedef BOOL (WINAPI *SetProcessDPIAware)();
    typedef BOOL (WINAPI *AddClipboardFormatListener)(HWND);
    typedef BOOL (WINAPI *RemoveClipboardFormatListener)(HWND);
    typedef BOOL (WINAPI *GetDisplayAutoRotationPreferences)(DWORD *);
    typedef BOOL (WINAPI *SetDisplayAutoRotationPreferences)(DWORD);
    typedef BOOL (WINAPI *AdjustWindowRectExForDpi)(LPRECT,DWORD,BOOL,DWORD,UINT);
    typedef BOOL (WINAPI *EnableNonClientDpiScaling)(HWND);
    typedef int  (WINAPI *GetWindowDpiAwarenessContext)(HWND);
    typedef int  (WINAPI *GetAwarenessFromDpiAwarenessContext)(int);
    typedef BOOL (WINAPI *SystemParametersInfoForDpi)(UINT, UINT, PVOID, UINT, UINT);

    // Windows pointer functions (Windows 8 or later).
    EnableMouseInPointer enableMouseInPointer = nullptr;
    GetPointerType getPointerType = nullptr;
    GetPointerInfo getPointerInfo = nullptr;
    GetPointerDeviceRects getPointerDeviceRects = nullptr;
    GetPointerTouchInfo getPointerTouchInfo = nullptr;
    GetPointerFrameTouchInfo getPointerFrameTouchInfo = nullptr;
    GetPointerFrameTouchInfoHistory getPointerFrameTouchInfoHistory = nullptr;
    GetPointerPenInfo getPointerPenInfo = nullptr;
    GetPointerPenInfoHistory getPointerPenInfoHistory = nullptr;
    SkipPointerFrameMessages skipPointerFrameMessages = nullptr;

    // Windows Vista onwards
    SetProcessDPIAware setProcessDPIAware = nullptr;

    // Clipboard listeners are present on Windows Vista onwards
    // but missing in MinGW 4.9 stub libs. Can be removed in MinGW 5.
    AddClipboardFormatListener addClipboardFormatListener = nullptr;
    RemoveClipboardFormatListener removeClipboardFormatListener = nullptr;

    // Rotation API
    GetDisplayAutoRotationPreferences getDisplayAutoRotationPreferences = nullptr;
    SetDisplayAutoRotationPreferences setDisplayAutoRotationPreferences = nullptr;

    AdjustWindowRectExForDpi adjustWindowRectExForDpi = nullptr;
    EnableNonClientDpiScaling enableNonClientDpiScaling = nullptr;
    GetWindowDpiAwarenessContext getWindowDpiAwarenessContext = nullptr;
    GetAwarenessFromDpiAwarenessContext getAwarenessFromDpiAwarenessContext = nullptr;
    SystemParametersInfoForDpi systemParametersInfoForDpi = nullptr;
};

// Shell scaling library (Windows 8.1 onwards)
struct QWindowsShcoreDLL {
    void init();
    inline bool isValid() const { return getProcessDpiAwareness && setProcessDpiAwareness && getDpiForMonitor; }

    typedef HRESULT (WINAPI *GetProcessDpiAwareness)(HANDLE,int *);
    typedef HRESULT (WINAPI *SetProcessDpiAwareness)(int);
    typedef HRESULT (WINAPI *GetDpiForMonitor)(HMONITOR,int,UINT *,UINT *);

    GetProcessDpiAwareness getProcessDpiAwareness = nullptr;
    SetProcessDpiAwareness setProcessDpiAwareness = nullptr;
    GetDpiForMonitor getDpiForMonitor = nullptr;
};

class QWindowsContext
{
    Q_DISABLE_COPY_MOVE(QWindowsContext)
public:

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
    bool initTablet(unsigned integrationOptions);
    bool initPointer(unsigned integrationOptions);

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

    static QString windowsErrorMessage(unsigned long errorCode);

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

    void setTabletAbsoluteRange(int a);
    void setProcessDpiAwareness(QtWindows::ProcessDpiAwareness dpiAwareness);
    static int processDpiAwareness();

    static bool isDarkMode();

    void setDetectAltGrModifier(bool a);

    // Returns a combination of SystemInfoFlags
    unsigned systemInfo() const;

    bool useRTLExtensions() const;
    QList<int> possibleKeys(const QKeyEvent *e) const;

    static bool isSessionLocked();

    QWindowsMimeConverter &mimeConverter() const;
    QWindowsScreenManager &screenManager();
    QWindowsTabletSupport *tabletSupport() const;

    static QWindowsUser32DLL user32dll;
    static QWindowsShcoreDLL shcoredll;

    static QByteArray comErrorString(HRESULT hr);
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

    QTouchDevice *touchDevice() const;

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
