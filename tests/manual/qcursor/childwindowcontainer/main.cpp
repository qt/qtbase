// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets>

class CursorWindow : public QRasterWindow
{
public:
    CursorWindow(QCursor cursor, QColor color)
    :m_cursor(cursor)
    ,m_color(color)
    {
        if (cursor.shape() == Qt::ArrowCursor)
            unsetCursor();
        else
            setCursor(cursor);
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), m_color);
    }

    void mousePressEvent(QMouseEvent *)
    {
        // Toggle cursor
        QCursor newCursor = (cursor().shape() == m_cursor.shape()) ? QCursor() : m_cursor;
        if (newCursor.shape() == Qt::ArrowCursor)
            unsetCursor();
        else
            setCursor(newCursor);
    }

private:
    QCursor m_cursor;
    QColor m_color;
};

class CursorWidget : public QWidget
{
public:
    CursorWidget(QCursor cursor, QColor color)
    :m_cursor(cursor)
    ,m_color(color)
    {
        if (cursor.shape() == Qt::ArrowCursor)
            unsetCursor();
        else
            setCursor(cursor);
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), m_color);
    }

    void mousePressEvent(QMouseEvent *)
    {
        // Toggle cursor
        QCursor newCursor = (cursor().shape() == m_cursor.shape()) ? QCursor() : m_cursor;
        if (newCursor.shape() == Qt::ArrowCursor)
            unsetCursor();
        else
            setCursor(newCursor);
    }

private:
    QCursor m_cursor;
    QColor m_color;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    {
        // Create top-level windowContainer with window. Setting the cursor
        // for the container should set the cursor for the window as well.
        // Setting the cursor for the window overrides the cursor for the
        // container. The example starts out with a window cursor; click
        // to fall back to the container cursor.
        CursorWindow *w1 = new CursorWindow(QCursor(Qt::OpenHandCursor), QColor(Qt::red).darker());
        QWidget* container = QWidget::createWindowContainer(w1);
        container->resize(200, 200);
        container->setCursor(Qt::PointingHandCursor);
        container->show();
    }

    {
        // Similar to above, but with a top-level QWiget
        CursorWidget *w1 = new CursorWidget(QCursor(Qt::IBeamCursor), QColor(Qt::green).darker());
        w1->resize(200, 200);

        CursorWindow *w2 = new CursorWindow(QCursor(Qt::OpenHandCursor), QColor(Qt::red).darker());
        QWidget* container = QWidget::createWindowContainer(w2);
        container->winId(); // must make the container native, otherwise setCursor
                            // sets the cursor on a QWindowContainerClassWindow which
                            // is outside the QWindow hierarchy (macOS).
        container->setParent(w1);
        container->setCursor(Qt::PointingHandCursor);
        container->setGeometry(0, 0, 100, 100);

        w1->show();
    }

    return app.exec();
}
