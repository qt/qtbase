// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QGraphicsEllipseItem>
#include <QRandomGenerator>
#include <QOpenGLWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->graphicsView->setViewport(new QOpenGLWidget);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    QRect rect = ui->graphicsView->rect();
    QGraphicsScene *m_scene = new QGraphicsScene(rect);
    ui->graphicsView->setScene(m_scene);

    QSize initialSize(150, 150);
    m_ellipse = m_scene->addEllipse(
        rect.width() / 2 - initialSize.width() / 2,
        rect.height() / 2 - initialSize.height() / 2,
        initialSize.width(),
        initialSize.height(),
        QPen(Qt::magenta, 5),
        Qt::blue);

    QPushButton *button = new QPushButton();
    button->setGeometry(QRect(rect.width() / 2 - 75, 100, 150, 50));
    button->setText("Random ellipse color");
    QObject::connect(button, &QPushButton::pressed, [&]() {
        m_ellipse->setBrush(QColor::fromRgb(QRandomGenerator::global()->generate()));
        m_ellipse->setPen(QPen(QColor::fromRgb(QRandomGenerator::global()->generate()), 5));
    });
    m_scene->addWidget(button);

    m_label = new QLabel();
    m_label->setGeometry(QRect(rect.width() / 2 - 50, rect.height() - 100, 100, 50));
    m_label->setAlignment(Qt::AlignCenter);
    m_scene->addWidget(m_label);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setGeometry(QRect(rect.width() / 2 - 100, rect.height() - 50, 200, 50));
    slider->setMinimum(10);
    slider->setMaximum(300);
    slider->setValue(initialSize.width());
    QObject::connect(slider, &QAbstractSlider::valueChanged, [this](int value){
        QRectF rect = m_ellipse->rect();
        rect.setWidth(value);
        rect.setHeight(value);
        m_ellipse->setRect(rect);
        m_label->setText("Ellipse size: " + QString::number(value));
    });
    emit slider->valueChanged(initialSize.width());
    m_scene->addWidget(slider);

    QMenu *screenShotMenu = menuBar()->addMenu("&Screenshot");
    screenShotMenu->addAction("Left buffer", this, [this](){
        ui->graphicsView->saveImage(QOpenGLWidget::LeftBuffer);
    });

    screenShotMenu->addAction("Right buffer", this, [this](){
        ui->graphicsView->saveImage(QOpenGLWidget::RightBuffer);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

