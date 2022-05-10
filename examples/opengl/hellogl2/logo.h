// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LOGO_H
#define LOGO_H

#include <qopengl.h>
#include <QList>
#include <QVector3D>

class Logo
{
public:
    Logo();
    const GLfloat *constData() const { return m_data.constData(); }
    int count() const { return m_count; }
    int vertexCount() const { return m_count / 6; }

private:
    void quad(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4);
    void extrude(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
    void add(const QVector3D &v, const QVector3D &n);

    QList<GLfloat> m_data;
    int m_count = 0;
};

#endif // LOGO_H
