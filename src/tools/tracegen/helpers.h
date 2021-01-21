/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef HELPERS_H
#define HELPERS_H

#include "provider.h"

#include <qvector.h>
#include <qstring.h>

enum ParamType {
    LTTNG,
    ETW
};

QString includeGuard(const QString &filename);
QString formatFunctionSignature(const QVector<Tracepoint::Argument> &args);
QString formatParameterList(const QVector<Tracepoint::Argument> &args, ParamType type);

#endif // HELPERS_H
