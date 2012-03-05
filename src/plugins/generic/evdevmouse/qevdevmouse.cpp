/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevmouse.h"

#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QWindowSystemInterface>

#include <qplatformdefs.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN
#include <QtPlatformSupport/private/qudevhelper_p.h>

#include <errno.h>

#include <linux/kd.h>
#include <linux/input.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QEvdevMouseHandler::QEvdevMouseHandler(const QString &key,
                                       const QString &specification)
    : m_notify(0), m_x(0), m_y(0), m_prevx(0), m_prevy(0),
      m_xoffset(0), m_yoffset(0), m_buttons(0)
{
    Q_UNUSED(key);
    setObjectName(QLatin1String("Evdev Mouse Handler"));

    QString dev;
    q_udev_devicePath(UDev_Mouse | UDev_Touchpad, &dev);
    if (dev.isEmpty())
        dev = QLatin1String("/dev/input/event0");

    m_compression = true;
    m_smooth = false;
    int jitterLimit = 0;

    QStringList args = specification.split(QLatin1Char(':'));
    foreach (const QString &arg, args) {
        if (arg == "nocompress")
            m_compression = false;
        else if (arg.startsWith("dejitter="))
            jitterLimit = arg.mid(9).toInt();
        else if (arg.startsWith("xoffset="))
            m_xoffset = arg.mid(8).toInt();
        else if (arg.startsWith("yoffset="))
            m_yoffset = arg.mid(8).toInt();
        else if (arg.startsWith(QLatin1String("/dev/")))
            dev = arg;
    }
    m_jitterLimitSquared = jitterLimit*jitterLimit;

    qDebug("evdevmouse: Using device %s", qPrintable(dev));
    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (m_fd >= 0) {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readMouseData()));
    } else {
        qWarning("Cannot open mouse input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }
}

QEvdevMouseHandler::~QEvdevMouseHandler()
{
    if (m_fd >= 0)
        QT_CLOSE(m_fd);
}

void QEvdevMouseHandler::sendMouseEvent()
{
    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    //qDebug("mouse event %d %d %d", pos.x(), pos.y(), int(m_buttons));
    QWindowSystemInterface::handleMouseEvent(0, pos, pos, m_buttons);
    m_prevx = m_x;
    m_prevy = m_y;
}

void QEvdevMouseHandler::readMouseData()
{
    struct ::input_event buffer[32];
    int n = 0;
    bool posChanged = false;
    bool pendingMouseEvent = false;
    int eventCompressCount = 0;
    forever {
        n = QT_READ(m_fd, reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);

        if (n == 0) {
            qWarning("Got EOF from the input device.");
            return;
        } else if (n < 0 && (errno != EINTR && errno != EAGAIN)) {
            qWarning("Could not read from input device: %s", strerror(errno));
            return;
        } else if (n % sizeof(buffer[0]) == 0) {
            break;
        }
    }

    n /= sizeof(buffer[0]);

    for (int i = 0; i < n; ++i) {
        struct ::input_event *data = &buffer[i];
        //qDebug() << ">>" << hex << data->type << data->code << dec << data->value;
        if (data->type == EV_ABS) {
            if (data->code == ABS_X && m_x != data->value) {
                m_x = data->value;
                posChanged = true;
            } else if (data->code == ABS_Y && m_y != data->value) {
                m_y = data->value;
                posChanged = true;
            }
        } else if (data->type == EV_REL) {
            if (data->code == REL_X) {
                m_x += data->value;
                posChanged = true;
            } else if (data->code == REL_Y) {
                m_y += data->value;
                posChanged = true;
            } else if (data->code == ABS_WHEEL) { // vertical scroll
                // data->value: 1 == up, -1 == down
                int delta = 120 * data->value;
                QWindowSystemInterface::handleWheelEvent(0, QPoint(m_x, m_y),
                                                         QPoint(m_x, m_y),
                                                         delta, Qt::Vertical);
            } else if (data->code == ABS_THROTTLE) { // horizontal scroll
                // data->value: 1 == right, -1 == left
                int delta = 120 * -data->value;
                QWindowSystemInterface::handleWheelEvent(0, QPoint(m_x, m_y),
                                                         QPoint(m_x, m_y),
                                                         delta, Qt::Horizontal);
            }
        } else if (data->type == EV_KEY && data->code == BTN_TOUCH) {
            m_buttons = data->value ? Qt::LeftButton : Qt::NoButton;

            sendMouseEvent();
            pendingMouseEvent = false;
        } else if (data->type == EV_KEY && data->code >= BTN_LEFT && data->code <= BTN_JOYSTICK) {
            Qt::MouseButton button = Qt::NoButton;
            // BTN_LEFT == 0x110 in kernel's input.h
            // The range of possible mouse buttons ends just before BTN_JOYSTICK, value 0x120.
            switch (data->code) {
            case 0x110: button = Qt::LeftButton; break;    // BTN_LEFT
            case 0x111: button = Qt::RightButton; break;
            case 0x112: button = Qt::MiddleButton; break;
            case 0x113: button = Qt::ExtraButton1; break;  // AKA Qt::BackButton
            case 0x114: button = Qt::ExtraButton2; break;  // AKA Qt::ForwardButton
            case 0x115: button = Qt::ExtraButton3; break;  // AKA Qt::TaskButton
            case 0x116: button = Qt::ExtraButton4; break;
            case 0x117: button = Qt::ExtraButton5; break;
            case 0x118: button = Qt::ExtraButton6; break;
            case 0x119: button = Qt::ExtraButton7; break;
            case 0x11a: button = Qt::ExtraButton8; break;
            case 0x11b: button = Qt::ExtraButton9; break;
            case 0x11c: button = Qt::ExtraButton10; break;
            case 0x11d: button = Qt::ExtraButton11; break;
            case 0x11e: button = Qt::ExtraButton12; break;
            case 0x11f: button = Qt::ExtraButton13; break;
            }
            if (data->value)
                m_buttons |= button;
            else
                m_buttons &= ~button;
            sendMouseEvent();
            pendingMouseEvent = false;
        } else if (data->type == EV_SYN && data->code == SYN_REPORT) {
            if (posChanged) {
                posChanged = false;
                if (m_compression) {
                    pendingMouseEvent = true;
                    eventCompressCount++;
                } else {
                    sendMouseEvent();
                }
            }
        } else if (data->type == EV_MSC && data->code == MSC_SCAN) {
            // kernel encountered an unmapped key - just ignore it
            continue;
        }
    }
    if (m_compression && pendingMouseEvent) {
        int distanceSquared = (m_x - m_prevx)*(m_x - m_prevx) + (m_y - m_prevy)*(m_y - m_prevy);
        if (distanceSquared > m_jitterLimitSquared)
            sendMouseEvent();
    }
}

QT_END_NAMESPACE
