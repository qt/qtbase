// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    static const int QSB_VERSION = 6;
    static const int QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS = 5;
    static const int QSB_VERSION_WITHOUT_VAR_ARRAYDIMS = 4;
    static const int QSB_VERSION_WITH_CBOR = 3;
    static const int QSB_VERSION_WITH_BINARY_JSON = 2;
    static const int QSB_VERSION_WITHOUT_BINDINGS = 1;

    QShaderPrivate()
        : ref(1)
    {
    }

    QShaderPrivate(const QShaderPrivate &other)
        : ref(1),
          qsbVersion(other.qsbVersion),
          stage(other.stage),
          desc(other.desc),
          shaders(other.shaders),
          bindings(other.bindings),
          combinedImageMap(other.combinedImageMap)
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
    QHash<QShaderKey, QShader::SeparateToCombinedImageSamplerMappingList> combinedImageMap;
};

QT_END_NAMESPACE

#endif
