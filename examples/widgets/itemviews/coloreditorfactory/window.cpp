// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    const QList<QPair<QString, QColor>> list = { { tr("Alice"), QColor("aliceblue") },
                                                 { tr("Neptun"), QColor("aquamarine") },
                                                 { tr("Ferdinand"), QColor("springgreen") } };

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
