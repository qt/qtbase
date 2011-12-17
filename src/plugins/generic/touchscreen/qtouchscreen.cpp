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
#include <QHash>
#include <QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>
#include <linux/input.h>
#include <libudev.h>

QT_BEGIN_NAMESPACE

class QTouchScreenData
{
public:
    QTouchScreenData(QTouchScreenHandler *q_ptr, const QStringList &args);

    void processInputEvent(input_event *data);
    void assignIds();

    QTouchScreenHandler *q;
    int m_lastEventType;
    QList<QWindowSystemInterface::TouchPoint> m_touchPoints;

    struct Contact {
        int trackingId;
        int x;
        int y;
        int maj;
        int pressure;
        Qt::TouchPointState state;
        QTouchEvent::TouchPoint::InfoFlags flags;
        Contact() : trackingId(-1),
            x(0), y(0), maj(1), pressure(0),
            state(Qt::TouchPointPressed), flags(0) { }
    };
    QHash<int, Contact> m_contacts, m_lastContacts;
    Contact m_currentData;

    int findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist);

    int hw_range_x_min;
    int hw_range_x_max;
    int hw_range_y_min;
    int hw_range_y_max;
    int hw_pressure_min;
    int hw_pressure_max;
    QString hw_name;

    QList<QTouchScreenObserver *> m_observers;
};

QTouchScreenData::QTouchScreenData(QTouchScreenHandler *q_ptr, const QStringList &args)
    : q(q_ptr),
      m_lastEventType(-1),
      hw_range_x_min(0), hw_range_x_max(0),
      hw_range_y_min(0), hw_range_y_max(0),
      hw_pressure_min(0), hw_pressure_max(0)
{
    Q_UNUSED(args);
}

QTouchScreenHandler::QTouchScreenHandler(const QString &spec)
    : m_notify(0), m_fd(-1), d(0)
{
    setObjectName(QLatin1String("Linux Touch Handler"));

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
    if (ioctl(m_fd, EVIOCGABS(ABS_MT_POSITION_X), &absInfo) >= 0) {
        qDebug("min X: %d max X: %d", absInfo.minimum, absInfo.maximum);
        d->hw_range_x_min = absInfo.minimum;
        d->hw_range_x_max = absInfo.maximum;
    }
    if (ioctl(m_fd, EVIOCGABS(ABS_MT_POSITION_Y), &absInfo) >= 0) {
        qDebug("min Y: %d max Y: %d", absInfo.minimum, absInfo.maximum);
        d->hw_range_y_min = absInfo.minimum;
        d->hw_range_y_max = absInfo.maximum;
    }
    if (ioctl(m_fd, EVIOCGABS(ABS_PRESSURE), &absInfo) >= 0) {
        qDebug("min pressure: %d max pressure: %d", absInfo.minimum, absInfo.maximum);
        if (absInfo.maximum > absInfo.minimum) {
            d->hw_pressure_min = absInfo.minimum;
            d->hw_pressure_max = absInfo.maximum;
        }
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
                              d->hw_range_y_min, d->hw_range_y_max,
                              d->hw_pressure_min, d->hw_pressure_max,
                              d->hw_name);
}

void QTouchScreenHandler::try_udev(QString *path)
{
    *path = QString();
    udev *u = udev_new();
    udev_enumerate *ue = udev_enumerate_new(u);
    udev_enumerate_add_match_subsystem(ue, "input");
    udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHPAD", "1");
    udev_enumerate_scan_devices(ue);
    udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(ue)) {
        const char *syspath = udev_list_entry_get_name(entry);
        udev_device *udevice = udev_device_new_from_syspath(u, syspath);
        QString candidate = QString::fromLocal8Bit(udev_device_get_devnode(udevice));
        udev_device_unref(udevice);
        if (path->isEmpty() && candidate.startsWith("/dev/input/event"))
            *path = candidate;
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
            m_currentData.x = qBound(hw_range_x_min, data->value, hw_range_x_max);
        } else if (data->code == ABS_MT_POSITION_Y) {
            m_currentData.y = qBound(hw_range_y_min, data->value, hw_range_y_max);
        } else if (data->code == ABS_MT_TRACKING_ID) {
            m_currentData.trackingId = data->value;
        } else if (data->code == ABS_MT_TOUCH_MAJOR) {
            m_currentData.maj = data->value;
            if (data->value == 0)
                m_currentData.state = Qt::TouchPointReleased;
        } else if (data->code == ABS_PRESSURE) {
            m_currentData.pressure = qBound(hw_pressure_min, data->value, hw_pressure_max);
        }

    } else if (data->type == EV_SYN && data->code == SYN_MT_REPORT && m_lastEventType != EV_SYN) {

        // If there is no tracking id, one will be generated later.
        // Until that use a temporary key.
        int key = m_currentData.trackingId;
        if (key == -1)
            key = m_contacts.count();

        // Mark the first point as primary.
        if (m_contacts.isEmpty())
            m_currentData.flags |= QTouchEvent::TouchPoint::Primary;

        m_contacts.insert(key, m_currentData);
        m_currentData = Contact();

    } else if (data->type == EV_SYN && data->code == SYN_REPORT) {

        // Ensure valid IDs even when the driver does not report ABS_MT_TRACKING_ID.
        if (!m_contacts.isEmpty() && m_contacts.constBegin().value().trackingId == -1)
            assignIds();

        m_touchPoints.clear();
        Qt::TouchPointStates combinedStates;
        QMutableHashIterator<int, Contact> it(m_contacts);
        while (it.hasNext()) {
            it.next();
            QWindowSystemInterface::TouchPoint tp;
            Contact &contact(it.value());
            tp.id = contact.trackingId;
            tp.flags = contact.flags;

            if (m_lastContacts.contains(contact.trackingId)) {
                const Contact &prev(m_lastContacts.value(contact.trackingId));
                if (contact.state == Qt::TouchPointReleased) {
                    // Copy over the previous values for released points, just in case.
                    contact.x = prev.x;
                    contact.y = prev.y;
                    contact.maj = prev.maj;
                } else {
                    contact.state = (prev.x == contact.x && prev.y == contact.y)
                            ? Qt::TouchPointStationary : Qt::TouchPointMoved;
                }
            }

            // Avoid reporting a contact in released state more than once.
            if (contact.state == Qt::TouchPointReleased
                    && !m_lastContacts.contains(contact.trackingId)) {
                it.remove();
                continue;
            }

            tp.state = contact.state;
            combinedStates |= tp.state;

            // Store the HW coordinates. Observers can then map it to screen space or something else.
            tp.area = QRectF(0, 0, contact.maj, contact.maj);
            tp.area.moveCenter(QPoint(contact.x, contact.y));
            tp.pressure = contact.pressure;

            // Get a normalized position in range 0..1.
            tp.normalPosition = QPointF((contact.x - hw_range_x_min) / qreal(hw_range_x_max - hw_range_x_min),
                                        (contact.y - hw_range_y_min) / qreal(hw_range_y_max - hw_range_y_min));

            m_touchPoints.append(tp);

            if (contact.state == Qt::TouchPointReleased)
                it.remove();
        }

        m_lastContacts = m_contacts;
        m_contacts.clear();

        if (!m_touchPoints.isEmpty() && combinedStates != Qt::TouchPointStationary) {
            for (int i = 0; i < m_observers.count(); ++i)
                m_observers.at(i)->touch_point(m_touchPoints);
        }
    }

    m_lastEventType = data->type;
}

int QTouchScreenData::findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist)
{
    int minDist = -1, id = -1;
    for (QHash<int, Contact>::const_iterator it = contacts.constBegin(), ite = contacts.constEnd();
         it != ite; ++it) {
        const Contact &contact(it.value());
        int dx = x - contact.x;
        int dy = y - contact.y;
        int dist = dx * dx + dy * dy;
        if (minDist == -1 || dist < minDist) {
            minDist = dist;
            id = contact.trackingId;
        }
    }
    if (dist)
        *dist = minDist;
    return id;
}

void QTouchScreenData::assignIds()
{
    QHash<int, Contact> candidates = m_lastContacts, pending = m_contacts, newContacts;
    int maxId = -1;
    QHash<int, Contact>::iterator it, ite, bestMatch;
    while (!pending.isEmpty() && !candidates.isEmpty()) {
        int bestDist = -1, bestId;
        for (it = pending.begin(), ite = pending.end(); it != ite; ++it) {
            int dist;
            int id = findClosestContact(candidates, it->x, it->y, &dist);
            if (id >= 0 && (bestDist == -1 || dist < bestDist)) {
                bestDist = dist;
                bestId = id;
                bestMatch = it;
            }
        }
        if (bestDist >= 0) {
            bestMatch->trackingId = bestId;
            newContacts.insert(bestId, *bestMatch);
            candidates.remove(bestId);
            pending.erase(bestMatch);
            if (bestId > maxId)
                maxId = bestId;
        }
    }
    if (candidates.isEmpty()) {
        for (it = pending.begin(), ite = pending.end(); it != ite; ++it) {
            it->trackingId = ++maxId;
            newContacts.insert(it->trackingId, *it);
        }
    }
    m_contacts = newContacts;
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
