/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfbvthandler_p.h"
#include <QtCore/QSocketNotifier>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && (!defined(QT_NO_EVDEV) || !defined(QT_NO_LIBINPUT))

#define VTH_ENABLED

#include <private/qcore_unix_p.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#ifndef KDSKBMUTE
#define KDSKBMUTE 0x4B51
#endif

#ifdef K_OFF
#define KBD_OFF_MODE K_OFF
#else
#define KBD_OFF_MODE K_RAW
#endif

#endif

QT_BEGIN_NAMESPACE

#ifdef VTH_ENABLED
static void setTTYCursor(bool enable)
{
    const char * const devs[] = { "/dev/tty0", "/dev/tty", "/dev/console", 0 };
    int fd = -1;
    for (const char * const *dev = devs; *dev; ++dev) {
        fd = QT_OPEN(*dev, O_RDWR);
        if (fd != -1) {
            // Enable/disable screen blanking and the blinking cursor.
            const char *termctl = enable ? "\033[9;15]\033[?33h\033[?25h\033[?0c" : "\033[9;0]\033[?33l\033[?25l\033[?1c";
            QT_WRITE(fd, termctl, strlen(termctl) + 1);
            QT_CLOSE(fd);
            return;
        }
    }
}
#endif

QFbVtHandler::QFbVtHandler(QObject *parent)
    : QObject(parent),
      m_tty(-1),
      m_signalFd(-1),
      m_signalNotifier(0)
{
#ifdef VTH_ENABLED
    setTTYCursor(false);

    if (isatty(0)) {
        m_tty = 0;
        ioctl(m_tty, KDGKBMODE, &m_oldKbdMode);

        if (!qEnvironmentVariableIntValue("QT_QPA_ENABLE_TERMINAL_KEYBOARD")) {
            // Disable the tty keyboard.
            ioctl(m_tty, KDSKBMUTE, 1);
            ioctl(m_tty, KDSKBMODE, KBD_OFF_MODE);
        }
    }

    // SIGSEGV and such cannot safely be blocked. We cannot handle them in an
    // async-safe manner either. Restoring the keyboard, video mode, etc. may
    // all contain calls that cannot safely be made from a signal handler.

    // Other signals: block them and use signalfd.
    sigset_t mask;
    sigemptyset(&mask);

    // Catch Ctrl+C.
    sigaddset(&mask, SIGINT);

    // Ctrl+Z. Up to the platform plugins to handle it in a meaningful way.
    sigaddset(&mask, SIGTSTP);
    sigaddset(&mask, SIGCONT);

    // Default signal used by kill. To overcome the common issue of no cleaning
    // up when killing a locally started app via a remote session.
    sigaddset(&mask, SIGTERM);

    m_signalFd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (m_signalFd < 0) {
        qErrnoWarning(errno, "signalfd() failed");
    } else {
        m_signalNotifier = new QSocketNotifier(m_signalFd, QSocketNotifier::Read, this);
        connect(m_signalNotifier, &QSocketNotifier::activated, this, &QFbVtHandler::handleSignal);

        // Block the signals that are handled via signalfd. Applies only to the current
        // thread, but new threads will inherit the creator's signal mask.
        pthread_sigmask(SIG_BLOCK, &mask, 0);
    }
#endif
}

QFbVtHandler::~QFbVtHandler()
{
#ifdef VTH_ENABLED
    restoreKeyboard();
    setTTYCursor(true);

    if (m_signalFd != -1)
        close(m_signalFd);
#endif
}

void QFbVtHandler::restoreKeyboard()
{
#ifdef VTH_ENABLED
    if (m_tty == -1)
        return;

    ioctl(m_tty, KDSKBMUTE, 0);
    ioctl(m_tty, KDSKBMODE, m_oldKbdMode);
#endif
}

// To be called from the slot connected to suspendRequested() in case the
// platform plugin does in fact allow suspending on Ctrl+Z.
void QFbVtHandler::suspend()
{
#ifdef VTH_ENABLED
    kill(getpid(), SIGSTOP);
#endif
}

void QFbVtHandler::handleSignal()
{
#ifdef VTH_ENABLED
    m_signalNotifier->setEnabled(false);

    signalfd_siginfo sig;
    if (read(m_signalFd, &sig, sizeof(sig)) == sizeof(sig)) {
        switch (sig.ssi_signo) {
        case SIGINT: // fallthrough
        case SIGTERM:
            handleInt();
            break;
        case SIGTSTP:
            emit suspendRequested();
            break;
        case SIGCONT:
            emit resumed();
            break;
        default:
            break;
        }
    }

    m_signalNotifier->setEnabled(true);
#endif
}

void QFbVtHandler::handleInt()
{
#ifdef VTH_ENABLED
    emit interrupted();
    restoreKeyboard();
    setTTYCursor(true);
    _exit(1);
#endif
}

QT_END_NAMESPACE
