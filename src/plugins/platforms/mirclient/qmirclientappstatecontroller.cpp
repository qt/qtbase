/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
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


#include "qmirclientappstatecontroller.h"

#include <qpa/qwindowsysteminterface.h>

/*
 * QMirClientAppStateController - updates Qt's QApplication::applicationState property.
 *
 * Tries to avoid active-inactive-active invocations using a timer. The rapid state
 * change can confuse some applications.
 */

QMirClientAppStateController::QMirClientAppStateController()
    : m_suspended(false)
    , m_lastActive(true)
{
    m_inactiveTimer.setSingleShot(true);
    m_inactiveTimer.setInterval(10);
    QObject::connect(&m_inactiveTimer, &QTimer::timeout, []()
    {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
    });
}

void QMirClientAppStateController::setSuspended()
{
    m_inactiveTimer.stop();
    if (!m_suspended) {
        m_suspended = true;

        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationSuspended);
    }
}

void QMirClientAppStateController::setResumed()
{
    m_inactiveTimer.stop();
    if (m_suspended) {
        m_suspended = false;

        if (m_lastActive) {
            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
        } else {
            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        }
    }
}

void QMirClientAppStateController::setWindowFocused(bool focused)
{
    if (m_suspended) {
        return;
    }

    if (focused) {
        m_inactiveTimer.stop();
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
    } else {
        m_inactiveTimer.start();
    }

    m_lastActive = focused;
}
