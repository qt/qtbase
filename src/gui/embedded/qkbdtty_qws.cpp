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

#include "qkbdtty_qws.h"

#if !defined(QT_NO_QWS_KEYBOARD) && !defined(QT_NO_QWS_KBD_TTY)

#include <QSocketNotifier>
#include <QStringList>

#include <qplatformdefs.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN

#include <errno.h>
#include <termios.h>

#if defined Q_OS_LINUX
#  include <linux/kd.h>
#  include <linux/vt.h> //TODO: move vt handling somewhere else (QLinuxFbScreen?)

#  include "qscreen_qws.h"
#  include "qwindowsystem_qws.h"
#  include "qapplication.h"
#  include "private/qwindowsurface_qws_p.h"
#  include "private/qwssignalhandler_p.h"

#  define VTACQSIG SIGUSR1
#  define VTRELSIG SIGUSR2
#endif


QT_BEGIN_NAMESPACE

class QWSTtyKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSTtyKbPrivate(QWSTtyKeyboardHandler *handler, const QString &device);
    ~QWSTtyKbPrivate();

private:
    void switchLed(char, bool);
    void switchConsole(int vt);

private Q_SLOTS:
    void readKeycode();
    void handleConsoleSwitch(int sig);

private:
    QWSTtyKeyboardHandler *m_handler;
    int                    m_tty_fd;
    struct termios         m_tty_attr;
    char                   m_last_keycode;
    int                    m_vt_qws;
    int                    m_orig_kbmode;
};


QWSTtyKeyboardHandler::QWSTtyKeyboardHandler(const QString &device)
    : QWSKeyboardHandler(device)
{
    d = new QWSTtyKbPrivate(this, device);
}

QWSTtyKeyboardHandler::~QWSTtyKeyboardHandler()
{
    delete d;
}

bool QWSTtyKeyboardHandler::filterKeycode(char &)
{
    return false;
}

QWSTtyKbPrivate::QWSTtyKbPrivate(QWSTtyKeyboardHandler *h, const QString &device)
    : m_handler(h), m_tty_fd(-1), m_last_keycode(0), m_vt_qws(0), m_orig_kbmode(K_XLATE)
{
    setObjectName(QLatin1String("TTY Keyboard Handler"));
#ifndef QT_NO_QWS_SIGNALHANDLER
    QWSSignalHandler::instance()->addObject(this);
#endif

    QString dev = QLatin1String("/dev/tty0");
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

    m_tty_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDWR, 0);
    if (m_tty_fd >= 0) {
        if (repeat_delay > 0 && repeat_rate > 0) {
#if defined(Q_OS_LINUX)
            struct ::kbd_repeat kbdrep = { repeat_delay, repeat_rate };
            ::ioctl(m_tty_fd, KDKBDREP, &kbdrep);
#endif
        }

        QSocketNotifier *notifier;
        notifier = new QSocketNotifier(m_tty_fd, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));

        // save tty config for restore.
        tcgetattr(m_tty_fd, &m_tty_attr);

        struct ::termios termdata;
        tcgetattr(m_tty_fd, &termdata);

#if defined(Q_OS_LINUX)
        // record the original mode so we can restore it again in the destructor.
        ::ioctl(m_tty_fd, KDGKBMODE, &m_orig_kbmode);

        // PLEASE NOTE:
        // the tty keycode interface can only report keycodes 0x01 .. 0x7f
        // KEY_MAX is however defined to 0x1ff. In practice this is sufficient
        // for a PC style keyboard though.
        // we don't support K_RAW anymore - if you need that, you have to add
        // a scan- to keycode converter yourself.
        ::ioctl(m_tty_fd, KDSKBMODE, K_MEDIUMRAW);
#endif

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

#if defined(Q_OS_LINUX)
        // VT switching is handled via unix signals
        connect(QApplication::instance(), SIGNAL(unixSignal(int)), this, SLOT(handleConsoleSwitch(int)));
        QApplication::instance()->watchUnixSignal(VTACQSIG, true);
        QApplication::instance()->watchUnixSignal(VTRELSIG, true);

        struct ::vt_mode vtMode;
        if (::ioctl(m_tty_fd, VT_GETMODE, &vtMode) == 0) {
            vtMode.mode = VT_PROCESS;
            vtMode.relsig = VTRELSIG;
            vtMode.acqsig = VTACQSIG;

            if (::ioctl(m_tty_fd, VT_SETMODE, &vtMode) == 0) {
                struct ::vt_stat vtStat;
                ::memset(&vtStat, 0, sizeof(vtStat));

                if (::ioctl(m_tty_fd, VT_GETSTATE, &vtStat) == 0 ) {
                    m_vt_qws = vtStat.v_active;
                }
            }
        }

        if (!m_vt_qws)
            qWarning("Could not initialize virtual console switching");
#endif
    } else {
        qWarning("Cannot open input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }

}

QWSTtyKbPrivate::~QWSTtyKbPrivate()
{
    if (m_tty_fd >= 0) {
#if defined(Q_OS_LINUX)
        ::ioctl(m_tty_fd, KDSKBMODE, m_orig_kbmode);
#endif
        tcsetattr(m_tty_fd, TCSANOW, &m_tty_attr);
        QT_CLOSE(m_tty_fd);
    }
}



void QWSTtyKbPrivate::switchLed(char led, bool state)
{
#if defined(Q_OS_LINUX)
    char ledstate;

    ::ioctl(m_tty_fd, KDGETLED, &ledstate);
    if (state)
        ledstate |= led;
    else
        ledstate &= ~led;
    ::ioctl(m_tty_fd, KDSETLED, ledstate);
#endif
}

void QWSTtyKbPrivate::readKeycode()
{
    char buffer[32];
    int n = 0;

    forever {
        n = QT_READ(m_tty_fd, buffer + n, 32 - n);

        if (n == 0) {
            qWarning("Got EOF from the input device.");
            return;
        } else if (n < 0 && (errno != EINTR && errno != EAGAIN)) {
            qWarning("Could not read from input device: %s", strerror(errno));
            return;
        } else {
            break;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (m_handler->filterKeycode(buffer[i]))
            continue;

        QWSKeyboardHandler::KeycodeAction ka;
        ka = m_handler->processKeycode(buffer[i] & 0x7f, (buffer[i] & 0x80) == 0x00, buffer[i] == m_last_keycode);
        m_last_keycode = buffer[i];

        switch (ka) {
        case QWSKeyboardHandler::CapsLockOn:
        case QWSKeyboardHandler::CapsLockOff:
            switchLed(LED_CAP, ka == QWSKeyboardHandler::CapsLockOn);
            break;

        case QWSKeyboardHandler::NumLockOn:
        case QWSKeyboardHandler::NumLockOff:
            switchLed(LED_NUM, ka == QWSKeyboardHandler::NumLockOn);
            break;

        case QWSKeyboardHandler::ScrollLockOn:
        case QWSKeyboardHandler::ScrollLockOff:
            switchLed(LED_SCR, ka == QWSKeyboardHandler::ScrollLockOn);
            break;

        case QWSKeyboardHandler::PreviousConsole:
            switchConsole(qBound(1, m_vt_qws - 1, 10));
            break;

        case QWSKeyboardHandler::NextConsole:
            switchConsole(qBound(1, m_vt_qws + 1, 10));
            break;

        default:
            if (ka >= QWSKeyboardHandler::SwitchConsoleFirst &&
                ka <= QWSKeyboardHandler::SwitchConsoleLast) {
                switchConsole(1 + (ka & QWSKeyboardHandler::SwitchConsoleMask));
            }
            //ignore reboot
            break;
        }
    }
}


void QWSTtyKbPrivate::switchConsole(int vt)
{
#if defined(Q_OS_LINUX)
    if (m_vt_qws && vt && (m_tty_fd >= 0 ))
        ::ioctl(m_tty_fd, VT_ACTIVATE, vt);
#endif
}

void QWSTtyKbPrivate::handleConsoleSwitch(int sig)
{
#if defined(Q_OS_LINUX)
    // received a notification from the kernel that the current VT is
    // changing: either enable or disable QWS painting accordingly.

    if (sig == VTACQSIG) {
        if (::ioctl(m_tty_fd, VT_RELDISP, VT_ACKACQ) == 0) {
            qwsServer->enablePainting(true);
            qt_screen->restore();
            qwsServer->resumeMouse();
            qwsServer->refresh();
        }
    } else if (sig == VTRELSIG) {
        qwsServer->enablePainting(false);

        // Check for reserved surfaces which might still do painting
        bool allWindowsHidden = true;
        const QList<QWSWindow*> windows = QWSServer::instance()->clientWindows();
        for (int i = 0; i < windows.size(); ++i) {
            const QWSWindow *w = windows.at(i);
            QWSWindowSurface *s = w->windowSurface();
            if (s && s->isRegionReserved() && !w->allocatedRegion().isEmpty()) {
                allWindowsHidden = false;
                break;
            }
        }

        if (!allWindowsHidden) {
            ::ioctl(m_tty_fd, VT_RELDISP, 0); // abort console switch
            qwsServer->enablePainting(true);
        } else if (::ioctl(m_tty_fd, VT_RELDISP, 1) == 0) {
            qt_screen->save();
            qwsServer->suspendMouse();
        } else {
            qwsServer->enablePainting(true);
        }
    }
#endif
}

QT_END_NAMESPACE

#include "qkbdtty_qws.moc"

#endif // QT_NO_QWS_KEYBOARD || QT_NO_QWS_KBD_TTY
