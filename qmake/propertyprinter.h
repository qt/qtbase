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
