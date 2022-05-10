// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QStandardItem>
#include <QtCore/qmimedata.h>
#include <QtGui/qdrag.h>
#include <QtOpenGLWidgets/qopenglwidget.h>
#include <QtPrintSupport/qprintdialog.h>
#include <QtPrintSupport/qprinter.h>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qgraphicsview.h>

void graphicsview_snippet_main()
{
//! [0]
QGraphicsScene scene;
QGraphicsRectItem *rect = scene.addRect(QRectF(0, 0, 100, 100));

QGraphicsItem *item = scene.itemAt(50, 50, QTransform());
//! [0]
Q_UNUSED(rect);
Q_UNUSED(item);
}

void myPopulateScene(QGraphicsScene *)
{
    // Intentionally left empty
}

void snippetThatUsesMyPopulateScene()
{
//! [1]
QGraphicsScene scene;
myPopulateScene(&scene);
QGraphicsView view(&scene);
view.show();
//! [1]
}

class CustomItem : public QStandardItem
{
public:
    using QStandardItem::QStandardItem;

    int type() const override { return UserType; }
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    QStandardItem *clone() const override { return new CustomItem; }
};


void printScene()
{
//! [3]
QGraphicsScene scene;
QPrinter printer;
scene.addRect(QRectF(0, 0, 100, 200), QPen(Qt::black), QBrush(Qt::green));

if (QPrintDialog(&printer).exec() == QDialog::Accepted) {
    QPainter painter(&printer);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
}
//! [3]
}

void pixmapScene()
{
//! [4]
QGraphicsScene scene;
scene.addRect(QRectF(0, 0, 100, 200), QPen(Qt::black), QBrush(Qt::green));

QPixmap pixmap;
QPainter painter(&pixmap);
painter.setRenderHint(QPainter::Antialiasing);
scene.render(&painter);
painter.end();

pixmap.save("scene.png");
//! [4]
}

//! [5]
void CustomItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QMimeData *data = new QMimeData;
    QDrag *drag = new QDrag(event->widget());
    drag->setMimeData(data);
    drag->exec();
}
//! [5]

void viewScene()
{
QGraphicsScene scene;
//! [6]
QGraphicsView view(&scene);
QOpenGLWidget *gl = new QOpenGLWidget();
QSurfaceFormat format;
format.setSamples(4);
gl->setFormat(format);
view.setViewport(gl);
//! [6]
}
