/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtWidgets>

class Widget : public QGraphicsWidget
{
public:
    Widget(const QColor &color, const QColor &textColor, const QString &caption,
           QGraphicsItem *parent = 0)
        : QGraphicsWidget(parent)
        , caption(caption)
        , color(color)
        , textColor(textColor)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget * = 0)
    {
        QFont font;
        font.setPixelSize(0.75 * qMin(boundingRect().width(), boundingRect().height()));

        painter->fillRect(boundingRect(), color);
        painter->save();
        painter->setFont(font);
        painter->setPen(textColor);
        painter->drawText(boundingRect(), Qt::AlignCenter, caption);
        painter->restore();
    }

private:
    QString caption;
    QColor color;
    QColor textColor;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene *scene = new QGraphicsScene();

    Widget *a = new Widget(Qt::blue, Qt::white, "a");
    a->setPreferredSize(100, 100);
    Widget *b = new Widget(Qt::green, Qt::black, "b");
    b->setPreferredSize(100, 100);
    Widget *c = new Widget(Qt::red, Qt::black, "c");
    c->setPreferredSize(100, 100);

    QGraphicsAnchorLayout *layout = new QGraphicsAnchorLayout();
/*
    //! [adding a corner anchor in two steps]
    layout->addAnchor(a, Qt::AnchorTop, layout, Qt::AnchorTop);
    layout->addAnchor(a, Qt::AnchorLeft, layout, Qt::AnchorLeft);
    //! [adding a corner anchor in two steps]
*/
    //! [adding a corner anchor]
    layout->addCornerAnchors(a, Qt::TopLeftCorner, layout, Qt::TopLeftCorner);
    //! [adding a corner anchor]

    //! [adding anchors]
    layout->addAnchor(b, Qt::AnchorLeft, a, Qt::AnchorRight);
    layout->addAnchor(b, Qt::AnchorTop, a, Qt::AnchorBottom);
    //! [adding anchors]

    // Place a third widget below the second.
    layout->addAnchor(b, Qt::AnchorBottom, c, Qt::AnchorTop);

/*
    //! [adding anchors to match sizes in two steps]
    layout->addAnchor(b, Qt::AnchorLeft, c, Qt::AnchorLeft);
    layout->addAnchor(b, Qt::AnchorRight, c, Qt::AnchorRight);
    //! [adding anchors to match sizes in two steps]
*/

    //! [adding anchors to match sizes]
    layout->addAnchors(b, c, Qt::Horizontal);
    //! [adding anchors to match sizes]

    // Anchor the bottom-right corner of the third widget to the bottom-right
    // corner of the layout.
    layout->addCornerAnchors(c, Qt::BottomRightCorner, layout, Qt::BottomRightCorner);

    QGraphicsWidget *w = new QGraphicsWidget(0, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    w->setPos(20, 20);
    w->setMinimumSize(100, 100);
    w->setPreferredSize(320, 240);
    w->setLayout(layout);
    w->setWindowTitle(QApplication::translate("simpleanchorlayout", "QGraphicsAnchorLayout in use"));
    scene->addItem(w);

    QGraphicsView *view = new QGraphicsView();
    view->setScene(scene);
    view->setWindowTitle(QApplication::translate("simpleanchorlayout", "Simple Anchor Layout"));

    view->resize(360, 320);
    view->show();

    return app.exec();
}
