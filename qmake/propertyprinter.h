// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PROPERTYPRINTER_H
#define PROPERTYPRINTER_H

#include <qglobal.h>
#include <qlist.h>
#include <qpair.h>
#include <qstring.h>

#include <functional>

QT_BEGIN_NAMESPACE

using PropertyPrinter = std::function<void(const QList<QPair<QString, QString>> &)>;
void qmakePropertyPrinter(const QList<QPair<QString, QString>> &values);
void jsonPropertyPrinter(const QList<QPair<QString, QString>> &values);

QT_END_NAMESPACE

#endif // PROPERTYPRINTER_H
