// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QRasterWindow>
#include <QPainter>
#include <QPainterPath>
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
        connect(&m_timer, &QTimer::timeout, this, qOverload<>(&PaintedWindow::update));
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
