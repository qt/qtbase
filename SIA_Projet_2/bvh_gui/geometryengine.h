// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <unordered_map>

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
    void updateSkinPos(Joint *root);

    std::vector<Joint*> jntVec;
    std::vector<std::unordered_map<std::string, float>> weightList;
    std::vector<VertexData> skinPos;
    std::vector<VertexData> skinPosCopy;
    int lenPts;
    int lenIndexes;

private:
    void getPos(Joint *jnt, std::vector<VertexData> *vec);
    void getSkinPos(Joint *jnt, std::vector<VertexData> &vec);
    void setIndexes(Joint *jnt, std::vector<GLushort> *vec);
    void setJointIndexes(Joint *jnt, int &vertexIndex);
    void initCubeGeometry();
    void initLineGeometry(Joint *root);
    void initSkinGeometry(Joint *root);
    void setWeights(std::vector<VertexData> vec);
    void resetSkinPos();

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
};

#endif // GEOMETRYENGINE_H
