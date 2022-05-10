// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QtGui/QVector3D>

class OpenGLWidgetPrivate;
class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    OpenGLWidget(int interval = 30, const QVector3D &rotAxis = QVector3D(0, 1, 0), QWidget *parent = nullptr);
    ~OpenGLWidget();

    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void setClearColor(const float *c);

private:
    QScopedPointer<OpenGLWidgetPrivate> d;
};

#endif // OPENGLWIDGET_H
