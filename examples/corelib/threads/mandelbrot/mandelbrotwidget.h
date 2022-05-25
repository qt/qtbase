// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MANDELBROTWIDGET_H
#define MANDELBROTWIDGET_H

#include <QGestureEvent>
#include <QPixmap>
#include <QWidget>
#include "renderthread.h"


//! [0]
class MandelbrotWidget : public QWidget
{
    Q_OBJECT

public:
    MandelbrotWidget(QWidget *parent = nullptr);

protected:
    QSize sizeHint() const override { return {1024, 768}; };
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#ifndef QT_NO_GESTURES
    bool event(QEvent *event) override;
#endif

private slots:
    void updatePixmap(const QImage &image, double scaleFactor);
    void zoom(double zoomFactor);

private:
    void scroll(int deltaX, int deltaY);
#ifndef QT_NO_GESTURES
    bool gestureEvent(QGestureEvent *event);
#endif

    RenderThread thread;
    QPixmap pixmap;
    QPoint pixmapOffset;
    QPoint lastDragPos;
    QString help;
    QString info;
    double centerX;
    double centerY;
    double pixmapScale;
    double curScale;
};
//! [0]

#endif // MANDELBROTWIDGET_H
