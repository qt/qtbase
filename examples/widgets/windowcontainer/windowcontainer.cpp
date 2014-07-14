/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QLineEdit>



// Making use of the class from the opengl example in gui.
class Window : public OpenGLWindow
{
    Q_OBJECT
public:
    Window()
        : m_mouseDown(false)
        , m_focus(false)
    {
    }

    void render(QPainter *p) Q_DECL_OVERRIDE {
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

        if (m_focus) {
            p->drawText(20, height() - 20, QLatin1String("Window has focus!"));
        }

        p->setRenderHint(QPainter::Antialiasing);
        p->drawPolyline(m_polygon);
    }

    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE {
        m_mouseDown = true;
        m_polygon.clear();
        m_polygon.append(e->pos());
        renderLater();
    }

    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE {
        if (m_mouseDown) {
            m_polygon.append(e->pos());
            renderLater();
        }
    }

    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE {
        m_mouseDown = false;
        m_polygon.append(e->pos());
        renderLater();
    }

    void focusInEvent(QFocusEvent *) Q_DECL_OVERRIDE {
        m_focus = true;
        renderLater();
    }

    void focusOutEvent(QFocusEvent *) Q_DECL_OVERRIDE {
        m_focus = false;
        m_polygon.clear();
        renderLater();
    }

    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE {
        m_key = e->text();
        renderLater();
    }

    void keyReleaseEvent(QKeyEvent *) Q_DECL_OVERRIDE {
        m_key = QString();
        renderLater();
    }
private:
    QPolygon m_polygon;
    bool m_mouseDown;

    bool m_focus;

    QString m_key;
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);

    Window *window = new Window();

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
