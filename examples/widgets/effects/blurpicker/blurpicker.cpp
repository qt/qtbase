// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "blurpicker.h"

#include <QtWidgets>
#include <QtCore/qmath.h>
#include <qmath.h>
#include "blureffect.h"

BlurPicker::BlurPicker(QWidget *parent): QGraphicsView(parent), m_index(0.0), m_animation(this, "index")
{
    setBackgroundBrush(QPixmap(":/images/background.jpg"));
    setScene(new QGraphicsScene(this));

    setupScene();
    setIndex(0);

    m_animation.setDuration(400);
    m_animation.setEasingCurve(QEasingCurve::InOutSine);

    setRenderHint(QPainter::Antialiasing, true);
    setFrameStyle(QFrame::NoFrame);
}

qreal BlurPicker::index() const
{
    return m_index;
}

void BlurPicker::setIndex(qreal index)
{
    m_index = index;

    qreal baseline = 0;
    const qreal iconAngle = 2 * M_PI / m_icons.count();
    for (int i = 0; i < m_icons.count(); ++i) {
        QGraphicsItem *icon = m_icons[i];
        qreal a = (i + m_index) * iconAngle;
        qreal xs = 170 * qSin(a);
        qreal ys = 100 * qCos(a);
        QPointF pos(xs, ys);
        pos = QTransform().rotate(-20).map(pos);
        pos -= QPointF(40, 40);
        icon->setPos(pos);
        baseline = qMax(baseline, ys);
        static_cast<BlurEffect *>(icon->graphicsEffect())->setBaseLine(baseline);
    }

    scene()->update();
}

void BlurPicker::setupScene()
{
    scene()->setSceneRect(-200, -120, 400, 240);

    QStringList names;
    names << ":/images/accessories-calculator.png";
    names << ":/images/accessories-text-editor.png";
    names << ":/images/help-browser.png";
    names << ":/images/internet-group-chat.png";
    names << ":/images/internet-mail.png";
    names << ":/images/internet-web-browser.png";
    names << ":/images/office-calendar.png";
    names << ":/images/system-users.png";

    for (int i = 0; i < names.count(); i++) {
        QPixmap pixmap(names[i]);
        QGraphicsPixmapItem *icon = scene()->addPixmap(pixmap);
        icon->setZValue(1);
        icon->setGraphicsEffect(new BlurEffect(icon));
        m_icons << icon;
    }

    QGraphicsPixmapItem *bg = scene()->addPixmap(QPixmap(":/images/background.jpg"));
    bg->setZValue(0);
    bg->setPos(-200, -150);
}

void BlurPicker::keyPressEvent(QKeyEvent *event)
{
    int delta = 0;
    switch (event->key())
    {
    case Qt::Key_Left:
        delta = -1;
        break;
    case  Qt::Key_Right:
        delta = 1;
        break;
    default:
        break;
    }
    if (m_animation.state() == QAbstractAnimation::Stopped && delta) {
        m_animation.setEndValue(m_index + delta);
        m_animation.start();
        event->accept();
    }
}

void BlurPicker::resizeEvent(QResizeEvent * /* event */)
{
}

void BlurPicker::mousePressEvent(QMouseEvent *event)
{
    int delta = 0;
    if (event->position().x() > (width() / 2))
    {
        delta = 1;
    }
    else
    {
        delta = -1;
    }

    if (m_animation.state() == QAbstractAnimation::Stopped && delta) {
        m_animation.setEndValue(m_index + delta);
        m_animation.start();
        event->accept();
    }
}
