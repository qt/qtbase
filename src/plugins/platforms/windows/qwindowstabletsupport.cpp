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

#include "qwindowstabletsupport.h"

#include "qwindowscontext.h"
#include "qwindowskeymapper.h"
#include "qwindowswindow.h"
#include "qwindowsscreen.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtGui/qevent.h>
#include <QtGui/qscreen.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qmath.h>

#include <private/qguiapplication_p.h>
#include <QtCore/private/qsystemlibrary_p.h>

// Note: The definition of the PACKET structure in pktdef.h depends on this define.
#define PACKETDATA (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE | PK_ORIENTATION | PK_CURSOR | PK_Z | PK_TIME)
#include <pktdef.h>

QT_BEGIN_NAMESPACE

enum {
    PacketMode = 0,
    TabletPacketQSize = 128,
    DeviceIdMask = 0xFF6, // device type mask && device color mask
    CursorTypeBitMask = 0x0F06 // bitmask to find the specific cursor type (see Wacom FAQ)
};

extern "C" LRESULT QT_WIN_CALLBACK qWindowsTabletSupportWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WT_PROXIMITY:
        if (QWindowsContext::instance()->tabletSupport()->translateTabletProximityEvent(wParam, lParam))
            return 0;
        break;
    case WT_PACKET:
        if (QWindowsContext::instance()->tabletSupport()->translateTabletPacketEvent())
            return 0;
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


// Scale tablet coordinates to screen coordinates.

static inline int sign(int x)
{
    return x >= 0 ? 1 : -1;
}

inline QPointF QWindowsTabletDeviceData::scaleCoordinates(int coordX, int coordY, const QRect &targetArea) const
{
    const int targetX = targetArea.x();
    const int targetY = targetArea.y();
    const int targetWidth = targetArea.width();
    const int targetHeight = targetArea.height();

    const qreal x = sign(targetWidth) == sign(maxX) ?
        ((coordX - minX) * qAbs(targetWidth) / qAbs(qreal(maxX - minX))) + targetX :
        ((qAbs(maxX) - (coordX - minX)) * qAbs(targetWidth) / qAbs(qreal(maxX - minX))) + targetX;

    const qreal y = sign(targetHeight) == sign(maxY) ?
        ((coordY - minY) * qAbs(targetHeight) / qAbs(qreal(maxY - minY))) + targetY :
        ((qAbs(maxY) - (coordY - minY)) * qAbs(targetHeight) / qAbs(qreal(maxY - minY))) + targetY;

    return {x, y};
}

template <class Stream>
static void formatOptions(Stream &str, unsigned options)
{
    if (options & CXO_SYSTEM)
        str << " CXO_SYSTEM";
    if (options & CXO_PEN)
        str << " CXO_PEN";
    if (options & CXO_MESSAGES)
        str << " CXO_MESSAGES";
    if (options & CXO_MARGIN)
        str << " CXO_MARGIN";
    if (options & CXO_MGNINSIDE)
        str << " CXO_MGNINSIDE";
    if (options & CXO_CSRMESSAGES)
        str << " CXO_CSRMESSAGES";
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QWindowsTabletDeviceData &t)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "TabletDevice id:" << t.uniqueId << " pressure: " << t.minPressure
      << ".." << t.maxPressure << " tan pressure: " << t.minTanPressure << ".."
      << t.maxTanPressure << " area: (" << t.minX << ',' << t.minY << ',' << t.minZ
      << ")..(" << t.maxX << ',' << t.maxY << ',' << t.maxZ << ") device "
      << t.currentDevice << " pointer " << t.currentPointerType;
    return d;
}

QDebug operator<<(QDebug d, const LOGCONTEXT &lc)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "LOGCONTEXT(\"" << QString::fromWCharArray(lc.lcName) << "\", options=0x"
        << Qt::hex << lc.lcOptions << Qt::dec;
    formatOptions(d, lc.lcOptions);
    d << ", status=0x" << Qt::hex << lc.lcStatus << ", device=0x" << lc.lcDevice
        << Qt::dec << ", PktRate=" << lc.lcPktRate
        << ", PktData=" << lc.lcPktData << ", PktMode=" << lc.lcPktMode
        << ", MoveMask=0x" << Qt::hex << lc.lcMoveMask << ", BtnDnMask=0x" << lc.lcBtnDnMask
        << ", BtnUpMask=0x" << lc.lcBtnUpMask << Qt::dec << ", SysMode=" << lc.lcSysMode
        << ", InOrg=(" << lc.lcInOrgX << ", " << lc.lcInOrgY << ", " << lc.lcInOrgZ
        <<  "), InExt=(" << lc.lcInExtX << ", " << lc.lcInExtY << ", " << lc.lcInExtZ
        << ") OutOrg=(" << lc.lcOutOrgX << ", " << lc.lcOutOrgY << ", "
        << lc.lcOutOrgZ <<  "), OutExt=(" << lc.lcOutExtX << ", " << lc.lcOutExtY
        << ", " << lc.lcOutExtZ
        << "), Sens=(" << lc.lcSensX << ", " << lc.lcSensX << ", " << lc.lcSensZ
        << ") SysOrg=(" << lc.lcSysOrgX << ", " << lc.lcSysOrgY
        << "), SysExt=(" << lc.lcSysExtX << ", " << lc.lcSysExtY
        << "), SysSens=(" << lc.lcSysSensX << ", " << lc.lcSysSensY << "))";
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

QWindowsWinTab32DLL QWindowsTabletSupport::m_winTab32DLL;

/*!
    \class QWindowsWinTab32DLL QWindowsTabletSupport
    \brief Functions from wintabl32.dll shipped with WACOM tablets used by QWindowsTabletSupport.

    \internal
*/

bool QWindowsWinTab32DLL::init()
{
    if (wTInfo)
        return true;
    QSystemLibrary library(QStringLiteral("wintab32"));
    if (!library.load())
        return false;
    wTOpen = (PtrWTOpen)library.resolve("WTOpenW");
    wTClose = (PtrWTClose)library.resolve("WTClose");
    wTInfo = (PtrWTInfo)library.resolve("WTInfoW");
    wTEnable = (PtrWTEnable)library.resolve("WTEnable");
    wTOverlap = (PtrWTEnable)library.resolve("WTOverlap");
    wTPacketsGet = (PtrWTPacketsGet)library.resolve("WTPacketsGet");
    wTGet = (PtrWTGet)library.resolve("WTGetW");
    wTQueueSizeGet = (PtrWTQueueSizeGet)library.resolve("WTQueueSizeGet");
    wTQueueSizeSet = (PtrWTQueueSizeSet)library.resolve("WTQueueSizeSet");
    return wTOpen && wTClose && wTInfo && wTEnable && wTOverlap && wTPacketsGet && wTQueueSizeGet && wTQueueSizeSet;
}

/*!
    \class QWindowsTabletSupport
    \brief Tablet support for Windows.

    Support for WACOM tablets.

    \sa http://www.wacomeng.com/windows/docs/Wintab_v140.htm

    \internal
    \since 5.2
*/

QWindowsTabletSupport::QWindowsTabletSupport(HWND window, HCTX context)
    : m_window(window)
    , m_context(context)
{
     AXIS orientation[3];
    // Some tablets don't support tilt, check if it is possible,
    if (QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEVICES, DVC_ORIENTATION, &orientation))
        m_tiltSupport = orientation[0].axResolution && orientation[1].axResolution;
}

QWindowsTabletSupport::~QWindowsTabletSupport()
{
    QWindowsTabletSupport::m_winTab32DLL.wTClose(m_context);
    DestroyWindow(m_window);
}

QWindowsTabletSupport *QWindowsTabletSupport::create()
{
    if (!m_winTab32DLL.init())
        return nullptr;
    const HWND window = QWindowsContext::instance()->createDummyWindow(QStringLiteral("TabletDummyWindow"),
                                                                       L"TabletDummyWindow",
                                                                       qWindowsTabletSupportWndProc);
    if (!window) {
        qCWarning(lcQpaTablet) << __FUNCTION__ << "Unable to create window for tablet.";
        return nullptr;
    }
    LOGCONTEXT lcMine;
    // build our context from the default context
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEFSYSCTX, 0, &lcMine);
    qCDebug(lcQpaTablet) << "Default: " << lcMine;
    // Go for the raw coordinates, the tablet event will return good stuff
    lcMine.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
    lcMine.lcPktData = lcMine.lcMoveMask = PACKETDATA;
    lcMine.lcPktMode = PacketMode;
    lcMine.lcOutOrgX = 0;
    lcMine.lcOutExtX = lcMine.lcInExtX;
    lcMine.lcOutOrgY = 0;
    lcMine.lcOutExtY = -lcMine.lcInExtY;
    qCDebug(lcQpaTablet) << "Requesting: " << lcMine;
    const HCTX context = QWindowsTabletSupport::m_winTab32DLL.wTOpen(window, &lcMine, true);
    if (!context) {
        qCDebug(lcQpaTablet) << __FUNCTION__ << "Unable to open tablet.";
        DestroyWindow(window);
        return nullptr;

    }
    // Set the size of the Packet Queue to the correct size
    const int currentQueueSize = QWindowsTabletSupport::m_winTab32DLL.wTQueueSizeGet(context);
    if (currentQueueSize != TabletPacketQSize) {
        if (!QWindowsTabletSupport::m_winTab32DLL.wTQueueSizeSet(context, TabletPacketQSize)) {
            if (!QWindowsTabletSupport::m_winTab32DLL.wTQueueSizeSet(context, currentQueueSize))  {
                qWarning("Unable to set queue size on tablet. The tablet will not work.");
                QWindowsTabletSupport::m_winTab32DLL.wTClose(context);
                DestroyWindow(window);
                return nullptr;
            } // cannot restore old size
        } // cannot set
    } // mismatch
    qCDebug(lcQpaTablet) << "Opened tablet context " << context << " on window "
        <<  window << "changed packet queue size " << currentQueueSize
        << "->" <<  TabletPacketQSize << "\nobtained: " << lcMine;
    return new QWindowsTabletSupport(window, context);
}

unsigned QWindowsTabletSupport::options() const
{
    UINT result = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_CTXOPTIONS, &result);
    return result;
}

QString QWindowsTabletSupport::description() const
{
    const unsigned size = m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_WINTABID, nullptr);
    if (!size)
        return QString();
    QVarLengthArray<TCHAR> winTabId(size + 1);
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_WINTABID, winTabId.data());
    WORD implementationVersion = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_IMPLVERSION, &implementationVersion);
    WORD specificationVersion = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_SPECVERSION, &specificationVersion);
    const unsigned opts = options();
    WORD devices = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_NDEVICES, &devices);
    WORD cursors = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_NCURSORS, &cursors);
    WORD extensions = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_NEXTENSIONS, &extensions);
    QString result;
    QTextStream str(&result);
    str << '"' << QString::fromWCharArray(winTabId.data())
        << "\" specification: v" << (specificationVersion >> 8)
        << '.' << (specificationVersion & 0xFF) << " implementation: v"
        << (implementationVersion >> 8) << '.' << (implementationVersion & 0xFF)
        << ' ' << devices << " device(s), " << cursors << " cursor(s), "
        << extensions << " extensions" << ", options: 0x" << Qt::hex << opts << Qt::dec;
    formatOptions(str, opts);
    if (m_tiltSupport)
        str << " tilt";
    return result;
}

void QWindowsTabletSupport::notifyActivate()
{
    // Cooperate with other tablet applications, but when we get focus, I want to use the tablet.
    const bool result = QWindowsTabletSupport::m_winTab32DLL.wTEnable(m_context, true)
        && QWindowsTabletSupport::m_winTab32DLL.wTOverlap(m_context, true);
   qCDebug(lcQpaTablet) << __FUNCTION__ << result;
}

static inline int indexOfDevice(const QVector<QWindowsTabletDeviceData> &devices, qint64 uniqueId)
{
    for (int i = 0; i < devices.size(); ++i)
        if (devices.at(i).uniqueId == uniqueId)
            return i;
    return -1;
}

static inline QTabletEvent::TabletDevice deviceType(const UINT cursorType)
{
    if (((cursorType & 0x0006) == 0x0002) && ((cursorType & CursorTypeBitMask) != 0x0902))
        return QTabletEvent::Stylus;
    if (cursorType == 0x4020) // Surface Pro 2 tablet device
        return QTabletEvent::Stylus;
    switch (cursorType & CursorTypeBitMask) {
    case 0x0802:
        return QTabletEvent::Stylus;
    case 0x0902:
        return QTabletEvent::Airbrush;
    case 0x0004:
        return QTabletEvent::FourDMouse;
    case 0x0006:
        return QTabletEvent::Puck;
    case 0x0804:
        return QTabletEvent::RotationStylus;
    default:
        break;
    }
    return QTabletEvent::NoDevice;
}

static inline QTabletEvent::PointerType pointerType(unsigned currentCursor)
{
    switch (currentCursor % 3) { // %3 for dual track
    case 0:
        return QTabletEvent::Cursor;
    case 1:
        return QTabletEvent::Pen;
    case 2:
        return QTabletEvent::Eraser;
    default:
        break;
    }
    return QTabletEvent::UnknownPointer;
}

QWindowsTabletDeviceData QWindowsTabletSupport::tabletInit(qint64 uniqueId, UINT cursorType) const
{
    QWindowsTabletDeviceData result;
    result.uniqueId = uniqueId;
    /* browse WinTab's many info items to discover pressure handling. */
    AXIS axis;
    LOGCONTEXT lc;
    /* get the current context for its device variable. */
    QWindowsTabletSupport::m_winTab32DLL.wTGet(m_context, &lc);
    /* get the size of the pressure axis. */
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEVICES + lc.lcDevice, DVC_NPRESSURE, &axis);
    result.minPressure = int(axis.axMin);
    result.maxPressure = int(axis.axMax);

    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEVICES + lc.lcDevice, DVC_TPRESSURE, &axis);
    result.minTanPressure = int(axis.axMin);
    result.maxTanPressure = int(axis.axMax);

    LOGCONTEXT defaultLc;
    /* get default region */
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEFCONTEXT, 0, &defaultLc);
    result.maxX = int(defaultLc.lcInExtX) - int(defaultLc.lcInOrgX);
    result.maxY = int(defaultLc.lcInExtY) - int(defaultLc.lcInOrgY);
    result.maxZ = int(defaultLc.lcInExtZ) - int(defaultLc.lcInOrgZ);
    result.currentDevice = deviceType(cursorType);
    return result;
}

bool QWindowsTabletSupport::translateTabletProximityEvent(WPARAM /* wParam */, LPARAM lParam)
{
    PACKET proximityBuffer[1]; // we are only interested in the first packet in this case
    const int totalPacks = QWindowsTabletSupport::m_winTab32DLL.wTPacketsGet(m_context, 1, proximityBuffer);

    if (!LOWORD(lParam)) {
        qCDebug(lcQpaTablet) << "leave proximity for device #" << m_currentDevice;
        if (m_currentDevice < 0 || m_currentDevice >= m_devices.size()) // QTBUG-65120, spurious leave observed
            return false;
        m_state = PenUp;
        if (totalPacks > 0) {
            QWindowSystemInterface::handleTabletLeaveProximityEvent(proximityBuffer[0].pkTime,
                                                                    m_devices.at(m_currentDevice).currentDevice,
                                                                    m_devices.at(m_currentDevice).currentPointerType,
                                                                    m_devices.at(m_currentDevice).uniqueId);
        } else {
            QWindowSystemInterface::handleTabletLeaveProximityEvent(m_devices.at(m_currentDevice).currentDevice,
                                                                    m_devices.at(m_currentDevice).currentPointerType,
                                                                    m_devices.at(m_currentDevice).uniqueId);

        }
        return true;
    }

    if (!totalPacks)
        return false;

    const UINT currentCursor = proximityBuffer[0].pkCursor;
    UINT physicalCursorId;
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_CURSORS + currentCursor, CSR_PHYSID, &physicalCursorId);
    UINT cursorType;
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_CURSORS + currentCursor, CSR_TYPE, &cursorType);
    const qint64 uniqueId = (qint64(cursorType & DeviceIdMask) << 32L) | qint64(physicalCursorId);
    // initializing and updating the cursor should be done in response to
    // WT_CSRCHANGE. We do it in WT_PROXIMITY because some wintab never send
    // the event WT_CSRCHANGE even if asked with CXO_CSRMESSAGES
    m_currentDevice = indexOfDevice(m_devices, uniqueId);
    if (m_currentDevice < 0) {
        m_currentDevice = m_devices.size();
        m_devices.push_back(tabletInit(uniqueId, cursorType));
    } else {
        // The user can switch pressure sensitivity level in the driver,which
        // will make our saved values invalid (this option is provided by Wacom
        // drivers for compatibility reasons, and it can be adjusted on the fly)
        m_devices[m_currentDevice] = tabletInit(uniqueId, cursorType);
    }

    /**
     * We should check button map for changes on every proximity event, not
     * only during initialization phase.
     *
     * WARNING: in 2016 there were some Wacom table drivers, which could mess up
     *          button mapping if the remapped button was pressed, while the
     *          application **didn't have input focus**. This bug is somehow
     *          related to the fact that Wacom drivers allow user to configure
     *          per-application button-mappings. If the bug shows up again,
     *          just move this button-map fetching into initialization block.
     *
     *          See https://bugs.kde.org/show_bug.cgi?id=359561
     */
    BYTE logicalButtons[32];
    memset(logicalButtons, 0, 32);
    m_winTab32DLL.wTInfo(WTI_CURSORS + currentCursor, CSR_SYSBTNMAP, &logicalButtons);
    m_devices[m_currentDevice].buttonsMap[0x1] = logicalButtons[0];
    m_devices[m_currentDevice].buttonsMap[0x2] = logicalButtons[1];
    m_devices[m_currentDevice].buttonsMap[0x4] = logicalButtons[2];

    m_devices[m_currentDevice].currentPointerType = pointerType(currentCursor);
    m_state = PenProximity;
    qCDebug(lcQpaTablet) << "enter proximity for device #"
        << m_currentDevice << m_devices.at(m_currentDevice);
    QWindowSystemInterface::handleTabletEnterProximityEvent(proximityBuffer[0].pkTime,
                                                            m_devices.at(m_currentDevice).currentDevice,
                                                            m_devices.at(m_currentDevice).currentPointerType,
                                                            m_devices.at(m_currentDevice).uniqueId);
    return true;
}

Qt::MouseButton buttonValueToEnum(DWORD button,
                                  const QWindowsTabletDeviceData &tdd) {

    enum : unsigned {
        leftButtonValue = 0x1,
        middleButtonValue = 0x2,
        rightButtonValue = 0x4,
        doubleClickButtonValue = 0x7
    };

    button = tdd.buttonsMap.value(button);

    return button == leftButtonValue ? Qt::LeftButton :
        button == rightButtonValue ? Qt::RightButton :
        button == doubleClickButtonValue ? Qt::MiddleButton :
        button == middleButtonValue ? Qt::MiddleButton :
        button ? Qt::LeftButton /* fallback item */ :
        Qt::NoButton;
}

Qt::MouseButtons convertTabletButtons(DWORD btnNew,
                                      const QWindowsTabletDeviceData &tdd) {

    Qt::MouseButtons buttons = Qt::NoButton;
    for (unsigned int i = 0; i < 3; i++) {
        unsigned int btn = 0x1 << i;

        if (btn & btnNew) {
            Qt::MouseButton convertedButton =
                buttonValueToEnum(btn, tdd);

            buttons |= convertedButton;

            /**
             * If a button that is present in hardware input is
             * mapped to a Qt::NoButton, it means that it is going
             * to be eaten by the driver, for example by its
             * "Pan/Scroll" feature. Therefore we shouldn't handle
             * any of the events associated to it. We'll just return
             * Qt::NoButtons here.
             */
        }
    }
    return buttons;
}

bool QWindowsTabletSupport::translateTabletPacketEvent()
{
    static PACKET localPacketBuf[TabletPacketQSize];  // our own tablet packet queue.
    const int packetCount = QWindowsTabletSupport::m_winTab32DLL.wTPacketsGet(m_context, TabletPacketQSize, &localPacketBuf);
    if (!packetCount || m_currentDevice < 0)
        return false;

    const int currentDevice = m_devices.at(m_currentDevice).currentDevice;
    const qint64 uniqueId = m_devices.at(m_currentDevice).uniqueId;

    // The tablet can be used in 2 different modes (reflected in enum Mode),
    // depending on its settings:
    // 1) Absolute (pen) mode:
    //    The coordinates are scaled to the virtual desktop (by default). The user
    //    can also choose to scale to the monitor or a region of the screen.
    //    When entering proximity, the tablet driver snaps the mouse pointer to the
    //    tablet position scaled to that area and keeps it in sync.
    // 2) Relative (mouse) mode:
    //    The pen follows the mouse. The constant 'absoluteRange' specifies the
    //    manhattanLength difference for detecting if a tablet input device is in this mode,
    //    in which case we snap the position to the mouse position.
    // It seems there is no way to find out the mode programmatically, the LOGCONTEXT orgX/Y/Ext
    // area is always the virtual desktop.
    const QRect virtualDesktopArea =
        QWindowsScreen::virtualGeometry(QGuiApplication::primaryScreen()->handle());

    if (QWindowsContext::verbose > 1)  {
        qCDebug(lcQpaTablet) << __FUNCTION__ << "processing" << packetCount
            << "mode=" << m_mode << "target:"
            << QGuiApplicationPrivate::tabletDevicePoint(uniqueId).target;
    }

    const Qt::KeyboardModifiers keyboardModifiers = QWindowsKeyMapper::queryKeyboardModifiers();

    for (int i = 0; i < packetCount ; ++i) {
        const PACKET &packet = localPacketBuf[i];

        const int z = currentDevice == QTabletEvent::FourDMouse ? int(packet.pkZ) : 0;

        const auto currentPointer = m_devices.at(m_currentDevice).currentPointerType;
        const auto packetPointerType = pointerType(packet.pkCursor);

        const Qt::MouseButtons buttons =
            convertTabletButtons(packet.pkButtons, m_devices.at(m_currentDevice));

        if (buttons == Qt::NoButton && packetPointerType != currentPointer) {

            QWindowSystemInterface::handleTabletLeaveProximityEvent(packet.pkTime,
                                                                    int(currentDevice),
                                                                    int(currentPointer),
                                                                    uniqueId);

            m_devices[m_currentDevice].currentPointerType = packetPointerType;

            QWindowSystemInterface::handleTabletEnterProximityEvent(packet.pkTime,
                                                                    int(currentDevice),
                                                                    int(packetPointerType),
                                                                    uniqueId);
        }

        QPointF globalPosF =
            m_devices.at(m_currentDevice).scaleCoordinates(packet.pkX, packet.pkY, virtualDesktopArea);

        QWindow *target = QGuiApplicationPrivate::tabletDevicePoint(uniqueId).target; // Pass to window that grabbed it.

        // Get Mouse Position and compare to tablet info
        const QPoint mouseLocation = QWindowsCursor::mousePosition();
        if (m_state == PenProximity) {
            m_state = PenDown;
            m_mode = (mouseLocation - globalPosF).manhattanLength() > m_absoluteRange
                ? MouseMode : PenMode;
            qCDebug(lcQpaTablet) << __FUNCTION__ << "mode=" << m_mode << "pen:"
                << globalPosF << "mouse:" << mouseLocation;
        }
        if (m_mode == MouseMode)
            globalPosF = mouseLocation;
        const QPoint globalPos = globalPosF.toPoint();

        if (!target)
            target = QWindowsScreen::windowAt(globalPos, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
        if (!target)
            continue;

        const QPlatformWindow *platformWindow = target->handle();
        Q_ASSERT(platformWindow);
        const QPoint localPos = platformWindow->mapFromGlobal(globalPos);

        const qreal pressureNew = packet.pkButtons && (currentPointer == QTabletEvent::Pen || currentPointer == QTabletEvent::Eraser) ?
            m_devices.at(m_currentDevice).scalePressure(packet.pkNormalPressure) :
            qreal(0);
        const qreal tangentialPressure = currentDevice == QTabletEvent::Airbrush ?
            m_devices.at(m_currentDevice).scaleTangentialPressure(packet.pkTangentPressure) :
            qreal(0);

        int tiltX = 0;
        int tiltY = 0;
        qreal rotation = 0;
        if (m_tiltSupport) {
            // Convert from azimuth and altitude to x tilt and y tilt. What
            // follows is the optimized version. Here are the equations used:
            // X = sin(azimuth) * cos(altitude)
            // Y = cos(azimuth) * cos(altitude)
            // Z = sin(altitude)
            // X Tilt = arctan(X / Z)
            // Y Tilt = arctan(Y / Z)
            const double radAzim = qDegreesToRadians(packet.pkOrientation.orAzimuth / 10.0);
            const double tanAlt = std::tan(qDegreesToRadians(std::abs(packet.pkOrientation.orAltitude / 10.0)));

            const double radX = std::atan(std::sin(radAzim) / tanAlt);
            const double radY = std::atan(std::cos(radAzim) / tanAlt);
            tiltX = int(qRadiansToDegrees(radX));
            tiltY = int(qRadiansToDegrees(-radY));
            rotation = 360.0 - (packet.pkOrientation.orTwist / 10.0);
            if (rotation > 180.0)
                rotation -= 360.0;
        }

        if (QWindowsContext::verbose > 1)  {
            qCDebug(lcQpaTablet)
                << "Packet #" << i << '/' << packetCount << "button:" << packet.pkButtons
                << globalPosF << z << "to:" << target << localPos << "(packet" << packet.pkX
                << packet.pkY << ") dev:" << currentDevice << "pointer:"
                << currentPointer << "P:" << pressureNew << "tilt:" << tiltX << ','
                << tiltY << "tanP:" << tangentialPressure << "rotation:" << rotation;
        }

        QWindowSystemInterface::handleTabletEvent(target, packet.pkTime, QPointF(localPos), globalPosF,
                                                  currentDevice, currentPointer,
                                                  buttons,
                                                  pressureNew, tiltX, tiltY,
                                                  tangentialPressure, rotation, z,
                                                  uniqueId,
                                                  keyboardModifiers);
    }
    return true;
}

QT_END_NAMESPACE
