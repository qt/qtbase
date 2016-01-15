/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qevdevtablethandler_p.h"

#include <QStringList>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <linux/input.h>

QT_BEGIN_NAMESPACE

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
    memset(&state, 0, sizeof(state));
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
            state.tool = ev->value ? QTabletEvent::Pen : 0;
            break;
        case BTN_TOOL_RUBBER:
            state.tool = ev->value ? QTabletEvent::Eraser : 0;
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
        QWindowSystemInterface::handleTabletEnterProximityEvent(QTabletEvent::Stylus, state.tool, q->deviceId());

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
        QWindowSystemInterface::handleTabletEvent(0, state.down, QPointF(), globalPos,
                                                  QTabletEvent::Stylus, pointer,
                                                  pressure, 0, 0, 0, 0, 0, q->deviceId(), qGuiApp->keyboardModifiers());
    }

    if (state.lastReportTool && !state.tool)
        QWindowSystemInterface::handleTabletLeaveProximityEvent(QTabletEvent::Stylus, state.tool, q->deviceId());

    state.lastReportDown = state.down;
    state.lastReportTool = state.tool;
    state.lastReportPos = globalPos;
}


QEvdevTabletHandler::QEvdevTabletHandler(const QString &device, const QString &spec, QObject *parent)
    : QObject(parent), m_fd(-1), m_device(device), m_notifier(0), d(0)
{
    Q_UNUSED(spec)

    setObjectName(QLatin1String("Evdev Tablet Handler"));

    qCDebug(qLcEvdevTablet, "evdevtablet: using %s", qPrintable(device));

    m_fd = QT_OPEN(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (m_fd < 0) {
        qErrnoWarning(errno, "evdevtablet: Cannot open input device %s", qPrintable(device));
        return;
    }

    bool grabSuccess = !ioctl(m_fd, EVIOCGRAB, (void *) 1);
    if (grabSuccess)
        ioctl(m_fd, EVIOCGRAB, (void *) 0);
    else
        qWarning("evdevtablet: %s: The device is grabbed by another process. No events will be read.", qPrintable(device));

    d = new QEvdevTabletData(this);
    if (!queryLimits())
        qWarning("evdevtablet: %s: Unset or invalid ABS limits. Behavior will be unspecified.", qPrintable(device));

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
        qCDebug(qLcEvdevTablet, "evdevtablet: %s: min X: %d max X: %d", qPrintable(m_device),
                d->minValues.x, d->maxValues.x);
    }
    ok &= ioctl(m_fd, EVIOCGABS(ABS_Y), &absInfo) >= 0;
    if (ok) {
        d->minValues.y = absInfo.minimum;
        d->maxValues.y = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %s: min Y: %d max Y: %d", qPrintable(m_device),
                d->minValues.y, d->maxValues.y);
    }
    if (ioctl(m_fd, EVIOCGABS(ABS_PRESSURE), &absInfo) >= 0) {
        d->minValues.p = absInfo.minimum;
        d->maxValues.p = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %s: min pressure: %d max pressure: %d", qPrintable(m_device),
                d->minValues.p, d->maxValues.p);
    }
    if (ioctl(m_fd, EVIOCGABS(ABS_DISTANCE), &absInfo) >= 0) {
        d->minValues.d = absInfo.minimum;
        d->maxValues.d = absInfo.maximum;
        qCDebug(qLcEvdevTablet, "evdevtablet: %s: min distance: %d max distance: %d", qPrintable(m_device),
                d->minValues.d, d->maxValues.d);
    }
    char name[128];
    if (ioctl(m_fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0) {
        d->devName = QString::fromLocal8Bit(name);
        qCDebug(qLcEvdevTablet, "evdevtablet: %s: device name: %s", qPrintable(m_device), name);
    }
    return ok;
}

void QEvdevTabletHandler::readData()
{
    static input_event buffer[32];
    int n = 0;
    for (; ;) {
        int result = QT_READ(m_fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);
        if (!result) {
            qWarning("evdevtablet: %s: Got EOF from input device", qPrintable(m_device));
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qErrnoWarning(errno, "evdevtablet: %s: Could not read from input device", qPrintable(m_device));
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
