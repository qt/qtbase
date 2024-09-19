// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
                for (const QEventPoint &tp : te->points()) {
                    QGraphicsEllipseItem *diameterItem = nullptr;
                    QSizeF ellipse = tp.ellipseDiameters();
                    if (ellipse.isNull()) {
                        ellipse = QSizeF(5, 5);
                    } else {
                        diameterItem = new QGraphicsEllipseItem(QRectF(tp.position().x() - ellipse.width() / 2, tp.position().y() - ellipse.height() / 2,
                                                                       ellipse.width(), ellipse.height()), this);
                        diameterItem->setPen(QPen(Qt::red));
                        diameterItem->setBrush(QBrush(Qt::red));
                        if (ellipse.width() > qreal(2) && ellipse.height() > qreal(2))
                            ellipse.scale(ellipse.width() - 2, ellipse.height() - 2, Qt::IgnoreAspectRatio);
                    }
                    QGraphicsItem *parent = diameterItem ? static_cast<QGraphicsItem *>(diameterItem) : static_cast<QGraphicsItem *>(this);
                    QGraphicsEllipseItem *ellipseItem = new QGraphicsEllipseItem(QRectF(tp.position().x() - ellipse.width() / 2,
                                                                                        tp.position().y() - ellipse.height() / 2,
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
    for (const QInputDevice *device : QInputDevice::devices()) {
        const QPointingDevice *dev = qobject_cast<const QPointingDevice *>(device);
        QString result;
        QTextStream str(&result);
        str << (device->type() == QInputDevice::DeviceType::TouchScreen ? "TouchScreen" : "TouchPad")
            << " \"" << device->name() << "\", max " << (dev ? dev->maximumPoints() : 0)
            << " touch points, capabilities:";
        const QInputDevice::Capabilities capabilities = device->capabilities();
        if (capabilities & QInputDevice::Capability::Position)
            str << " Position";
        if (capabilities & QInputDevice::Capability::Area)
            str << " Area";
        if (capabilities & QInputDevice::Capability::Pressure)
            str << " Pressure";
        if (capabilities & QInputDevice::Capability::Velocity)
            str << " Velocity";
        if (capabilities & QInputDevice::Capability::NormalizedPosition)
            str << " NormalizedPosition";
        if (capabilities & QInputDevice::Capability::MouseEmulation)
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


