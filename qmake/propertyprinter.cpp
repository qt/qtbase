// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "propertyprinter.h"

#include <iostream>

QT_BEGIN_NAMESPACE

void qmakePropertyPrinter(const QList<QPair<QString, QString>> &values)
{
    // Assume single property request
    if (values.size() == 1) {
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
