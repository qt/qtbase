// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

#include "../joint.h"
#include "../super_parser.h"

class GeometryEngine : protected QOpenGLFunctions
{
public:
    GeometryEngine(Joint *root, std::vector<Joint*> jntVec);
    virtual ~GeometryEngine();

    void drawCubeGeometry(QOpenGLShaderProgram *program);
    void drawLineGeometry(QOpenGLShaderProgram *program);
    void drawSkinGeometry(QOpenGLShaderProgram *program);
    void updatePos(Joint *root);

    std::vector<Joint*> jntVec;
    int lenPts;
    int lenIndexes;

private:
    void getPos(Joint *jnt, std::vector<VertexData> *vec);
    void setWeights(std::vector<VertexData> *vec);
    void setIndexes(Joint *jnt, std::vector<GLushort> *vec);
    void setJointIndexes(Joint *jnt, int &vertexIndex);
    void initCubeGeometry();
    void initLineGeometry(Joint *root);
    void initSkinGeometry();

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
};

#endif // GEOMETRYENGINE_H
