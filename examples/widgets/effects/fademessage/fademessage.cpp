// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "fademessage.h"

#include <QtWidgets>

FadeMessage::FadeMessage(QWidget *parent): QGraphicsView(parent)
{
    setScene(&m_scene);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setupScene();

    m_animation = new QPropertyAnimation(m_effect, "strength", this);
    m_animation->setDuration(500);
    m_animation->setEasingCurve(QEasingCurve::InOutSine);
    m_animation->setStartValue(0);
    m_animation->setEndValue(1);

    setRenderHint(QPainter::Antialiasing, true);
    setFrameStyle(QFrame::NoFrame);
}

void FadeMessage::togglePopup()
{
    if (m_message->isVisible()) {
        m_message->setVisible(false);
        m_animation->setDirection(QAbstractAnimation::Backward);
    } else {
        m_message->setVisible(true);
        m_animation->setDirection(QAbstractAnimation::Forward);
    }
    m_animation->start();
}

void FadeMessage::setupScene()
{
    QGraphicsRectItem *parent = m_scene.addRect(0, 0, 800, 600);
    parent->setPen(Qt::NoPen);
    parent->setZValue(0);

    QGraphicsPixmapItem *bg = m_scene.addPixmap(QPixmap(":/background.jpg"));
    bg->setParentItem(parent);
    bg->setZValue(-1);

    for (int i = 1; i < 5; ++i)
        for (int j = 2; j < 5; ++j) {
            QGraphicsRectItem *item = m_scene.addRect(i * 50, (j - 1) * 50, 38, 38);
            item->setParentItem(parent);
            item->setZValue(1);
            int hue = 12 * (i * 5 + j);
            item->setBrush(QColor::fromHsv(hue, 128, 128));
        }

    QFont font;
    font.setPointSize(font.pointSize() * 2);
    font.setBold(true);
    QFontMetrics fontMetrics(font);
    int fh = fontMetrics.height();

    QString sceneText = "Qt Everywhere!";
    int sceneTextWidth = fontMetrics.horizontalAdvance(sceneText);

    QGraphicsRectItem *block = m_scene.addRect(50, 300, sceneTextWidth, fh + 3);
    block->setPen(Qt::NoPen);
    block->setBrush(QColor(102, 153, 51));

    QGraphicsTextItem *text = m_scene.addText(sceneText, font);
    text->setDefaultTextColor(Qt::white);
    text->setPos(50, 300);
    block->setZValue(2);
    block->hide();

    text->setParentItem(block);
    m_message = block;

    m_effect = new QGraphicsColorizeEffect;
    m_effect->setColor(QColor(122, 193, 66));
    m_effect->setStrength(0);
    m_effect->setEnabled(true);
    parent->setGraphicsEffect(m_effect);

    QPushButton *press = new QPushButton;
    press->setText(tr("Press me"));
    connect(press, &QAbstractButton::clicked, this, &FadeMessage::togglePopup);
    m_scene.addWidget(press);

    press->move(300, 500);
}
