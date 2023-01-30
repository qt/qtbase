// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PAINTEDWINDOW_H
#define PAINTEDWINDOW_H

#include <QWindow>

#include <QtGui/qopengl.h>
#include <QtOpenGL/qopenglshaderprogram.h>
#include <QtOpenGL/qopenglframebufferobject.h>

#include <QPropertyAnimation>

#include <QColor>
#include <QImage>
#include <QTime>

QT_BEGIN_NAMESPACE
class QOpenGLContext;
QT_END_NAMESPACE

class PaintedWindow : public QWindow
{
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)

public:
    PaintedWindow();

    qreal rotation() const { return m_rotation; }

signals:
    void rotationChanged(qreal rotation);

private slots:
    void paint();
    void setRotation(qreal r);
    void orientationChanged(Qt::ScreenOrientation newOrientation);
    void rotationDone();

private:
    void exposeEvent(QExposeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

    void paint(QPainter *painter, const QRect &rect);

    QOpenGLContext *m_context;
    qreal m_rotation;

    QImage m_prevImage;
    QImage m_nextImage;
    qreal m_deltaRotation;

    Qt::ScreenOrientation m_targetOrientation;
    Qt::ScreenOrientation m_nextTargetOrientation;

    QPropertyAnimation *m_animation;
    QTimer *m_paintTimer;
};

#endif // PAINTEDWINDOW_H
