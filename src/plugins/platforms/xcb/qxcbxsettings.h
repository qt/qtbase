/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QXCBXSETTINGS_H
#define QXCBXSETTINGS_H

#include "qxcbscreen.h"

QT_BEGIN_NAMESPACE

class QXcbXSettingsPrivate;

class QXcbXSettings : public QXcbWindowEventListener
{
    Q_DECLARE_PRIVATE(QXcbXSettings)
public:
    QXcbXSettings(QXcbVirtualDesktop *screen);
    ~QXcbXSettings();
    bool initialized() const;

    QVariant setting(const QByteArray &property) const;

    typedef void (*PropertyChangeFunc)(QXcbVirtualDesktop *screen, const QByteArray &name, const QVariant &property, void *handle);
    void registerCallbackForProperty(const QByteArray &property, PropertyChangeFunc func, void *handle);
    void removeCallbackForHandle(const QByteArray &property, void *handle);
    void removeCallbackForHandle(void *handle);

    void handlePropertyNotifyEvent(const xcb_property_notify_event_t *event) override;
private:
    QXcbXSettingsPrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QXCBXSETTINGS_H
