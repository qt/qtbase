/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QDesktopWidget>
#include <QApplication>
#include <QDebug>

class DesktopView : public QGraphicsView
{
    Q_OBJECT
public:
    DesktopView()
        : that(0)
    {
        scene = new QGraphicsScene;
        setScene(scene);

        QDesktopWidget *desktop = QApplication::desktop();
        connect(desktop, SIGNAL(resized(int)), this, SLOT(updateScene()));
        connect(desktop, SIGNAL(resized(int)), this, SLOT(desktopResized(int)));
        connect(desktop, SIGNAL(workAreaResized(int)), this, SLOT(updateScene()));
        connect(desktop, SIGNAL(workAreaResized(int)), this, SLOT(desktopWorkAreaResized(int)));
        connect(desktop, SIGNAL(screenCountChanged(int)), this, SLOT(updateScene()));
        connect(desktop, SIGNAL(screenCountChanged(int)), this, SLOT(desktopScreenCountChanged(int)));

        updateScene();

        QTransform transform;
        transform.scale(0.25, 0.25);
        setTransform(transform);

        setBackgroundBrush(Qt::darkGray);
        desktopScreenCountChanged(-1);
    }

protected:
    void moveEvent(QMoveEvent *e)
    {
        if (that) {
            that->setRect(appRect());
            scene->update();
        }
        QGraphicsView::moveEvent(e);
    }
    void resizeEvent(QResizeEvent *e)
    {
        if (that) {
            that->setRect(appRect());
        }
        QGraphicsView::resizeEvent(e);
    }

private slots:
    void updateScene()
    {
        scene->clear();

        const QDesktopWidget *desktop = QApplication::desktop();
        const bool isVirtualDesktop = desktop->isVirtualDesktop();
        const int homeScreen = desktop->screenNumber(this);

        QRect sceneRect;
        int screenCount = desktop->screenCount();
        for (int s = 0; s < screenCount; ++s) {
            const bool isPrimary = desktop->primaryScreen() == s;
            const QRect screenRect = desktop->screenGeometry(s);
            const QRect workRect = desktop->availableGeometry(s);
            const QBrush fillBrush = palette().brush(isPrimary ? QPalette::Active : QPalette::Inactive, QPalette::Highlight);
            QGraphicsRectItem *screen = new QGraphicsRectItem(0, 0, screenRect.width(), screenRect.height());

            if (isVirtualDesktop) {
                thatRoot = QPoint();
                screen->setPos(screenRect.x(), screenRect.y());
            } else {
               // for non-virtual desktops we assume that screens are
               // simply next to each other
                if (s)
                    screen->setPos(sceneRect.right(), 0);
                if (s == homeScreen)
                    thatRoot = screen->pos().toPoint();
            }

            screen->setBrush(fillBrush);
            scene->addItem(screen);
            sceneRect.setLeft(qMin(sceneRect.left(), screenRect.left()));
            sceneRect.setRight(qMax(sceneRect.right(), screenRect.right()));
            sceneRect.setTop(qMin(sceneRect.top(), screenRect.top()));
            sceneRect.setBottom(qMax(sceneRect.bottom(), screenRect.bottom()));

            QGraphicsRectItem *workArea = new QGraphicsRectItem(screen);
            workArea->setRect(0, 0, workRect.width(), workRect.height());
            workArea->setPos(workRect.x() - screenRect.x(), workRect.y() - screenRect.y());
            workArea->setBrush(Qt::white);

            QGraphicsSimpleTextItem *screenNumber = new QGraphicsSimpleTextItem(workArea);
            screenNumber->setText(QString::number(s));
            screenNumber->setPen(QPen(Qt::black, 1));
            screenNumber->setBrush(fillBrush);
            screenNumber->setFont(QFont("Arial Black", 18));
            screenNumber->setTransform(QTransform().scale(10, 10));
            screenNumber->setTransformOriginPoint(screenNumber->boundingRect().center());
            QSizeF center = (workRect.size() - screenNumber->boundingRect().size()) / 2;
            screenNumber->setPos(center.width(), center.height());

            screen->show();
            screen->setZValue(1);
        }

        if (isVirtualDesktop) {
            QGraphicsRectItem *virtualDesktop = new QGraphicsRectItem;
            virtualDesktop->setRect(sceneRect);
            virtualDesktop->setPen(QPen(Qt::black));
            virtualDesktop->setBrush(Qt::DiagCrossPattern);
            scene->addItem(virtualDesktop);
            virtualDesktop->setZValue(-1);
            virtualDesktop->show();
        }

        that = new QGraphicsRectItem;
        that->setBrush(Qt::red);
        that->setOpacity(0.5);
        that->setZValue(2);
        that->setRect(appRect());
        that->show();
        scene->addItem(that);

        scene->setSceneRect(sceneRect);
        scene->update();
    }

    QRect appRect() const
    {
        QRect rect = frameGeometry();
        if (!QApplication::desktop()->isVirtualDesktop()) {
            rect.translate(thatRoot);
        }
        return rect;
    }

    void desktopResized(int screen)
    {
        qDebug() << "Screen was resized: " << screen
                << ", new size =" << QApplication::desktop()->screenGeometry(screen);
    }
    void desktopWorkAreaResized(int screen)
    {
        qDebug() << "Screen workarea was resized: " << screen
                << ", new size =" << QApplication::desktop()->availableGeometry(screen);
    }
    void desktopScreenCountChanged(int screenCount)
    {
        QDesktopWidget *desktop = QApplication::desktop();
        qDebug() << "";
        if (screenCount != -1) {
            qDebug() << "Screen count was changed to " << screenCount;
        } else {
            screenCount = desktop->screenCount();
            qDebug() << "Screen count: " << screenCount;
        }
        for (int i = 0; i < screenCount; ++i) {
            qDebug() << "  #" << i << ": geometry =" << desktop->screenGeometry(i)
                    << "; available geometry =" << desktop->availableGeometry(i);
        }
        qDebug() << "";
    }

private:
    QGraphicsScene *scene;
    QGraphicsRectItem *that;
    QPoint thatRoot;
};

#include "main.moc"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    DesktopView view;
    view.show();

    return app.exec();
}
