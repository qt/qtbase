// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "car.h"
#include "car_adaptor.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtDBus/QDBusConnection>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(-500, -500, 1000, 1000);
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);

    auto car = new Car();
    scene.addItem(car);

    QGraphicsView view(&scene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setBackgroundBrush(Qt::darkGray);
    view.setWindowTitle(QT_TRANSLATE_NOOP(QGraphicsView, "Qt DBus Controlled Car"));
    view.resize(400, 300);
    view.show();

    new CarInterfaceAdaptor(car);
    auto connection = QDBusConnection::sessionBus();
    connection.registerObject("/Car", car);
    connection.registerService("org.example.CarExample");

    return app.exec();
}
