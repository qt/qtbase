/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevtouch_p.h"
#include <QStringList>
#include <QHash>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QDebug>
#include <QtCore/private/qcore_unix_p.h>
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <linux/input.h>

#ifdef USE_MTDEV
extern "C" {
#include <mtdev.h>
}
#endif

QT_BEGIN_NAMESPACE

/* android (and perhaps some other linux-derived stuff) don't define everything
 * in linux/input.h, so we'll need to do that ourselves.
 */
#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#endif
#ifndef ABS_MT_POSITION_X
#define ABS_MT_POSITION_X 0x35    /* Center X ellipse position */
#endif
#ifndef ABS_MT_POSITION_Y
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#endif
#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif
#ifndef ABS_CNT
#define ABS_CNT                 (ABS_MAX+1)
#endif
#ifndef ABS_MT_TRACKING_ID
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#endif
#ifndef SYN_MT_REPORT
#define SYN_MT_REPORT           2
#endif

class QEvdevTouchScreenData
{
public:
    QEvdevTouchScreenData(QEvdevTouchScreenHandler *q_ptr, const QStringList &args);

    void processInputEvent(input_event *data);
    void assignIds();

    QEvdevTouchScreenHandler *q;
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
            x(0), y(0), maj(-1), pressure(0),
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
    QTransform m_rotate;
};

QEvdevTouchScreenData::QEvdevTouchScreenData(QEvdevTouchScreenHandler *q_ptr, const QStringList &args)
    : q(q_ptr),
      m_lastEventType(-1),
      m_currentSlot(0),
      hw_range_x_min(0), hw_range_x_max(0),
      hw_range_y_min(0), hw_range_y_max(0),
      hw_pressure_min(0), hw_pressure_max(0),
      m_device(0), m_typeB(false)
{
    m_forceToActiveWindow = args.contains(QLatin1String("force_window"));
}

void QEvdevTouchScreenData::registerDevice()
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
    return (array[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}

QEvdevTouchScreenHandler::QEvdevTouchScreenHandler(const QString &specification, QObject *parent)
    : QObject(parent), m_notify(0), m_fd(-1), d(0)
#ifdef USE_MTDEV
      , m_mtdev(0)
#endif
{
    setObjectName(QLatin1String("Evdev Touch Handler"));

    QString dev;

    // only the first device argument is used for now
    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    QStringList args = spec.split(QLatin1Char(':'));

    int rotationAngle = 0;
    for (int i = 0; i < args.count(); ++i) {
        if (args.at(i).startsWith(QLatin1String("/dev/")) && dev.isEmpty()) {
            dev = args.at(i);
        } else if (args.at(i).startsWith(QLatin1String("rotate"))) {
            QString rotateArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = rotateArg.toUInt(&ok);
            if (ok) {
                switch (argValue) {
                case 90:
                case 180:
                case 270:
                    rotationAngle = argValue;
                default:
                    break;
                }
            }
        }
    }

    if (dev.isEmpty()) {
        // try to let udev scan for already connected devices
        QScopedPointer<QDeviceDiscovery> deviceDiscovery(QDeviceDiscovery::create(QDeviceDiscovery::Device_Touchpad | QDeviceDiscovery::Device_Touchscreen, this));
        if (deviceDiscovery) {
            QStringList devices = deviceDiscovery->scanConnectedDevices();

            // only the first device found is used for now
            if (devices.size() > 0)
                dev = devices[0];
        }
    }

    if (dev.isEmpty())
        return;

    qDebug("evdevtouch: Using device %s", qPrintable(dev));
    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);

    if (m_fd >= 0) {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readData()));
    } else {
        qErrnoWarning(errno, "Cannot open input device %s", qPrintable(dev));
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

    d = new QEvdevTouchScreenData(this, args);

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

    bool grabSuccess = !ioctl(m_fd, EVIOCGRAB, (void *) 1);
    if (grabSuccess)
        ioctl(m_fd, EVIOCGRAB, (void *) 0);
    else
        qWarning("ERROR: The device is grabbed by another process. No events will be read.");

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

    if (rotationAngle)
        d->m_rotate = QTransform::fromTranslate(0.5, 0.5).rotate(rotationAngle).translate(-0.5, -0.5);

    d->registerDevice();
}

QEvdevTouchScreenHandler::~QEvdevTouchScreenHandler()
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

void QEvdevTouchScreenHandler::readData()
{
    ::input_event buffer[32];
    int n = 0;
    for (; ;) {
#ifdef USE_MTDEV
        int result = mtdev_get(m_mtdev, m_fd, buffer, sizeof(buffer) / sizeof(::input_event));
        if (result > 0)
            result *= sizeof(::input_event);
#else
        int result = QT_READ(m_fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);
#endif
        if (!result) {
            qWarning("Got EOF from input device");
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qWarning("Could not read from input device: %s", strerror(errno));
                if (errno == ENODEV) { // device got disconnected -> stop reading
                    delete m_notify;
                    m_notify = 0;
                    QT_CLOSE(m_fd);
                    m_fd = -1;
                }
                return;
            }
        } else {
            n += result;
            if (n % sizeof(::input_event) == 0)
                break;
        }
    }

    n /= sizeof(::input_event);

    for (int i = 0; i < n; ++i)
        d->processInputEvent(&buffer[i]);
}

void QEvdevTouchScreenData::processInputEvent(input_event *data)
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

    } else if (data->type == EV_KEY && !m_typeB) {
        if (data->code == BTN_TOUCH && data->value == 0)
          {
            m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
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

            if (!m_rotate.isIdentity())
                tp.normalPosition = m_rotate.map(tp.normalPosition);

            tp.rawPositions.append(QPointF(contact.x, contact.y));

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

int QEvdevTouchScreenData::findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist)
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

void QEvdevTouchScreenData::assignIds()
{
    QHash<int, Contact> candidates = m_lastContacts, pending = m_contacts, newContacts;
    int maxId = -1;
    QHash<int, Contact>::iterator it, ite, bestMatch;
    while (!pending.isEmpty() && !candidates.isEmpty()) {
        int bestDist = -1, bestId = 0;
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

void QEvdevTouchScreenData::reportPoints()
{
    QRect winRect;
    if (m_forceToActiveWindow) {
        QWindow *win = QGuiApplication::focusWindow();
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
        // or the primary screen.  Even though we report this as a QRectF, internally
        // Qt uses QRect/QPoint so we need to bound the size to winRect.size() - QSize(1, 1)
        const qreal wx = winRect.left() + tp.normalPosition.x() * (winRect.width() - 1);
        const qreal wy = winRect.top() + tp.normalPosition.y() * (winRect.height() - 1);
        const qreal sizeRatio = (winRect.width() + winRect.height()) / qreal(hw_w + hw_h);
        if (tp.area.width() == -1) // touch major was not provided
            tp.area = QRectF(0, 0, 8, 8);
        else
            tp.area = QRectF(0, 0, tp.area.width() * sizeRatio, tp.area.height() * sizeRatio);
        tp.area.moveCenter(QPointF(wx, wy));

        // Calculate normalized pressure.
        if (!hw_pressure_min && !hw_pressure_max)
            tp.pressure = tp.state == Qt::TouchPointReleased ? 0 : 1;
        else
            tp.pressure = (tp.pressure - hw_pressure_min) / qreal(hw_pressure_max - hw_pressure_min);
    }

    QWindowSystemInterface::handleTouchEvent(0, m_device, m_touchPoints);
}


QEvdevTouchScreenHandlerThread::QEvdevTouchScreenHandlerThread(const QString &spec, QObject *parent)
    : QThread(parent), m_spec(spec), m_handler(0)
{
    start();
}

QEvdevTouchScreenHandlerThread::~QEvdevTouchScreenHandlerThread()
{
    quit();
    wait();
}

void QEvdevTouchScreenHandlerThread::run()
{
    m_handler = new QEvdevTouchScreenHandler(m_spec);
    exec();
    delete m_handler;
    m_handler = 0;
}


QT_END_NAMESPACE
