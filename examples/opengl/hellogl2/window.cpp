// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include "glwidget.h"
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QApplication>
#include <QMessageBox>
#include <QMainWindow>

static QMainWindow *findMainWindow()
{
    for (auto *w : QApplication::topLevelWidgets()) {
        if (auto *mw = qobject_cast<QMainWindow *>(w))
            return mw;
    }
    return nullptr;
}

Window::Window()
{
    glWidget = new GLWidget;

    xSlider = createSlider();
    ySlider = createSlider();
    zSlider = createSlider();

    connect(xSlider, &QSlider::valueChanged, glWidget, &GLWidget::setXRotation);
    connect(glWidget, &GLWidget::xRotationChanged, xSlider, &QSlider::setValue);
    connect(ySlider, &QSlider::valueChanged, glWidget, &GLWidget::setYRotation);
    connect(glWidget, &GLWidget::yRotationChanged, ySlider, &QSlider::setValue);
    connect(zSlider, &QSlider::valueChanged, glWidget, &GLWidget::setZRotation);
    connect(glWidget, &GLWidget::zRotationChanged, zSlider, &QSlider::setValue);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QWidget *w = new QWidget;
    QHBoxLayout *container = new QHBoxLayout(w);
    container->addWidget(glWidget);
    container->addWidget(xSlider);
    container->addWidget(ySlider);
    container->addWidget(zSlider);

    mainLayout->addWidget(w);
    dockBtn = new QPushButton(tr("Undock"), this);
    connect(dockBtn, &QPushButton::clicked, this, &Window::dockUndock);
    mainLayout->addWidget(dockBtn);

    xSlider->setValue(15 * 16);
    ySlider->setValue(345 * 16);
    zSlider->setValue(0 * 16);

    setWindowTitle(tr("Hello GL"));
}

QSlider *Window::createSlider()
{
    QSlider *slider = new QSlider(Qt::Vertical);
    slider->setRange(0, 360 * 16);
    slider->setSingleStep(16);
    slider->setPageStep(15 * 16);
    slider->setTickInterval(15 * 16);
    slider->setTickPosition(QSlider::TicksRight);
    return slider;
}

void Window::keyPressEvent(QKeyEvent *e)
{
    if (isWindow() && e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}

void Window::dockUndock()
{
    if (parent())
        undock();
    else
        dock();
}

void Window::dock()
{
    auto *mainWindow = findMainWindow();
    if (mainWindow == nullptr || !mainWindow->isVisible()) {
        QMessageBox::information(this, tr("Cannot Dock"),
                                 tr("Main window already closed"));
        return;
    }
    if (mainWindow->centralWidget()) {
        QMessageBox::information(this, tr("Cannot Dock"),
                                 tr("Main window already occupied"));
        return;
    }
    setAttribute(Qt::WA_DeleteOnClose, false);
    dockBtn->setText(tr("Undock"));
    mainWindow->setCentralWidget(this);
}

void Window::undock()
{
    setParent(nullptr);
    setAttribute(Qt::WA_DeleteOnClose);
    const auto geometry = screen()->availableGeometry();
    move(geometry.x() + (geometry.width() - width()) / 2,
         geometry.y() + (geometry.height() - height()) / 2);
    dockBtn->setText(tr("Dock"));
    show();
}
