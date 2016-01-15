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
#include "qtwindows_additional.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QLoggingCategory>

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <shlwapi.h>

struct IBindCtx;
struct _SHSTOCKICONINFO;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindows)
Q_DECLARE_LOGGING_CATEGORY(lcQpaBackingStore)
Q_DECLARE_LOGGING_CATEGORY(lcQpaEvents)
Q_DECLARE_LOGGING_CATEGORY(lcQpaFonts)
Q_DECLARE_LOGGING_CATEGORY(lcQpaGl)
Q_DECLARE_LOGGING_CATEGORY(lcQpaMime)
Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods)
Q_DECLARE_LOGGING_CATEGORY(lcQpaDialogs)
Q_DECLARE_LOGGING_CATEGORY(lcQpaTablet)
Q_DECLARE_LOGGING_CATEGORY(lcQpaAccessibility)

class QWindow;
class QPlatformScreen;
class QWindowsScreenManager;
class QWindowsTabletSupport;
class QWindowsWindow;
class QWindowsMimeConverter;
struct QWindowCreationContext;
struct QWindowsContextPrivate;
class QPoint;
class QKeyEvent;
class QTouchDevice;

#ifndef Q_OS_WINCE
struct QWindowsUser32DLL
{
    QWindowsUser32DLL();
    inline void init();
    inline bool initTouch();

    typedef BOOL (WINAPI *IsTouchWindow)(HWND, PULONG);
    typedef BOOL (WINAPI *RegisterTouchWindow)(HWND, ULONG);
    typedef BOOL (WINAPI *UnregisterTouchWindow)(HWND);
    typedef BOOL (WINAPI *GetTouchInputInfo)(HANDLE, UINT, PVOID, int);
    typedef BOOL (WINAPI *CloseTouchInputHandle)(HANDLE);
    typedef BOOL (WINAPI *SetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
    typedef BOOL (WINAPI *UpdateLayeredWindow)(HWND, HDC , const POINT *,
                 const SIZE *, HDC, const POINT *, COLORREF,
                 const BLENDFUNCTION *, DWORD);
    typedef BOOL (WINAPI *UpdateLayeredWindowIndirect)(HWND, const UPDATELAYEREDWINDOWINFO *);
    typedef BOOL (WINAPI *IsHungAppWindow)(HWND);
    typedef BOOL (WINAPI *SetProcessDPIAware)();
    typedef BOOL (WINAPI *AddClipboardFormatListener)(HWND);
    typedef BOOL (WINAPI *RemoveClipboardFormatListener)(HWND);
    typedef BOOL (WINAPI *GetDisplayAutoRotationPreferences)(DWORD *);
    typedef BOOL (WINAPI *SetDisplayAutoRotationPreferences)(DWORD);

    // Functions missing in Q_CC_GNU stub libraries.
    SetLayeredWindowAttributes setLayeredWindowAttributes;
    UpdateLayeredWindow updateLayeredWindow;

    // Functions missing in older versions of Windows
    UpdateLayeredWindowIndirect updateLayeredWindowIndirect;
    IsHungAppWindow isHungAppWindow;

    // Touch functions from Windows 7 onwards (also for use with Q_CC_MSVC).
    IsTouchWindow isTouchWindow;
    RegisterTouchWindow registerTouchWindow;
    UnregisterTouchWindow unregisterTouchWindow;
    GetTouchInputInfo getTouchInputInfo;
    CloseTouchInputHandle closeTouchInputHandle;

    // Windows Vista onwards
    SetProcessDPIAware setProcessDPIAware;

    // Clipboard listeners, Windows Vista onwards
    AddClipboardFormatListener addClipboardFormatListener;
    RemoveClipboardFormatListener removeClipboardFormatListener;

    // Rotation API
    GetDisplayAutoRotationPreferences getDisplayAutoRotationPreferences;
    SetDisplayAutoRotationPreferences setDisplayAutoRotationPreferences;
};

struct QWindowsShell32DLL
{
    QWindowsShell32DLL();
    inline void init();

    typedef HRESULT (WINAPI *SHCreateItemFromParsingName)(PCWSTR, IBindCtx *, const GUID&, void **);
    typedef HRESULT (WINAPI *SHGetKnownFolderIDList)(const GUID &, DWORD, HANDLE, PIDLIST_ABSOLUTE *);
    typedef HRESULT (WINAPI *SHGetStockIconInfo)(int , int , _SHSTOCKICONINFO *);
    typedef HRESULT (WINAPI *SHGetImageList)(int, REFIID , void **);
    typedef HRESULT (WINAPI *SHCreateItemFromIDList)(PCIDLIST_ABSOLUTE, REFIID, void **);

    SHCreateItemFromParsingName sHCreateItemFromParsingName;
    SHGetKnownFolderIDList sHGetKnownFolderIDList;
    SHGetStockIconInfo sHGetStockIconInfo;
    SHGetImageList sHGetImageList;
    SHCreateItemFromIDList sHCreateItemFromIDList;
};

// Shell scaling library (Windows 8.1 onwards)
struct QWindowsShcoreDLL {
    QWindowsShcoreDLL();
    void init();
    inline bool isValid() const { return getProcessDpiAwareness && setProcessDpiAwareness && getDpiForMonitor; }

    typedef HRESULT (WINAPI *GetProcessDpiAwareness)(HANDLE,int *);
    typedef HRESULT (WINAPI *SetProcessDpiAwareness)(int);
    typedef HRESULT (WINAPI *GetDpiForMonitor)(HMONITOR,int,UINT *,UINT *);

    GetProcessDpiAwareness getProcessDpiAwareness;
    SetProcessDpiAwareness setProcessDpiAwareness;
    GetDpiForMonitor getDpiForMonitor;
};

#endif // Q_OS_WINCE

class QWindowsContext
{
    Q_DISABLE_COPY(QWindowsContext)
public:

    enum SystemInfoFlags
    {
        SI_RTL_Extensions = 0x1,
        SI_SupportsTouch = 0x2
    };

    // Verbose flag set by environment variable QT_QPA_VERBOSE
    static int verbose;

    explicit QWindowsContext();
    ~QWindowsContext();

    bool initTouch();
    bool initTouch(unsigned integrationOptions); // For calls from QWindowsIntegration::QWindowsIntegration() only.

    int defaultDPI() const;

    QString registerWindowClass(const QWindow *w);
    QString registerWindowClass(QString cname, WNDPROC proc,
                                unsigned style = 0, HBRUSH brush = 0,
                                bool icon = false);
    HWND createDummyWindow(const QString &classNameIn,
                           const wchar_t *windowName,
                           WNDPROC wndProc = 0, DWORD style = WS_OVERLAPPED);

    HDC displayContext() const;
    int screenDepth() const;

    static QWindowsContext *instance();

    static QString windowsErrorMessage(unsigned long errorCode);

    void addWindow(HWND, QWindowsWindow *w);
    void removeWindow(HWND);

    QWindowsWindow *findClosestPlatformWindow(HWND) const;
    QWindowsWindow *findPlatformWindow(HWND) const;
    QWindow *findWindow(HWND) const;
    QWindowsWindow *findPlatformWindowAt(HWND parent, const QPoint &screenPoint,
                                             unsigned cwex_flags) const;

    QWindow *windowUnderMouse() const;
    void clearWindowUnderMouse();

    inline bool windowsProc(HWND hwnd, UINT message,
                            QtWindows::WindowsEventType et,
                            WPARAM wParam, LPARAM lParam, LRESULT *result);

    QWindow *keyGrabber() const;
    void setKeyGrabber(QWindow *hwnd);

    void setWindowCreationContext(const QSharedPointer<QWindowCreationContext> &ctx);

    void setTabletAbsoluteRange(int a);
    void setProcessDpiAwareness(QtWindows::ProcessDpiAwareness dpiAwareness);
    static int processDpiAwareness();

    // Returns a combination of SystemInfoFlags
    unsigned systemInfo() const;

    bool useRTLExtensions() const;
    QList<int> possibleKeys(const QKeyEvent *e) const;

    QWindowsMimeConverter &mimeConverter() const;
    QWindowsScreenManager &screenManager();
    QWindowsTabletSupport *tabletSupport() const;
#ifndef Q_OS_WINCE
    static QWindowsUser32DLL user32dll;
    static QWindowsShell32DLL shell32dll;
    static QWindowsShcoreDLL shcoredll;
#endif

    static QByteArray comErrorString(HRESULT hr);
    bool asyncExpose() const;
    void setAsyncExpose(bool value);

    QTouchDevice *touchDevice() const;

private:
    void handleFocusEvent(QtWindows::WindowsEventType et, QWindowsWindow *w);
#ifndef QT_NO_CONTEXTMENU
    bool handleContextMenuEvent(QWindow *window, const MSG &msg);
#endif
    void unregisterWindowClasses();

    QScopedPointer<QWindowsContextPrivate> d;
    static QWindowsContext *m_instance;
};

extern "C" LRESULT QT_WIN_CALLBACK qWindowsWndProc(HWND, UINT, WPARAM, LPARAM);

QT_END_NAMESPACE

#endif // QWINDOWSCONTEXT_H
