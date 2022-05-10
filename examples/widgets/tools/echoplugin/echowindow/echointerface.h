// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ECHOINTERFACE_H
#define ECHOINTERFACE_H

#include <QObject>
#include <QString>

//! [0]
class EchoInterface
{
public:
    virtual ~EchoInterface() = default;
    virtual QString echo(const QString &message) = 0;
};


QT_BEGIN_NAMESPACE

#define EchoInterface_iid "org.qt-project.Qt.Examples.EchoInterface"

Q_DECLARE_INTERFACE(EchoInterface, EchoInterface_iid)
QT_END_NAMESPACE

//! [0]
#endif
