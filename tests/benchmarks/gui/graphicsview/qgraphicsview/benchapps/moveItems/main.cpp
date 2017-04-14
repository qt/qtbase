/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtGui>

#if 0 // Used to be included in Qt4 for Q_WS_WIN
#define CALLGRIND_START_INSTRUMENTATION  {}
#define CALLGRIND_STOP_INSTRUMENTATION   {}
#else
#include "valgrind/callgrind.h"
#endif

#if 0 // Used to be included in Qt4 for Q_WS_X11
extern void qt_x11_wait_for_window_manager(QWidget *);
#endif

class View : public QGraphicsView
{
    Q_OBJECT
public:
    View(QGraphicsScene *scene, QGraphicsItem *item)
        : QGraphicsView(scene), _item(item)
    {
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        static int n = 0;
        if (n)
            CALLGRIND_START_INSTRUMENTATION
        QGraphicsView::paintEvent(event);
        _item->moveBy(1, 1);
        if (n)
            CALLGRIND_STOP_INSTRUMENTATION
        if (++n == 200)
            qApp->quit();
    }

private:
    QGraphicsItem *_item;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (argc < 2) {
        qDebug("usage: ./%s <numItems>", argv[0]);
        return 1;
    }

    QGraphicsScene scene(-150, -150, 300, 300);
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);

    QGraphicsRectItem *item = scene.addRect(-50, -50, 100, 100, QPen(Qt::NoPen), QBrush(Qt::blue));
    item->setFlag(QGraphicsItem::ItemIsMovable);

    for (int i = 0; i < atoi(argv[1]); ++i) {
        QGraphicsRectItem *child = scene.addRect(-5, -5, 10, 10, QPen(Qt::NoPen), QBrush(Qt::blue));
        child->setPos(-50 + QRandomGenerator::global()->bounded(100), -50 + QRandomGenerator::global()->bounded(100));
        child->setParentItem(item);
    }

    View view(&scene, item);
    view.resize(300, 300);
    view.show();
#if 0 // Used to be included in Qt4 for Q_WS_X11
    qt_x11_wait_for_window_manager(&view);
#endif

    return app.exec();
}

#include "main.moc"
