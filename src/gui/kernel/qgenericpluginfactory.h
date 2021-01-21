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
