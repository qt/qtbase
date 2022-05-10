// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHADERDESCRIPTION_P_H
#define QSHADERDESCRIPTION_P_H

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

#include "qshaderdescription_p.h"
#include <QtCore/QList>
#include <QtCore/QAtomicInt>
#include <QtCore/QJsonDocument>

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QShaderDescriptionPrivate
{
    QShaderDescriptionPrivate()
        : ref(1)
    {
        localSize[0] = localSize[1] = localSize[2] = 0;
    }

    QShaderDescriptionPrivate(const QShaderDescriptionPrivate &other)
        : ref(1),
          inVars(other.inVars),
          outVars(other.outVars),
          uniformBlocks(other.uniformBlocks),
          pushConstantBlocks(other.pushConstantBlocks),
          storageBlocks(other.storageBlocks),
          combinedImageSamplers(other.combinedImageSamplers),
          separateImages(other.separateImages),
          separateSamplers(other.separateSamplers),
          storageImages(other.storageImages),
          localSize(other.localSize)
    {
    }

    static QShaderDescriptionPrivate *get(QShaderDescription *desc) { return desc->d; }
    static const QShaderDescriptionPrivate *get(const QShaderDescription *desc) { return desc->d; }

    QJsonDocument makeDoc();
    void writeToStream(QDataStream *stream);
    void loadFromStream(QDataStream *stream, int version);

    QAtomicInt ref;
    QList<QShaderDescription::InOutVariable> inVars;
    QList<QShaderDescription::InOutVariable> outVars;
    QList<QShaderDescription::UniformBlock> uniformBlocks;
    QList<QShaderDescription::PushConstantBlock> pushConstantBlocks;
    QList<QShaderDescription::StorageBlock> storageBlocks;
    QList<QShaderDescription::InOutVariable> combinedImageSamplers;
    QList<QShaderDescription::InOutVariable> separateImages;
    QList<QShaderDescription::InOutVariable> separateSamplers;
    QList<QShaderDescription::InOutVariable> storageImages;
    std::array<uint, 3> localSize;
};

QT_END_NAMESPACE

#endif
