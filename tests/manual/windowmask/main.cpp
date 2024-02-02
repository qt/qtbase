// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets>

template<typename Paintable>
class Circle : public Paintable
{
public:
    using Paintable::setMinimumSize;
    using Paintable::setMaximumSize;
    using Paintable::width;
    using Paintable::height;
    using Paintable::screen;
    using Paintable::devicePixelRatio;
    using Paintable::metaObject;
    using Paintable::setMask;
    using Paintable::frameGeometry;
    using Paintable::setPosition;
    using Paintable::startSystemMove;
    using Paintable::setFlags;
    using Paintable::requestUpdate;

    Circle()
    {
        setMinimumSize({200, 200});
        setMaximumSize({200, 200});
        setFlags(Qt::Window | Qt::FramelessWindowHint);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        qWarning() << "Painting into a" << this << "with DPR" << devicePixelRatio()
                    << "on a screen with DPR" << screen()->devicePixelRatio();
        QPainter painter(static_cast<Paintable *>(this));
        painter.fillRect(0, 0, width(), height(), devicePixelRatio() == 1 ? Qt::red : Qt::green);
        painter.setPen(QPen(Qt::black, 5));
        painter.drawRect(10, 10, width() - 20, height() - 20);
        painter.drawText(0, 0, width(), height(), Qt::AlignHCenter | Qt::AlignVCenter, metaObject()->className());
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (event->modifiers() & Qt::ControlModifier)
                requestUpdate();
            else if (event->modifiers() & Qt::AltModifier)
                updateMask();
            else if (event->modifiers() & Qt::ShiftModifier && startSystemMove())
                dragPosition = {};
            else
                dragPosition = event->globalPosition() - frameGeometry().topLeft();
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton && !dragPosition.isNull())
            setPosition((event->globalPosition() - dragPosition).toPoint());
    }
    void resizeEvent(QResizeEvent *) override
    {
        updateMask();
    }

    void updateMask()
    {
        int side = qMin(width(), height());
        QRegion maskedRegion(width() / 2 - side / 2, height() / 2 - side / 2, side,
                            side, QRegion::Ellipse);
        qDebug() << "Updating mask for" << this << "to" << maskedRegion.boundingRect();
        setMask(maskedRegion);
    }

    QPointF dragPosition;
};

class WindowLikeWidget : public QWidget
{
public:
    void setPosition(const QPoint &point) { QWidget::move(point); }
    bool startSystemMove() { return windowHandle()->startSystemMove(); }
    void setFlags(Qt::WindowFlags flags) { setWindowFlags(flags); }
    void requestUpdate() { update(); }
};

class Widget : public Circle<WindowLikeWidget>
{
public:
    Widget()
    {
        setAttribute(Qt::WA_TranslucentBackground);
    }
};

class Window : public Circle<QRasterWindow>
{
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Widget widget;
    widget.show();

    Window window;
    window.show();

    auto dumpScreenData = [](QScreen *screen){
        qDebug() << "- name:" << screen->name();
        qDebug() << "- dpr :" << screen->devicePixelRatio();
    };

    QObject::connect(widget.windowHandle(), &QWindow::screenChanged, &widget, [&]{
        qDebug() << "Screen changed for" << &widget;
        dumpScreenData(widget.screen());
    });
    QObject::connect(&window, &QWindow::screenChanged, &window, [&]{
        qDebug() << "Screen changed for" << &window;
        dumpScreenData(window.screen());
    });

    return app.exec();
}
