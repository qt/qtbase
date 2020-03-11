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
