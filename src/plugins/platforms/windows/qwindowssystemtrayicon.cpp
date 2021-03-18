/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qwindowssystemtrayicon.h"
#include "qwindowscontext.h"
#include "qwindowstheme.h"
#include "qwindowsmenu.h"
#include "qwindowsscreen.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qpixmap.h>
#include <QtCore/qdebug.h>
#include <QtCore/qrect.h>
#include <QtCore/qvector.h>
#include <QtCore/qsettings.h>
#include <qpa/qwindowsysteminterface.h>

#include <qt_windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
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

static inline void initNotifyIconData(NOTIFYICONDATA &tnd)
{
    memset(&tnd, 0, sizeof(NOTIFYICONDATA));
    tnd.cbSize = sizeof(NOTIFYICONDATA);
    tnd.uVersion = NOTIFYICON_VERSION_4;
}

static void setIconContents(NOTIFYICONDATA &tnd, const QString &tip, HICON hIcon)
{
    tnd.uFlags |= NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage = MYWM_NOTIFYICON;
    tnd.hIcon = hIcon;
    qStringToLimitedWCharArray(tip, tnd.szTip, sizeof(tnd.szTip) / sizeof(wchar_t));
}

static void setIconVisibility(NOTIFYICONDATA &tnd, bool v)
{
    tnd.uFlags |= NIF_STATE;
    tnd.dwStateMask = NIS_HIDDEN;
    tnd.dwState = v ? 0 : NIS_HIDDEN;
}

// Match the HWND of the dummy window to the instances
struct QWindowsHwndSystemTrayIconEntry
{
    HWND hwnd;
    QWindowsSystemTrayIcon *trayIcon;
};

using HwndTrayIconEntries = QVector<QWindowsHwndSystemTrayIconEntry>;

Q_GLOBAL_STATIC(HwndTrayIconEntries, hwndTrayIconEntries)

static int indexOfHwnd(HWND hwnd)
{
    const HwndTrayIconEntries *entries = hwndTrayIconEntries();
    for (int i = 0, size = entries->size(); i < size; ++i) {
        if (entries->at(i).hwnd == hwnd)
            return i;
    }
    return -1;
}

extern "C" LRESULT QT_WIN_CALLBACK qWindowsTrayIconWndProc(HWND hwnd, UINT message,
                                                           WPARAM wParam, LPARAM lParam)
{
    // QTBUG-79248: Trigger screen update if there are no other windows.
    if (message == WM_DPICHANGED && QGuiApplication::topLevelWindows().isEmpty())
        QWindowsContext::instance()->screenManager().handleScreenChanges();
    if (message == MYWM_TASKBARCREATED || message == MYWM_NOTIFYICON
        || message == WM_INITMENU || message == WM_INITMENUPOPUP
        || message == WM_CLOSE || message == WM_COMMAND) {
        const int index = indexOfHwnd(hwnd);
        if (index >= 0) {
            MSG msg;
            msg.hwnd = hwnd;         // re-create MSG structure
            msg.message = message;   // time and pt fields ignored
            msg.wParam = wParam;
            msg.lParam = lParam;
            msg.pt.x = GET_X_LPARAM(lParam);
            msg.pt.y = GET_Y_LPARAM(lParam);
            long result = 0;
            if (hwndTrayIconEntries()->at(index).trayIcon->winEvent(msg, &result))
                return result;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Note: Message windows (HWND_MESSAGE) are not sufficient, they
// will not receive the "TaskbarCreated" message.
static inline HWND createTrayIconMessageWindow()
{
    QWindowsContext *ctx = QWindowsContext::instance();
    if (!ctx)
        return nullptr;
    // Register window class in the platform plugin.
    const QString className =
        ctx->registerWindowClass(QWindowsContext::classNamePrefix() + QStringLiteral("TrayIconMessageWindowClass"),
                                 qWindowsTrayIconWndProc);
    const wchar_t windowName[] = L"QTrayIconMessageWindow";
    return CreateWindowEx(0, reinterpret_cast<const wchar_t *>(className.utf16()),
                          windowName, WS_OVERLAPPED,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          nullptr, nullptr,
                          static_cast<HINSTANCE>(GetModuleHandle(nullptr)), nullptr);
}

/*!
    \class QWindowsSystemTrayIcon
    \brief Windows native system tray icon

    \internal
*/

QWindowsSystemTrayIcon::QWindowsSystemTrayIcon()
{
}

QWindowsSystemTrayIcon::~QWindowsSystemTrayIcon()
{
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << this;
    ensureCleanup();
}

void QWindowsSystemTrayIcon::init()
{
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << this;
    m_visible = true;
    if (!setIconVisible(m_visible))
        ensureInstalled();
}

void QWindowsSystemTrayIcon::cleanup()
{
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << this;
    m_visible = false;
    ensureCleanup();
}

void QWindowsSystemTrayIcon::updateIcon(const QIcon &icon)
{
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << '(' << icon << ')' << this;
    if (icon.cacheKey() == m_icon.cacheKey())
        return;
    const HICON hIconToDestroy = createIcon(icon);
    if (ensureInstalled())
        sendTrayMessage(NIM_MODIFY);
    if (hIconToDestroy)
        DestroyIcon(hIconToDestroy);
}

void QWindowsSystemTrayIcon::updateToolTip(const QString &tooltip)
{
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << '(' << tooltip << ')' << this;
    if (m_toolTip == tooltip)
        return;
    m_toolTip = tooltip;
    if (isInstalled())
        sendTrayMessage(NIM_MODIFY);
}

QRect QWindowsSystemTrayIcon::geometry() const
{
    NOTIFYICONIDENTIFIER nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = m_hwnd;
    nid.uID = q_uNOTIFYICONID;
    RECT rect;
    const QRect result = SUCCEEDED(Shell_NotifyIconGetRect(&nid, &rect))
        ? QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top)
        : QRect();
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << this << "returns" << result;
    return result;
}

void QWindowsSystemTrayIcon::showMessage(const QString &title, const QString &messageIn,
                                         const QIcon &icon,
                                         QPlatformSystemTrayIcon::MessageIcon iconType,
                                         int msecsIn)
{
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << '(' << title << messageIn << icon
        << iconType << msecsIn  << ')' << this;
    if (!supportsMessages())
        return;
    // For empty messages, ensures that they show when only title is set
    QString message = messageIn;
    if (message.isEmpty() && !title.isEmpty())
        message.append(u' ');

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
    tnd.uTimeout = msecsIn <= 0 ?  UINT(10000) : UINT(msecsIn); // 10s default
    tnd.uFlags = NIF_INFO | NIF_SHOWTIP;

    Shell_NotifyIcon(NIM_MODIFY, &tnd);
}

bool QWindowsSystemTrayIcon::supportsMessages() const
{
    // The key does typically not exist on Windows 10, default to true.
    return QWindowsContext::readAdvancedExplorerSettings(L"EnableBalloonTips", 1) != 0;
}

QPlatformMenu *QWindowsSystemTrayIcon::createMenu() const
{
    if (QWindowsTheme::useNativeMenus() && m_menu.isNull())
        m_menu = new QWindowsPopupMenu;
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << this << "returns" << m_menu.data();
    return m_menu.data();
}

// Delay-install until an Icon exists
bool QWindowsSystemTrayIcon::ensureInstalled()
{
    if (isInstalled())
        return true;
    if (m_hIcon == nullptr)
        return false;
    m_hwnd = createTrayIconMessageWindow();
    if (Q_UNLIKELY(m_hwnd == nullptr))
        return false;
    // For restoring the tray icon after explorer crashes
    if (!MYWM_TASKBARCREATED)
        MYWM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
    // Allow the WM_TASKBARCREATED message through the UIPI filter
    ChangeWindowMessageFilterEx(m_hwnd, MYWM_TASKBARCREATED, MSGFLT_ALLOW, nullptr);
    qCDebug(lcQpaTrayIcon) << __FUNCTION__ << this << "MYWM_TASKBARCREATED=" << MYWM_TASKBARCREATED;

    QWindowsHwndSystemTrayIconEntry entry{m_hwnd, this};
    hwndTrayIconEntries()->append(entry);
    sendTrayMessage(NIM_ADD);
    return true;
}

void QWindowsSystemTrayIcon::ensureCleanup()
{
    if (isInstalled()) {
        const int index = indexOfHwnd(m_hwnd);
        if (index >= 0)
            hwndTrayIconEntries()->removeAt(index);
        sendTrayMessage(NIM_DELETE);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    if (m_hIcon != nullptr)
        DestroyIcon(m_hIcon);
    m_hIcon = nullptr;
    m_menu = nullptr; // externally owned
    m_toolTip.clear();
}

bool QWindowsSystemTrayIcon::setIconVisible(bool visible)
{
    if (!isInstalled())
        return false;
    NOTIFYICONDATA tnd;
    initNotifyIconData(tnd);
    tnd.uID = q_uNOTIFYICONID;
    tnd.hWnd = m_hwnd;
    setIconVisibility(tnd, visible);
    return Shell_NotifyIcon(NIM_MODIFY, &tnd) == TRUE;
}

bool QWindowsSystemTrayIcon::sendTrayMessage(DWORD msg)
{
    NOTIFYICONDATA tnd;
    initNotifyIconData(tnd);
    tnd.uID = q_uNOTIFYICONID;
    tnd.hWnd = m_hwnd;
    tnd.uFlags = NIF_SHOWTIP;
    if (msg != NIM_DELETE && !m_visible)
        setIconVisibility(tnd, m_visible);
    if (msg == NIM_ADD || msg == NIM_MODIFY)
        setIconContents(tnd, m_toolTip, m_hIcon);
    if (!Shell_NotifyIcon(msg, &tnd))
        return false;
    return msg != NIM_ADD || Shell_NotifyIcon(NIM_SETVERSION, &tnd);
}

// Return the old icon to be freed after modifying the tray icon.
HICON QWindowsSystemTrayIcon::createIcon(const QIcon &icon)
{
    const HICON oldIcon = m_hIcon;
    m_hIcon = nullptr;
    if (icon.isNull())
        return oldIcon;
    const QSize requestedSize = QSize(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    const QSize size = icon.actualSize(requestedSize);
    const QPixmap pm = icon.pixmap(size);
    if (!pm.isNull())
        m_hIcon = qt_pixmapToWinHICON(pm);
    return oldIcon;
}

bool QWindowsSystemTrayIcon::winEvent(const MSG &message, long *result)
{
    *result = 0;
    switch (message.message) {
    case MYWM_NOTIFYICON: {
        Q_ASSERT(q_uNOTIFYICONID == HIWORD(message.lParam));
        const int trayMessage = LOWORD(message.lParam);
        switch (trayMessage) {
        case NIN_SELECT:
        case NIN_KEYSELECT:
            if (m_ignoreNextMouseRelease)
                m_ignoreNextMouseRelease = false;
            else
                emit activated(Trigger);
            break;
        case WM_LBUTTONDBLCLK:
            m_ignoreNextMouseRelease = true; // Since DBLCLICK Generates a second mouse
            emit activated(DoubleClick);     // release we must ignore it
            break;
        case WM_CONTEXTMENU: {
            // QTBUG-67966: Coordinates may be out of any screen in PROCESS_DPI_UNAWARE mode
            // since hi-res coordinates are delivered in this case (Windows issue).
            // Default to primary screen with check to prevent a crash.
            const QPoint globalPos = QPoint(GET_X_LPARAM(message.wParam), GET_Y_LPARAM(message.wParam));
            const auto &screenManager = QWindowsContext::instance()->screenManager();
            const QPlatformScreen *screen = screenManager.screenAtDp(globalPos);
            if (!screen)
                screen = screenManager.screens().value(0);
            if (screen) {
                emit contextMenuRequested(globalPos, screen);
                emit activated(Context);
                if (m_menu) {
                    // Set the foreground window to the controlling window so that clicking outside
                    // of the menu or window will cause the menu to close
                    SetForegroundWindow(m_hwnd);
                    m_menu->trackPopupMenu(message.hwnd, globalPos.x(), globalPos.y());
                }
            }
        }
            break;
        case NIN_BALLOONUSERCLICK:
            emit messageClicked();
            break;
        case WM_MBUTTONUP:
            emit activated(MiddleClick);
            break;
        default:
            break;
        }
    }
        break;
    case WM_INITMENU:
    case WM_INITMENUPOPUP:
        QWindowsPopupMenu::notifyAboutToShow(reinterpret_cast<HMENU>(message.wParam));
        break;
    case WM_CLOSE:
        QWindowSystemInterface::handleApplicationTermination<QWindowSystemInterface::SynchronousDelivery>();
        break;
    case WM_COMMAND:
        QWindowsPopupMenu::notifyTriggered(LOWORD(message.wParam));
        break;
    default:
        if (message.message == MYWM_TASKBARCREATED) // self-registered message id (tray crashed)
            sendTrayMessage(NIM_ADD);
        break;
    }
    return false;
}

#ifndef QT_NO_DEBUG_STREAM

void QWindowsSystemTrayIcon::formatDebug(QDebug &d) const
{
    d << static_cast<const void *>(this) << ", \"" << m_toolTip
        << "\", hwnd=" << m_hwnd << ", m_hIcon=" << m_hIcon << ", menu="
        << m_menu.data();
}

QDebug operator<<(QDebug d, const QWindowsSystemTrayIcon *t)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "QWindowsSystemTrayIcon(";
    if (t)
        t->formatDebug(d);
    else
        d << '0';
    d << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
