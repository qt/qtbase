/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "glwidget.h"
#include <math.h>

#include "cube.h"

#include <QGLPixelBuffer>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

static GLfloat colorArray[][4] = {
    {0.243f, 0.423f, 0.125f, 1.0f},
    {0.176f, 0.31f, 0.09f, 1.0f},
    {0.4f, 0.69f, 0.212f, 1.0f},
    {0.317f, 0.553f, 0.161f, 1.0f}
};

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
    , geom(0)
    , cube(0)
{
    // create the pbuffer
    QGLFormat pbufferFormat = format();
    pbufferFormat.setSampleBuffers(false);
    pbuffer = new QGLPixelBuffer(QSize(512, 512), pbufferFormat, this);
    setWindowTitle(tr("OpenGL pbuffers"));
    initializeGeometry();
}

GLWidget::~GLWidget()
{
    pbuffer->releaseFromDynamicTexture();
    glDeleteTextures(1, &dynamicTexture);
    delete pbuffer;

    qDeleteAll(cubes);
    qDeleteAll(tiles);
    delete cube;
}

void GLWidget::initializeGL()
{
    initCommon();
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    initPbuffer();
    cube->startAnimation();
    connect(cube, SIGNAL(changed()), this, SLOT(update()));
    for (int i = 0; i < 3; ++i)
    {
        cubes[i]->startAnimation();
        connect(cubes[i], SIGNAL(changed()), this, SLOT(update()));
    }
}

void GLWidget::paintGL()
{
    pbuffer->makeCurrent();
    drawPbuffer();
    // On direct render platforms, drawing onto the pbuffer context above
    // automatically updates the dynamic texture.  For cases where rendering
    // directly to a texture is not supported, explicitly copy.
    if (!hasDynamicTextureUpdate)
        pbuffer->updateDynamicTexture(dynamicTexture);
    makeCurrent();

    // Use the pbuffer as a texture to render the scene
    glBindTexture(GL_TEXTURE_2D, dynamicTexture);

    // set up to render the scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -10.0f);

    // draw the background
    glPushMatrix();
    glScalef(aspect, 1.0f, 1.0f);
    for (int i = 0; i < tiles.count(); ++i)
        tiles[i]->draw();
    glPopMatrix();

    // draw the bouncing cubes
    for (int i = 0; i < cubes.count(); ++i)
        cubes[i]->draw();
}

void GLWidget::initializeGeometry()
{
    geom = new Geometry();
    CubeBuilder cBuilder(geom, 0.5);
    cBuilder.setColor(QColor(255, 255, 255, 212));
    // build the 3 bouncing, spinning cubes
    for (int i = 3; i > 0; --i)
        cubes.append(cBuilder.newCube(QVector3D((float)(i-1), -1.5f, 5 - i)));

    // build the spinning cube which goes in the dynamic texture
    cube = cBuilder.newCube();
    cube->removeBounce();

    // build the background tiles
    TileBuilder tBuilder(geom);
    tBuilder.setColor(QColor(Qt::white));
    for (int c = -2; c <= +2; ++c)
        for (int r = -2; r <= +2; ++r)
            tiles.append(tBuilder.newTile(QVector3D(c, r, 0)));

    // graded backdrop for the pbuffer scene
    TileBuilder bBuilder(geom, 0.0f, 2.0f);
    bBuilder.setColor(QColor(102, 176, 54, 210));
    backdrop = bBuilder.newTile(QVector3D(0.0f, 0.0f, -1.5f));
    backdrop->setColors(colorArray);
}

void GLWidget::initCommon()
{
    qglClearColor(QColor(Qt::darkGray));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glEnable(GL_TEXTURE_2D);

    geom->loadArrays();
}

void GLWidget::perspectiveProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifdef QT_OPENGL_ES
    glFrustumf(-aspect, +aspect, -1.0, +1.0, 4.0, 15.0);
#else
    glFrustum(-aspect, +aspect, -1.0, +1.0, 4.0, 15.0);
#endif
    glMatrixMode(GL_MODELVIEW);
}

void GLWidget::orthographicProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifdef QT_OPENGL_ES
    glOrthof(-1.0, +1.0, -1.0, +1.0, -90.0, +90.0);
#else
    glOrtho(-1.0, +1.0, -1.0, +1.0, -90.0, +90.0);
#endif
    glMatrixMode(GL_MODELVIEW);
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    aspect = (qreal)width / (qreal)(height ? height : 1);
    perspectiveProjection();
}

void GLWidget::drawPbuffer()
{
    orthographicProjection();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_TEXTURE_2D);
    backdrop->draw();
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    glDisable(GL_CULL_FACE);
    cube->draw();
    glEnable(GL_CULL_FACE);

    glFlush();
}

void GLWidget::initPbuffer()
{
    pbuffer->makeCurrent();

    cubeTexture = bindTexture(QImage(":res/cubelogo.png"));

    initCommon();

    // generate a texture that has the same size/format as the pbuffer
    dynamicTexture = pbuffer->generateDynamicTexture();

    // bind the dynamic texture to the pbuffer - this is a no-op under X11
    hasDynamicTextureUpdate = pbuffer->bindToDynamicTexture(dynamicTexture);
    makeCurrent();
}

void GLWidget::setAnimationPaused(bool enable)
{
    cube->setAnimationPaused(enable);
    for (int i = 0; i < 3; ++i)
        cubes[i]->setAnimationPaused(enable);
}
