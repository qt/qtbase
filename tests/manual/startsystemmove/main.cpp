// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>

constexpr qreal border = 20;

class Window : public QRasterWindow
{
public:
    explicit Window(QWindow *parent = nullptr) : QRasterWindow(parent)
    {
        resize(300, 200);
        setMinimumSize(QSize(border*2, border*2));
    }
protected:
    void resizeOrMove(const QPointF &p);
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

void Window::resizeOrMove(const QPointF &p)
{
    Qt::Edges edges;
    if (p.x() > width() - border)
        edges |= Qt::RightEdge;
    if (p.x() < border)
        edges |= Qt::LeftEdge;
    if (p.y() < border)
        edges |= Qt::TopEdge;
    if (p.y() > height() - border)
        edges |= Qt::BottomEdge;

    if (edges != 0) {
        qDebug() << "startSystemResize" << edges;
        if (startSystemResize(edges))
            qDebug() << " -> supported";
        else
            qDebug() << " -> not supported";
    } else {
        qDebug() << "startSystemMove";
        if (startSystemMove())
            qDebug() << " -> supported";
        else
            qDebug() << " -> not supported";
    }
}

bool Window::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        qDebug() << "Mouse press";
        resizeOrMove(static_cast<QMouseEvent *>(event)->position());
        return true;
    case QEvent::TouchUpdate:
        qDebug() << "Touch update";
        resizeOrMove(static_cast<QTouchEvent *>(event)->points().first().position());
        return true;
    case QEvent::TouchBegin:
        qDebug() << "Touch begin";
        resizeOrMove(static_cast<QTouchEvent *>(event)->points().first().position());
        return true;
    case QEvent::TouchEnd:
        qDebug() << "Touch end";
        return true;
    default:
        return QRasterWindow::event(event);
    }
}

void Window::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QRect fullRect(0, 0, width(), height());
    QRect innerRect = fullRect.marginsRemoved(QMargins(border, border, border, border));
    painter.fillRect(fullRect, QGradient::WarmFlame);
    painter.fillRect(innerRect, QGradient::NightFade);
    painter.drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, QStringLiteral("Click mouse or touch to move window\nDrag along the sides to resize."));
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    Window window;
    window.show();
    return app.exec();
}
