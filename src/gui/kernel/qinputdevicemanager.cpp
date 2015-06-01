/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qinputdevicemanager_p.h"
#include "qinputdevicemanager_p_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QInputDeviceManager
  \internal

  \brief QInputDeviceManager acts as a communication hub between QtGui and the input handlers.

  On embedded platforms the input handling code is either compiled into the platform
  plugin or is loaded dynamically as a generic plugin without any interface. The input
  handler in use may also change between each run (e.g. evdevmouse/keyboard/touch
  vs. libinput). QWindowSystemInterface is too limiting when Qt (the platform plugin) is
  acting as a windowing system, and is one way only.

  QInputDeviceManager solves this by providing a global object that is used to communicate
  from the input handlers to the rest of Qt (e.g. the number of connected mice, which may
  be important information for the cursor drawing code), and vice-versa (e.g. to indicate
  to the input handler that a manual cursor position change was requested by the
  application via QCursor::setPos and thus any internal state has to be updated accordingly).
*/

QInputDeviceManager::QInputDeviceManager(QObject *parent)
    : QObject(*new QInputDeviceManagerPrivate, parent)
{
    qRegisterMetaType<DeviceType>();
}

int QInputDeviceManager::deviceCount(DeviceType type) const
{
    Q_D(const QInputDeviceManager);
    return d->deviceCount(type);
}

int QInputDeviceManagerPrivate::deviceCount(QInputDeviceManager::DeviceType type) const
{
    return m_deviceCount.value(type);
}

void QInputDeviceManagerPrivate::setDeviceCount(QInputDeviceManager::DeviceType type, int count)
{
    Q_Q(QInputDeviceManager);
    if (m_deviceCount.value(type) != count) {
        m_deviceCount[type] = count;
        emit q->deviceListChanged(type);
    }
}

void QInputDeviceManager::setCursorPos(const QPoint &pos)
{
    emit cursorPositionChangeRequested(pos);
}

QT_END_NAMESPACE
