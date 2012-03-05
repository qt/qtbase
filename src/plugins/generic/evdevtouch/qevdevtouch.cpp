/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevtouch.h"
#include <QStringList>
#include <QHash>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QDebug>
#include <QtCore/private/qcore_unix_p.h>
#include <QtPlatformSupport/private/qudevhelper_p.h>
#include <linux/input.h>

#ifdef USE_MTDEV
extern "C" {
#include <mtdev.h>
}
#endif

QT_BEGIN_NAMESPACE

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif

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
    QHash<int, Contact> m_contacts; // The key is a tracking id for type A, slot number for type B.
    QHash<int, Contact> m_lastContacts;
    Contact m_currentData;
    int m_currentSlot;

    int findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist);
    void reportPoints();
    void registerDevice();

    int hw_range_x_min;
    int hw_range_x_max;
    int hw_range_y_min;
    int hw_range_y_max;
    int hw_pressure_min;
    int hw_pressure_max;
    QString hw_name;
    bool m_forceToActiveWindow;
    QTouchDevice *m_device;
    bool m_typeB;
};

QTouchScreenData::QTouchScreenData(QTouchScreenHandler *q_ptr, const QStringList &args)
    : q(q_ptr),
      m_lastEventType(-1),
      m_currentSlot(0),
      hw_range_x_min(0), hw_range_x_max(0),
      hw_range_y_min(0), hw_range_y_max(0),
      hw_pressure_min(0), hw_pressure_max(0)
{
    m_forceToActiveWindow = args.contains(QLatin1String("force_window"));
}

void QTouchScreenData::registerDevice()
{
    m_device = new QTouchDevice;
    m_device->setName(hw_name);
    m_device->setType(QTouchDevice::TouchScreen);
    m_device->setCapabilities(QTouchDevice::Position | QTouchDevice::Area);
    if (hw_pressure_max > hw_pressure_min)
        m_device->setCapabilities(m_device->capabilities() | QTouchDevice::Pressure);

    QWindowSystemInterface::registerTouchDevice(m_device);
}

#define LONG_BITS (sizeof(long) << 3)
#define NUM_LONGS(bits) (((bits) + LONG_BITS - 1) / LONG_BITS)

static inline bool testBit(long bit, const long *array)
{
    return array[bit / LONG_BITS] & (1 << (bit & (LONG_BITS - 1)));
}

QTouchScreenHandler::QTouchScreenHandler(const QString &spec)
    : m_notify(0), m_fd(-1), d(0)
#ifdef USE_MTDEV
      , m_mtdev(0)
#endif
{
    setObjectName(QLatin1String("Evdev Touch Handler"));

    QString dev;
    q_udev_devicePath(UDev_Touchpad | UDev_Touchscreen, &dev);
    if (dev.isEmpty())
        dev = QLatin1String("/dev/input/event0");

    QStringList args = spec.split(QLatin1Char(':'));
    for (int i = 0; i < args.count(); ++i)
        if (args.at(i).startsWith(QLatin1String("/dev/")))
            dev = args.at(i);

    qDebug("evdevtouch: Using device %s", qPrintable(dev));
    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);

    if (m_fd >= 0) {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readData()));
    } else {
        qWarning("Cannot open input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }

#ifdef USE_MTDEV
    m_mtdev = static_cast<mtdev *>(calloc(1, sizeof(mtdev)));
    int mtdeverr = mtdev_open(m_mtdev, m_fd);
    if (mtdeverr) {
        qWarning("mtdev_open failed: %d", mtdeverr);
        QT_CLOSE(m_fd);
        return;
    }
#endif

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

#ifdef USE_MTDEV
    const char *mtdevStr = "(mtdev)";
    d->m_typeB = true;
#else
    const char *mtdevStr = "";
    d->m_typeB = false;
    long absbits[NUM_LONGS(ABS_CNT)];
    if (ioctl(m_fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) >= 0)
        d->m_typeB = testBit(ABS_MT_SLOT, absbits);
#endif
    qDebug("Protocol type %c %s", d->m_typeB ? 'B' : 'A', mtdevStr);

    d->registerDevice();
}

QTouchScreenHandler::~QTouchScreenHandler()
{
#ifdef USE_MTDEV
    if (m_mtdev) {
        mtdev_close(m_mtdev);
        free(m_mtdev);
    }
#endif

    if (m_fd >= 0)
        QT_CLOSE(m_fd);

    delete d;
}

void QTouchScreenHandler::readData()
{
    ::input_event buffer[32];
    int n = 0;
    for (; ;) {
#ifdef USE_MTDEV
        n = mtdev_get(m_mtdev, m_fd, buffer, sizeof(buffer) / sizeof(::input_event));
        if (n > 0)
            n *= sizeof(::input_event);
#else
        n = QT_READ(m_fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);
#endif
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
            if (m_typeB)
                m_contacts[m_currentSlot].x = m_currentData.x;
        } else if (data->code == ABS_MT_POSITION_Y) {
            m_currentData.y = qBound(hw_range_y_min, data->value, hw_range_y_max);
            if (m_typeB)
                m_contacts[m_currentSlot].y = m_currentData.y;
        } else if (data->code == ABS_MT_TRACKING_ID) {
            m_currentData.trackingId = data->value;
            if (m_typeB) {
                if (m_currentData.trackingId == -1)
                    m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
                else
                    m_contacts[m_currentSlot].trackingId = m_currentData.trackingId;
            }
        } else if (data->code == ABS_MT_TOUCH_MAJOR) {
            m_currentData.maj = data->value;
            if (data->value == 0)
                m_currentData.state = Qt::TouchPointReleased;
            if (m_typeB)
                m_contacts[m_currentSlot].maj = m_currentData.maj;
        } else if (data->code == ABS_PRESSURE) {
            m_currentData.pressure = qBound(hw_pressure_min, data->value, hw_pressure_max);
            if (m_typeB)
                m_contacts[m_currentSlot].pressure = m_currentData.pressure;
        } else if (data->code == ABS_MT_SLOT) {
            m_currentSlot = data->value;
        }

    } else if (data->type == EV_SYN && data->code == SYN_MT_REPORT && m_lastEventType != EV_SYN) {

        // If there is no tracking id, one will be generated later.
        // Until that use a temporary key.
        int key = m_currentData.trackingId;
        if (key == -1)
            key = m_contacts.count();

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

            int key = m_typeB ? it.key() : contact.trackingId;
            if (m_lastContacts.contains(key)) {
                const Contact &prev(m_lastContacts.value(key));
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
                    && !m_lastContacts.contains(key)) {
                it.remove();
                continue;
            }

            tp.state = contact.state;
            combinedStates |= tp.state;

            // Store the HW coordinates for now, will be updated later.
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
        if (!m_typeB)
            m_contacts.clear();

        if (!m_touchPoints.isEmpty() && combinedStates != Qt::TouchPointStationary)
            reportPoints();
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

void QTouchScreenData::reportPoints()
{
    QRect winRect;
    if (m_forceToActiveWindow) {
        QWindow *win = QGuiApplication::activeWindow();
        if (!win)
            return;
        winRect = win->geometry();
    } else {
        winRect = QGuiApplication::primaryScreen()->geometry();
    }

    const int hw_w = hw_range_x_max - hw_range_x_min;
    const int hw_h = hw_range_y_max - hw_range_y_min;

    // Map the coordinates based on the normalized position. QPA expects 'area'
    // to be in screen coordinates.
    const int pointCount = m_touchPoints.count();
    for (int i = 0; i < pointCount; ++i) {
        QWindowSystemInterface::TouchPoint &tp(m_touchPoints[i]);

        // Generate a screen position that is always inside the active window
        // or the primary screen.
        const int wx = winRect.left() + int(tp.normalPosition.x() * winRect.width());
        const int wy = winRect.top() + int(tp.normalPosition.y() * winRect.height());
        const qreal sizeRatio = (winRect.width() + winRect.height()) / qreal(hw_w + hw_h);
        tp.area = QRect(0, 0, tp.area.width() * sizeRatio, tp.area.height() * sizeRatio);
        tp.area.moveCenter(QPoint(wx, wy));

        // Calculate normalized pressure.
        if (!hw_pressure_min && !hw_pressure_max)
            tp.pressure = tp.state == Qt::TouchPointReleased ? 0 : 1;
        else
            tp.pressure = (tp.pressure - hw_pressure_min) / qreal(hw_pressure_max - hw_pressure_min);
    }

    QWindowSystemInterface::handleTouchEvent(0, m_device, m_touchPoints);
}


QTouchScreenHandlerThread::QTouchScreenHandlerThread(const QString &spec)
    : m_spec(spec), m_handler(0)
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
    exec();
    delete m_handler;
    m_handler = 0;
}


QT_END_NAMESPACE
