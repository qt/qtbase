/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CUBE_H
#define CUBE_H

#include <QtOpenGL/qgl.h>
#include <QtCore/qvector.h>
#include <QtCore/qsequentialanimationgroup.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector2d.h>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE

class Geometry
{
public:
    void loadArrays() const;
    void addQuad(const QVector3D &a, const QVector3D &b,
                 const QVector3D &c, const QVector3D &d,
                 const QVector<QVector2D> &tex);
    void setColors(int start, GLfloat colors[4][4]);
    const GLushort *indices() const { return faces.constData(); }
    int count() const { return faces.count(); }
private:
    QVector<GLushort> faces;
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector2D> texCoords;
    QVector<QVector4D> colors;
    int append(const QVector3D &a, const QVector3D &n, const QVector2D &t);
    void addTri(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &n);
    friend class Tile;
};

class Tile
{
public:
    void draw() const;
    void setColors(GLfloat[4][4]);
protected:
    Tile(const QVector3D &loc = QVector3D());
    QVector3D location;
    QQuaternion orientation;
private:
    int start;
    int count;
    bool useFlatColor;
    GLfloat faceColor[4];
    Geometry *geom;
    friend class TileBuilder;
};

class TileBuilder
{
public:
    enum { bl, br, tr, tl };
    explicit TileBuilder(Geometry *, qreal depth = 0.0f, qreal size = 1.0f);
    Tile *newTile(const QVector3D &loc = QVector3D()) const;
    void setColor(QColor c) { color = c; }
protected:
    void initialize(Tile *) const;
    QVector<QVector3D> verts;
    QVector<QVector2D> tex;
    int start;
    int count;
    Geometry *geom;
    QColor color;
};

class Cube : public QObject, public Tile
{
    Q_OBJECT
    Q_PROPERTY(qreal range READ range WRITE setRange)
    Q_PROPERTY(qreal altitude READ altitude WRITE setAltitude)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
public:
    Cube(const QVector3D &loc = QVector3D());
    ~Cube();
    qreal range() { return location.x(); }
    void setRange(qreal r);
    qreal altitude() { return location.y(); }
    void setAltitude(qreal a);
    qreal rotation() { return rot; }
    void setRotation(qreal r);
    void removeBounce();
    void startAnimation();
    void setAnimationPaused(bool paused);
signals:
    void changed();
private:
    qreal rot;
    QPropertyAnimation *r;
    QPropertyAnimation *rtn;
    QSequentialAnimationGroup *animGroup;
    qreal startx;
    friend class CubeBuilder;
};

class CubeBuilder : public TileBuilder
{
public:
    explicit CubeBuilder(Geometry *, qreal depth = 0.0f, qreal size = 1.0f);
    Cube *newCube(const QVector3D &loc = QVector3D()) const;
private:
    mutable int ix;
};

#endif // CUBE_H
