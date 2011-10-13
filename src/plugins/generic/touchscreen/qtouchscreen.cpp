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
#include <QtCore/private/qcore_unix_p.h>
#include <QTimer>
#include <QDebug>

#include <libudev.h>

extern "C" {
#include <mtdev.h>
}

//#define POINT_DEBUG

QT_BEGIN_NAMESPACE

class QTouchScreenData
{
public:
    QTouchScreenData(QTouchScreenHandler *q_ptr, const QStringList &args);

    void processInputEvent(input_event *data);

    QTouchScreenHandler *q;
    QEvent::Type m_state;
    QEvent::Type m_prevState;
    QList<QWindowSystemInterface::TouchPoint> m_touchPoints;

    struct Slot {
        int trackingId;
        int x;
        int y;
        int maj;
        Qt::TouchPointState state;
        bool primary;
        Slot() : trackingId(0), x(0), y(0), maj(1), state(Qt::TouchPointPressed), primary(false) { }
    };
    QMap<int, Slot> m_slots;
    QMap<int, QPoint> m_lastReport;
    int m_currentSlot;
    QTimer m_clearTimer;
    bool m_clearTimerEnabled;

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
      m_currentSlot(0),
      hw_range_x_min(0), hw_range_x_max(0),
      hw_range_y_min(0), hw_range_y_max(0)
{
    m_clearTimerEnabled = !args.contains(QLatin1String("no_timeout"));
    if (m_clearTimerEnabled) {
        QObject::connect(&m_clearTimer, SIGNAL(timeout()), q, SLOT(onTimeout()));
        m_clearTimer.setSingleShot(true);
        m_clearTimer.setInterval(2000); // default timeout is 2 seconds
        for (int i = 0; i < args.count(); ++i)
            if (args.at(i).startsWith(QLatin1String("timeout=")))
                m_clearTimer.setInterval(args.at(i).mid(8).toInt());
    }
}

QTouchScreenHandler::QTouchScreenHandler(const QString &spec)
    : m_notify(0), m_fd(-1), m_mtdev(0), d(0)
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

    m_mtdev = (mtdev *) calloc(1, sizeof(mtdev));
    int mtdeverr = mtdev_open(m_mtdev, m_fd);
    if (mtdeverr) {
        qWarning("mtdev_open failed: %d", mtdeverr);
        QT_CLOSE(m_fd);
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
        d->hw_name = QString::fromUtf8(name);
        qDebug() << "device name" << d->hw_name;
    }
}

QTouchScreenHandler::~QTouchScreenHandler()
{
    if (m_fd >= 0)
        QT_CLOSE(m_fd);

    if (m_mtdev) {
        mtdev_close(m_mtdev);
        free(m_mtdev);
    }

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
    udev_enumerate_add_match_property(ue, "QT_TOUCH", "1");
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
    input_event buffer[32];
    int n = 0;
    n = mtdev_get(m_mtdev, m_fd, buffer, sizeof(buffer) / sizeof(input_event));
    if (n < 0) {
        if (errno != EINTR && errno != EAGAIN)
            qWarning("Could not read from input device: %s", strerror(errno));
    } else if (n > 0) {
        for (int i = 0; i < n; ++i) {
            input_event *data = &buffer[i];
            d->processInputEvent(data);
        }
    }
}

void QTouchScreenHandler::onTimeout()
{
#ifdef POINT_DEBUG
    qDebug("TIMEOUT (%d slots)", d->m_slots.count());
#endif
    d->m_slots.clear();
    if (d->m_state != QEvent::TouchEnd)
        for (int i = 0; i < d->m_observers.count(); ++i)
            d->m_observers.at(i)->touch_point(QEvent::TouchEnd,
                                              QList<QWindowSystemInterface::TouchPoint>());
    d->m_state = QEvent::TouchBegin;
}

void QTouchScreenData::processInputEvent(input_event *data)
{
    if (data->type == EV_ABS) {
         if (data->code == ABS_MT_POSITION_X) {
             m_slots[m_currentSlot].x = data->value;
         } else if (data->code == ABS_MT_POSITION_Y) {
             m_slots[m_currentSlot].y = data->value;
         } else if (data->code == ABS_MT_SLOT) {
             m_currentSlot = data->value;
         } else if (data->code == ABS_MT_TRACKING_ID) {
             if (data->value == -1) {
                 bool wasPrimary = m_slots[m_currentSlot].primary;
                 m_lastReport.remove(m_slots[m_currentSlot].trackingId);
                 m_slots.remove(m_currentSlot);
                 if (wasPrimary && !m_slots.isEmpty())
                     m_slots[m_slots.keys().at(0)].primary = true;
             } else {
                m_slots[m_currentSlot].trackingId = data->value;
                m_slots[m_currentSlot].primary = m_slots.count() == 1;
             }
         } else if (data->code == ABS_MT_TOUCH_MAJOR) {
             m_slots[m_currentSlot].maj = data->value;
             if (data->value == 0)
                 m_slots[m_currentSlot].state = Qt::TouchPointReleased;
         }
    } else if (data->type == EV_SYN && data->code == SYN_REPORT) {
        if (m_clearTimerEnabled)
            m_clearTimer.stop();
        m_touchPoints.clear();
        QList<int> keys = m_slots.keys();
        int ignoredSlotCount = 0;
        for (int i = 0; i < keys.count(); ++i) {
            const Slot &slot(m_slots.value(keys.at(i)));
            if (slot.trackingId == 0) {
                ++ignoredSlotCount;
                continue;
            }
            QWindowSystemInterface::TouchPoint tp;
            tp.id = slot.trackingId;
            tp.isPrimary = slot.primary;
            tp.pressure = slot.state == Qt::TouchPointReleased ? 0 : 1;
            tp.area = QRectF(slot.x, slot.y, slot.maj, slot.maj);
            tp.state = slot.state;
            if (slot.state == Qt::TouchPointMoved && m_lastReport.contains(slot.trackingId)) {
                QPoint lastPos = m_lastReport.value(slot.trackingId);
                if (lastPos.x() == slot.x && lastPos.y() == slot.y)
                    tp.state = Qt::TouchPointStationary;
            }
            m_touchPoints.append(tp);
            m_lastReport.insert(slot.trackingId, QPoint(slot.x, slot.y));
        }
        if (m_slots.count() - ignoredSlotCount == 0)
            m_state = QEvent::TouchEnd;

        // Skip if state is TouchUpdate and all points are Stationary.
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
        qDebug() << m_touchPoints.count() << "touchpoints, event type" << m_state;
        for (int i = 0; i < m_touchPoints.count(); ++i)
            qDebug() << "    " << m_touchPoints[i].id << m_touchPoints[i].state << m_touchPoints[i].area;
#endif

        if (!skip && !(m_state == m_prevState && m_state == QEvent::TouchEnd))
            for (int i = 0; i < m_observers.count(); ++i)
                m_observers.at(i)->touch_point(m_state, m_touchPoints);

        for (int i = 0; i < keys.count(); ++i) {
            Slot &slot(m_slots[keys.at(i)]);
            if (slot.state == Qt::TouchPointPressed)
                slot.state = Qt::TouchPointMoved;
        }
        m_prevState = m_state;
        if (m_state == QEvent::TouchBegin)
            m_state = QEvent::TouchUpdate;
        else if (m_state == QEvent::TouchEnd)
            m_state = QEvent::TouchBegin;

        // The user's finger may fall off the touchscreen which in some rare
        // cases may mean there will be no released event ever received for that
        // particular point. Use a timer to clear all points when no activity
        // occurs for a certain period of time.
        if (m_clearTimerEnabled && m_state != QEvent::TouchBegin)
            m_clearTimer.start();
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
