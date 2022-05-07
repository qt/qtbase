/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
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

#ifndef QSHADER_P_P_H
#define QSHADER_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qshader_p.h"
#include <QtCore/QAtomicInt>
#include <QtCore/QHash>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QShaderPrivate
{
    static const int QSB_VERSION = 5;
    static const int QSB_VERSION_WITHOUT_VAR_ARRAYDIMS = 4;
    static const int QSB_VERSION_WITH_CBOR = 3;
    static const int QSB_VERSION_WITH_BINARY_JSON = 2;
    static const int QSB_VERSION_WITHOUT_BINDINGS = 1;

    QShaderPrivate()
        : ref(1)
    {
    }

    QShaderPrivate(const QShaderPrivate *other)
        : ref(1),
          qsbVersion(other->qsbVersion),
          stage(other->stage),
          desc(other->desc),
          shaders(other->shaders),
          bindings(other->bindings)
    {
    }

    static QShaderPrivate *get(QShader *s) { return s->d; }
    static const QShaderPrivate *get(const QShader *s) { return s->d; }

    QAtomicInt ref;
    int qsbVersion = QSB_VERSION;
    QShader::Stage stage = QShader::VertexStage;
    QShaderDescription desc;
    QHash<QShaderKey, QShaderCode> shaders;
    QHash<QShaderKey, QShader::NativeResourceBindingMap> bindings;
};

QT_END_NAMESPACE

#endif
