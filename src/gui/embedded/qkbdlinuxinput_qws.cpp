/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qkbdlinuxinput_qws.h"

#ifndef QT_NO_QWS_KEYBOARD

#include <QSocketNotifier>
#include <QStringList>

#include <qplatformdefs.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN

#include <errno.h>
#include <termios.h>

#include <linux/kd.h>
#include <linux/input.h>

QT_BEGIN_NAMESPACE


class QWSLinuxInputKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSLinuxInputKbPrivate(QWSLinuxInputKeyboardHandler *, const QString &);
    ~QWSLinuxInputKbPrivate();

private:
    void switchLed(int, bool);

private Q_SLOTS:
    void readKeycode();

private:
    QWSLinuxInputKeyboardHandler *m_handler;
    int                           m_fd;
    int                           m_tty_fd;
    struct termios                m_tty_attr;
    int                           m_orig_kbmode;
};

QWSLinuxInputKeyboardHandler::QWSLinuxInputKeyboardHandler(const QString &device)
    : QWSKeyboardHandler(device)
{
    d = new QWSLinuxInputKbPrivate(this, device);
}

QWSLinuxInputKeyboardHandler::~QWSLinuxInputKeyboardHandler()
{
    delete d;
}

bool QWSLinuxInputKeyboardHandler::filterInputEvent(quint16 &, qint32 &)
{
    return false;
}

QWSLinuxInputKbPrivate::QWSLinuxInputKbPrivate(QWSLinuxInputKeyboardHandler *h, const QString &device)
    : m_handler(h), m_fd(-1), m_tty_fd(-1), m_orig_kbmode(K_XLATE)
{
    setObjectName(QLatin1String("LinuxInputSubsystem Keyboard Handler"));

    QString dev = QLatin1String("/dev/input/event1");
    int repeat_delay = -1;
    int repeat_rate = -1;

    QStringList args = device.split(QLatin1Char(':'));
    foreach (const QString &arg, args) {
        if (arg.startsWith(QLatin1String("repeat-delay=")))
            repeat_delay = arg.mid(13).toInt();
        else if (arg.startsWith(QLatin1String("repeat-rate=")))
            repeat_rate = arg.mid(12).toInt();
        else if (arg.startsWith(QLatin1String("/dev/")))
            dev = arg;
    }

    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDWR, 0);
    if (m_fd >= 0) {
        if (repeat_delay > 0 && repeat_rate > 0) {
            int kbdrep[2] = { repeat_delay, repeat_rate };
            ::ioctl(m_fd, EVIOCSREP, kbdrep);
        }

        QSocketNotifier *notifier;
        notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));

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
    } else {
        qWarning("Cannot open keyboard input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }
}

QWSLinuxInputKbPrivate::~QWSLinuxInputKbPrivate()
{
    if (m_tty_fd >= 0) {
        ::ioctl(m_tty_fd, KDSKBMODE, m_orig_kbmode);
        tcsetattr(m_tty_fd, TCSANOW, &m_tty_attr);
    }
    if (m_fd >= 0)
        QT_CLOSE(m_fd);
}

void QWSLinuxInputKbPrivate::switchLed(int led, bool state)
{
    struct ::input_event led_ie;
    ::gettimeofday(&led_ie.time, 0);
    led_ie.type = EV_LED;
    led_ie.code = led;
    led_ie.value = state;

    QT_WRITE(m_fd, &led_ie, sizeof(led_ie));
}

void QWSLinuxInputKbPrivate::readKeycode()
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

#include "qkbdlinuxinput_qws.moc"

#endif // QT_NO_QWS_KEYBOARD
