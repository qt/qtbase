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

#include "qwindowsscreen.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowscursor.h"
#include "qwindowstheme.h"

#include <QtCore/qt_windows.h>

#include <QtCore/qsettings.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qwindowsfontdatabase_p.h>
#include <QtGui/qscreen.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

static inline QDpi deviceDPI(HDC hdc)
{
    return QDpi(GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY));
}

static inline QDpi monitorDPI(HMONITOR hMonitor)
{
    if (QWindowsContext::shcoredll.isValid()) {
        UINT dpiX;
        UINT dpiY;
        if (SUCCEEDED(QWindowsContext::shcoredll.getDpiForMonitor(hMonitor, 0, &dpiX, &dpiY)))
            return QDpi(dpiX, dpiY);
    }
    return {0, 0};
}

using WindowsScreenDataList = QVector<QWindowsScreenData>;

static bool monitorData(HMONITOR hMonitor, QWindowsScreenData *data)
{
    MONITORINFOEX info;
    memset(&info, 0, sizeof(MONITORINFOEX));
    info.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &info) == FALSE)
        return false;

    data->hMonitor = hMonitor;
    data->geometry = QRect(QPoint(info.rcMonitor.left, info.rcMonitor.top), QPoint(info.rcMonitor.right - 1, info.rcMonitor.bottom - 1));
    data->availableGeometry = QRect(QPoint(info.rcWork.left, info.rcWork.top), QPoint(info.rcWork.right - 1, info.rcWork.bottom - 1));
    data->name = QString::fromWCharArray(info.szDevice);
    if (data->name == u"WinDisc") {
        data->flags |= QWindowsScreenData::LockScreen;
    } else {
        if (const HDC hdc = CreateDC(info.szDevice, nullptr, nullptr, nullptr)) {
            const QDpi dpi = monitorDPI(hMonitor);
            data->dpi = dpi.first > 0 ? dpi : deviceDPI(hdc);
            data->depth = GetDeviceCaps(hdc, BITSPIXEL);
            data->format = data->depth == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
            data->physicalSizeMM = QSizeF(GetDeviceCaps(hdc, HORZSIZE), GetDeviceCaps(hdc, VERTSIZE));
            const int refreshRate = GetDeviceCaps(hdc, VREFRESH);
            if (refreshRate > 1) // 0,1 means hardware default.
                data->refreshRateHz = refreshRate;
            DeleteDC(hdc);
        } else {
            qWarning("%s: Unable to obtain handle for monitor '%s', defaulting to %g DPI.",
                     __FUNCTION__, qPrintable(QString::fromWCharArray(info.szDevice)),
                     data->dpi.first);
        } // CreateDC() failed
    } // not lock screen
    data->orientation = data->geometry.height() > data->geometry.width() ?
                       Qt::PortraitOrientation : Qt::LandscapeOrientation;
    // EnumDisplayMonitors (as opposed to EnumDisplayDevices) enumerates only
    // virtual desktop screens.
    data->flags |= QWindowsScreenData::VirtualDesktop;
    if (info.dwFlags & MONITORINFOF_PRIMARY) {
        data->flags |= QWindowsScreenData::PrimaryScreen;
        if ((data->flags & QWindowsScreenData::LockScreen) == 0)
            QWindowsFontDatabase::setDefaultVerticalDPI(data->dpi.second);
    }
    return true;
}

// from QDesktopWidget, taking WindowsScreenDataList as LPARAM
BOOL QT_WIN_CALLBACK monitorEnumCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM p)
{
    QWindowsScreenData data;
    if (monitorData(hMonitor, &data)) {
        auto *result = reinterpret_cast<WindowsScreenDataList *>(p);
        // QWindowSystemInterface::handleScreenAdded() documentation specifies that first
        // added screen will be the primary screen, so order accordingly.
        // Note that the side effect of this policy is that there is no way to change primary
        // screen reported by Qt, unless we want to delete all existing screens and add them
        // again whenever primary screen changes.
        if (data.flags & QWindowsScreenData::PrimaryScreen)
            result->prepend(data);
        else
            result->append(data);
    }
    return TRUE;
}

static inline WindowsScreenDataList monitorData()
{
    WindowsScreenDataList result;
    EnumDisplayMonitors(nullptr, nullptr, monitorEnumCallback, reinterpret_cast<LPARAM>(&result));
    return result;
}

#ifndef QT_NO_DEBUG_STREAM
static QDebug operator<<(QDebug dbg, const QWindowsScreenData &d)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg.noquote();
    dbg << "Screen \"" << d.name << "\" "
        << d.geometry.width() << 'x' << d.geometry.height() << '+' << d.geometry.x() << '+' << d.geometry.y()
        << " avail: "
        << d.availableGeometry.width() << 'x' << d.availableGeometry.height() << '+' << d.availableGeometry.x() << '+' << d.availableGeometry.y()
        << " physical: " << d.physicalSizeMM.width() << 'x' << d.physicalSizeMM.height()
        << " DPI: " << d.dpi.first << 'x' << d.dpi.second << " Depth: " << d.depth
        << " Format: " << d.format
        << " hMonitor: " << d.hMonitor;
    if (d.flags & QWindowsScreenData::PrimaryScreen)
        dbg << " primary";
    if (d.flags & QWindowsScreenData::VirtualDesktop)
        dbg << " virtual desktop";
    if (d.flags & QWindowsScreenData::LockScreen)
        dbg << " lock screen";
    return dbg;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QWindowsScreen
    \brief Windows screen.
    \sa QWindowsScreenManager
    \internal
*/

QWindowsScreen::QWindowsScreen(const QWindowsScreenData &data) :
    m_data(data)
#ifndef QT_NO_CURSOR
    , m_cursor(new QWindowsCursor(this))
#endif
{
}

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);

QPixmap QWindowsScreen::grabWindow(WId window, int xIn, int yIn, int width, int height) const
{
    QSize windowSize;
    int x = xIn;
    int y = yIn;
    HWND hwnd = reinterpret_cast<HWND>(window);
    if (hwnd) {
        RECT r;
        GetClientRect(hwnd, &r);
        windowSize = QSize(r.right - r.left, r.bottom - r.top);
    } else {
        // Grab current screen. The client rectangle of GetDesktopWindow() is the
        // primary screen, but it is possible to grab other screens from it.
        hwnd = GetDesktopWindow();
        const QRect screenGeometry = geometry();
        windowSize = screenGeometry.size();
        x += screenGeometry.x();
        y += screenGeometry.y();
    }

    if (width < 0)
        width = windowSize.width() - xIn;
    if (height < 0)
        height = windowSize.height() - yIn;

    // Create and setup bitmap
    HDC display_dc = GetDC(nullptr);
    HDC bitmap_dc = CreateCompatibleDC(display_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(display_dc, width, height);
    HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    // copy data
    HDC window_dc = GetDC(hwnd);
    BitBlt(bitmap_dc, 0, 0, width, height, window_dc, x, y, SRCCOPY | CAPTUREBLT);

    // clean up all but bitmap
    ReleaseDC(hwnd, window_dc);
    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    const QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap);

    DeleteObject(bitmap);
    ReleaseDC(nullptr, display_dc);

    return pixmap;
}

/*!
    \brief Find a top level window taking the flags of ChildWindowFromPointEx.
*/

QWindow *QWindowsScreen::topLevelAt(const QPoint &point) const
{
    QWindow *result = nullptr;
    if (QWindow *child = QWindowsScreen::windowAt(point, CWP_SKIPINVISIBLE))
        result = QWindowsWindow::topLevelOf(child);
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaWindows) <<__FUNCTION__ << point << result;
    return result;
}

QWindow *QWindowsScreen::windowAt(const QPoint &screenPoint, unsigned flags)
{
    QWindow* result = nullptr;
    if (QPlatformWindow *bw = QWindowsContext::instance()->
            findPlatformWindowAt(GetDesktopWindow(), screenPoint, flags))
        result = bw->window();
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaWindows) <<__FUNCTION__ << screenPoint << " returns " << result;
    return result;
}

/*!
    \brief Determine siblings in a virtual desktop system.

    Self is by definition a sibling, else collect all screens
    within virtual desktop.
*/

QList<QPlatformScreen *> QWindowsScreen::virtualSiblings() const
{
    QList<QPlatformScreen *> result;
    if (m_data.flags & QWindowsScreenData::VirtualDesktop) {
        const QWindowsScreenManager::WindowsScreenList screens
            = QWindowsContext::instance()->screenManager().screens();
        for (QWindowsScreen *screen : screens) {
            if (screen->data().flags & QWindowsScreenData::VirtualDesktop)
                result.push_back(screen);
        }
    } else {
        result.push_back(const_cast<QWindowsScreen *>(this));
    }
    return result;
}

/*!
    \brief Notify QWindowSystemInterface about changes of a screen and synchronize data.
*/

void QWindowsScreen::handleChanges(const QWindowsScreenData &newData)
{
    m_data.physicalSizeMM = newData.physicalSizeMM;

    if (m_data.hMonitor != newData.hMonitor) {
        qCDebug(lcQpaWindows) << "Monitor" << m_data.name
            << "has had its hMonitor handle changed from"
            << m_data.hMonitor << "to" << newData.hMonitor;
        m_data.hMonitor = newData.hMonitor;
    }

    // QGuiApplicationPrivate::processScreenGeometryChange() checks and emits
    // DPI and orientation as well, so, assign new values and emit DPI first.
    const bool geometryChanged = m_data.geometry != newData.geometry
        || m_data.availableGeometry != newData.availableGeometry;
    const bool dpiChanged = !qFuzzyCompare(m_data.dpi.first, newData.dpi.first)
        || !qFuzzyCompare(m_data.dpi.second, newData.dpi.second);
    const bool orientationChanged = m_data.orientation != newData.orientation;
    m_data.dpi = newData.dpi;
    m_data.orientation = newData.orientation;
    m_data.geometry = newData.geometry;
    m_data.availableGeometry = newData.availableGeometry;

    if (dpiChanged) {
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(),
                                                                     newData.dpi.first,
                                                                     newData.dpi.second);
    }
    if (orientationChanged)
       QWindowSystemInterface::handleScreenOrientationChange(screen(), newData.orientation);
    if (geometryChanged) {
        QWindowSystemInterface::handleScreenGeometryChange(screen(),
                                                           newData.geometry, newData.availableGeometry);
    }
}

HMONITOR QWindowsScreen::handle() const
{
    return m_data.hMonitor;
}

QRect QWindowsScreen::virtualGeometry(const QPlatformScreen *screen) // cf QScreen::virtualGeometry()
{
    QRect result;
    const auto siblings = screen->virtualSiblings();
    for (const QPlatformScreen *sibling : siblings)
        result |= sibling->geometry();
    return result;
}

enum OrientationPreference : DWORD // matching Win32 API ORIENTATION_PREFERENCE
{
    orientationPreferenceNone = 0,
    orientationPreferenceLandscape = 0x1,
    orientationPreferencePortrait = 0x2,
    orientationPreferenceLandscapeFlipped = 0x4,
    orientationPreferencePortraitFlipped = 0x8
};

bool QWindowsScreen::setOrientationPreference(Qt::ScreenOrientation o)
{
    bool result = false;
    if (QWindowsContext::user32dll.setDisplayAutoRotationPreferences) {
        DWORD orientationPreference = 0;
        switch (o) {
        case Qt::PrimaryOrientation:
            orientationPreference = orientationPreferenceNone;
            break;
        case Qt::PortraitOrientation:
            orientationPreference = orientationPreferencePortrait;
            break;
        case Qt::LandscapeOrientation:
            orientationPreference = orientationPreferenceLandscape;
            break;
        case Qt::InvertedPortraitOrientation:
            orientationPreference = orientationPreferencePortraitFlipped;
            break;
        case Qt::InvertedLandscapeOrientation:
            orientationPreference = orientationPreferenceLandscapeFlipped;
            break;
        }
        result = QWindowsContext::user32dll.setDisplayAutoRotationPreferences(orientationPreference);
    }
    return result;
}

Qt::ScreenOrientation QWindowsScreen::orientationPreference()
{
    Qt::ScreenOrientation result = Qt::PrimaryOrientation;
    if (QWindowsContext::user32dll.getDisplayAutoRotationPreferences) {
        DWORD orientationPreference = 0;
        if (QWindowsContext::user32dll.getDisplayAutoRotationPreferences(&orientationPreference)) {
            switch (orientationPreference) {
            case orientationPreferenceLandscape:
                result = Qt::LandscapeOrientation;
                break;
            case orientationPreferencePortrait:
                result = Qt::PortraitOrientation;
                break;
            case orientationPreferenceLandscapeFlipped:
                result = Qt::InvertedLandscapeOrientation;
                break;
            case orientationPreferencePortraitFlipped:
                result = Qt::InvertedPortraitOrientation;
                break;
            }
        }
    }
    return result;
}

/*!
    \brief Queries ClearType settings to check the pixel layout
*/
QPlatformScreen::SubpixelAntialiasingType QWindowsScreen::subpixelAntialiasingTypeHint() const
{
    QPlatformScreen::SubpixelAntialiasingType type = QPlatformScreen::subpixelAntialiasingTypeHint();
    if (type == QPlatformScreen::Subpixel_None) {
        QSettings settings(QLatin1String(R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Avalon.Graphics\DISPLAY1)"),
                           QSettings::NativeFormat);
        int registryValue = settings.value(QLatin1String("PixelStructure"), -1).toInt();
        switch (registryValue) {
        case 0:
            type = QPlatformScreen::Subpixel_None;
            break;
        case 1:
            type = QPlatformScreen::Subpixel_RGB;
            break;
        case 2:
            type = QPlatformScreen::Subpixel_BGR;
            break;
        default:
            type = QPlatformScreen::Subpixel_None;
            break;
        }
    }
    return type;
}

/*!
    \class QWindowsScreenManager
    \brief Manages a list of QWindowsScreen.

    Listens for changes and notifies QWindowSystemInterface about changed/
    added/deleted screens.

    \sa QWindowsScreen
    \internal
*/

QWindowsScreenManager::QWindowsScreenManager() = default;


bool QWindowsScreenManager::isSingleScreen()
{
    return QWindowsContext::instance()->screenManager().screens().size() < 2;
}

/*!
    \brief Triggers synchronization of screens (WM_DISPLAYCHANGE).

    Subsequent events are compressed since WM_DISPLAYCHANGE is sent to
    each top level window.
*/

bool QWindowsScreenManager::handleDisplayChange(WPARAM wParam, LPARAM lParam)
{
    const int newDepth = int(wParam);
    const WORD newHorizontalResolution = LOWORD(lParam);
    const WORD newVerticalResolution = HIWORD(lParam);
    if (newDepth != m_lastDepth || newHorizontalResolution != m_lastHorizontalResolution
        || newVerticalResolution != m_lastVerticalResolution) {
        m_lastDepth = newDepth;
        m_lastHorizontalResolution = newHorizontalResolution;
        m_lastVerticalResolution = newVerticalResolution;
        qCDebug(lcQpaWindows) << __FUNCTION__ << "Depth=" << newDepth
            << ", resolution " << newHorizontalResolution << 'x' << newVerticalResolution;
        handleScreenChanges();
    }
    return false;
}

static inline int indexOfMonitor(const QWindowsScreenManager::WindowsScreenList &screens,
                                 const QString &monitorName)
{
    for (int i= 0; i < screens.size(); ++i)
        if (screens.at(i)->data().name == monitorName)
            return i;
    return -1;
}

static inline int indexOfMonitor(const WindowsScreenDataList &screenData,
                                 const QString &monitorName)
{
    for (int i = 0; i < screenData.size(); ++i)
        if (screenData.at(i).name == monitorName)
            return i;
    return -1;
}

// Move a window to a new virtual screen, accounting for varying sizes.
static void moveToVirtualScreen(QWindow *w, const QScreen *newScreen)
{
    QRect geometry = w->geometry();
    const QRect oldScreenGeometry = w->screen()->geometry();
    const QRect newScreenGeometry = newScreen->geometry();
    QPoint relativePosition = geometry.topLeft() - oldScreenGeometry.topLeft();
    if (oldScreenGeometry.size() != newScreenGeometry.size()) {
        const qreal factor =
            qreal(QPoint(newScreenGeometry.width(), newScreenGeometry.height()).manhattanLength()) /
            qreal(QPoint(oldScreenGeometry.width(), oldScreenGeometry.height()).manhattanLength());
        relativePosition = (QPointF(relativePosition) * factor).toPoint();
    }
    geometry.moveTopLeft(relativePosition);
    w->setGeometry(geometry);
}

void QWindowsScreenManager::removeScreen(int index)
{
    qCDebug(lcQpaWindows) << "Removing Monitor:" << m_screens.at(index)->data();
    QScreen *screen = m_screens.at(index)->screen();
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    // QTBUG-38650: When a screen is disconnected, Windows will automatically
    // move the Window to another screen. This will trigger a geometry change
    // event, but unfortunately after the screen destruction signal. To prevent
    // QtGui from automatically hiding the QWindow, pretend all Windows move to
    // the primary screen first (which is likely the correct, final screen).
    // QTBUG-39320: Windows does not automatically move WS_EX_TOOLWINDOW (dock) windows;
    // move those manually.
    if (screen != primaryScreen) {
        unsigned movedWindowCount = 0;
        const QWindowList tlws = QGuiApplication::topLevelWindows();
        for (QWindow *w : tlws) {
            if (w->screen() == screen && w->handle() && w->type() != Qt::Desktop) {
                if (w->isVisible() && w->windowState() != Qt::WindowMinimized
                    && (QWindowsWindow::baseWindowOf(w)->exStyle() & WS_EX_TOOLWINDOW)) {
                    moveToVirtualScreen(w, primaryScreen);
                } else {
                    QWindowSystemInterface::handleWindowScreenChanged(w, primaryScreen);
                }
                ++movedWindowCount;
            }
        }
        if (movedWindowCount)
            QWindowSystemInterface::flushWindowSystemEvents();
    }
    QWindowSystemInterface::handleScreenRemoved(m_screens.takeAt(index));
}

/*!
    \brief Synchronizes the screen list, adds new screens, removes deleted
    ones and propagates resolution changes to QWindowSystemInterface.
*/

bool QWindowsScreenManager::handleScreenChanges()
{
    // Look for changed monitors, add new ones
    const WindowsScreenDataList newDataList = monitorData();
    const bool lockScreen = newDataList.size() == 1 && (newDataList.front().flags & QWindowsScreenData::LockScreen);
    bool primaryScreenChanged = false;
    for (const QWindowsScreenData &newData : newDataList) {
        const int existingIndex = indexOfMonitor(m_screens, newData.name);
        if (existingIndex != -1) {
            m_screens.at(existingIndex)->handleChanges(newData);
            if (existingIndex == 0)
                primaryScreenChanged = true;
        } else {
            auto *newScreen = new QWindowsScreen(newData);
            m_screens.push_back(newScreen);
            QWindowSystemInterface::handleScreenAdded(newScreen,
                                                             newData.flags & QWindowsScreenData::PrimaryScreen);
            qCDebug(lcQpaWindows) << "New Monitor: " << newData;
        }    // exists
    }        // for new screens.
    // Remove deleted ones but keep main monitors if we get only the
    // temporary lock screen to avoid window recreation (QTBUG-33062).
    if (!lockScreen) {
        for (int i = m_screens.size() - 1; i >= 0; --i) {
            if (indexOfMonitor(newDataList, m_screens.at(i)->data().name) == -1)
                removeScreen(i);
        }     // for existing screens
    }     // not lock screen
    if (primaryScreenChanged) {
        if (auto theme = QWindowsTheme::instance()) // QTBUG-85734/Wine
            theme->refreshFonts();
    }
    return true;
}

void QWindowsScreenManager::clearScreens()
{
    // Delete screens in reverse order to avoid crash in case of multiple screens
    while (!m_screens.isEmpty())
        QWindowSystemInterface::handleScreenRemoved(m_screens.takeLast());
}

const QWindowsScreen *QWindowsScreenManager::screenAtDp(const QPoint &p) const
{
    for (QWindowsScreen *scr : m_screens) {
        if (scr->geometry().contains(p))
            return scr;
    }
    return nullptr;
}

const QWindowsScreen *QWindowsScreenManager::screenForHwnd(HWND hwnd) const
{
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    if (hMonitor == nullptr)
        return nullptr;
    const auto it =
        std::find_if(m_screens.cbegin(), m_screens.cend(),
                     [hMonitor](const QWindowsScreen *s)
                     {
                         return s->data().hMonitor == hMonitor
                             && (s->data().flags & QWindowsScreenData::VirtualDesktop) != 0;
                     });
    return it != m_screens.cend() ? *it : nullptr;
}

QT_END_NAMESPACE
