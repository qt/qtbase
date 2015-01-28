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
#include <QtGui>

#ifdef Q_DEAD_CODE_FROM_QT4_WIN
#define CALLGRIND_START_INSTRUMENTATION  {}
#define CALLGRIND_STOP_INSTRUMENTATION   {}
#else
#include "valgrind/callgrind.h"
#endif

#ifdef Q_DEAD_CODE_FROM_QT4_X11
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
        child->setPos(-50 + qrand() % 100, -50 + qrand() % 100);
        child->setParentItem(item);
    }

    View view(&scene, item);
    view.resize(300, 300);
    view.show();
#ifdef Q_DEAD_CODE_FROM_QT4_X11
    qt_x11_wait_for_window_manager(&view);
#endif

    return app.exec();
}

#include "main.moc"
