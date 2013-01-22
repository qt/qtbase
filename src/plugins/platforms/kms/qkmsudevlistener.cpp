/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qkmsudevlistener.h>

QT_BEGIN_NAMESPACE

QKmsUdevListener::QKmsUdevListener(QObject *parent)
    : QObject(parent)
{
    m_udev = udev_new();
}

QKmsUdevListener::~QKmsUdevListener()
{
    udev_unref(m_udev);
}

void QKmsUdevListener::addHandler(QKmsUdevHandler *handler)
{
    m_handlers.removeAll((QKmsUdevHandler *) 0);
    m_handlers.removeAll(handler);
    m_handlers.prepend(handler);

    scan();
}

bool QKmsUdevListener::create(struct udev_device *device)
{
    foreach (QKmsUdevHandler *handler, m_handlers) {
        if (!handler)
            continue;

        QObject *obj = handler->create(device);
        if (obj) {
            m_devices[udev_device_get_syspath(device)] = obj;
            return true;
        }
    }

    return false;
}

void QKmsUdevListener::scan()
{
    struct udev_enumerate *e;
    struct udev_list_entry *entry;

    e = udev_enumerate_new(m_udev);
    udev_enumerate_scan_devices(e);
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
        const char *path = udev_list_entry_get_name(entry);
        if (m_devices.contains(path))
            continue;

        struct udev_device *device = udev_device_new_from_syspath(m_udev, path);
        create(device);
        udev_device_unref(device);
    }
    udev_enumerate_unref(e);
}

QT_END_NAMESPACE
