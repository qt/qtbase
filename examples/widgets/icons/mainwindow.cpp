/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "iconpreviewarea.h"
#include "iconsizespinbox.h"
#include "imagedelegate.h"
#include "mainwindow.h"

//! [0]
MainWindow::MainWindow()
{
    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    createPreviewGroupBox();
    createImagesGroupBox();
    createIconSizeGroupBox();

    createActions();
    createMenus();
    createContextMenu();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(previewGroupBox, 0, 0, 1, 2);
    mainLayout->addWidget(imagesGroupBox, 1, 0);
    mainLayout->addWidget(iconSizeGroupBox, 1, 1);
    centralWidget->setLayout(mainLayout);

    setWindowTitle(tr("Icons"));
    checkCurrentStyle();
    otherRadioButton->click();

    resize(minimumSizeHint());
}
//! [0]

//! [1]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Icons"),
            tr("The <b>Icons</b> example illustrates how Qt renders an icon in "
               "different modes (active, normal, disabled, and selected) and "
               "states (on and off) based on a set of images."));
}
//! [1]

//! [2]
void MainWindow::changeStyle(bool checked)
{
    if (!checked)
        return;

    QAction *action = qobject_cast<QAction *>(sender());
//! [2] //! [3]
    QStyle *style = QStyleFactory::create(action->data().toString());
//! [3] //! [4]
    Q_ASSERT(style);
    QApplication::setStyle(style);

    smallRadioButton->setText(tr("Small (%1 x %1)")
            .arg(style->pixelMetric(QStyle::PM_SmallIconSize)));
    largeRadioButton->setText(tr("Large (%1 x %1)")
            .arg(style->pixelMetric(QStyle::PM_LargeIconSize)));
    toolBarRadioButton->setText(tr("Toolbars (%1 x %1)")
            .arg(style->pixelMetric(QStyle::PM_ToolBarIconSize)));
    listViewRadioButton->setText(tr("List views (%1 x %1)")
            .arg(style->pixelMetric(QStyle::PM_ListViewIconSize)));
    iconViewRadioButton->setText(tr("Icon views (%1 x %1)")
            .arg(style->pixelMetric(QStyle::PM_IconViewIconSize)));
    tabBarRadioButton->setText(tr("Tab bars (%1 x %1)")
            .arg(style->pixelMetric(QStyle::PM_TabBarIconSize)));

    changeSize();
}
//! [4]

//! [5]
void MainWindow::changeSize(bool checked)
{
    if (!checked)
        return;

    int extent;

    if (otherRadioButton->isChecked()) {
        extent = otherSpinBox->value();
    } else {
        QStyle::PixelMetric metric;

        if (smallRadioButton->isChecked()) {
            metric = QStyle::PM_SmallIconSize;
        } else if (largeRadioButton->isChecked()) {
            metric = QStyle::PM_LargeIconSize;
        } else if (toolBarRadioButton->isChecked()) {
            metric = QStyle::PM_ToolBarIconSize;
        } else if (listViewRadioButton->isChecked()) {
            metric = QStyle::PM_ListViewIconSize;
        } else if (iconViewRadioButton->isChecked()) {
            metric = QStyle::PM_IconViewIconSize;
        } else {
            metric = QStyle::PM_TabBarIconSize;
        }
        extent = QApplication::style()->pixelMetric(metric);
    }
    previewArea->setSize(QSize(extent, extent));
    otherSpinBox->setEnabled(otherRadioButton->isChecked());
}
//! [5]

//! [6]
void MainWindow::changeIcon()
{
    QIcon icon;

    for (int row = 0; row < imagesTable->rowCount(); ++row) {
        QTableWidgetItem *item0 = imagesTable->item(row, 0);
        QTableWidgetItem *item1 = imagesTable->item(row, 1);
        QTableWidgetItem *item2 = imagesTable->item(row, 2);

        if (item0->checkState() == Qt::Checked) {
            QIcon::Mode mode;
            if (item1->text() == tr("Normal")) {
                mode = QIcon::Normal;
            } else if (item1->text() == tr("Active")) {
                mode = QIcon::Active;
            } else if (item1->text() == tr("Disabled")) {
                mode = QIcon::Disabled;
            } else {
                mode = QIcon::Selected;
            }

            QIcon::State state;
            if (item2->text() == tr("On")) {
                state = QIcon::On;
            } else {
                state = QIcon::Off;
//! [6] //! [7]
            }
//! [7]

//! [8]
            QString fileName = item0->data(Qt::UserRole).toString();
            QImage image(fileName);
            if (!image.isNull())
                icon.addPixmap(QPixmap::fromImage(image), mode, state);
//! [8] //! [9]
        }
//! [9] //! [10]
    }
//! [10]

//! [11]
    previewArea->setIcon(icon);
}
//! [11]

//! [12]
void MainWindow::addImages()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                    tr("Open Images"), "",
                                    tr("Images (*.png *.xpm *.jpg);;"
                                       "All Files (*)"));
    if (!fileNames.isEmpty()) {
        foreach (QString fileName, fileNames) {
            int row = imagesTable->rowCount();
            imagesTable->setRowCount(row + 1);
//! [12]

//! [13]
            QString imageName = QFileInfo(fileName).baseName();
//! [13] //! [14]
            QTableWidgetItem *item0 = new QTableWidgetItem(imageName);
            item0->setData(Qt::UserRole, fileName);
            item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
//! [14]

//! [15]
            QTableWidgetItem *item1 = new QTableWidgetItem(tr("Normal"));
//! [15] //! [16]
            QTableWidgetItem *item2 = new QTableWidgetItem(tr("Off"));

            if (guessModeStateAct->isChecked()) {
                if (fileName.contains("_act")) {
                    item1->setText(tr("Active"));
                } else if (fileName.contains("_dis")) {
                    item1->setText(tr("Disabled"));
                } else if (fileName.contains("_sel")) {
                    item1->setText(tr("Selected"));
                }

                if (fileName.contains("_on"))
                    item2->setText(tr("On"));
//! [16] //! [17]
            }
//! [17]

//! [18]
            imagesTable->setItem(row, 0, item0);
//! [18] //! [19]
            imagesTable->setItem(row, 1, item1);
            imagesTable->setItem(row, 2, item2);
            imagesTable->openPersistentEditor(item1);
            imagesTable->openPersistentEditor(item2);

            item0->setCheckState(Qt::Checked);
        }
    }
}
//! [19]

//! [20]
void MainWindow::removeAllImages()
{
    imagesTable->setRowCount(0);
    changeIcon();
}
//! [20]

void MainWindow::createPreviewGroupBox()
{
    previewGroupBox = new QGroupBox(tr("Preview"));

    previewArea = new IconPreviewArea;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(previewArea);
    previewGroupBox->setLayout(layout);
}

//! [21]
void MainWindow::createImagesGroupBox()
{
    imagesGroupBox = new QGroupBox(tr("Images"));

    imagesTable = new QTableWidget;
    imagesTable->setSelectionMode(QAbstractItemView::NoSelection);
    imagesTable->setItemDelegate(new ImageDelegate(this));
//! [21]

//! [22]
    QStringList labels;
//! [22] //! [23]
    labels << tr("Image") << tr("Mode") << tr("State");

    imagesTable->horizontalHeader()->setDefaultSectionSize(90);
    imagesTable->setColumnCount(3);
    imagesTable->setHorizontalHeaderLabels(labels);
    imagesTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    imagesTable->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
    imagesTable->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
    imagesTable->verticalHeader()->hide();
//! [23]

//! [24]
    connect(imagesTable, SIGNAL(itemChanged(QTableWidgetItem*)),
//! [24] //! [25]
            this, SLOT(changeIcon()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(imagesTable);
    imagesGroupBox->setLayout(layout);
}
//! [25]

//! [26]
void MainWindow::createIconSizeGroupBox()
{
    iconSizeGroupBox = new QGroupBox(tr("Icon Size"));

    smallRadioButton = new QRadioButton;
    largeRadioButton = new QRadioButton;
    toolBarRadioButton = new QRadioButton;
    listViewRadioButton = new QRadioButton;
    iconViewRadioButton = new QRadioButton;
    tabBarRadioButton = new QRadioButton;
    otherRadioButton = new QRadioButton(tr("Other:"));

    otherSpinBox = new IconSizeSpinBox;
    otherSpinBox->setRange(8, 128);
    otherSpinBox->setValue(64);
//! [26]

//! [27]
    connect(smallRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(largeRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(toolBarRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(listViewRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(iconViewRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(tabBarRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(otherRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(changeSize(bool)));
    connect(otherSpinBox, SIGNAL(valueChanged(int)), this, SLOT(changeSize()));

    QHBoxLayout *otherSizeLayout = new QHBoxLayout;
    otherSizeLayout->addWidget(otherRadioButton);
    otherSizeLayout->addWidget(otherSpinBox);
    otherSizeLayout->addStretch();

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(smallRadioButton, 0, 0);
    layout->addWidget(largeRadioButton, 1, 0);
    layout->addWidget(toolBarRadioButton, 2, 0);
    layout->addWidget(listViewRadioButton, 0, 1);
    layout->addWidget(iconViewRadioButton, 1, 1);
    layout->addWidget(tabBarRadioButton, 2, 1);
    layout->addLayout(otherSizeLayout, 3, 0, 1, 2);
    layout->setRowStretch(4, 1);
    iconSizeGroupBox->setLayout(layout);
}
//! [27]

//! [28]
void MainWindow::createActions()
{
    addImagesAct = new QAction(tr("&Add Images..."), this);
    addImagesAct->setShortcut(tr("Ctrl+A"));
    connect(addImagesAct, SIGNAL(triggered()), this, SLOT(addImages()));

    removeAllImagesAct = new QAction(tr("&Remove All Images"), this);
    removeAllImagesAct->setShortcut(tr("Ctrl+R"));
    connect(removeAllImagesAct, SIGNAL(triggered()),
            this, SLOT(removeAllImages()));

    exitAct = new QAction(tr("&Quit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    styleActionGroup = new QActionGroup(this);
    foreach (QString styleName, QStyleFactory::keys()) {
        QAction *action = new QAction(styleActionGroup);
        action->setText(tr("%1 Style").arg(styleName));
        action->setData(styleName);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(changeStyle(bool)));
    }

    guessModeStateAct = new QAction(tr("&Guess Image Mode/State"), this);
    guessModeStateAct->setCheckable(true);
    guessModeStateAct->setChecked(true);

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}
//! [28]

//! [29]
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(addImagesAct);
    fileMenu->addAction(removeAllImagesAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    foreach (QAction *action, styleActionGroup->actions())
        viewMenu->addAction(action);
    viewMenu->addSeparator();
    viewMenu->addAction(guessModeStateAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}
//! [29]

//! [30]
void MainWindow::createContextMenu()
{
    imagesTable->setContextMenuPolicy(Qt::ActionsContextMenu);
    imagesTable->addAction(addImagesAct);
    imagesTable->addAction(removeAllImagesAct);
}
//! [30]

//! [31]
void MainWindow::checkCurrentStyle()
{
    foreach (QAction *action, styleActionGroup->actions()) {
        QString styleName = action->data().toString();
        QStyle *candidate = QStyleFactory::create(styleName);
        Q_ASSERT(candidate);
        if (candidate->metaObject()->className()
                == QApplication::style()->metaObject()->className()) {
            action->trigger();
            return;
        }
        delete candidate;
    }
}
//! [31]
