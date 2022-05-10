// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSTABLETSUPPORT_H
#define QWINDOWSTABLETSUPPORT_H

#include "qtwindowsglobal.h"
#include <QtGui/qtguiglobal.h>
#include <QtGui/qpointingdevice.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsharedpointer.h>

#include <wintab.h>

QT_REQUIRE_CONFIG(tabletevent);

QT_BEGIN_NAMESPACE

class QDebug;
class QWindow;
class QRect;

struct QWindowsWinTab32DLL
{
    bool init();

    typedef HCTX (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
    typedef BOOL (API *PtrWTClose)(HCTX);
    typedef UINT (API *PtrWTInfo)(UINT, UINT, LPVOID);
    typedef BOOL (API *PtrWTEnable)(HCTX, BOOL);
    typedef BOOL (API *PtrWTOverlap)(HCTX, BOOL);
    typedef int  (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
    typedef BOOL (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
    typedef int  (API *PtrWTQueueSizeGet)(HCTX);
    typedef BOOL (API *PtrWTQueueSizeSet)(HCTX, int);

    PtrWTOpen wTOpen = nullptr;
    PtrWTClose wTClose = nullptr;
    PtrWTInfo wTInfo = nullptr;
    PtrWTEnable wTEnable = nullptr;
    PtrWTOverlap wTOverlap = nullptr;
    PtrWTPacketsGet wTPacketsGet = nullptr;
    PtrWTGet wTGet = nullptr;
    PtrWTQueueSizeGet wTQueueSizeGet = nullptr;
    PtrWTQueueSizeSet wTQueueSizeSet = nullptr;
};

// Data associated with a physical cursor (system ID) which is shared between
// devices of varying device type/pointer type.
struct QWindowsTabletDeviceData
{
    QPointF scaleCoordinates(int coordX, int coordY,const QRect &targetArea) const;
    qreal scalePressure(qreal p) const { return p / qreal(maxPressure - minPressure); }
    qreal scaleTangentialPressure(qreal p) const { return p / qreal(maxTanPressure - minTanPressure); }

    int minPressure = 0;
    int maxPressure = 0;
    int minTanPressure = 0;
    int maxTanPressure = 0;
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    int minZ = 0;
    int maxZ = 0;
    qint64 systemId = 0;
    bool zCapability = false;
    bool tiltCapability = false;
    QHash<quint8, quint8> buttonsMap;
};

class QWinTabPointingDevice : public QPointingDevice
{
public:
    using DeviceDataPtr = QSharedPointer<QWindowsTabletDeviceData>;

    explicit QWinTabPointingDevice(const DeviceDataPtr &data,
                                   const QString &name, qint64 systemId,
                                   QInputDevice::DeviceType devType,
                                   PointerType pType, Capabilities caps, int maxPoints,
                                   int buttonCount, const QString &seatName = QString(),
                                   QPointingDeviceUniqueId uniqueId = QPointingDeviceUniqueId(),
                                   QObject *parent = nullptr);

    const DeviceDataPtr &deviceData() const { return m_deviceData; }

private:
    DeviceDataPtr m_deviceData;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QWindowsTabletDeviceData &t);
#endif

class QWindowsTabletSupport
{
    Q_DISABLE_COPY_MOVE(QWindowsTabletSupport)

    explicit QWindowsTabletSupport(HWND window, HCTX context);

public:
    using DevicePtr = QSharedPointer<QWinTabPointingDevice>;
    using Devices = QList<DevicePtr>;

    enum Mode
    {
        PenMode,
        MouseMode
    };

    enum State
    {
        PenUp,
        PenProximity,
        PenDown
    };

    ~QWindowsTabletSupport();

    static QWindowsTabletSupport *create();

    void notifyActivate();
    QString description() const;

    bool translateTabletProximityEvent(WPARAM wParam, LPARAM lParam);
    bool translateTabletPacketEvent();

    static int absoluteRange() { return m_absoluteRange; }
    static void setAbsoluteRange(int a) { m_absoluteRange = a; }

private:
    unsigned options() const;
    QWindowsTabletDeviceData tabletInit(qint64 uniqueId, UINT cursorType) const;
    void updateData(QWindowsTabletDeviceData *data) const;
    void updateButtons(unsigned currentCursor, QWindowsTabletDeviceData *data) const;
    void enterProximity(ulong time = 0, QWindow *window = nullptr);
    void leaveProximity(ulong time = 0, QWindow *window = nullptr);
    void enterLeaveProximity(bool enter, ulong time, QWindow *window = nullptr);
    DevicePtr findDevice(qint64 systemId) const;
    DevicePtr findDevice(qint64 systemId,
                         QInputDevice::DeviceType deviceType,
                         QPointingDevice::PointerType pointerType) const;
    DevicePtr clonePhysicalDevice(qint64 systemId,
                                  QInputDevice::DeviceType deviceType,
                                  QPointingDevice::PointerType pointerType);

    static QWindowsWinTab32DLL m_winTab32DLL;
    const HWND m_window;
    const HCTX m_context;
    static int m_absoluteRange;
    bool m_tiltSupport = false;
    Devices m_devices;
    DevicePtr m_currentDevice;
    Mode m_mode = PenMode;
    State m_state = PenUp;
    ulong m_eventTime = 0;
};

QT_END_NAMESPACE

#endif // QWINDOWSTABLETSUPPORT_H
