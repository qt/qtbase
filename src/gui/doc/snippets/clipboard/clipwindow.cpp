// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <QMimeData>

#include "clipwindow.h"

ClipWindow::ClipWindow(QWidget *parent)
    : QMainWindow(parent)
{
    clipboard = QApplication::clipboard();

    QWidget *centralWidget = new QWidget(this);
    QWidget *currentItem = new QWidget(centralWidget);
    QLabel *mimeTypeLabel = new QLabel(tr("MIME types:"), currentItem);
    mimeTypeCombo = new QComboBox(currentItem);
    QLabel *dataLabel = new QLabel(tr("Data:"), currentItem);
    dataInfoLabel = new QLabel("", currentItem);

    previousItems = new QListWidget(centralWidget);

//! [0]
    connect(clipboard, &QClipboard::dataChanged,
            this, &ClipWindow::updateClipboard);
//! [0]

    QVBoxLayout *currentLayout = new QVBoxLayout(currentItem);
    currentLayout->addWidget(mimeTypeLabel);
    currentLayout->addWidget(mimeTypeCombo);
    currentLayout->addWidget(dataLabel);
    currentLayout->addWidget(dataInfoLabel);
    currentLayout->addStretch(1);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(currentItem, 1);
    mainLayout->addWidget(previousItems);

    setCentralWidget(centralWidget);
    setWindowTitle(tr("Clipboard"));
}

//! [1]
void ClipWindow::updateClipboard()
{
    mimeTypeCombo->clear();

    QStringList formats = clipboard->mimeData()->formats();
    if (formats.isEmpty())
        return;

    for (const auto &format : formats) {
        QByteArray data = clipboard->mimeData()->data(format);
        // ...
    }
//! [1]

    mimeTypeCombo->addItems(formats);

    int size = clipboard->mimeData()->data(formats[0]).size();
    QListWidgetItem *newItem = new QListWidgetItem(previousItems);
    newItem->setText(tr("%1 (%2 bytes)").arg(formats[0]).arg(size));

    updateData(formats[0]);
}

void ClipWindow::updateData(const QString &format)
{
    QByteArray data = clipboard->mimeData()->data(format);
    dataInfoLabel->setText(tr("%1 bytes").arg(data.size()));
}
