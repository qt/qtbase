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

#ifndef QINPUTDEVICEMANAGER_P_H
#define QINPUTDEVICEMANAGER_P_H

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
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QInputDeviceManagerPrivate;

class Q_GUI_EXPORT QInputDeviceManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputDeviceManager)

public:
    enum DeviceType {
        DeviceTypeUnknown,
        DeviceTypePointer,
        DeviceTypeKeyboard,
        DeviceTypeTouch,
        DeviceTypeTablet,

        NumDeviceTypes
    };

    QInputDeviceManager(QObject *parent = nullptr);

    int deviceCount(DeviceType type) const;

    void setCursorPos(const QPoint &pos);

    Qt::KeyboardModifiers keyboardModifiers() const;
    void setKeyboardModifiers(Qt::KeyboardModifiers mods);

signals:
    void deviceListChanged(QInputDeviceManager::DeviceType type);
    void cursorPositionChangeRequested(const QPoint &pos);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QInputDeviceManager::DeviceType)

#endif // QINPUTDEVICEMANAGER_P_H
