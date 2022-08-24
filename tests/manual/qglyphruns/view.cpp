// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "view.h"
#include <QTextLayout>
#include <QPainter>

View::View(QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum));
}

View::~View()
{
    delete m_layout;
}

void View::updateLayout(const QString &sourceString,
                        float width,
                        QTextOption::WrapMode mode,
                        const QFont &font)
{
    if (m_layout == nullptr)
        m_layout = new QTextLayout;

    m_layout->setText(sourceString);
    QTextOption option;
    option.setWrapMode(mode);
    m_layout->setTextOption(option);
    m_layout->setFont(font);
    m_layout->beginLayout();
    float y = 0.0f;
    forever {
        QTextLine line = m_layout->createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(width);
        line.setPosition(QPointF(0, y));
        y += line.height();
    }
    m_layout->endLayout();

    update();
    updateGeometry();
}

QSize View::sizeHint() const
{
    if (m_layout != nullptr)
        return m_layout->boundingRect().size().toSize();
    else
        return QSize(100, 100);
}

void View::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (m_layout != nullptr)
        m_layout->draw(&p, QPointF(0, 0));
    if (!m_bounds.isEmpty()) {
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::yellow);
        p.setOpacity(0.25);
        for (const QRect &r : m_bounds) {
            p.drawRect(r);
        }
    }
}
