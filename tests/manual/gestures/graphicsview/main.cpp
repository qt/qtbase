// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QScrollBar>
#include <QImageReader>
#include <QVBoxLayout>
#include <QGestureEvent>
#include <QDir>
#include <QFileDialog>

#include "imageitem.h"
#include "gestures.h"
#include "mousepangesturerecognizer.h"

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
    }
protected:
    bool viewportEvent(QEvent *event)
    {
        if (event->type() == QEvent::Gesture) {
            QGestureEvent *ge = static_cast<QGestureEvent *>(event);
            if (QPanGesture *pan = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture))) {
                switch (pan->state()) {
                case Qt::GestureStarted: qDebug("view: Pan: started"); break;
                case Qt::GestureFinished: qDebug("view: Pan: finished"); break;
                case Qt::GestureCanceled: qDebug("view: Pan: canceled"); break;
                case Qt::GestureUpdated: break;
                default: qDebug("view: Pan: <unknown state>"); break;
                }

                const QPointF delta = pan->delta();
                QScrollBar *vbar = verticalScrollBar();
                QScrollBar *hbar = horizontalScrollBar();
                vbar->setValue(vbar->value() - delta.y());
                hbar->setValue(hbar->value() - delta.x());
                ge->accept(pan);
                return true;
            }
        }
        return QGraphicsView::viewportEvent(event);
    }
};

class StandardGestures : public QWidget
{
public:
    StandardGestures(QWidget *parent = nullptr)
            : QWidget(parent)
    {
        scene = new QGraphicsScene(this);
        scene->setSceneRect(-2000, -2000, 4000, 4000);
        view = new QGraphicsView(scene, 0);
        QVBoxLayout *l = new QVBoxLayout(this);
        l->addWidget(view);
    }

    QGraphicsScene *scene;
    QGraphicsView *view;
};

class GlobalViewGestures : public QWidget
{
    Q_OBJECT
public:
    GlobalViewGestures(QWidget *parent = nullptr)
            : QWidget(parent)
    {
        scene = new QGraphicsScene(this);
        scene->setSceneRect(-2000, -2000, 4000, 4000);
        view = new GraphicsView(scene, 0);
        view->viewport()->grabGesture(Qt::PanGesture);
        view->viewport()->grabGesture(ThreeFingerSlideGesture::Type);
        QVBoxLayout *l = new QVBoxLayout(this);
        l->addWidget(view);
    }

    QGraphicsScene *scene;
    QGraphicsView *view;
};

class GraphicsItemGestures : public QWidget
{
    Q_OBJECT
public:
    GraphicsItemGestures(QWidget *parent = nullptr)
            : QWidget(parent)
    {
        scene = new QGraphicsScene(this);
        scene->setSceneRect(-2000, -2000, 4000, 4000);
        view = new QGraphicsView(scene, 0);
        view->viewport()->grabGesture(Qt::PanGesture);
        view->viewport()->grabGesture(ThreeFingerSlideGesture::Type);
        QVBoxLayout *l = new QVBoxLayout(this);
        l->addWidget(view);
    }

    QGraphicsScene *scene;
    QGraphicsView *view;
};

class MainWindow : public QMainWindow
{
public:
    MainWindow();

    void setDirectory(const QString &path);

private:
    QTabWidget *tabWidget;
    StandardGestures *standardGestures;
    GlobalViewGestures *globalViewGestures;
    GraphicsItemGestures *graphicsItemGestures;
};

MainWindow::MainWindow()
{
    (void)QGestureRecognizer::registerRecognizer(new MousePanGestureRecognizer);
    ThreeFingerSlideGesture::Type = QGestureRecognizer::registerRecognizer(new ThreeFingerSlideGestureRecognizer);

    tabWidget = new QTabWidget;

    standardGestures = new StandardGestures;
    tabWidget->addTab(standardGestures, "Standard gestures");

    globalViewGestures = new GlobalViewGestures;
    tabWidget->addTab(globalViewGestures , "Global gestures");

    graphicsItemGestures = new GraphicsItemGestures;
    tabWidget->addTab(graphicsItemGestures, "Graphics item gestures");

    setCentralWidget(tabWidget);
}

void MainWindow::setDirectory(const QString &path)
{
    QDir dir(path);
    const QStringList files = dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    for (const QString &file : files) {
        QImageReader img(path + QLatin1Char('/') +file);
        QImage image = img.read();
        if (!image.isNull()) {
            {
                ImageItem *item = new ImageItem(image);
                item->setPos(0, 0);
                item->setFlags(QGraphicsItem::ItemIsMovable);
                standardGestures->scene->addItem(item);
            }
            {
                ImageItem *item = new ImageItem(image);
                item->setPos(0, 0);
                item->setFlags(QGraphicsItem::ItemIsMovable);
                globalViewGestures->scene->addItem(item);
            }
            {
                GestureImageItem *item = new GestureImageItem(image);
                item->setPos(0, 0);
                item->setFlags(QGraphicsItem::ItemIsMovable);
                graphicsItemGestures->scene->addItem(item);
            }
        }
    }

    {
        QList<QGraphicsItem*> items = standardGestures->scene->items();
        if (!items.isEmpty())
            standardGestures->view->ensureVisible(items.at(0));
    }
    {
        QList<QGraphicsItem*> items = globalViewGestures->scene->items();
        if (!items.isEmpty())
            globalViewGestures->view->ensureVisible(items.at(0));
    }
    {
        QList<QGraphicsItem*> items = graphicsItemGestures->scene->items();
        if (!items.isEmpty())
            graphicsItemGestures->view->ensureVisible(items.at(0));
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow window;
    if (QApplication::arguments().size() > 1)
        window.setDirectory(QApplication::arguments().at(1));
    else
        window.setDirectory(QFileDialog::getExistingDirectory(0, "Select image folder"));
    window.show();
    return app.exec();
}

#include "main.moc"
