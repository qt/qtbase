/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "chip.h"
#include "mainwindow.h"
#include "view.h"

#include <QHBoxLayout>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    populateScene();

    h1Splitter = new QSplitter;
    h2Splitter = new QSplitter;

    QSplitter *vSplitter = new QSplitter;
    vSplitter->setOrientation(Qt::Vertical);
    vSplitter->addWidget(h1Splitter);
    vSplitter->addWidget(h2Splitter);

    View *view = new View("Top left view");
    view->view()->setScene(scene);
    h1Splitter->addWidget(view);

    view = new View("Top right view");
    view->view()->setScene(scene);
    h1Splitter->addWidget(view);

    view = new View("Bottom left view");
    view->view()->setScene(scene);
    h2Splitter->addWidget(view);

    view = new View("Bottom right view");
    view->view()->setScene(scene);
    h2Splitter->addWidget(view);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(vSplitter);
    setLayout(layout);

    setWindowTitle(tr("Chip Example"));
}

void MainWindow::populateScene()
{
    scene = new QGraphicsScene;

    QImage image(":/qt4logo.png");

    // Populate scene
    int xx = 0;
    int nitems = 0;
    for (int i = -11000; i < 11000; i += 110) {
        ++xx;
        int yy = 0;
        for (int j = -7000; j < 7000; j += 70) {
            ++yy;
            qreal x = (i + 11000) / 22000.0;
            qreal y = (j + 7000) / 14000.0;

            QColor color(image.pixel(int(image.width() * x), int(image.height() * y)));
            QGraphicsItem *item = new Chip(color, xx, yy);
            item->setPos(QPointF(i, j));
            scene->addItem(item);

            ++nitems;
        }
    }
}
