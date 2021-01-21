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

#ifndef QPLATFORMPRINTPLUGIN_H
#define QPLATFORMPRINTPLUGIN_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE


class QPlatformPrinterSupport;

#define QPlatformPrinterSupportFactoryInterface_iid "org.qt-project.QPlatformPrinterSupportFactoryInterface.5.1"

class Q_PRINTSUPPORT_EXPORT QPlatformPrinterSupportPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformPrinterSupportPlugin(QObject *parent = nullptr);
    ~QPlatformPrinterSupportPlugin();

    virtual QPlatformPrinterSupport *create(const QString &key) = 0;

    static QPlatformPrinterSupport *get();
};

QT_END_NAMESPACE

#endif

#endif // QPLATFORMPRINTPLUGIN_H
