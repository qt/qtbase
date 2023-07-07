// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "propertyprinter.h"

#include <iostream>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

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
    QJsonObject object;
    for (const auto &val : values)
        object.insert(val.first, val.second);

    QJsonDocument document(object);
    std::cout << document.toJson().constData();
}

QT_END_NAMESPACE
