// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>

QT_FORWARD_DECLARE_CLASS(QThread)

class Renderer;

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

protected:
    void paintEvent(QPaintEvent *) override { }

signals:
    void renderRequested();

public slots:
    void grabContext();

private slots:
    void onAboutToCompose();
    void onFrameSwapped();
    void onAboutToResize();
    void onResized();

private:
    QThread *m_thread;
    Renderer *m_renderer;
};

#endif // GLWIDGET_H
