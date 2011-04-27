/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtGui>
#include <QtOpenGL>

#include <math.h>

#include "glwidget.h"

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent)
{
    gear1 = 0;
    gear2 = 0;
    gear3 = 0;
    xRot = 0;
    yRot = 0;
    zRot = 0;
    gear1Rot = 0;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(advanceGears()));
    timer->start(20);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    glDeleteLists(gear1, 1);
    glDeleteLists(gear2, 1);
    glDeleteLists(gear3, 1);
}

void GLWidget::setXRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != xRot) {
        xRot = angle;
        emit xRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setYRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != yRot) {
        yRot = angle;
        emit yRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setZRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != zRot) {
        zRot = angle;
        emit zRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::initializeGL()
{
    static const GLfloat lightPos[4] = { 5.0f, 5.0f, 10.0f, 1.0f };
    static const GLfloat reflectance1[4] = { 0.8f, 0.1f, 0.0f, 1.0f };
    static const GLfloat reflectance2[4] = { 0.0f, 0.8f, 0.2f, 1.0f };
    static const GLfloat reflectance3[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    gear1 = makeGear(reflectance1, 1.0, 4.0, 1.0, 0.7, 20);
    gear2 = makeGear(reflectance2, 0.5, 2.0, 2.0, 0.7, 10);
    gear3 = makeGear(reflectance3, 1.3, 2.0, 0.5, 0.7, 10);

    glEnable(GL_NORMALIZE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotated(xRot / 16.0, 1.0, 0.0, 0.0);
    glRotated(yRot / 16.0, 0.0, 1.0, 0.0);
    glRotated(zRot / 16.0, 0.0, 0.0, 1.0);

    drawGear(gear1, -3.0, -2.0, 0.0, gear1Rot / 16.0);
    drawGear(gear2, +3.1, -2.0, 0.0, -2.0 * (gear1Rot / 16.0) - 9.0);

    glRotated(+90.0, 1.0, 0.0, 0.0);
    drawGear(gear3, -3.1, -1.8, -2.2, +2.0 * (gear1Rot / 16.0) - 2.0);

    glPopMatrix();
}

void GLWidget::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, +1.0, -1.0, 1.0, 5.0, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(0.0, 0.0, -40.0);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        setXRotation(xRot + 8 * dy);
        setYRotation(yRot + 8 * dx);
    } else if (event->buttons() & Qt::RightButton) {
        setXRotation(xRot + 8 * dy);
        setZRotation(zRot + 8 * dx);
    }
    lastPos = event->pos();
}

void GLWidget::advanceGears()
{
    gear1Rot += 2 * 16;
    updateGL();
}

GLuint GLWidget::makeGear(const GLfloat *reflectance, GLdouble innerRadius,
                          GLdouble outerRadius, GLdouble thickness,
                          GLdouble toothSize, GLint toothCount)
{
    const double Pi = 3.14159265358979323846;

    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, reflectance);

    GLdouble r0 = innerRadius;
    GLdouble r1 = outerRadius - toothSize / 2.0;
    GLdouble r2 = outerRadius + toothSize / 2.0;
    GLdouble delta = (2.0 * Pi / toothCount) / 4.0;
    GLdouble z = thickness / 2.0;
    int i, j;

    glShadeModel(GL_FLAT);

    for (i = 0; i < 2; ++i) {
        GLdouble sign = (i == 0) ? +1.0 : -1.0;

        glNormal3d(0.0, 0.0, sign);

        glBegin(GL_QUAD_STRIP);
        for (j = 0; j <= toothCount; ++j) {
            GLdouble angle = 2.0 * Pi * j / toothCount;
	    glVertex3d(r0 * cos(angle), r0 * sin(angle), sign * z);
	    glVertex3d(r1 * cos(angle), r1 * sin(angle), sign * z);
	    glVertex3d(r0 * cos(angle), r0 * sin(angle), sign * z);
	    glVertex3d(r1 * cos(angle + 3 * delta), r1 * sin(angle + 3 * delta),
                       sign * z);
        }
        glEnd();

        glBegin(GL_QUADS);
        for (j = 0; j < toothCount; ++j) {
            GLdouble angle = 2.0 * Pi * j / toothCount;
	    glVertex3d(r1 * cos(angle), r1 * sin(angle), sign * z);
	    glVertex3d(r2 * cos(angle + delta), r2 * sin(angle + delta),
                       sign * z);
	    glVertex3d(r2 * cos(angle + 2 * delta), r2 * sin(angle + 2 * delta),
                       sign * z);
	    glVertex3d(r1 * cos(angle + 3 * delta), r1 * sin(angle + 3 * delta),
                       sign * z);
        }
        glEnd();
    }

    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < toothCount; ++i) {
        for (j = 0; j < 2; ++j) {
            GLdouble angle = 2.0 * Pi * (i + (j / 2.0)) / toothCount;
            GLdouble s1 = r1;
            GLdouble s2 = r2;
            if (j == 1)
                qSwap(s1, s2);

	    glNormal3d(cos(angle), sin(angle), 0.0);
	    glVertex3d(s1 * cos(angle), s1 * sin(angle), +z);
	    glVertex3d(s1 * cos(angle), s1 * sin(angle), -z);

	    glNormal3d(s2 * sin(angle + delta) - s1 * sin(angle),
                       s1 * cos(angle) - s2 * cos(angle + delta), 0.0);
	    glVertex3d(s2 * cos(angle + delta), s2 * sin(angle + delta), +z);
	    glVertex3d(s2 * cos(angle + delta), s2 * sin(angle + delta), -z);
        }
    }
    glVertex3d(r1, 0.0, +z);
    glVertex3d(r1, 0.0, -z);
    glEnd();

    glShadeModel(GL_SMOOTH);

    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= toothCount; ++i) {
	GLdouble angle = i * 2.0 * Pi / toothCount;
	glNormal3d(-cos(angle), -sin(angle), 0.0);
	glVertex3d(r0 * cos(angle), r0 * sin(angle), +z);
	glVertex3d(r0 * cos(angle), r0 * sin(angle), -z);
    }
    glEnd();

    glEndList();

    return list;
}

void GLWidget::drawGear(GLuint gear, GLdouble dx, GLdouble dy, GLdouble dz,
                        GLdouble angle)
{
    glPushMatrix();
    glTranslated(dx, dy, dz);
    glRotated(angle, 0.0, 0.0, 1.0);
    glCallList(gear);
    glPopMatrix();
}

void GLWidget::normalizeAngle(int *angle)
{
    while (*angle < 0)
        *angle += 360 * 16;
    while (*angle > 360 * 16)
        *angle -= 360 * 16;
}
