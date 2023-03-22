// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QOpenGLWindow>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QStaticText>
#include <QKeyEvent>

#include "background_renderer.h"

static QPainterPath painterPathForTriangle()
{
    static const QPointF bottomLeft(-1.0, -1.0);
    static const QPointF top(0.0, 1.0);
    static const QPointF bottomRight(1.0, -1.0);

    QPainterPath path(bottomLeft);
    path.lineTo(top);
    path.lineTo(bottomRight);
    path.closeSubpath();
    return path;
}

class OpenGLWindow : public QOpenGLWindow
{
    Q_OBJECT

public:
    OpenGLWindow();

protected:
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void setAnimating(bool enabled);

    QMatrix4x4 m_window_normalised_matrix;
    QMatrix4x4 m_window_painter_matrix;
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model_triangle;
    QMatrix4x4 m_model_text;

    FragmentToy m_fragment_toy;
    QStaticText m_text_layout;
    bool m_animate;
};

// Use NoPartialUpdate. This means that all the rendering goes directly to
// the window surface, no additional framebuffer object stands in the
// middle. This is fine since we will clear the entire framebuffer on each
// paint. Under the hood this means that the behavior is equivalent to the
// manual makeCurrent - perform OpenGL calls - swapBuffers loop that is
// typical in pure QWindow-based applications.
OpenGLWindow::OpenGLWindow()
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
    , m_fragment_toy("./background.frag")
    , m_text_layout("The triangle and this text is rendered with QPainter")
    , m_animate(true)
{
    setGeometry(300, 300, 500, 500);

    m_view.lookAt(QVector3D(3,1,1),
                  QVector3D(0,0,0),
                  QVector3D(0,1,0));

    setAnimating(m_animate);
}

void OpenGLWindow::paintGL()
{
    m_fragment_toy.draw(size());

    QPainter p(this);
    p.setWorldTransform(m_window_normalised_matrix.toTransform());

    QMatrix4x4 mvp = m_projection * m_view * m_model_triangle;
    p.setTransform(mvp.toTransform(), true);

    p.fillPath(painterPathForTriangle(), QBrush(QGradient(QGradient::NightFade)));

    QTransform text_transform = (m_window_painter_matrix * m_view * m_model_text).toTransform();
    p.setTransform(text_transform, false);
    p.setPen(QPen(Qt::black));
    m_text_layout.prepare(text_transform);
    qreal x = - (m_text_layout.size().width() / 2);
    qreal y = 0;
    p.drawStaticText(x, y, m_text_layout);

    m_model_triangle.rotate(-1, 0, 1, 0);
    m_model_text.rotate(1, 0, 1, 0);
}

void OpenGLWindow::resizeGL(int w, int h)
{
    m_window_normalised_matrix.setToIdentity();
    m_window_normalised_matrix.translate(w / 2.0, h / 2.0);
    m_window_normalised_matrix.scale(w / 2.0, -h / 2.0);

    m_window_painter_matrix.setToIdentity();
    m_window_painter_matrix.translate(w / 2.0, h / 2.0);

    m_text_layout.setTextWidth(std::max(w * 0.2, 80.0));

    m_projection.setToIdentity();
    m_projection.perspective(45.f, qreal(w) / qreal(h), 0.1f, 100.f);
}

void OpenGLWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_P) { // pause
        m_animate = !m_animate;
        setAnimating(m_animate);
    }
}

void OpenGLWindow::setAnimating(bool enabled)
{
    if (enabled) {
        // Animate continuously, throttled by the blocking swapBuffers() call the
        // QOpenGLWindow internally executes after each paint. Once that is done
        // (frameSwapped signal is emitted), we schedule a new update. This
        // obviously assumes that the swap interval (see
        // QSurfaceFormat::setSwapInterval()) is non-zero.
        connect(this, &QOpenGLWindow::frameSwapped,
                this, QOverload<>::of(&QPaintDeviceWindow::update));
        update();
    } else {
        disconnect(this, &QOpenGLWindow::frameSwapped,
                   this, QOverload<>::of(&QPaintDeviceWindow::update));
    }
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    OpenGLWindow window;
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    window.setFormat(fmt);
    window.show();

    return app.exec();
}

#include "main.moc"
