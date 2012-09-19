/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "stickman.h"
#include "node.h"

#include <QPainter>
#include <QTimer>

#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const qreal Coords[NodeCount * 2] = {
    0.0, -150.0, // head, #0

    0.0, -100.0, // body pentagon, top->bottom, left->right, #1 - 5
    -50.0, -50.0,
    50.0, -50.0,
    -25.0, 50.0,
    25.0, 50.0,

    -100.0, 0.0, // right arm, #6 - 7
    -125.0, 50.0,

    100.0, 0.0, // left arm, #8 - 9
    125.0, 50.0,

    -35.0, 75.0, // lower body, #10 - 11
    35.0, 75.0,

    -25.0, 200.0, // right leg, #12 - 13
    -30.0, 300.0,

    25.0, 200.0, // left leg, #14 - 15
    30.0, 300.0

};

static const int Bones[BoneCount * 2] = {
    0, 1, // neck

    1, 2, // body
    1, 3,
    1, 4,
    1, 5,
    2, 3,
    2, 4,
    2, 5,
    3, 4,
    3, 5,
    4, 5,

    2, 6, // right arm
    6, 7,

    3, 8, // left arm
    8, 9,

    4, 10, // lower body
    4, 11,
    5, 10,
    5, 11,
    10, 11,

    10, 12, // right leg
    12, 13,

    11, 14, // left leg
    14, 15

};

StickMan::StickMan()
{
    m_sticks = true;
    m_isDead = false;
    m_pixmap = QPixmap("images/head.png");
    m_penColor = Qt::white;
    m_fillColor = Qt::black;

    // Set up start position of limbs
    for (int i=0; i<NodeCount; ++i) {
        m_nodes[i] = new Node(QPointF(Coords[i * 2], Coords[i * 2 + 1]), this);
        connect(m_nodes[i], SIGNAL(positionChanged()), this, SLOT(childPositionChanged()));
    }

    for (int i=0; i<BoneCount; ++i) {
        int n1 = Bones[i * 2];
        int n2 = Bones[i * 2 + 1];

        Node *node1 = m_nodes[n1];
        Node *node2 = m_nodes[n2];

        QPointF dist = node1->pos() - node2->pos();
        m_perfectBoneLengths[i] = sqrt(pow(dist.x(),2) + pow(dist.y(),2));
    }

    startTimer(10);
}

StickMan::~StickMan()
{
}

void StickMan::childPositionChanged()
{
    prepareGeometryChange();
}

void StickMan::setDrawSticks(bool on)
{
    m_sticks = on;
    for (int i=0;i<nodeCount();++i) {
        Node *node = m_nodes[i];
        node->setVisible(on);
    }
}

QRectF StickMan::boundingRect() const
{
    // account for head radius=50.0 plus pen which is 5.0
    return childrenBoundingRect().adjusted(-55.0, -55.0, 55.0, 55.0);
}

int StickMan::nodeCount() const
{
    return NodeCount;
}

Node *StickMan::node(int idx) const
{
    if (idx >= 0 && idx < NodeCount)
        return m_nodes[idx];
    else
        return 0;
}

void StickMan::timerEvent(QTimerEvent *)
{
    update();
}

void StickMan::stabilize()
{
    static const qreal threshold = 0.001;

    for (int i=0; i<BoneCount; ++i) {
        int n1 = Bones[i * 2];
        int n2 = Bones[i * 2 + 1];

        Node *node1 = m_nodes[n1];
        Node *node2 = m_nodes[n2];

        QPointF pos1 = node1->pos();
        QPointF pos2 = node2->pos();

        QPointF dist = pos1 - pos2;
        qreal length = sqrt(pow(dist.x(),2) + pow(dist.y(),2));
        qreal diff = (length - m_perfectBoneLengths[i]) / length;

        QPointF p = dist * (0.5 * diff);
        if (p.x() > threshold && p.y() > threshold) {
            pos1 -= p;
            pos2 += p;

            node1->setPos(pos1);
            node2->setPos(pos2);
        }
    }
}

QPointF StickMan::posFor(int idx) const
{
    return m_nodes[idx]->pos();
}

//#include <QTime>
void StickMan::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
  /*  static int frames = 0;
    static QTime time;
    if (frames++ % 100 == 0) {
        frames = 1;
        time.restart();
    }

    if (time.elapsed() > 0) {
        painter->setPen(Qt::white);
        painter->drawText(0, 0, QString::number(frames / (time.elapsed() / 1000.0)));
    }*/

    stabilize();
    if (m_sticks) {
        painter->setPen(Qt::white);
        for (int i=0; i<BoneCount; ++i) {
            int n1 = Bones[i * 2];
            int n2 = Bones[i * 2 + 1];

            Node *node1 = m_nodes[n1];
            Node *node2 = m_nodes[n2];

            painter->drawLine(node1->pos(), node2->pos());
        }
    } else {
        // first bone is neck and will be used for head

        QPainterPath path;
        path.moveTo(posFor(0));
        path.lineTo(posFor(1));

        // right arm
        path.lineTo(posFor(2));
        path.lineTo(posFor(6));
        path.lineTo(posFor(7));

        // left arm
        path.moveTo(posFor(3));
        path.lineTo(posFor(8));
        path.lineTo(posFor(9));

        // body
        path.moveTo(posFor(2));
        path.lineTo(posFor(4));
        path.lineTo(posFor(10));
        path.lineTo(posFor(11));
        path.lineTo(posFor(5));
        path.lineTo(posFor(3));
        path.lineTo(posFor(1));

        // right leg
        path.moveTo(posFor(10));
        path.lineTo(posFor(12));
        path.lineTo(posFor(13));

        // left leg
        path.moveTo(posFor(11));
        path.lineTo(posFor(14));
        path.lineTo(posFor(15));

        painter->setPen(QPen(m_penColor, 5.0, Qt::SolidLine, Qt::RoundCap));
        painter->drawPath(path);

        {
            int n1 = Bones[0];
            int n2 = Bones[1];
            Node *node1 = m_nodes[n1];
            Node *node2 = m_nodes[n2];

            QPointF dist = node2->pos() - node1->pos();

            qreal sinAngle = dist.x() / sqrt(pow(dist.x(), 2) + pow(dist.y(), 2));
            qreal angle = asin(sinAngle) * 180.0 / M_PI;

            QPointF headPos = node1->pos();
            painter->translate(headPos);
            painter->rotate(-angle);

            painter->setBrush(m_fillColor);
            painter->drawEllipse(QPointF(0,0), 50.0, 50.0);

            painter->setBrush(m_penColor);
            painter->setPen(QPen(m_penColor, 2.5, Qt::SolidLine, Qt::RoundCap));

            // eyes
            if (m_isDead) {
                painter->drawLine(-30.0, -30.0, -20.0, -20.0);
                painter->drawLine(-20.0, -30.0, -30.0, -20.0);

                painter->drawLine(20.0, -30.0, 30.0, -20.0);
                painter->drawLine(30.0, -30.0, 20.0, -20.0);
            } else {
                painter->drawChord(QRectF(-30.0, -30.0, 25.0, 70.0), 30.0*16, 120.0*16);
                painter->drawChord(QRectF(5.0, -30.0, 25.0, 70.0), 30.0*16, 120.0*16);
            }

            // mouth
            if (m_isDead) {
                painter->drawLine(-28.0, 2.0, 29.0, 2.0);
            } else {
                painter->setBrush(QColor(128, 0, 64 ));
                painter->drawChord(QRectF(-28.0, 2.0-55.0/2.0, 57.0, 55.0), 0.0, -180.0*16);
            }

            // pupils
            if (!m_isDead) {
                painter->setPen(QPen(m_fillColor, 1.0, Qt::SolidLine, Qt::RoundCap));
                painter->setBrush(m_fillColor);
                painter->drawEllipse(QPointF(-12.0, -25.0), 5.0, 5.0);
                painter->drawEllipse(QPointF(22.0, -25.0), 5.0, 5.0);
            }
        }
    }
}



