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

#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMessageBox>

class MyObject : public QObject
{
public:
    MyObject(QGraphicsItem *i, QObject *parent = 0) : QObject(parent), itemToToggle(i)
    {
        startTimer(500);
    }
protected:
    void timerEvent(QTimerEvent *)
    {
        itemToToggle->setVisible(!itemToToggle->isVisible());
    }
private:
    QGraphicsItem *itemToToggle;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QGraphicsView gv;
    QGraphicsScene *scene = new QGraphicsScene(&gv);
    gv.setScene(scene);
    QGraphicsItem *rect = scene->addRect(0, 0, 200, 200, QPen(Qt::NoPen), QBrush(Qt::yellow));
    rect->setFlag(QGraphicsItem::ItemHasNoContents);
    rect->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QGraphicsItem *childRect = scene->addRect(0, 0, 100, 100, QPen(Qt::NoPen), QBrush(Qt::red));
    childRect->setParentItem(rect);
    gv.show();
    MyObject o(rect);
    QMessageBox::information(0, "What you should see",
                             "The red rectangle should toggle visibility, so you should see it flash on and off");
    return a.exec();
}
