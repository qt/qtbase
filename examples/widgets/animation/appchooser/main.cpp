/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore>
#include <QtWidgets>


class Pixmap : public QGraphicsWidget
{
    Q_OBJECT

public:
    Pixmap(const QPixmap &pix, QGraphicsItem *parent = 0)
        : QGraphicsWidget(parent), orig(pix), p(pix)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) Q_DECL_OVERRIDE
    {
        painter->drawPixmap(QPointF(), p);
    }

    virtual void mousePressEvent(QGraphicsSceneMouseEvent * ) Q_DECL_OVERRIDE
    {
        emit clicked();
    }

    virtual void setGeometry(const QRectF &rect) Q_DECL_OVERRIDE
    {
        QGraphicsWidget::setGeometry(rect);

        if (rect.size().width() > orig.size().width())
            p = orig.scaled(rect.size().toSize());
        else
            p = orig;
    }

Q_SIGNALS:
    void clicked();

private:
    QPixmap orig;
    QPixmap p;
};

class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent = 0) : QGraphicsView(scene, parent)
    {
    }

    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE
    {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }
};


void createStates(const QObjectList &objects,
                  const QRect &selectedRect, QState *parent)
{
    for (int i = 0; i < objects.size(); ++i) {
        QState *state = new QState(parent);
        state->assignProperty(objects.at(i), "geometry", selectedRect);
        parent->addTransition(objects.at(i), SIGNAL(clicked()), state);
    }
}

void createAnimations(const QObjectList &objects, QStateMachine *machine)
{
    for (int i=0; i<objects.size(); ++i)
        machine->addDefaultAnimation(new QPropertyAnimation(objects.at(i), "geometry"));
}

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(appchooser);

    QApplication app(argc, argv);

    Pixmap *p1 = new Pixmap(QPixmap(":/digikam.png"));
    Pixmap *p2 = new Pixmap(QPixmap(":/akregator.png"));
    Pixmap *p3 = new Pixmap(QPixmap(":/accessories-dictionary.png"));
    Pixmap *p4 = new Pixmap(QPixmap(":/k3b.png"));

    p1->setObjectName("p1");
    p2->setObjectName("p2");
    p3->setObjectName("p3");
    p4->setObjectName("p4");

    p1->setGeometry(QRectF(  0.0,   0.0, 64.0, 64.0));
    p2->setGeometry(QRectF(236.0,   0.0, 64.0, 64.0));
    p3->setGeometry(QRectF(236.0, 236.0, 64.0, 64.0));
    p4->setGeometry(QRectF(  0.0, 236.0, 64.0, 64.0));

    QGraphicsScene scene(0, 0, 300, 300);
    scene.setBackgroundBrush(Qt::white);
    scene.addItem(p1);
    scene.addItem(p2);
    scene.addItem(p3);
    scene.addItem(p4);

    GraphicsView window(&scene);
    window.setFrameStyle(0);
    window.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    window.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    window.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QState *group = new QState(&machine);
    group->setObjectName("group");

    QRect selectedRect(86, 86, 128, 128);

    QState *idleState = new QState(group);
    group->setInitialState(idleState);

    QObjectList objects;
    objects << p1 << p2 << p3 << p4;
    createStates(objects, selectedRect, group);
    createAnimations(objects, &machine);

    machine.setInitialState(group);
    machine.start();

    window.resize(300, 300);
    window.show();

    return app.exec();
}

#include "main.moc"
