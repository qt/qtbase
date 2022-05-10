// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FINALWIDGET_H
#define FINALWIDGET_H

#include <QFrame>
#include <QImage>
#include <QPoint>
#include <QSize>

class QGridLayout;
class QLabel;
class QMouseEvent;
class QWidget;

class FinalWidget : public QFrame
{
    Q_OBJECT

public:
    FinalWidget(QWidget *parent, const QString &name, const QSize &labelSize);
    void setPixmap(const QPixmap &pixmap);
    QPixmap pixmap() const;

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void createImage();

    bool hasImage;
    QImage originalImage;
    QLabel *imageLabel;
    QLabel *nameLabel;
    QPoint dragStartPosition;
};

#endif
