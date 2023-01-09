// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

#include "../joint.h"

class GeometryEngine : protected QOpenGLFunctions
{
public:
    GeometryEngine(Joint *root);
    virtual ~GeometryEngine();

    void drawCubeGeometry(QOpenGLShaderProgram *program);
    void drawLineGeometry(QOpenGLShaderProgram *program);
    void updatePos(Joint *root);

    int lenPts;

private:
    void initCubeGeometry();
    void initLineGeometry(Joint *root);

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
};

#endif // GEOMETRYENGINE_H
