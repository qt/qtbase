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

#ifndef QICONENGINEPLUGIN_H
#define QICONENGINEPLUGIN_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE


class QIconEngine;

#define QIconEngineFactoryInterface_iid "org.qt-project.Qt.QIconEngineFactoryInterface"

class Q_GUI_EXPORT QIconEnginePlugin : public QObject
{
    Q_OBJECT
public:
    QIconEnginePlugin(QObject *parent = nullptr);
    ~QIconEnginePlugin();

    virtual QIconEngine *create(const QString &filename = QString()) = 0;
};

QT_END_NAMESPACE

#endif // QICONENGINEPLUGIN_H
