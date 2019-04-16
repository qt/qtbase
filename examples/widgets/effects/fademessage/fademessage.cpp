/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
