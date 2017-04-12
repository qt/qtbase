/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qsystemtrayicon_p.h"
#ifndef QT_NO_SYSTEMTRAYICON

#if defined(WINVER) && WINVER < 0x0601
#  undef WINVER
#endif
#if !defined(WINVER)
#  define WINVER 0x0601 // required for NOTIFYICONDATA_V2_SIZE, ChangeWindowMessageFilterEx() (MinGW 5.3)
#endif

#if defined(NTDDI_VERSION) && NTDDI_VERSION < 0x06010000
#  undef NTDDI_VERSION
#endif
#if !defined(NTDDI_VERSION)
#  define NTDDI_VERSION 0x06010000 // required for Shell_NotifyIconGetRect (MinGW 5.3)
#endif

#include <private/qsystemlibrary_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <QSettings>
#include <QDebug>
#include <QHash>

#include <qt_windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>

QT_BEGIN_NAMESPACE

static const UINT q_uNOTIFYICONID = 0;

static uint MYWM_TASKBARCREATED = 0;
#define MYWM_NOTIFYICON (WM_APP+101)

Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &);

// Copy QString data to a limited wchar_t array including \0.
static inline void qStringToLimitedWCharArray(QString in, wchar_t *target, int maxLength)
{
    const int length = qMin(maxLength - 1, in.size());
    if (length < in.size())
        in.truncate(length);
    in.toWCharArray(target);
    target[length] = wchar_t(0);
}

class QSystemTrayIconSys
{
public:
    QSystemTrayIconSys(HWND hwnd, QSystemTrayIcon *object);
    ~QSystemTrayIconSys();
    bool trayMessage(DWORD msg);
    void setIconContents(NOTIFYICONDATA &data);
    bool showMessage(const QString &title, const QString &message, const QIcon &icon, uint uSecs);
    QRect findIconGeometry(UINT iconId);
    HICON createIcon();
    bool winEvent(MSG *m, long *result);

private:
    const HWND m_hwnd;
    HICON hIcon;
    QPoint globalPos;
    QSystemTrayIcon *q;
    bool ignoreNextMouseRelease;
};

static bool allowsMessages()
{
#ifndef QT_NO_SETTINGS
    const QString key = QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
    const QSettings settings(key, QSettings::NativeFormat);
    return settings.value(QStringLiteral("EnableBalloonTips"), true).toBool();
#else
    return false;
#endif
}

typedef QHash<HWND, QSystemTrayIconSys *> HandleTrayIconHash;

Q_GLOBAL_STATIC(HandleTrayIconHash, handleTrayIconHash)

extern "C" LRESULT QT_WIN_CALLBACK qWindowsTrayconWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == MYWM_TASKBARCREATED || message == MYWM_NOTIFYICON) {
        if (QSystemTrayIconSys *trayIcon = handleTrayIconHash()->value(hwnd)) {
            MSG msg;
            msg.hwnd = hwnd;         // re-create MSG structure
            msg.message = message;   // time and pt fields ignored
            msg.wParam = wParam;
            msg.lParam = lParam;
            msg.pt.x = GET_X_LPARAM(lParam);
            msg.pt.y = GET_Y_LPARAM(lParam);
            long result = 0;
            if (trayIcon->winEvent(&msg, &result))
                return result;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Invoke a service of the native Windows interface to create
// a non-visible toplevel window to receive tray messages.
// Note: Message windows (HWND_MESSAGE) are not sufficient, they
// will not receive the "TaskbarCreated" message.
static inline HWND createTrayIconMessageWindow()
{
    QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();
    if (!ni)
        return 0;
    // Register window class in the platform plugin.
    QString className;
    void *wndProc = reinterpret_cast<void *>(qWindowsTrayconWndProc);
    if (!QMetaObject::invokeMethod(ni, "registerWindowClass", Qt::DirectConnection,
                                   Q_RETURN_ARG(QString, className),
                                   Q_ARG(QString, QStringLiteral("QTrayIconMessageWindowClass")),
                                   Q_ARG(void *, wndProc))) {
        return 0;
    }
    const wchar_t windowName[] = L"QTrayIconMessageWindow";
    return CreateWindowEx(0, (wchar_t*)className.utf16(),
                          windowName, WS_OVERLAPPED,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL, NULL, (HINSTANCE)GetModuleHandle(0), NULL);
}

static inline void initNotifyIconData(NOTIFYICONDATA &tnd)
{
    memset(&tnd, 0, sizeof(NOTIFYICONDATA));
    tnd.cbSize = sizeof(NOTIFYICONDATA);
    tnd.uVersion = NOTIFYICON_VERSION_4;
}

QSystemTrayIconSys::QSystemTrayIconSys(HWND hwnd, QSystemTrayIcon *object)
    : m_hwnd(hwnd), hIcon(0), q(object)
    , ignoreNextMouseRelease(false)

{
    handleTrayIconHash()->insert(m_hwnd, this);

    // For restoring the tray icon after explorer crashes
    if (!MYWM_TASKBARCREATED) {
        MYWM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
    }

    // Allow the WM_TASKBARCREATED message through the UIPI filter
    ChangeWindowMessageFilterEx(m_hwnd, MYWM_TASKBARCREATED, MSGFLT_ALLOW, 0);
}

QSystemTrayIconSys::~QSystemTrayIconSys()
{
    handleTrayIconHash()->remove(m_hwnd);
    if (hIcon)
        DestroyIcon(hIcon);
    DestroyWindow(m_hwnd);
}

void QSystemTrayIconSys::setIconContents(NOTIFYICONDATA &tnd)
{
    tnd.uFlags |= NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage = MYWM_NOTIFYICON;
    tnd.hIcon = hIcon;
    const QString tip = q->toolTip();
    if (!tip.isNull())
        qStringToLimitedWCharArray(tip, tnd.szTip, sizeof(tnd.szTip)/sizeof(wchar_t));
}

bool QSystemTrayIconSys::showMessage(const QString &title, const QString &message, const QIcon &icon, uint uSecs)
{
    NOTIFYICONDATA tnd;
    initNotifyIconData(tnd);
    qStringToLimitedWCharArray(message, tnd.szInfo, 256);
    qStringToLimitedWCharArray(title, tnd.szInfoTitle, 64);

    tnd.uID = q_uNOTIFYICONID;
    tnd.dwInfoFlags = NIIF_USER;

    QSize size(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    const QSize largeIcon(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    const QSize more = icon.actualSize(largeIcon);
    if (more.height() > (largeIcon.height() * 3/4) || more.width() > (largeIcon.width() * 3/4)) {
        tnd.dwInfoFlags |= NIIF_LARGE_ICON;
        size = largeIcon;
    }
    QPixmap pm = icon.pixmap(size);
    if (pm.isNull()) {
        tnd.dwInfoFlags = NIIF_INFO;
    } else {
        if (pm.size() != size) {
            qWarning("QSystemTrayIcon::showMessage: Wrong icon size (%dx%d), please add standard one: %dx%d",
                      pm.size().width(), pm.size().height(), size.width(), size.height());
            pm = pm.scaled(size, Qt::IgnoreAspectRatio);
        }
        tnd.hBalloonIcon = qt_pixmapToWinHICON(pm);
    }
    tnd.hWnd = m_hwnd;
    tnd.uTimeout = uSecs;
    tnd.uFlags = NIF_INFO | NIF_SHOWTIP;

    return Shell_NotifyIcon(NIM_MODIFY, &tnd);
}

bool QSystemTrayIconSys::trayMessage(DWORD msg)
{
    NOTIFYICONDATA tnd;
    initNotifyIconData(tnd);

    tnd.uID = q_uNOTIFYICONID;
    tnd.hWnd = m_hwnd;
    tnd.uFlags = NIF_SHOWTIP;

    if (msg == NIM_ADD || msg == NIM_MODIFY) {
        setIconContents(tnd);
    }

    bool success = Shell_NotifyIcon(msg, &tnd);

    if (msg == NIM_ADD)
        return success && Shell_NotifyIcon(NIM_SETVERSION, &tnd);
    else
        return success;
}

HICON QSystemTrayIconSys::createIcon()
{
    const HICON oldIcon = hIcon;
    hIcon = 0;
    const QIcon icon = q->icon();
    if (icon.isNull())
        return oldIcon;
    const QSize requestedSize(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    const QSize size = icon.actualSize(requestedSize);
    const QPixmap pm = icon.pixmap(size);
    if (pm.isNull())
        return oldIcon;
    hIcon = qt_pixmapToWinHICON(pm);
    return oldIcon;
}

bool QSystemTrayIconSys::winEvent( MSG *m, long *result )
{
    *result = 0;
    switch(m->message) {
    case MYWM_NOTIFYICON:
        {
            Q_ASSERT(q_uNOTIFYICONID == HIWORD(m->lParam));
            const int message = LOWORD(m->lParam);
            const QPoint gpos = QPoint(GET_X_LPARAM(m->wParam), GET_Y_LPARAM(m->wParam));

            switch (message) {
            case NIN_SELECT:
            case NIN_KEYSELECT:
                if (ignoreNextMouseRelease)
                    ignoreNextMouseRelease = false;
                else
                    emit q->activated(QSystemTrayIcon::Trigger);
                break;

            case WM_LBUTTONDBLCLK:
                ignoreNextMouseRelease = true; // Since DBLCLICK Generates a second mouse
                                               // release we must ignore it
                emit q->activated(QSystemTrayIcon::DoubleClick);
                break;

            case WM_CONTEXTMENU:
                if (q->contextMenu()) {
                    q->contextMenu()->popup(gpos);
                    q->contextMenu()->activateWindow();
                }
                emit q->activated(QSystemTrayIcon::Context);
                break;

            case NIN_BALLOONUSERCLICK:
                emit q->messageClicked();
                break;

            case WM_MBUTTONUP:
                emit q->activated(QSystemTrayIcon::MiddleClick);
                break;

            default:
                break;
            }
            break;
        }
    default:
        if (m->message == MYWM_TASKBARCREATED) // self-registered message id.
            trayMessage(NIM_ADD);
        break;
    }
    return false;
}

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : sys(0),
      visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
}

void QSystemTrayIconPrivate::install_sys()
{
    Q_Q(QSystemTrayIcon);
    if (!sys) {
        if (const HWND hwnd = createTrayIconMessageWindow()) {
            sys = new QSystemTrayIconSys(hwnd, q);
            sys->createIcon();
            sys->trayMessage(NIM_ADD);
        } else {
            qWarning("The platform plugin failed to create a message window.");
        }
    }
}

/*
* This function tries to determine the icon geometry from the tray
*
* If it fails an invalid rect is returned.
*/

QRect QSystemTrayIconSys::findIconGeometry(UINT iconId)
{
    struct AppData
    {
        HWND hwnd;
        UINT uID;
    };

    NOTIFYICONIDENTIFIER nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = m_hwnd;
    nid.uID = iconId;

    RECT rect;
    if (SUCCEEDED(Shell_NotifyIconGetRect(&nid, &rect)))
        return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

    QRect ret;

    TBBUTTON buttonData;
    DWORD processID = 0;
    HWND trayHandle = FindWindow(L"Shell_TrayWnd", NULL);

    //find the toolbar used in the notification area
    if (trayHandle) {
        trayHandle = FindWindowEx(trayHandle, NULL, L"TrayNotifyWnd", NULL);
        if (trayHandle) {
            HWND hwnd = FindWindowEx(trayHandle, NULL, L"SysPager", NULL);
            if (hwnd) {
                hwnd = FindWindowEx(hwnd, NULL, L"ToolbarWindow32", NULL);
                if (hwnd)
                    trayHandle = hwnd;
            }
        }
    }

    if (!trayHandle)
        return ret;

    GetWindowThreadProcessId(trayHandle, &processID);
    if (processID <= 0)
        return ret;

    HANDLE trayProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ, 0, processID);
    if (!trayProcess)
        return ret;

    int buttonCount = SendMessage(trayHandle, TB_BUTTONCOUNT, 0, 0);
    LPVOID data = VirtualAllocEx(trayProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);

    if ( buttonCount < 1 || !data ) {
        CloseHandle(trayProcess);
        return ret;
    }

    //search for our icon among all toolbar buttons
    for (int toolbarButton = 0; toolbarButton  < buttonCount; ++toolbarButton ) {
        SIZE_T numBytes = 0;
        AppData appData = { 0, 0 };
        SendMessage(trayHandle, TB_GETBUTTON, toolbarButton , (LPARAM)data);

        if (!ReadProcessMemory(trayProcess, data, &buttonData, sizeof(TBBUTTON), &numBytes))
            continue;

        if (!ReadProcessMemory(trayProcess, (LPVOID) buttonData.dwData, &appData, sizeof(AppData), &numBytes))
            continue;

        bool isHidden = buttonData.fsState & TBSTATE_HIDDEN;

        if (m_hwnd == appData.hwnd && appData.uID == iconId && !isHidden) {
            SendMessage(trayHandle, TB_GETITEMRECT, toolbarButton , (LPARAM)data);
            RECT iconRect = {0, 0, 0, 0};
            if(ReadProcessMemory(trayProcess, data, &iconRect, sizeof(RECT), &numBytes)) {
                MapWindowPoints(trayHandle, NULL, (LPPOINT)&iconRect, 2);
                QRect geometry(iconRect.left + 1, iconRect.top + 1,
                                iconRect.right - iconRect.left - 2,
                                iconRect.bottom - iconRect.top - 2);
                if (geometry.isValid())
                    ret = geometry;
                break;
            }
        }
    }
    VirtualFreeEx(trayProcess, data, 0, MEM_RELEASE);
    CloseHandle(trayProcess);
    return ret;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &title,
                                             const QString &messageIn,
                                             const QIcon &icon,
                                             QSystemTrayIcon::MessageIcon,
                                             int timeOut)
{
    if (!sys || !allowsMessages())
        return;

    // 10 sec default
    const uint uSecs = timeOut < 0 ? uint(10000) : uint(timeOut);
    // For empty messages, ensures that they show when only title is set
    QString message = messageIn;
    if (message.isEmpty() && !title.isEmpty())
        message.append(QLatin1Char(' '));

    sys->showMessage(title, message, icon, uSecs);
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (!sys)
        return QRect();

    return sys->findIconGeometry(q_uNOTIFYICONID);
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (!sys)
        return;

    sys->trayMessage(NIM_DELETE);
    delete sys;
    sys = 0;
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (!sys)
        return;

    const HICON hIconToDestroy = sys->createIcon();
    sys->trayMessage(NIM_MODIFY);

    if (hIconToDestroy)
        DestroyIcon(hIconToDestroy);
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
#if QT_CONFIG(menu)
#endif
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (!sys)
        return;

    sys->trayMessage(NIM_MODIFY);
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    return true;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    return allowsMessages();
}

QT_END_NAMESPACE

#endif
