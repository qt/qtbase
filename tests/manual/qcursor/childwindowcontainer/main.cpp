/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
