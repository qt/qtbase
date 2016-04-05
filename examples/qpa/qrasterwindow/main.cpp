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

#include <QRasterWindow>
#include <QPainter>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QTimer>

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

class PaintedWindow : public QRasterWindow
{
    Q_OBJECT

public:
    PaintedWindow()
    {
        m_view.lookAt(QVector3D(3,1,1),
                      QVector3D(0,0,0),
                      QVector3D(0,1,0));
        m_timer.setInterval(16);
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
        m_timer.start();
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.fillRect(QRect(0,0,width(),height()),Qt::gray);

        p.setWorldTransform(m_window_matrix.toTransform());

        QMatrix4x4 mvp = m_projection * m_view * m_model;
        p.setTransform(mvp.toTransform(), true);

        p.fillPath(painterPathForTriangle(), m_brush);

        m_model.rotate(1, 0, 1, 0);
    }

    void resizeEvent(QResizeEvent *)
    {
        m_window_matrix = QTransform();
        m_window_matrix.translate(width() / 2.0, height() / 2.0);
        m_window_matrix.scale(width() / 2.0, -height() / 2.0);

        m_projection.setToIdentity();
        m_projection.perspective(45.f, qreal(width()) / qreal(height()), 0.1f, 100.f);

        QLinearGradient gradient(QPointF(-1,-1), QPointF(1,1));
        gradient.setColorAt(0, Qt::red);
        gradient.setColorAt(1, Qt::green);

        m_brush = QBrush(gradient);
    }

private:
    QMatrix4x4 m_window_matrix;
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model;
    QBrush m_brush;
    QTimer m_timer;
};

int main (int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    PaintedWindow window;
    window.create();
    window.show();

    return app.exec();
}

#include "main.moc"
