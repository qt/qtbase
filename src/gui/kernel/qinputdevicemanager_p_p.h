/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QINPUTDEVICEMANAGER_P_P_H
#define QINPUTDEVICEMANAGER_P_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qobject_p.h>
#include "qinputdevicemanager_p.h"

#include <array>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QInputDeviceManagerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QInputDeviceManager)

public:
    static QInputDeviceManagerPrivate *get(QInputDeviceManager *mgr) { return mgr->d_func(); }

    int deviceCount(QInputDeviceManager::DeviceType type) const;
    void setDeviceCount(QInputDeviceManager::DeviceType type, int count);

    std::array<int, QInputDeviceManager::NumDeviceTypes> m_deviceCount = {};

    Qt::KeyboardModifiers keyboardModifiers;
};

QT_END_NAMESPACE

#endif // QINPUTDEVICEMANAGER_P_P_H
