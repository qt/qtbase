// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qevdevtablethandler_p.h"

#include <QStringList>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QPointingDevice>
#include <QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>
#include <qpa/qwindowsysteminterface.h>
#ifdef Q_OS_FREEBSD
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(qLcEvdevTablet, "qt.qpa.input")

class QEvdevTabletData
{
public:
    QEvdevTabletData(QEvdevTabletHandler *q_ptr);

    void processInputEvent(input_event *ev);
    void report();

    QEvdevTabletHandler *q;
    int lastEventType;
    QString devName;
    struct {
        int x, y, p, d;
    } minValues, maxValues;
    struct {
        int x, y, p, d;
        bool down, lastReportDown;
        int tool, lastReportTool;
        QPointF lastReportPos;
    } state;
};

QEvdevTabletData::QEvdevTabletData(QEvdevTabletHandler *q_ptr)
    : q(q_ptr), lastEventType(0)
{
    memset(&minValues, 0, sizeof(minValues));
    memset(&maxValues, 0, sizeof(maxValues));
    memset(static_cast<void *>(&state), 0, sizeof(state));
}

void QEvdevTabletData::processInputEvent(input_event *ev)
{
    if (ev->type == EV_ABS) {
        switch (ev->code) {
        case ABS_X:
            state.x = ev->value;
            break;
        case ABS_Y:
            state.y = ev->value;
            break;
        case ABS_PRESSURE:
            state.p = ev->value;
            break;
        case ABS_DISTANCE:
            state.d = ev->value;
            break;
        default:
            break;
        }
    } else if (ev->type == EV_KEY) {
        // code BTN_TOOL_* value 1 -> proximity enter
        // code BTN_TOOL_* value 0 -> proximity leave
        // code BTN_TOUCH value 1 -> contact with screen
        // code BTN_TOUCH value 0 -> no contact
        switch (ev->code) {
        case BTN_TOUCH:
            state.down = ev->value != 0;
            break;
        case BTN_TOOL_PEN:
            state.tool = ev->value ? int(QPointingDevice::PointerType::Pen) : 0;
            break;
        case BTN_TOOL_RUBBER:
            state.tool = ev->value ? int(QPointingDevice::PointerType::Eraser) : 0;
            break;
        default:
            break;
        }
    } else if (ev->type == EV_SYN && ev->code == SYN_REPORT && lastEventType != ev->type) {
        report();
    }
    lastEventType = ev->type;
}

void QEvdevTabletData::report()
{
    if (!state.lastReportTool && state.tool)
        QWindowSystemInterface::handleTabletEnterProximityEvent(int(QInputDevice::DeviceType::Stylus), state.tool, q->deviceId());

    qreal nx = (state.x - minValues.x) / qreal(maxValues.x - minValues.x);
    qreal ny = (state.y - minValues.y) / qreal(maxValues.y - minValues.y);

    QRect winRect = QGuiApplication::primaryScreen()->geometry();
    QPointF globalPos(nx * winRect.width(), ny * winRect.height());
    int pointer = state.tool;
    // Prevent sending confusing values of 0 when moving the pen outside the active area.
    if (!state.down && state.lastReportDown) {
        globalPos = state.lastReportPos;
        pointer = state.lastReportTool;
    }

    int pressureRange = maxValues.p - minValues.p;
    qreal pressure = pressureRange ? (state.p - minValues.p) / qreal(pressureRange) : qreal(1);

    if (state.down || state.lastReportDown) {
        QWindowSystemInterface::handleTabletEvent(0, QPointF(), globalPos,
                                                  int(QInputDevice::DeviceType::Stylus), pointer,
                                                  state.down ? Qt::LeftButton : Qt::NoButton,
                                                  pressure, 0, 0, 0, 0, 0, q->deviceId(),
                                                  qGuiApp->keyboardModifiers());
    }

    if (state.lastReportTool && !state.tool)
        QWindowSystemInterface::handleTabletLeaveProximityEvent(int(QInputDevice::DeviceType::Stylus), state.tool, q->deviceId());

    state.lastReportDown = state.down;
    state.lastReportTool = state.tool;
    state.lastReportPos = globalPos;
}


QEvdevTabletHandler::QEvdevTabletHandler(const QString &device, const QString &spec, QObject *parent)
    : QObject(parent), m_fd(-1), m_device(device), m_notifier(0), d(0)
{
    Q_UNUSED(spec);

    setObjectName("Evdev Tablet Handler"_L1);

    qCDebug(qLcEvdevTablet, "evdevtablet: using %ls", qUtf16Printable(device));

    m_fd = QT_OPEN(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (m_fd < 0) {
        qErrnoWarning("evdevtablet: Cannot open input device %ls", qUtf16Printable(device));
        return;
    }

    bool grabSuccess = !ioctl(m_fd, EVIOCGRAB, (void *) 1);
    if (grabSuccess)
        ioctl(m_fd, EVIOCGRAB, (void *) 0);
    else
        qWarning("evdevtablet: %ls: The device is grabbed by another process. No events will be read.", qUtf16Printable(device));

    d = new QEvdevTabletData(this);
    if (!queryLimits())
        qWarning("evdevtablet: %ls: Unset or invalid ABS limits. Behavior will be unspecified.", qUtf16Printable(device));

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &QEvdevTabletHandler::readData);
}

QEvdevTabletHandler::~QEvdevTabletHandler()
{
    if (m_fd >= 0)
        QT_CLOSE(m_fd);

    delete d;
}

qint64 QEvdevTabletHandler::deviceId() const
{
    return m_fd;
}

bool QEvdevTabletHandler::queryLimits()
{
    bool ok = true;
    input_absinfo absInfo;
    memset(&absInfo, 0, sizeof(input_absinfo));
    ok &= ioctl(m_fd, EVIOCGABS(ABS_X), &absInfo) >= 0;
    if (ok) {
        d->minValues.x = absInfo.minimum;
        d->maxValues.x = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %ls: min X: %d max X: %d", qUtf16Printable(m_device),
                d->minValues.x, d->maxValues.x);
    }
    ok &= ioctl(m_fd, EVIOCGABS(ABS_Y), &absInfo) >= 0;
    if (ok) {
        d->minValues.y = absInfo.minimum;
        d->maxValues.y = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %ls: min Y: %d max Y: %d", qUtf16Printable(m_device),
                d->minValues.y, d->maxValues.y);
    }
    if (ioctl(m_fd, EVIOCGABS(ABS_PRESSURE), &absInfo) >= 0) {
        d->minValues.p = absInfo.minimum;
        d->maxValues.p = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %ls: min pressure: %d max pressure: %d", qUtf16Printable(m_device),
                d->minValues.p, d->maxValues.p);
    }
    if (ioctl(m_fd, EVIOCGABS(ABS_DISTANCE), &absInfo) >= 0) {
        d->minValues.d = absInfo.minimum;
        d->maxValues.d = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %ls: min distance: %d max distance: %d", qUtf16Printable(m_device),
                d->minValues.d, d->maxValues.d);
    }
    char name[128];
    if (ioctl(m_fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0) {
        d->devName = QString::fromLocal8Bit(name);
        qCDebug(qLcEvdevTablet, "evdevtablet: %ls: device name: %s", qUtf16Printable(m_device), name);
    }
    return ok;
}

void QEvdevTabletHandler::readData()
{
    input_event buffer[32];
    int n = 0;
    for (; ;) {
        int result = QT_READ(m_fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);
        if (!result) {
            qWarning("evdevtablet: %ls: Got EOF from input device", qUtf16Printable(m_device));
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qErrnoWarning("evdevtablet: %ls: Could not read from input device", qUtf16Printable(m_device));
                if (errno == ENODEV) { // device got disconnected -> stop reading
                    delete m_notifier;
                    m_notifier = 0;
                    QT_CLOSE(m_fd);
                    m_fd = -1;
                }
                return;
            }
        } else {
            n += result;
            if (n % sizeof(input_event) == 0)
                break;
        }
    }

    n /= sizeof(input_event);

    for (int i = 0; i < n; ++i)
        d->processInputEvent(&buffer[i]);
}


QEvdevTabletHandlerThread::QEvdevTabletHandlerThread(const QString &device, const QString &spec, QObject *parent)
    : QDaemonThread(parent), m_device(device), m_spec(spec), m_handler(0)
{
    start();
}

QEvdevTabletHandlerThread::~QEvdevTabletHandlerThread()
{
    quit();
    wait();
}

void QEvdevTabletHandlerThread::run()
{
    m_handler = new QEvdevTabletHandler(m_device, m_spec);
    exec();
    delete m_handler;
    m_handler = 0;
}


QT_END_NAMESPACE
