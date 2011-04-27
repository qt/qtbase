/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QApplication>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QGraphicsAnchorLayout>
#include <QGraphicsSceneResizeEvent>


class PixmapWidget : public QGraphicsLayoutItem
{

public:
    PixmapWidget(const QPixmap &pix) : QGraphicsLayoutItem()
    {
        original = new QGraphicsPixmapItem(pix);
        setGraphicsItem(original);
        original->show();
        r = QRectF(QPointF(0, 0), pix.size());
    }

    ~PixmapWidget()
    {
        setGraphicsItem(0);
        delete original;
    }

    void setZValue(qreal z)
    {
        original->setZValue(z);
    }

    void setGeometry (const QRectF &rect)
    {
        original->scale(rect.width() / r.width(), rect.height() / r.height());
        original->setPos(rect.x(), rect.y());
        r = rect;
    }

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const
    {
        Q_UNUSED(constraint);
        QSizeF sh;
        switch (which) {
            case Qt::MinimumSize:
                sh = QSizeF(0, 0);
                break;
            case Qt::PreferredSize:
                sh = QSizeF(50, 50);
                break;
            case Qt::MaximumSize:
                sh = QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
                break;
            default:
                break;
        }
         return sh;
    }

private:
    QGraphicsPixmapItem *original;
    QRectF r;
};


class PlaceWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    PlaceWidget(const QPixmap &pix) : QGraphicsWidget(), original(pix), scaled(pix)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
    {
        QPointF reflection = QPointF();
        reflection.setY(scaled.height() + 2);

        painter->drawPixmap(QPointF(), scaled);

        QPixmap tmp(scaled.size());
        tmp.fill(Qt::transparent);
        QPainter p(&tmp);

        // create gradient
        QPoint p1(scaled.width() / 2, 0);
        QPoint p2(scaled.width() / 2, scaled.height());
        QLinearGradient linearGrad(p1, p2);
        linearGrad.setColorAt(0, QColor(0, 0, 0, 0));
        linearGrad.setColorAt(0.65, QColor(0, 0, 0, 127));
        linearGrad.setColorAt(1, QColor(0, 0, 0, 255));

        // apply 'mask'
        p.setBrush(linearGrad);
        p.fillRect(0, 0, tmp.width(), tmp.height(), QBrush(linearGrad));
        p.fillRect(0, 0, tmp.width(), tmp.height(), QBrush(linearGrad));

        // paint the image flipped
        p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        p.drawPixmap(0, 0, QPixmap::fromImage(scaled.toImage().mirrored(false, true)));
        p.end();

        painter->drawPixmap(reflection, tmp);
    }

    void resizeEvent(QGraphicsSceneResizeEvent *event)
    {
        QSize newSize = event->newSize().toSize();
        newSize.setHeight(newSize.height() / 2);
        scaled = original.scaled(newSize);
    }

    QRectF boundingRect() const
    {
        QSize size(scaled.width(), scaled.height() * 2 + 2);
        return QRectF(QPointF(0, 0), size);
    }

private:
    QPixmap original;
    QPixmap scaled;
};


int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(weatheranchorlayout);

    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 800, 480);

    // pixmaps widgets
    PixmapWidget *title = new PixmapWidget(QPixmap(":/images/title.jpg"));
    PlaceWidget *place = new PlaceWidget(QPixmap(":/images/place.jpg"));
    PixmapWidget *details = new PixmapWidget(QPixmap(":/images/5days.jpg"));
    PixmapWidget *sunnyWeather = new PixmapWidget(QPixmap(":/images/weather-few-clouds.png"));
    PixmapWidget *tabbar = new PixmapWidget(QPixmap(":/images/tabbar.jpg"));


    // setup sizes
    title->setPreferredSize(QSizeF(348, 45));
    title->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    place->setPreferredSize(QSizeF(96, 72));
    place->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    details->setMinimumSize(QSizeF(200, 112));
    details->setPreferredSize(QSizeF(200, 112));
    details->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    tabbar->setPreferredSize(QSizeF(70, 24));
    tabbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    sunnyWeather->setPreferredSize(QSizeF(128, 97));
    sunnyWeather->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    sunnyWeather->setZValue(9999);

    // start anchor layout
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setSpacing(0);

    // setup the main widget
    QGraphicsWidget *w = new QGraphicsWidget(0, Qt::Window);
    QPalette p;
    p.setColor(QPalette::Window, Qt::black);
    w->setPalette(p);
    w->setPos(20, 20);
    w->setLayout(l);

    // vertical anchors
    QGraphicsAnchor *anchor = l->addAnchor(title, Qt::AnchorTop, l, Qt::AnchorTop);
    anchor = l->addAnchor(place, Qt::AnchorTop, title, Qt::AnchorBottom);
    anchor->setSpacing(12);
    anchor = l->addAnchor(place, Qt::AnchorBottom, l, Qt::AnchorBottom);
    anchor->setSpacing(12);

    anchor = l->addAnchor(sunnyWeather, Qt::AnchorTop, title, Qt::AnchorTop);
    anchor = l->addAnchor(sunnyWeather, Qt::AnchorBottom, l, Qt::AnchorVerticalCenter);

    anchor = l->addAnchor(tabbar, Qt::AnchorTop, title, Qt::AnchorBottom);
    anchor->setSpacing(5);
    anchor = l->addAnchor(details, Qt::AnchorTop, tabbar, Qt::AnchorBottom);
    anchor->setSpacing(2);
    anchor = l->addAnchor(details, Qt::AnchorBottom, l, Qt::AnchorBottom);
    anchor->setSpacing(12);

    // horizontal anchors
    anchor = l->addAnchor(l, Qt::AnchorLeft, title, Qt::AnchorLeft);
    anchor = l->addAnchor(title, Qt::AnchorRight, l, Qt::AnchorRight);

    anchor = l->addAnchor(place, Qt::AnchorLeft, l, Qt::AnchorLeft);
    anchor->setSpacing(15);
    anchor = l->addAnchor(place, Qt::AnchorRight, details, Qt::AnchorLeft);
    anchor->setSpacing(35);

    anchor = l->addAnchor(sunnyWeather, Qt::AnchorLeft, place, Qt::AnchorHorizontalCenter);
    anchor = l->addAnchor(sunnyWeather, Qt::AnchorRight, l, Qt::AnchorHorizontalCenter);

    anchor = l->addAnchor(tabbar, Qt::AnchorHorizontalCenter, details, Qt::AnchorHorizontalCenter);
    anchor = l->addAnchor(details, Qt::AnchorRight, l, Qt::AnchorRight);

    // QGV setup
    scene.addItem(w);
    scene.setBackgroundBrush(Qt::white);
    QGraphicsView *view = new QGraphicsView(&scene);
    view->show();

    return app.exec();
}

#include "main.moc"
