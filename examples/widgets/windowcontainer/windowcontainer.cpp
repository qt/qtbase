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

#include "openglwindow.h"

#include <QApplication>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>


// Making use of the class from the opengl example in gui.
class Window : public OpenGLWindow
{
    Q_OBJECT
public:
    using OpenGLWindow::OpenGLWindow;

    void render(QPainter *p) override
    {
        QLinearGradient g(0, 0, 0, height());
        g.setColorAt(0, QColor("lightsteelblue"));
        g.setColorAt(1, Qt::black);
        p->fillRect(0, 0, width(), height(), g);

        p->setPen(Qt::white);

        p->drawText(20, 30, QLatin1String("This is an OpenGL based QWindow"));

        if (m_key.trimmed().length() > 0) {
            QRect bounds = p->boundingRect(QRect(0, 0, width(), height()), Qt::AlignTop | Qt::AlignLeft, m_key);
            p->save();
            p->translate(width() / 2.0, height() / 2.0);
            p->scale(10, 10);
            p->translate(-bounds.width() / 2.0, -bounds.height() / 2.0);
            p->drawText(bounds, Qt::AlignCenter, m_key);
            p->restore();
        }

        if (m_focus)
            p->drawText(20, height() - 20, QLatin1String("Window has focus!"));

        p->setRenderHint(QPainter::Antialiasing);
        p->drawPolyline(m_polygon);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (!m_mouseDown) {
            m_mouseDown = true;
            m_polygon.clear();
            m_polygon.append(e->pos());
            renderLater();
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (m_mouseDown) {
            m_polygon.append(e->pos());
            renderLater();
        }
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (m_mouseDown) {
            m_mouseDown = false;
            m_polygon.append(e->pos());
            renderLater();
        }
    }

    void focusInEvent(QFocusEvent *) override
    {
        m_focus = true;
        renderLater();
    }

    void focusOutEvent(QFocusEvent *) override
    {
        m_focus = false;
        m_polygon.clear();
        renderLater();
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        m_key = e->text();
        renderLater();
    }

    void keyReleaseEvent(QKeyEvent *) override
    {
        m_key = QString();
        renderLater();
    }
private:
    QPolygon m_polygon;
    QString m_key;
    bool m_mouseDown = false;
    bool m_focus = false;
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);

    Window *window = new Window;

    QWidget *container = QWidget::createWindowContainer(window);
    container->setMinimumSize(300, 300);
    container->setMaximumSize(600, 600);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setFocusPolicy(Qt::StrongFocus);

    window->setGeometry(100, 100, 300, 200);

    layout->addWidget(new QLineEdit(QLatin1String("A QLineEdit")));
    layout->addWidget(container);
    layout->addWidget(new QLineEdit(QLatin1String("A QLabel")));

    widget->show();

    return app.exec();
}

#include "windowcontainer.moc"
