/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        resizeOrMove(static_cast<QMouseEvent *>(event)->localPos());
        return true;
    case QEvent::TouchUpdate:
        qDebug() << "Touch update";
        resizeOrMove(static_cast<QTouchEvent *>(event)->touchPoints().first().pos());
        return true;
    case QEvent::TouchBegin:
        qDebug() << "Touch begin";
        resizeOrMove(static_cast<QTouchEvent *>(event)->touchPoints().first().pos());
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
