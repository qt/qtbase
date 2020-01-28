/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "window.h"
#include "colorlisteditor.h"

#include <QtWidgets>

//! [0]
Window::Window()
{
    QItemEditorFactory *factory = new QItemEditorFactory;

    QItemEditorCreatorBase *colorListCreator =
        new QStandardItemEditorCreator<ColorListEditor>();

    factory->registerEditor(QMetaType::QColor, colorListCreator);

    QItemEditorFactory::setDefaultFactory(factory);

    createGUI();
}
//! [0]

void Window::createGUI()
{
    const QVector<QPair<QString, QColor> > list =
        {{ tr("Alice"), QColor("aliceblue") },
         { tr("Neptun"), QColor("aquamarine") },
         { tr("Ferdinand"), QColor("springgreen") }};

    QTableWidget *table = new QTableWidget(3, 2);
    table->setHorizontalHeaderLabels({ tr("Name"), tr("Hair Color") });
    table->verticalHeader()->setVisible(false);
    table->resize(150, 50);

    for (int i = 0; i < 3; ++i) {
        const QPair<QString, QColor> &pair = list.at(i);

        QTableWidgetItem *nameItem = new QTableWidgetItem(pair.first);
        QTableWidgetItem *colorItem = new QTableWidgetItem;
        colorItem->setData(Qt::DisplayRole, pair.second);

        table->setItem(i, 0, nameItem);
        table->setItem(i, 1, colorItem);
    }
    table->resizeColumnToContents(0);
    table->horizontalHeader()->setStretchLastSection(true);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(table, 0, 0);

    setLayout(layout);

    setWindowTitle(tr("Color Editor Factory"));
}
