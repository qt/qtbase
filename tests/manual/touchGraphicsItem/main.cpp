/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtWidgets>

class TouchableItem : public QGraphicsRectItem
{
public:
    TouchableItem() : QGraphicsRectItem(50, 50, 400, 400)
    {
        setBrush(Qt::yellow);
        setAcceptTouchEvents(true);
    }
protected:
    bool sceneEvent(QEvent *e)
    {
        const bool ret = QGraphicsRectItem::sceneEvent(e);
        switch (e->type()) {
            case QEvent::TouchBegin:
            case QEvent::TouchUpdate:
            case QEvent::TouchEnd:
            {
                QTouchEvent *te = static_cast<QTouchEvent *>(e);
                for (const QTouchEvent::TouchPoint &tp : te->touchPoints()) {
                    QGraphicsEllipseItem *diameterItem = nullptr;
                    QSizeF ellipse = tp.ellipseDiameters();
                    if (ellipse.isNull()) {
                        ellipse = QSizeF(5, 5);
                    } else {
                        diameterItem = new QGraphicsEllipseItem(QRectF(tp.pos().x() - ellipse.width() / 2, tp.pos().y() - ellipse.height() / 2,
                                                                       ellipse.width(), ellipse.height()), this);
                        diameterItem->setPen(QPen(Qt::red));
                        diameterItem->setBrush(QBrush(Qt::red));
                        if (ellipse.width() > qreal(2) && ellipse.height() > qreal(2))
                            ellipse.scale(ellipse.width() - 2, ellipse.height() - 2, Qt::IgnoreAspectRatio);
                    }
                    QGraphicsItem *parent = diameterItem ? static_cast<QGraphicsItem *>(diameterItem) : static_cast<QGraphicsItem *>(this);
                    QGraphicsEllipseItem *ellipseItem = new QGraphicsEllipseItem(QRectF(tp.pos().x() - ellipse.width() / 2,
                                                                                        tp.pos().y() - ellipse.height() / 2,
                                                                                        ellipse.width(), ellipse.height()), parent);
                    ellipseItem->setPen(QPen(Qt::blue));
                    ellipseItem->setBrush(QBrush(Qt::blue));
                }
                te->accept();
                return true;
            }
            default:
                break;
        }
        return ret;
    }
};

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QMainWindow mw;
    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(new QLabel("The blue ellipses should indicate touch point contact patches"));
    qDebug() << "Touch devices:";
    for (const QTouchDevice *device : QTouchDevice::devices()) {
        QString result;
        QTextStream str(&result);
        str << (device->type() == QTouchDevice::TouchScreen ? "TouchScreen" : "TouchPad")
            << " \"" << device->name() << "\", max " << device->maximumTouchPoints()
            << " touch points, capabilities:";
        const QTouchDevice::Capabilities capabilities = device->capabilities();
        if (capabilities & QTouchDevice::Position)
            str << " Position";
        if (capabilities & QTouchDevice::Area)
            str << " Area";
        if (capabilities & QTouchDevice::Pressure)
            str << " Pressure";
        if (capabilities & QTouchDevice::Velocity)
            str << " Velocity";
        if (capabilities & QTouchDevice::RawPositions)
            str << " RawPositions";
        if (capabilities & QTouchDevice::NormalizedPosition)
            str << " NormalizedPosition";
        if (capabilities & QTouchDevice::MouseEmulation)
            str << " MouseEmulation";
        vbox->addWidget(new QLabel(result));
        qDebug() << "   " << result;
    }
    QGraphicsView *view = new QGraphicsView;
    view->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 500, 500);
    TouchableItem *touchableItem = new TouchableItem;
    scene->addItem(touchableItem);
    view->setScene(scene);
    vbox->addWidget(view);
    w->setLayout(vbox);
    mw.setCentralWidget(w);
    QMenu *menu = mw.menuBar()->addMenu("Menu");
    QAction *clear = new QAction("Clear");
    QObject::connect(clear, &QAction::triggered, [=]() {
        qDeleteAll(touchableItem->childItems());
    });
    menu->addAction(clear);
    QAction *ignoreTransform = new QAction("Ignore transformations");
    QObject::connect(ignoreTransform, &QAction::triggered, [=]() {
        view->scale(1.5, 1.5);
        touchableItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    });
    menu->addAction(ignoreTransform);
    QAction *quit = new QAction("Quit");
    quit->setShortcut(QKeySequence::Quit);
    QObject::connect(quit, &QAction::triggered, &QApplication::quit);
    menu->addAction(quit);
    mw.show();

    return a.exec();
}


