/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QSTYLEPLUGIN_H
#define QSTYLEPLUGIN_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE


class QStyle;

#define QStyleFactoryInterface_iid "org.qt-project.Qt.QStyleFactoryInterface"

class Q_WIDGETS_EXPORT QStylePlugin : public QObject
{
    Q_OBJECT
public:
    explicit QStylePlugin(QObject *parent = nullptr);
    ~QStylePlugin();

    virtual QStyle *create(const QString &key) = 0;
};

QT_END_NAMESPACE

#endif // QSTYLEPLUGIN_H
