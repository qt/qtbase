/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevkeyboardmanager.h"

#include <QStringList>
#include <QCoreApplication>

//#define QT_QPA_KEYMAP_DEBUG

#ifdef QT_QPA_KEYMAP_DEBUG
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

QEvdevKeyboardManager::QEvdevKeyboardManager(const QString &key, const QString &specification)
{
    Q_UNUSED(key);

#ifndef QT_NO_LIBUDEV
    bool useUDev = true;
#else
    bool useUDev = false;
#endif // QT_NO_LIBUDEV
    QStringList args = specification.split(QLatin1Char(':'));
    QStringList devices;

    foreach (const QString &arg, args) {
        if (arg.startsWith("udev") && arg.contains("no")) {
            useUDev = false;
        } else if (arg.startsWith("/dev/")) {
            // if device is specified try to use it
            devices.append(arg);
            args.removeAll(arg);
        }
    }

    // build new specification without /dev/ elements
    m_spec = args.join(":");

    // add all keyboards for devices specified in the argument list
    foreach (const QString &device, devices)
        addKeyboard(device);

#ifndef QT_NO_LIBUDEV
    if (useUDev) {
#ifdef QT_QPA_KEYMAP_DEBUG
        qWarning() << "Use UDev for device discovery";
#endif

        m_udeviceHelper = QUDeviceHelper::createUDeviceHelper(QUDeviceHelper::UDev_Keyboard, this);
        if (m_udeviceHelper) {
            // scan and add already connected keyboards
            QStringList devices = m_udeviceHelper->scanConnectedDevices();
            foreach (QString device, devices) {
                addKeyboard(device);
            }

            connect(m_udeviceHelper, SIGNAL(deviceDetected(QString,QUDeviceTypes)), this, SLOT(addKeyboard(QString)));
            connect(m_udeviceHelper, SIGNAL(deviceRemoved(QString,QUDeviceTypes)), this, SLOT(removeKeyboard(QString)));
        }
    }
#endif // QT_NO_LIBUDEV
}

QEvdevKeyboardManager::~QEvdevKeyboardManager()
{
    qDeleteAll(m_keyboards);
    m_keyboards.clear();
}

void QEvdevKeyboardManager::addKeyboard(const QString &deviceNode)
{
#ifdef QT_QPA_KEYMAP_DEBUG
    qWarning() << "Adding keyboard at" << deviceNode;
#endif

    QString specification = m_spec;

    if (!deviceNode.isEmpty()) {
        specification.append(":");
        specification.append(deviceNode);
    }

    QEvdevKeyboardHandler *keyboard;
    keyboard = QEvdevKeyboardHandler::createLinuxInputKeyboardHandler("EvdevKeyboard", specification);
    if (keyboard)
        m_keyboards.insert(deviceNode, keyboard);
    else
        qWarning("Failed to open keyboard");
}

void QEvdevKeyboardManager::removeKeyboard(const QString &deviceNode)
{
    if (m_keyboards.contains(deviceNode)) {
#ifdef QT_QPA_KEYMAP_DEBUG
        qWarning() << "Removing keyboard at" << deviceNode;
#endif
        QEvdevKeyboardHandler *keyboard = m_keyboards.value(deviceNode);
        m_keyboards.remove(deviceNode);
        delete keyboard;
    }
}

QT_END_NAMESPACE
