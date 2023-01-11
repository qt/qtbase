// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

#include "../joint.h"

struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
};


class GeometryEngine : protected QOpenGLFunctions
{
public:
    GeometryEngine(Joint *root);
    virtual ~GeometryEngine();

    void drawCubeGeometry(QOpenGLShaderProgram *program);
    void drawLineGeometry(QOpenGLShaderProgram *program);
    void updatePos(Joint *root);

    int lenPts;
    int lenIndexes;

private:
    void getPos(Joint *jnt, std::vector<VertexData> *vec);
    void setIndexes(Joint *jnt, std::vector<GLushort> *vec);
    void setJointIndexes(Joint *jnt, int &vertexIndex);
    void initCubeGeometry();
    void initLineGeometry(Joint *root);

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
};

#endif // GEOMETRYENGINE_H
