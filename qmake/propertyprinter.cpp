/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
******************************************************************************/

#include "propertyprinter.h"

#include <iostream>

QT_BEGIN_NAMESPACE

void qmakePropertyPrinter(const QList<QPair<QString, QString>> &values)
{
    // Assume single property request
    if (values.count() == 1) {
        std::cout << qPrintable(values.at(0).second) << std::endl;
        return;
    }

    for (const auto &val : values) {
        std::cout << qPrintable(val.first) << ":" << qPrintable(val.second) << std::endl;
    }
}

void jsonPropertyPrinter(const QList<QPair<QString, QString>> &values)
{
    std::cout << "{\n";
    for (const auto &val : values) {
        std::cout << "\"" << qPrintable(val.first) << "\":\"" << qPrintable(val.second) << "\",\n";
    }
    std::cout << "}\n";
}

QT_END_NAMESPACE
