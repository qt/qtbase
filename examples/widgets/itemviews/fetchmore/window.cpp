// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include "filelistmodel.h"

#include <QtWidgets>

Window::Window(QWidget *parent)
    : QWidget(parent)
{
    model = new FileListModel(this);
    model->setDirPath(QDir::rootPath());

    view = new QListView;
    view->setModel(model);

    logViewer = new QPlainTextEdit(this);
    logViewer->setReadOnly(true);
    logViewer->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                                         QSizePolicy::Preferred));

    connect(model, &FileListModel::numberPopulated,
            this, &Window::updateLog);
    connect(view, &QAbstractItemView::activated,
            this, &Window::activated);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(view);
    layout->addWidget(logViewer);

    setWindowTitle(tr("Fetch More Example"));
}

void Window::updateLog(const QString &path, int start, int number, int total)
{
    const int last = start + number - 1;
    const QString nativePath = QDir::toNativeSeparators(path);
    const QString message = tr("%1..%2/%3 items from \"%4\" added.")
                            .arg(start).arg(last).arg(total).arg(nativePath);
    logViewer->appendPlainText(message);
}

void Window::activated(const QModelIndex &index)
{
    const QFileInfo fi = model->fileInfoAt(index);
    if (fi.isDir()) {
        logViewer->clear();
        model->setDirPath(fi.absoluteFilePath());
    }
}
