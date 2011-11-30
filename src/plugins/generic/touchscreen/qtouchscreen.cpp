/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtouchscreen.h"
#include <QStringList>
#include <QSocketNotifier>
#include <QDebug>
#include <QtCore/private/qcore_unix_p.h>
#include <linux/input.h>
#include <libudev.h>

QT_BEGIN_NAMESPACE

//#define POINT_DEBUG

class QTouchScreenData
{
public:
    QTouchScreenData(QTouchScreenHandler *q_ptr, const QStringList &args);

    void processInputEvent(input_event *data);

    void dump();

    QTouchScreenHandler *q;
    QEvent::Type m_state;
    QEvent::Type m_prevState;
    int m_lastEventType;
    QList<QWindowSystemInterface::TouchPoint> m_touchPoints;

    struct Contact {
        int trackingId;
        int x;
        int y;
        int maj;
        Qt::TouchPointState state;
        QTouchEvent::TouchPoint::InfoFlags flags;
        Contact() : trackingId(0), x(0), y(0), maj(1), state(Qt::TouchPointPressed), flags(0) { }
    };
    QMap<int, Contact> m_contacts, m_lastContacts;
    Contact m_currentData;

    int hw_range_x_min;
    int hw_range_x_max;
    int hw_range_y_min;
    int hw_range_y_max;
    QString hw_name;

    QList<QTouchScreenObserver *> m_observers;
};

QTouchScreenData::QTouchScreenData(QTouchScreenHandler *q_ptr, const QStringList &args)
    : q(q_ptr),
      m_state(QEvent::TouchBegin),
      m_prevState(m_state),
      m_lastEventType(-1),
      hw_range_x_min(0), hw_range_x_max(0),
      hw_range_y_min(0), hw_range_y_max(0)
{
    Q_UNUSED(args);
}

QTouchScreenHandler::QTouchScreenHandler(const QString &spec)
    : m_notify(0), m_fd(-1), d(0)
{
    setObjectName(QLatin1String("LinuxInputSubsystem Touch Handler"));

    QString dev = QLatin1String("/dev/input/event5");
    try_udev(&dev);

    QStringList args = spec.split(QLatin1Char(':'));
    for (int i = 0; i < args.count(); ++i)
        if (args.at(i).startsWith(QLatin1String("/dev/")))
            dev = args.at(i);

    qDebug("Using device '%s'", qPrintable(dev));
    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);

    if (m_fd >= 0) {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readData()));
    } else {
        qWarning("Cannot open input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }

    d = new QTouchScreenData(this, args);

    input_absinfo absInfo;
    memset(&absInfo, 0, sizeof(input_absinfo));
    if (!ioctl(m_fd, EVIOCGABS(ABS_X), &absInfo) >= 0) {
        qDebug("min X: %d max X: %d", absInfo.minimum, absInfo.maximum);
        d->hw_range_x_min = absInfo.minimum;
        d->hw_range_x_max = absInfo.maximum;
    }
    if (!ioctl(m_fd, EVIOCGABS(ABS_Y), &absInfo) >= 0) {
        qDebug("min Y: %d max Y: %d", absInfo.minimum, absInfo.maximum);
        d->hw_range_y_min = absInfo.minimum;
        d->hw_range_y_max = absInfo.maximum;
    }
    char name[1024];
    if (ioctl(m_fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0) {
        d->hw_name = QString::fromLocal8Bit(name);
        qDebug("device name: %s", name);
    }
}

QTouchScreenHandler::~QTouchScreenHandler()
{
    if (m_fd >= 0)
        QT_CLOSE(m_fd);

    delete d;
}

void QTouchScreenHandler::addObserver(QTouchScreenObserver *observer)
{
    if (!d || !observer)
        return;
    d->m_observers.append(observer);
    observer->touch_configure(d->hw_range_x_min, d->hw_range_x_max,
                              d->hw_range_y_min, d->hw_range_y_max);
}

void QTouchScreenHandler::try_udev(QString *path)
{
    udev *u = udev_new();
    udev_enumerate *ue = udev_enumerate_new(u);
    udev_enumerate_add_match_subsystem(ue, "input");
    udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHPAD", "1");
    udev_enumerate_scan_devices(ue);
    udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(ue)) {
        const char *syspath = udev_list_entry_get_name(entry);
        udev_device *udevice = udev_device_new_from_syspath(u, syspath);
        *path = QString::fromLocal8Bit(udev_device_get_devnode(udevice));
        qDebug("from udev: %s", qPrintable(*path));
        udev_device_unref(udevice);
    }
    udev_enumerate_unref(ue);
    udev_unref(u);
}

void QTouchScreenHandler::readData()
{
    ::input_event buffer[32];
    int n = 0;
    for (; ;) {
        n = QT_READ(m_fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);

        if (!n) {
            qWarning("Got EOF from input device");
            return;
        } else if (n < 0 && (errno != EINTR && errno != EAGAIN)) {
            qWarning("Could not read from input device: %s", strerror(errno));
            if (errno == ENODEV) { // device got disconnected -> stop reading
                delete m_notify;
                m_notify = 0;
                QT_CLOSE(m_fd);
                m_fd = -1;
            }
            return;
        } else if (n % sizeof(::input_event) == 0) {
            break;
        }
    }

    n /= sizeof(::input_event);

    for (int i = 0; i < n; ++i)
        d->processInputEvent(&buffer[i]);
}

void QTouchScreenData::processInputEvent(input_event *data)
{
    if (data->type == EV_ABS) {

         if (data->code == ABS_MT_POSITION_X) {
             m_currentData.x = data->value;
         } else if (data->code == ABS_MT_POSITION_Y) {
             m_currentData.y = data->value;
         } else if (data->code == ABS_MT_TRACKING_ID) {
             m_currentData.trackingId = data->value;
             if (m_contacts.isEmpty())
                 m_currentData.flags |= QTouchEvent::TouchPoint::Primary;
         } else if (data->code == ABS_MT_TOUCH_MAJOR) {
             m_currentData.maj = data->value;
             if (data->value == 0)
                 m_currentData.state = Qt::TouchPointReleased;
         }

    } else if (data->type == EV_SYN && data->code == SYN_MT_REPORT && m_lastEventType != EV_SYN) {

        m_contacts.insert(m_currentData.trackingId, m_currentData);
        m_currentData = Contact();

    } else if (data->type == EV_SYN && data->code == SYN_REPORT) {

        m_touchPoints.clear();
        for (QMap<int, Contact>::iterator it = m_contacts.begin(), ite = m_contacts.end();
             it != ite; ++it) {
            QWindowSystemInterface::TouchPoint tp;
            tp.id = it->trackingId;
            tp.flags = it->flags;
            tp.pressure = it->state == Qt::TouchPointReleased ? 0 : 1;

            if (m_lastContacts.contains(it->trackingId)) {
                const Contact &prev(m_lastContacts.value(it->trackingId));
                if (it->state == Qt::TouchPointReleased) {
                    // Copy over the previous values for released points, just in case.
                    it->x = prev.x;
                    it->y = prev.y;
                    it->maj = prev.maj;
                } else {
                    it->state = (prev.x == it->x && prev.y == it->y) ? Qt::TouchPointStationary : Qt::TouchPointMoved;
                }
            }

            tp.state = it->state;
            tp.area = QRectF(it->x, it->y, it->maj, it->maj);

            // Translate so that (0, 0) is the top-left corner.
            const int hw_x = qBound(hw_range_x_min, int(tp.area.left()), hw_range_x_max) - hw_range_x_min;
            const int hw_y = qBound(hw_range_y_min, int(tp.area.top()), hw_range_y_max) - hw_range_y_min;
            // Get a normalized position in range 0..1.
            const int hw_w = hw_range_x_max - hw_range_x_min;
            const int hw_h = hw_range_y_max - hw_range_y_min;
            tp.normalPosition = QPointF(hw_x / qreal(hw_w),
                                        hw_y / qreal(hw_h));

            m_touchPoints.append(tp);
        }

        if (m_contacts.isEmpty())
            m_state = QEvent::TouchEnd;

        m_lastContacts = m_contacts;
        m_contacts.clear();

        // No need to deliver if all points are stationary.
        bool skip = false;
        if (m_state == QEvent::TouchUpdate) {
            skip = true;
            for (int i = 0; i < m_touchPoints.count(); ++i)
                if (m_touchPoints.at(i).state != Qt::TouchPointStationary) {
                    skip = false;
                    break;
                }
        }

#ifdef POINT_DEBUG
        dump();
#endif

        if (!skip && !(m_state == m_prevState && m_state == QEvent::TouchEnd))
            for (int i = 0; i < m_observers.count(); ++i)
                m_observers.at(i)->touch_point(m_touchPoints);

        m_prevState = m_state;
        if (m_state == QEvent::TouchBegin)
            m_state = QEvent::TouchUpdate;
        else if (m_state == QEvent::TouchEnd)
            m_state = QEvent::TouchBegin;
    }

    m_lastEventType = data->type;
}

void QTouchScreenData::dump()
{
    const char *eventType;
    switch (m_state) {
    case QEvent::TouchBegin:
        eventType = "TouchBegin";
        break;
    case QEvent::TouchUpdate:
        eventType = "TouchUpdate";
        break;
    case QEvent::TouchEnd:
        eventType = "TouchEnd";
        break;
    default:
        eventType = "unknown";
        break;
    }
    qDebug() << "touch event" << eventType;
    foreach (const QWindowSystemInterface::TouchPoint &tp, m_touchPoints) {
        const char *pointState;
        switch (tp.state) {
        case Qt::TouchPointPressed:
            pointState = "pressed";
            break;
        case Qt::TouchPointMoved:
            pointState = "moved";
            break;
        case Qt::TouchPointStationary:
            pointState = "stationary";
            break;
        case Qt::TouchPointReleased:
            pointState = "released";
            break;
        default:
            pointState = "unknown";
            break;
        }
        qDebug() << "  " << tp.id << tp.area << pointState << tp.normalPosition
                 << tp.pressure << tp.flags << tp.area.center();
    }
}


QTouchScreenHandlerThread::QTouchScreenHandlerThread(const QString &spec,
                                                     QTouchScreenObserver *observer)
    : m_spec(spec), m_handler(0), m_observer(observer)
{
    start();
}

QTouchScreenHandlerThread::~QTouchScreenHandlerThread()
{
    quit();
    wait();
}

void QTouchScreenHandlerThread::run()
{
    m_handler = new QTouchScreenHandler(m_spec);
    m_handler->addObserver(m_observer);
    exec();
    delete m_handler;
    m_handler = 0;
}


QT_END_NAMESPACE
