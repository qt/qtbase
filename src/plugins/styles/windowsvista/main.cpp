// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qstyleplugin.h>
#include "qwindowsvistastyle_p.h"

QT_BEGIN_NAMESPACE

class QWindowsVistaStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "windowsvistastyle.json")
public:
    QStyle *create(const QString &key) override;
};

QStyle *QWindowsVistaStylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String("windowsvista"), Qt::CaseInsensitive) == 0)
        return new QWindowsVistaStyle();

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
