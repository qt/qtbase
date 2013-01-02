/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include "glwidget.h"
#include <QtGui/QImage>

#include <math.h>

static GLint cubeArray[][3] = {
    {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1},
    {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1},
    {0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0},
    {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1},
    {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}
};

static GLint cubeTextureArray[][2] = {
    {0, 0}, {1, 0}, {1, 1}, {0, 1},
    {0, 0}, {0, 1}, {1, 1}, {1, 0},
    {0, 0}, {1, 0}, {1, 1}, {0, 1},
    {1, 0}, {0, 0}, {0, 1}, {1, 1},
    {0, 0}, {1, 0}, {1, 1}, {0, 1},
    {1, 0}, {0, 0}, {0, 1}, {1, 1}
};

static GLint faceArray[][2] = {
    {1, -1}, {1, 1}, {-1, 1}, {-1, -1}
};

static GLubyte colorArray[][4] = {
    {102, 176, 54, 255},
    {81, 141, 41, 255},
    {62, 108, 32, 255},
    {45, 79, 23, 255}
};

GLWidget::GLWidget(QWidget *parent)
  : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    // create the framebuffer object - make sure to have a current
    // context before creating it
    makeCurrent();
    fbo = new QGLFramebufferObject(512, 512);
    timerId = startTimer(20);
    setWindowTitle(tr("OpenGL framebuffer objects 2"));
}

GLWidget::~GLWidget()
{
    glDeleteLists(pbufferList, 1);
    delete fbo;
}

void GLWidget::initializeGL()
{
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_CULL_FACE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_INT, 0, cubeArray);
    glTexCoordPointer(2, GL_INT, 0, cubeTextureArray);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colorArray);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    pbufferList = glGenLists(1);
    glNewList(pbufferList, GL_COMPILE);
    {
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	// draw cube background
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.5f, 0.5f, -2.0f);
	glDisable(GL_TEXTURE_2D);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_INT, 0, faceArray);
	glDrawArrays(GL_QUADS, 0, 4);
	glVertexPointer(3, GL_INT, 0, cubeArray);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
        glPopMatrix();

	// draw cube
        glTranslatef(0.5f, 0.5f, 0.5f);
        glRotatef(3.0f, 1.0f, 1.0f, 1.0f);
        glTranslatef(-0.5f, -0.5f, -0.5f);
        glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
        glDrawArrays(GL_QUADS, 0, 24);

        glPushMatrix(); // this state is popped back in the paintGL() function
    }
    glEndList();

    for (int i = 0; i < 3; ++i) {
        yOffs[i] = 0.0f;
        xInc[i] = 0.005f;
        rot[i] = 0.0f;
    }
    xOffs[0]= 0.0f;
    xOffs[1]= 0.5f;
    xOffs[2]= 1.0f;

    cubeTexture = bindTexture(QImage(":res/cubelogo.png"));

    glPushMatrix(); // push to avoid stack underflow in the first paintGL() call
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = w/(float)(h ? h : 1);
    glFrustum(-aspect, aspect, -1, 1, 10, 100);
    glTranslatef(-0.5f, -0.5f, -0.5f);
    glTranslatef(0.0f, 0.0f, -15.0f);
}

void GLWidget::paintGL()
{
    glPopMatrix(); // pop the matrix pushed in the pbuffer list

    // push the projection matrix and the entire GL state before
    // doing any rendering into our framebuffer object
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glViewport(0, 0, fbo->size().width(), fbo->size().height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -99, 99);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    glMatrixMode(GL_MODELVIEW);

    // render to the framebuffer object
    fbo->bind();
    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    glCallList(pbufferList);
    fbo->release();

    // pop the projection matrix and GL state back for rendering
    // to the actual widget
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the background
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glVertexPointer(2, GL_INT, 0, faceArray);
    glTranslatef(-1.2f, -0.8f, 0.0f);
    glScalef(0.2f, 0.2f, 0.2f);
    for (int y = 0; y < 5; ++y) {
	for (int x = 0; x < 5; ++x) {
	    glTranslatef(2.0f, 0, 0);
	    glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
	    glDrawArrays(GL_QUADS, 0, 4);
	}
 	glTranslatef(-10.0f, 2.0f, 0);
    }
    glVertexPointer(3, GL_INT, 0, cubeArray);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    // draw the bouncing cubes
    drawCube(0, 0.0f, 1.5f, 2.5f, 1.5f);
    drawCube(1, 1.0f, 2.0f, 2.5f, 2.0f);
    drawCube(2, 2.0f, 3.5f, 2.5f, 2.5f);
    glPopMatrix();
}

void GLWidget::drawCube(int i, GLfloat z, GLfloat rotation, GLfloat jmp, GLfloat amp)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(xOffs[i], yOffs[i], z);
    glTranslatef(0.5f, 0.5f, 0.5f);
    GLfloat scale = 0.75 + i*(0.25f/2);
    glScalef(scale, scale, scale);
    glRotatef(rot[i], 1.0f, 1.0f, 1.0f);
    glTranslatef(-0.5f, -0.5f, -0.5f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glDrawArrays(GL_QUADS, 0, 24);

    if (xOffs[i] > 1.0f || xOffs[i] < -1.0f) {
        xInc[i] = -xInc[i];
        xOffs[i] = xOffs[i] > 1.0f ? 1.0f : -1.0f;
    }
    xOffs[i] += xInc[i];
    yOffs[i] = qAbs(cos((-3.141592f * jmp) * xOffs[i]) * amp) - 1;
    rot[i] += rotation;
}
