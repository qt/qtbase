/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfbvthandler_p.h"
#include <QtCore/QSocketNotifier>
#include <QtCore/private/qglobal_p.h>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && (QT_CONFIG(evdev) || QT_CONFIG(libinput))

#define VTH_ENABLED

#include <private/qcore_unix_p.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
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
    static bool ignore = qEnvironmentVariableIntValue("QT_QPA_PRESERVE_CONSOLE_STATE");
    if (ignore)
        return;

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

#ifdef VTH_ENABLED
static QFbVtHandler *vth;

void QFbVtHandler::signalHandler(int sigNo)
{
    char a = sigNo;
    QT_WRITE(vth->m_sigFd[0], &a, sizeof(a));
}
#endif

QFbVtHandler::QFbVtHandler(QObject *parent)
    : QObject(parent),
      m_tty(-1),
      m_signalNotifier(0)
{
#ifdef VTH_ENABLED
    if (isatty(0))
        m_tty = 0;

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigFd)) {
        qErrnoWarning(errno, "QFbVtHandler: socketpair() failed");
        return;
    }

    vth = this;
    setTTYCursor(false);
    setKeyboardEnabled(false);

    m_signalNotifier = new QSocketNotifier(m_sigFd[1], QSocketNotifier::Read, this);
    connect(m_signalNotifier, &QSocketNotifier::activated, this, &QFbVtHandler::handleSignal);

    if (!qEnvironmentVariableIntValue("QT_QPA_NO_SIGNAL_HANDLER")) {
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, 0); // Ctrl+C
        sigaction(SIGTSTP, &sa, 0); // Ctrl+Z
        sigaction(SIGCONT, &sa, 0);
        sigaction(SIGTERM, &sa, 0); // default signal used by kill
    }
#endif
}

QFbVtHandler::~QFbVtHandler()
{
#ifdef VTH_ENABLED
    setKeyboardEnabled(true);
    setTTYCursor(true);

    if (m_signalNotifier) {
        close(m_sigFd[0]);
        close(m_sigFd[1]);
    }
#endif
}

void QFbVtHandler::setKeyboardEnabled(bool enable)
{
#ifdef VTH_ENABLED
    if (m_tty == -1)
        return;

    if (enable) {
        ::ioctl(m_tty, KDSKBMUTE, 0);
        ::ioctl(m_tty, KDSKBMODE, m_oldKbdMode);
    } else {
        ::ioctl(m_tty, KDGKBMODE, &m_oldKbdMode);
        if (!qEnvironmentVariableIntValue("QT_QPA_ENABLE_TERMINAL_KEYBOARD")) {
            ::ioctl(m_tty, KDSKBMUTE, 1);
            ::ioctl(m_tty, KDSKBMODE, KBD_OFF_MODE);
        }
    }
#else
    Q_UNUSED(enable);
#endif
}

void QFbVtHandler::handleSignal()
{
#ifdef VTH_ENABLED
    m_signalNotifier->setEnabled(false);

    char sigNo;
    if (QT_READ(m_sigFd[1], &sigNo, sizeof(sigNo)) == sizeof(sigNo)) {
        switch (sigNo) {
        case SIGINT:
        case SIGTERM:
            handleInt();
            break;
        case SIGTSTP:
            emit aboutToSuspend();
            setKeyboardEnabled(true);
            setTTYCursor(true);
            ::kill(getpid(), SIGSTOP);
            break;
        case SIGCONT:
            setTTYCursor(false);
            setKeyboardEnabled(false);
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
    setKeyboardEnabled(true);
    setTTYCursor(true);
    _exit(1);
#endif
}

QT_END_NAMESPACE
