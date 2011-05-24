/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qlinuxinput.h"


#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QWindowSystemInterface>

#include <qkbd_qws.h>


#include <qplatformdefs.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN

#include <errno.h>
#include <termios.h>

#include <linux/kd.h>
#include <linux/input.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE


//#define QT_QPA_EXPERIMENTAL_TOUCHEVENT

#ifdef QT_QPA_EXPERIMENTAL_TOUCHEVENT
class QLinuxInputMouseHandlerData
{
public:
    QLinuxInputMouseHandlerData() :seenMT(false), state(QEvent::TouchBegin), currentIdx(0) {}

    void ensureCurrentPoint() {
        if (currentIdx >= touchPoints.size()) {
            Q_ASSERT(currentIdx == touchPoints.size());
            QWindowSystemInterface::TouchPoint tp;
            tp.id = currentIdx;
            tp.isPrimary = (currentIdx == 0);
            tp.pressure = 1;
            tp.area = QRectF(0,0,1,1);
            tp.state = Qt::TouchPointReleased; // init in neutral state
            touchPoints.append(tp);
        }
    }
    void setCurrentPoint(int i) {
        currentIdx = i;
        if (currentIdx < touchPoints.size()) {
            currentX = int(touchPoints[currentIdx].area.left());
            currentY = int(touchPoints[currentIdx].area.top());
        } else {
            currentY = currentX = -999;
        }
    }
    void advanceCurrentPoint() {
        setCurrentPoint(currentIdx + 1);
    }
    int currentPoint() { return currentIdx; }
    void   setCurrentX(int value) {
        ensureCurrentPoint();
        touchPoints[currentIdx].area.moveLeft(value);
    }
    bool currentMoved() {
        return currentX != touchPoints[currentIdx].area.left() || currentY != touchPoints[currentIdx].area.top();
    }
    void updateCurrentPos() {
        ensureCurrentPoint();
        touchPoints[currentIdx].area.moveTopLeft(QPointF(currentX, currentY));
    }
    void setCurrentState(Qt::TouchPointState state) {
        ensureCurrentPoint();
        touchPoints[currentIdx].state = state;
    }
    Qt::TouchPointState currentState() const {
        if (currentIdx < touchPoints.size())
            return touchPoints[currentIdx].state;
        return Qt::TouchPointReleased;
    }
    QList<QWindowSystemInterface::TouchPoint> touchPoints;
    int currentX;
    int currentY;
    bool seenMT;
    QEvent::Type state;
private:
        int currentIdx;
};
#endif


QLinuxInputMouseHandler::QLinuxInputMouseHandler(const QString &key,
                                                 const QString &specification)
    : m_notify(0), m_x(0), m_y(0), m_prevx(0), m_prevy(0), m_xoffset(0), m_yoffset(0), m_buttons(0), d(0)
{
    qDebug() << "QLinuxInputMouseHandler" << key << specification;


    setObjectName(QLatin1String("LinuxInputSubsystem Mouse Handler"));

    QString dev = QLatin1String("/dev/input/event0");
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
    
    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (m_fd >= 0) {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readMouseData()));
    } else {
        qWarning("Cannot open mouse input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }
#ifdef QT_QPA_EXPERIMENTAL_TOUCHEVENT
    d = new QLinuxInputMouseHandlerData;
#endif    
}


QLinuxInputMouseHandler::~QLinuxInputMouseHandler()
{
    if (m_fd >= 0)
        QT_CLOSE(m_fd);
#ifdef QT_QPA_EXPERIMENTAL_TOUCHEVENT
    delete d;
#endif
}

void QLinuxInputMouseHandler::sendMouseEvent(int x, int y, Qt::MouseButtons buttons)
{
    QPoint pos(x+m_xoffset, y+m_yoffset);
    QWindowSystemInterface::handleMouseEvent(0, pos, pos, m_buttons);
    m_prevx = x;
    m_prevy = y;
}

void QLinuxInputMouseHandler::readMouseData()
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
        bool unknown = false;
        if (data->type == EV_ABS) {
            if (data->code == ABS_X && m_x != data->value) {
                m_x = data->value;
                posChanged = true;
            } else if (data->code == ABS_Y && m_y != data->value) {
                m_y = data->value;
                posChanged = true;
            } else if (data->code == ABS_PRESSURE) {
                //ignore for now...
            } else if (data->code == ABS_TOOL_WIDTH) {
                //ignore for now...
            } else if (data->code == ABS_HAT0X) {
                //ignore for now...
            } else if (data->code == ABS_HAT0Y) {
                //ignore for now...
#ifdef QT_QPA_EXPERIMENTAL_TOUCHEVENT
            } else if (data->code == ABS_MT_POSITION_X) {
                d->currentX = data->value;
                d->seenMT = true;
            } else if (data->code == ABS_MT_POSITION_Y) {
                d->currentY = data->value;
                d->seenMT = true;
            } else if (data->code == ABS_MT_TOUCH_MAJOR) {
                if (data->value == 0)
                    d->setCurrentState(Qt::TouchPointReleased);
                //otherwise, ignore for now...
            } else if (data->code == ABS_MT_TOUCH_MINOR) {
                //ignore for now...
#endif
            } else {
                unknown = true;
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
            } else {
                unknown = true;
            }
        } else if (data->type == EV_KEY && data->code == BTN_TOUCH) {
            m_buttons = data->value ? Qt::LeftButton : Qt::NoButton;

            sendMouseEvent(m_x, m_y, m_buttons);
            pendingMouseEvent = false;
        } else if (data->type == EV_KEY && data->code >= BTN_LEFT && data->code <= BTN_MIDDLE) {
            Qt::MouseButton button = Qt::NoButton;
            switch (data->code) {
            case BTN_LEFT: button = Qt::LeftButton; break;
            case BTN_MIDDLE: button = Qt::MidButton; break;
            case BTN_RIGHT: button = Qt::RightButton; break;
            }
            if (data->value)
                m_buttons |= button;
            else
                m_buttons &= ~button;
            sendMouseEvent(m_x, m_y, m_buttons);
            pendingMouseEvent = false;
        } else if (data->type == EV_SYN && data->code == SYN_REPORT) {
            if (posChanged) {
                posChanged = false;
                if (m_compression) {
                    pendingMouseEvent = true;
                    eventCompressCount++;
                } else {
                    sendMouseEvent(m_x, m_y, m_buttons);
                }
            }
#ifdef QT_QPA_EXPERIMENTAL_TOUCHEVENT
            if (d->state == QEvent::TouchBegin && !d->seenMT) {
                //no multipoint-touch events to send
            } else {
                if (!d->seenMT)
                    d->state = QEvent::TouchEnd;

                for (int i = d->currentPoint(); i < d->touchPoints.size(); ++i) {
                    d->touchPoints[i].pressure = 0;
                    d->touchPoints[i].state = Qt::TouchPointReleased;
                }
                //qDebug() << "handleTouchEvent" << d->state << d->touchPoints.size() << d->touchPoints[0].state;
                QWindowSystemInterface::handleTouchEvent(0, d->state, QTouchEvent::TouchScreen, d->touchPoints);
                if (d->seenMT) {
                    d->state = QEvent::TouchUpdate;
                } else {
                    d->state = QEvent::TouchBegin;
                    d->touchPoints.clear();
                }
                d->setCurrentPoint(0);
                d->seenMT = false;
            }
        } else if (data->type == EV_SYN && data->code == SYN_MT_REPORT) {
            //store data for this touch point

            if (!d->seenMT) {
                d->setCurrentState(Qt::TouchPointReleased);
            } else if (d->currentState() == Qt::TouchPointReleased) {
                d->updateCurrentPos();
                d->setCurrentState(Qt::TouchPointPressed);
            } else if (d->currentMoved()) {
                d->updateCurrentPos();
                d->setCurrentState(Qt::TouchPointMoved);
            } else  {
                d->setCurrentState(Qt::TouchPointStationary);
            }
            //qDebug() << "end of point" << d->currentPoint() << d->currentX << d->currentY << d->currentState();

            //advance to next tp:
            d->advanceCurrentPoint();
#endif
        } else if (data->type == EV_MSC && data->code == MSC_SCAN) {
            // kernel encountered an unmapped key - just ignore it
            continue;
        } else {
            unknown = true;
        }
#ifdef QLINUXINPUT_EXTRA_DEBUG
        if (unknown) {
            qWarning("unknown mouse event type=%x, code=%x, value=%x", data->type, data->code, data->value);
        }
#endif        
    }
    if (m_compression && pendingMouseEvent) {
        int distanceSquared = (m_x - m_prevx)*(m_x - m_prevx) + (m_y - m_prevy)*(m_y - m_prevy);
        if (distanceSquared > m_jitterLimitSquared)
            sendMouseEvent(m_x, m_y, m_buttons);
    }
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Keyboard handler




class QWSLinuxInputKeyboardHandler : public QWSKeyboardHandler
{
public:
    QWSLinuxInputKeyboardHandler(const QString&);
    ~QWSLinuxInputKeyboardHandler();

    virtual bool filterInputEvent(quint16 &input_code, qint32 &input_value);

//private:
//    QWSLinuxInputKbPrivate *d;
};


QWSLinuxInputKeyboardHandler::QWSLinuxInputKeyboardHandler(const QString &device)
    : QWSKeyboardHandler(device)
{
}

QWSLinuxInputKeyboardHandler::~QWSLinuxInputKeyboardHandler()
{
}

bool QWSLinuxInputKeyboardHandler::filterInputEvent(quint16 &, qint32 &)
{
    return false;
}


QLinuxInputKeyboardHandler::QLinuxInputKeyboardHandler(const QString &key, const QString &specification)
    : m_handler(0), m_fd(-1), m_tty_fd(-1), m_orig_kbmode(K_XLATE)
{
    setObjectName(QLatin1String("LinuxInputSubsystem Keyboard Handler"));

    QString dev = QLatin1String("/dev/input/event1");
    int repeat_delay = -1;
    int repeat_rate = -1;

    bool ttymode = false;

    QStringList args = specification.split(QLatin1Char(':'));
    foreach (const QString &arg, args) {
        if (arg.startsWith(QLatin1String("repeat-delay=")))
            repeat_delay = arg.mid(13).toInt();
        else if (arg.startsWith(QLatin1String("repeat-rate=")))
            repeat_rate = arg.mid(12).toInt();
        else if (arg.startsWith(QLatin1String("ttymode")))
            ttymode = true;
        else if (arg.startsWith(QLatin1String("/dev/")))
            dev = arg;
    }

    m_handler = new QWSLinuxInputKeyboardHandler(dev); //This is a hack to avoid copying all the QWS code

    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDWR, 0);
    if (m_fd >= 0) {
        if (repeat_delay > 0 && repeat_rate > 0) {
            int kbdrep[2] = { repeat_delay, repeat_rate };
            ::ioctl(m_fd, EVIOCSREP, kbdrep);
        }

        QSocketNotifier *notifier;
        notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));

        if (ttymode) {
            // play nice in case we are started from a shell (e.g. for debugging)
            m_tty_fd = isatty(0) ? 0 : -1;

            if (m_tty_fd >= 0) {
                // save tty config for restore.
                tcgetattr(m_tty_fd, &m_tty_attr);

                struct ::termios termdata;
                tcgetattr(m_tty_fd, &termdata);

                // record the original mode so we can restore it again in the destructor.
                ::ioctl(m_tty_fd, KDGKBMODE, &m_orig_kbmode);

                // setting this translation mode is even needed in INPUT mode to prevent
                // the shell from also interpreting codes, if the process has a tty
                // attached: e.g. Ctrl+C wouldn't copy, but kill the application.
                ::ioctl(m_tty_fd, KDSKBMODE, K_MEDIUMRAW);

                // set the tty layer to pass-through
                termdata.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
                termdata.c_oflag = 0;
                termdata.c_cflag = CREAD | CS8;
                termdata.c_lflag = 0;
                termdata.c_cc[VTIME]=0;
                termdata.c_cc[VMIN]=1;
                cfsetispeed(&termdata, 9600);
                cfsetospeed(&termdata, 9600);
                tcsetattr(m_tty_fd, TCSANOW, &termdata);
            }
        }
    } else {
        qWarning("Cannot open keyboard input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }
}

QLinuxInputKeyboardHandler::~QLinuxInputKeyboardHandler()
{
    if (m_tty_fd >= 0) {
        ::ioctl(m_tty_fd, KDSKBMODE, m_orig_kbmode);
        tcsetattr(m_tty_fd, TCSANOW, &m_tty_attr);
    }
    if (m_fd >= 0)
        QT_CLOSE(m_fd);
    delete m_handler;
}

void QLinuxInputKeyboardHandler::switchLed(int led, bool state)
{
    struct ::input_event led_ie;
    ::gettimeofday(&led_ie.time, 0);
    led_ie.type = EV_LED;
    led_ie.code = led;
    led_ie.value = state;

    QT_WRITE(m_fd, &led_ie, sizeof(led_ie));
}



void QLinuxInputKeyboardHandler::readKeycode()
{
    struct ::input_event buffer[32];
    int n = 0;

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
        if (buffer[i].type != EV_KEY)
            continue;

        quint16 code = buffer[i].code;
        qint32 value = buffer[i].value;

        if (m_handler->filterInputEvent(code, value))
            continue;

        QWSKeyboardHandler::KeycodeAction ka;
        ka = m_handler->processKeycode(code, value != 0, value == 2);

        switch (ka) {
        case QWSKeyboardHandler::CapsLockOn:
        case QWSKeyboardHandler::CapsLockOff:
            switchLed(LED_CAPSL, ka == QWSKeyboardHandler::CapsLockOn);
            break;

        case QWSKeyboardHandler::NumLockOn:
        case QWSKeyboardHandler::NumLockOff:
            switchLed(LED_NUML, ka == QWSKeyboardHandler::NumLockOn);
            break;

        case QWSKeyboardHandler::ScrollLockOn:
        case QWSKeyboardHandler::ScrollLockOff:
            switchLed(LED_SCROLLL, ka == QWSKeyboardHandler::ScrollLockOn);
            break;

        default:
            // ignore console switching and reboot
            break;
        }
    }
}





QT_END_NAMESPACE

