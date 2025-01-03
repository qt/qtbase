// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qinputdevicemanager_p.h"
#include "qinputdevicemanager_p_p.h"

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN_TAGGED(QInputDeviceManager::DeviceType, QInputDeviceManager__DeviceType)

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

QInputDeviceManager::~QInputDeviceManager() = default;

int QInputDeviceManager::deviceCount(DeviceType type) const
{
    Q_D(const QInputDeviceManager);
    return d->deviceCount(type);
}

int QInputDeviceManagerPrivate::deviceCount(QInputDeviceManager::DeviceType type) const
{
    return m_deviceCount[type];
}

void QInputDeviceManagerPrivate::setDeviceCount(QInputDeviceManager::DeviceType type, int count)
{
    Q_Q(QInputDeviceManager);
    if (m_deviceCount[type] != count) {
        m_deviceCount[type] = count;
        emit q->deviceListChanged(type);
    }
}

void QInputDeviceManager::setCursorPos(const QPoint &pos)
{
    emit cursorPositionChangeRequested(pos);
}

/*!
    \return the keyboard modifier state stored in the QInputDeviceManager object.

    Keyboard input handlers are expected to keep this up-to-date via
    setKeyboardModifiers().

    Querying the state via this function (e.g. from a mouse handler that needs
    to include the modifier state in mouse events) is the preferred alternative
    over QGuiApplication::keyboardModifiers() since the latter may not report
    the current state due to asynchronous QPA event processing.
 */
Qt::KeyboardModifiers QInputDeviceManager::keyboardModifiers() const
{
    Q_D(const QInputDeviceManager);
    return d->keyboardModifiers;
}

void QInputDeviceManager::setKeyboardModifiers(Qt::KeyboardModifiers mods)
{
    Q_D(QInputDeviceManager);
    d->keyboardModifiers = mods;
}

QT_END_NAMESPACE

#include "moc_qinputdevicemanager_p.cpp"
