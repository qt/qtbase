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

#include "glwidget.h"
#include <QPainter>
#include <math.h>

#include "bubble.h"


const int bubbleNum = 8;

inline void CrossProduct(qreal &xOut, qreal &yOut, qreal &zOut, qreal x1, qreal y1, qreal z1, qreal x2, qreal y2, qreal z2)
{
   xOut = y1 * z2 - z1 * y2;
   yOut = z1 * x2 - x1 * z2;
   zOut = x1 * y2 - y1 * x2;
}

inline void Normalize(qreal &x, qreal &y, qreal &z)
{
    qreal l = sqrt(x*x + y*y + z*z);
    x = x / l;
    y = y / l;
    z = z / l;
}

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent)
{
    qtLogo = true;
    createdVertices = 0;
    createdNormals = 0;
    m_vertexNumber = 0;
    frames = 0;
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoBufferSwap(false);
    m_showBubbles = true;
}

GLWidget::~GLWidget()
{
  if (createdVertices)
      delete[] createdVertices;
  if (createdNormals)
      delete[] createdNormals;
}

void GLWidget::setScaling(int scale) {

    if (scale > 50)
        m_fScale = 1 + qreal(scale -50) / 50 * 0.5;
    else if (scale < 50)
        m_fScale =  1- (qreal(50 - scale) / 50 * 1/2);
    else 
      m_fScale = 1;
}

void GLWidget::setLogo() {
    qtLogo = true;
}

void GLWidget::setTexture() {
    qtLogo = false;
}

void GLWidget::showBubbles(bool bubbles)
{
   m_showBubbles = bubbles;
}

//! [2]
void GLWidget::paintQtLogo()
{
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_FLOAT,0, createdVertices);
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT,0,createdNormals);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexNumber / 3);
}
//! [2]

void GLWidget::paintTexturedCube()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_uiTexture);
    GLfloat afVertices[] = {
        -0.5, 0.5, 0.5, 0.5,-0.5,0.5,-0.5,-0.5,0.5,
        0.5, -0.5, 0.5, -0.5,0.5,0.5,0.5,0.5,0.5,
        -0.5, -0.5, -0.5, 0.5,-0.5,-0.5,-0.5,0.5,-0.5,
        0.5, 0.5, -0.5, -0.5,0.5,-0.5,0.5,-0.5,-0.5,

        0.5, -0.5, -0.5, 0.5,-0.5,0.5,0.5,0.5,-0.5,
        0.5, 0.5, 0.5, 0.5,0.5,-0.5,0.5,-0.5,0.5,
        -0.5, 0.5, -0.5, -0.5,-0.5,0.5,-0.5,-0.5,-0.5,
        -0.5, -0.5, 0.5, -0.5,0.5,-0.5,-0.5,0.5,0.5,

        0.5, 0.5,  -0.5, -0.5, 0.5,  0.5,  -0.5,  0.5,  -0.5,
        -0.5,  0.5,  0.5,  0.5,  0.5,  -0.5, 0.5, 0.5,  0.5,
        -0.5,  -0.5, -0.5, -0.5, -0.5, 0.5,  0.5, -0.5, -0.5,
        0.5, -0.5, 0.5,  0.5,  -0.5, -0.5, -0.5,  -0.5, 0.5
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_FLOAT,0,afVertices);

    GLfloat afTexCoord[] = {
        0.0f,0.0f, 1.0f,1.0f, 1.0f,0.0f,
        1.0f,1.0f, 0.0f,0.0f, 0.0f,1.0f,
        1.0f,1.0f, 1.0f,0.0f, 0.0f,1.0f,
        0.0f,0.0f, 0.0f,1.0f, 1.0f,0.0f,

        1.0f,1.0f, 1.0f,0.0f, 0.0f,1.0f,
        0.0f,0.0f, 0.0f,1.0f, 1.0f,0.0f,
        0.0f,0.0f, 1.0f,1.0f, 1.0f,0.0f,
        1.0f,1.0f, 0.0f,0.0f, 0.0f,1.0f,

        0.0f,1.0f, 1.0f,0.0f, 1.0f,1.0f,
        1.0f,0.0f, 0.0f,1.0f, 0.0f,0.0f,
        1.0f,0.0f, 1.0f,1.0f, 0.0f,0.0f,
        0.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f
    };
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,afTexCoord);

    GLfloat afNormals[] = {

        0,0,-1, 0,0,-1, 0,0,-1,
        0,0,-1, 0,0,-1, 0,0,-1,
        0,0,1, 0,0,1, 0,0,1,
        0,0,1, 0,0,1, 0,0,1,

        -1,0,0, -1,0,0, -1,0,0,
        -1,0,0, -1,0,0, -1,0,0,
        1,0,0, 1,0,0, 1,0,0,
        1,0,0, 1,0,0, 1,0,0,

        0,-1,0, 0,-1,0, 0,-1,0,
        0,-1,0, 0,-1,0, 0,-1,0,
        0,1,0, 0,1,0, 0,1,0,
        0,1,0, 0,1,0, 0,1,0
    };
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT,0,afNormals);

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void GLWidget::initializeGL ()
{
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &m_uiTexture);
    m_uiTexture = bindTexture(QImage(":/qt.png"));

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat aLightPosition[] = {0.0f,0.3f,1.0f,0.0f};

    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, aLightPosition);
    m_fAngle = 0;
    m_fScale = 1;
    createGeometry();
    createBubbles(bubbleNum - bubbles.count());
}

void GLWidget::paintGL()
{
    createBubbles(bubbleNum - bubbles.count());

//! [3]    
    QPainter painter;
    painter.begin(this);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
//! [3]    

//! [4]    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();

    //Since OpenGL ES does not support glPush/PopAttrib(GL_ALL_ATTRIB_BITS) 
    //we have to take care of the states ourselves

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glShadeModel(GL_FLAT);
    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(m_fAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(m_fAngle, 1.0f, 0.0f, 0.0f);
    glRotatef(m_fAngle, 0.0f, 0.0f, 1.0f);
    glScalef(m_fScale, m_fScale,m_fScale);
    glTranslatef(0.0f,-0.2f,0.0f);

    GLfloat matDiff[] = {0.40f, 1.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);

    if (qtLogo)
        paintQtLogo();
    else
        paintTexturedCube();
//! [4]    

//![5]
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
//![5]

//! [6]
    if (m_showBubbles)
        foreach (Bubble *bubble, bubbles) {
            bubble->drawBubble(&painter);
    }
//! [6]

//! [7]
    QString framesPerSecond;
    framesPerSecond.setNum(frames /(time.elapsed() / 1000.0), 'f', 2);

    painter.setPen(Qt::white);

    painter.drawText(20, 40, framesPerSecond + " fps");

    painter.end();
//! [7]
    
//! [8]
    swapBuffers();
//! [8]

    QMutableListIterator<Bubble*> iter(bubbles);

    while (iter.hasNext()) {
        Bubble *bubble = iter.next();
        bubble->move(rect());
    }
    if (!(frames % 100)) {
        time.start();
        frames = 0;
    }
    m_fAngle += 1.0f;
    frames ++;
}

void GLWidget::createBubbles(int number)
{
    for (int i = 0; i < number; ++i) {
        QPointF position(width()*(0.1 + (0.8*qrand()/(RAND_MAX+1.0))),
                        height()*(0.1 + (0.8*qrand()/(RAND_MAX+1.0))));
        qreal radius = qMin(width(), height())*(0.0175 + 0.0875*qrand()/(RAND_MAX+1.0));
        QPointF velocity(width()*0.0175*(-0.5 + qrand()/(RAND_MAX+1.0)),
                        height()*0.0175*(-0.5 + qrand()/(RAND_MAX+1.0)));

        bubbles.append(new Bubble(position, radius, velocity));
    }
}

void GLWidget::createGeometry()
{
    vertices.clear();
    normals.clear();

    qreal x1 = +0.06f;
    qreal y1 = -0.14f;
    qreal x2 = +0.14f;
    qreal y2 = -0.06f;
    qreal x3 = +0.08f;
    qreal y3 = +0.00f;
    qreal x4 = +0.30f;
    qreal y4 = +0.22f;

    quad(x1, y1, x2, y2, y2, x2, y1, x1);
    quad(x3, y3, x4, y4, y4, x4, y3, x3);

    extrude(x1, y1, x2, y2);
    extrude(x2, y2, y2, x2);
    extrude(y2, x2, y1, x1);
    extrude(y1, x1, x1, y1);
    extrude(x3, y3, x4, y4);
    extrude(x4, y4, y4, x4);
    extrude(y4, x4, y3, x3);

    const qreal Pi = 3.14159f;
    const int NumSectors = 100;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle1 = (i * 2 * Pi) / NumSectors;
        qreal x5 = 0.30 * sin(angle1);
        qreal y5 = 0.30 * cos(angle1);
        qreal x6 = 0.20 * sin(angle1);
        qreal y6 = 0.20 * cos(angle1);

        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;
        qreal x7 = 0.20 * sin(angle2);
        qreal y7 = 0.20 * cos(angle2);
        qreal x8 = 0.30 * sin(angle2);
        qreal y8 = 0.30 * cos(angle2);

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

//! [1]
    m_vertexNumber = vertices.size();
    createdVertices = new GLfloat[m_vertexNumber];
    createdNormals = new GLfloat[m_vertexNumber];
    for (int i = 0;i < m_vertexNumber;i++) {
      createdVertices[i] = vertices.at(i) * 2;
      createdNormals[i] = normals.at(i);
    }
    vertices.clear();
    normals.clear();
}
//! [1]

//! [0]
void GLWidget::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
{
    qreal nx, ny, nz;

    vertices << x1 << y1 << -0.05f;
    vertices << x2 << y2 << -0.05f;
    vertices << x4 << y4 << -0.05f;

    vertices << x3 << y3 << -0.05f;
    vertices << x4 << y4 << -0.05f;
    vertices << x2 << y2 << -0.05f;

    CrossProduct(nx, ny, nz, x2 - x1, y2 - y1, 0, x4 - x1, y4 - y1, 0);
    Normalize(nx, ny, nz);

    normals << nx << ny << nz;
    normals << nx << ny << nz;
    normals << nx << ny << nz;

    normals << nx << ny << nz;
    normals << nx << ny << nz;
    normals << nx << ny << nz;

    vertices << x4 << y4 << 0.05f;
    vertices << x2 << y2 << 0.05f;
    vertices << x1 << y1 << 0.05f;

    vertices << x2 << y2 << 0.05f;
    vertices << x4 << y4 << 0.05f;
    vertices << x3 << y3 << 0.05f;

    CrossProduct(nx, ny, nz, x2 - x4, y2 - y4, 0, x1 - x4, y1 - y4, 0);
    Normalize(nx, ny, nz);

    normals << nx << ny << nz;
    normals << nx << ny << nz;
    normals << nx << ny << nz;

    normals << nx << ny << nz;
    normals << nx << ny << nz;
    normals << nx << ny << nz;
}
//! [0]

void GLWidget::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
{
    qreal nx, ny, nz;

    vertices << x1 << y1 << +0.05f;
    vertices << x2 << y2 << +0.05f;
    vertices << x1 << y1 << -0.05f;

    vertices << x2 << y2 << -0.05f;
    vertices << x1 << y1 << -0.05f;
    vertices << x2 << y2 << +0.05f;

    CrossProduct(nx, ny, nz, x2 - x1, y2 - y1, 0.0f, 0.0f, 0.0f, -0.1f);
    Normalize(nx, ny, nz);

    normals << nx << ny << nz;
    normals << nx << ny << nz;
    normals << nx << ny << nz;

    normals << nx << ny << nz;
    normals << nx << ny << nz;
    normals << nx << ny << nz;
}
