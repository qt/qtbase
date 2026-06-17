// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ARTHURWIDGETS_H
#define ARTHURWIDGETS_H

#include <QBitmap>
#include <QPushButton>
#include <QGroupBox>

QT_FORWARD_DECLARE_CLASS(QPainter)

class ArthurFrame : public QWidget
{
    Q_OBJECT
public:
    ArthurFrame(QWidget *parent);
    virtual void paint(QPainter *) {}

    bool preferImage() const { return m_preferImage; }

public slots:
    void setPreferImage(bool pi) { m_preferImage = pi; }

protected:
    void paintEvent(QPaintEvent *) override;

    QPixmap m_tile;
    bool m_preferImage = false;
};

#endif
