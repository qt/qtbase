// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVXTOUCHMANAGER_P_H
#define QVXTOUCHMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtInputSupport/private/devicehandlerlist_p.h>

#include <QObject>
#include <QHash>
#include <QSocketNotifier>

QT_BEGIN_NAMESPACE

class QVxTouchScreenHandlerThread;

class QVxTouchManager : public QObject
{
public:
    QVxTouchManager(const QString &key, const QString &spec, QObject *parent = nullptr);
    ~QVxTouchManager();

    void addDevice(const QString &deviceNode);
    void removeDevice(const QString &deviceNode);

    void updateInputDeviceCount();

private:
    QString m_spec;
    QtInputSupport::DeviceHandlerList<QVxTouchScreenHandlerThread> m_activeDevices;
};

QT_END_NAMESPACE

#endif // QVXTOUCHMANAGER_P_H
