// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QFACTORYINTERFACE_H
#define QFACTORYINTERFACE_H

#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

struct Q_CORE_EXPORT QFactoryInterface
{
    virtual ~QFactoryInterface();
    virtual QStringList keys() const = 0;
};

#ifndef Q_QDOC
Q_DECLARE_INTERFACE(QFactoryInterface, "org.qt-project.Qt.QFactoryInterface")
#endif

QT_END_NAMESPACE

#endif // QFACTORYINTERFACE_H
