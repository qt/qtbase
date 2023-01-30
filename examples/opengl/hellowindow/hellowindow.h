// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HELLOWINDOW_H
#define HELLOWINDOW_H

#include <QWindow>

#include <QColor>
#include <QMutex>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QSharedPointer>
#include <QTimer>

class HelloWindow;

class Renderer : public QObject
{
    Q_OBJECT

public:
    explicit Renderer(const QSurfaceFormat &format, Renderer *share = nullptr,
                      QScreen *screen = nullptr);

    QSurfaceFormat format() const { return m_format; }

public slots:
    void render(HelloWindow *surface, qreal angle, const QColor &color);

signals:
    void requestUpdate();

private:
    void initialize();

    void createGeometry();
    void createBubbles(int number);
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);

    QList<QVector3D> vertices;
    QList<QVector3D> normals;
    int vertexAttr;
    int normalAttr;
    int matrixUniform;
    int colorUniform;

    bool m_initialized;
    QSurfaceFormat m_format;
    QOpenGLContext *m_context;
    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer m_vbo;
    QColor m_backgroundColor;
};

class HelloWindow : public QWindow
{
public:
    explicit HelloWindow(const QSharedPointer<Renderer> &renderer, QScreen *screen = nullptr);

    QColor color() const;
    void updateColor();

protected:
    bool event(QEvent *ev) override;
    void exposeEvent(QExposeEvent *event) override;
    void mousePressEvent(QMouseEvent *) override;
    void render();

private:
    int m_colorIndex;
    QColor m_color;
    const QSharedPointer<Renderer> m_renderer;
};

#endif // HELLOWINDOW_H
