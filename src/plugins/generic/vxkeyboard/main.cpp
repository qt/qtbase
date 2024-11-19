// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qvxkeyboardmanager_p.h>

QT_BEGIN_NAMESPACE

class QVxKeyboardPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "vxkeyboard.json")

public:
    QVxKeyboardPlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QVxKeyboardPlugin::QVxKeyboardPlugin()
    : QGenericPlugin()
{
}

QObject* QVxKeyboardPlugin::create(const QString &key,
                                                 const QString &specification)
{
    if (!key.compare(QLatin1String("VxKeyboard"), Qt::CaseInsensitive))
        return new QVxKeyboardManager(key, specification);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
