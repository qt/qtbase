// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
