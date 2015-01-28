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

#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFileSystemModel model;
    QWidget window;
    QTreeView *tree = new QTreeView(&window);
    tree->setMaximumSize(1000, 600);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(tree);

    window.setLayout(layout);
    model.setRootPath("");
    tree->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    tree->setModel(&model);

    tree->setAnimated(false);
    tree->setIndentation(20);
    tree->setSortingEnabled(true);
    tree->header()->setStretchLastSection(false);

    window.setWindowTitle(QObject::tr("Dir View"));
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    window.show();

    return app.exec();
}
