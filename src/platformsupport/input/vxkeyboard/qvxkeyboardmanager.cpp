// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvxkeyboardmanager_p.h"

#include <QtInputSupport/private/qevdevutil_p.h>

#include <QStringList>
#include <QCoreApplication>
#include <QLoggingCategory>

#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QVxKeyboardManager::QVxKeyboardManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(key);


    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_VXEVDEV_KEYBOARD_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    auto parsed = QEvdevUtil::parseSpecification(spec);
    m_spec = std::move(parsed.spec);

    // add all keyboards for devices specified in the argument list
    for (const QString &device : std::as_const(parsed.devices))
        addKeyboard(device);

    if (parsed.devices.isEmpty()) {
        qCDebug(qLcVxKey, "vxkeyboard: Using device discovery");
        if (auto deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Keyboard, this)) {
            // scan and add already connected keyboards
            const QStringList devices = deviceDiscovery->scanConnectedDevices();
            for (const QString &device : devices)
                addKeyboard(device);

            connect(deviceDiscovery, &QDeviceDiscovery::deviceDetected,
                    this, &QVxKeyboardManager::addKeyboard);
            connect(deviceDiscovery, &QDeviceDiscovery::deviceRemoved,
                    this, &QVxKeyboardManager::removeKeyboard);
        }
    }
}

QVxKeyboardManager::~QVxKeyboardManager()
{
}

void QVxKeyboardManager::addKeyboard(const QString &deviceNode)
{
    qCDebug(qLcVxKey, "Adding keyboard at %ls", qUtf16Printable(deviceNode));
    auto keyboard = QVxKeyboardHandler::create(deviceNode, m_spec, m_defaultKeymapFile);
    if (keyboard) {
        m_keyboards.add(deviceNode, std::move(keyboard));
        updateDeviceCount();
    } else {
        qWarning("Failed to open keyboard device %ls", qUtf16Printable(deviceNode));
    }
}

void QVxKeyboardManager::removeKeyboard(const QString &deviceNode)
{
    if (m_keyboards.remove(deviceNode)) {
        qCDebug(qLcVxKey, "Removing keyboard at %ls", qUtf16Printable(deviceNode));
        updateDeviceCount();
    }
}

void QVxKeyboardManager::updateDeviceCount()
{
    QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
        QInputDeviceManager::DeviceTypeKeyboard, m_keyboards.count());
}

void QVxKeyboardManager::loadKeymap(const QString &file)
{
    m_defaultKeymapFile = file;

    if (file.isEmpty()) {
        // Restore the default, which is either the built-in keymap or
        // the one given in the plugin spec.
        QString keymapFromSpec;
        const auto specs = QStringView{m_spec}.split(u':');
        for (const auto &arg : specs) {
            if (arg.startsWith("keymap="_L1))
                keymapFromSpec = arg.mid(7).toString();
        }
        for (const auto &keyboard : m_keyboards) {
            if (keymapFromSpec.isEmpty())
                keyboard.handler->unloadKeymap();
            else
                keyboard.handler->loadKeymap(keymapFromSpec);
        }
    } else {
        for (const auto &keyboard : m_keyboards)
            keyboard.handler->loadKeymap(file);
    }
}

void QVxKeyboardManager::switchLang()
{
    for (const auto &keyboard : m_keyboards)
        keyboard.handler->switchLang();
}

QT_END_NAMESPACE
