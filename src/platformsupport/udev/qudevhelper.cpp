/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qudevhelper_p.h"
#include <libudev.h>

QT_BEGIN_NAMESPACE

void q_udev_devicePath(int type, QString *path)
{
    *path = QString();
    udev *u = udev_new();
    udev_enumerate *ue = udev_enumerate_new(u);
    udev_enumerate_add_match_subsystem(ue, "input");
    if (type & UDev_Mouse)
        udev_enumerate_add_match_property(ue, "ID_INPUT_MOUSE", "1");
    if (type & UDev_Touchpad)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHPAD", "1");
    if (type & UDev_Touchscreen)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHSCREEN", "1");
    udev_enumerate_scan_devices(ue);
    udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(ue)) {
        const char *syspath = udev_list_entry_get_name(entry);
        udev_device *udevice = udev_device_new_from_syspath(u, syspath);
        QString candidate = QString::fromLocal8Bit(udev_device_get_devnode(udevice));
        udev_device_unref(udevice);
        if (path->isEmpty() && candidate.startsWith(QLatin1String("/dev/input/event")))
            *path = candidate;
    }
    udev_enumerate_unref(ue);
    udev_unref(u);
}

QT_END_NAMESPACE
