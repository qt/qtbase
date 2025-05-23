// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QGENERICPLUGINFACTORY_H
#define QGENERICPLUGINFACTORY_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE


class QString;
class QObject;

class Q_GUI_EXPORT QGenericPluginFactory
{
public:
    static QStringList keys();
    static QObject *create(const QString&, const QString &);
};

QT_END_NAMESPACE

#endif // QGENERICPLUGINFACTORY_H
