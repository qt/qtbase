// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagewidget.h"
#include "mainwidget.h"

#include <QScreen>
#include <QGuiApplication>
#include <QDir>

MainWidget::MainWidget(QWidget *parent)
    : QMainWindow(parent)
    , imageWidget(new ImageWidget(this))
{
    setCentralWidget(imageWidget);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QRect geometry(QPoint(0, 0), QSize(screenGeometry.width() * 3 / 4, screenGeometry.height() * 3 / 4));
    geometry.moveCenter(screenGeometry.center());
    setGeometry(geometry);
}

void MainWidget::openDirectory(const QString &path)
{
    setWindowTitle(QDir::toNativeSeparators(path));
    imageWidget->openDirectory(path);
}

void MainWidget::grabGestures(const QList<Qt::GestureType> &gestures)
{
    imageWidget->grabGestures(gestures);
}
