// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "customproxy.h"
#include "embeddeddialog.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setStickyFocus(true);
    const int gridSize = 10;

    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            CustomProxy *proxy = new CustomProxy(nullptr, Qt::Window);
            proxy->setWidget(new EmbeddedDialog);

            QRectF rect = proxy->boundingRect();

            proxy->setPos(x * rect.width() * 1.05, y * rect.height() * 1.05);
            proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

            scene.addItem(proxy);
        }
    }
    scene.setSceneRect(scene.itemsBoundingRect());

    QGraphicsView view(&scene);
    view.scale(0.5, 0.5);
    view.setRenderHints(view.renderHints() | QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    view.setBackgroundBrush(QPixmap(":/No-Ones-Laughing-3.jpg"));
    view.setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    view.show();
    view.setWindowTitle("Embedded Dialogs Example");
    return app.exec();
}
