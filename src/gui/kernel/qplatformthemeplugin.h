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

#ifndef QPLATFORMTHEMEPLUGIN_H
#define QPLATFORMTHEMEPLUGIN_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE

class QPlatformTheme;

#define QPlatformThemeFactoryInterface_iid "org.qt-project.Qt.QPA.QPlatformThemeFactoryInterface.5.1"

class Q_GUI_EXPORT QPlatformThemePlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformThemePlugin(QObject *parent = nullptr);
    ~QPlatformThemePlugin();

    virtual QPlatformTheme *create(const QString &key, const QStringList &paramList) = 0;
};

QT_END_NAMESPACE

#endif // QPLATFORMTHEMEPLUGIN_H
