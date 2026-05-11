// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/qstyleplugin.h>
#include "qohosstyle_p.h"

QT_BEGIN_NAMESPACE

class QOhosStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "ohosstyle.json")

public:
    QStyle *create(const QString &key) override;
};

QStyle *QOhosStylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String("ohos"), Qt::CaseInsensitive) == 0)
        return new QOhosStyle();

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
