// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "block.h"
#include "renderthread.h"
#include "window.h"

#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>

//! [Window constructor start]
Window::Window(QWidget *parent)
    : QWidget(parent), thread(new RenderThread(this))
{
//! [Window constructor start] //! [set up widgets and connections]

    label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);

    loadButton = new QPushButton(tr("&Load image..."), this);
    resetButton = new QPushButton(tr("&Stop"), this);
    resetButton->setEnabled(false);

    connect(loadButton, &QPushButton::clicked,
            this, QOverload<>::of(&Window::loadImage));
    connect(resetButton, &QPushButton::clicked,
            thread, &RenderThread::requestInterruption);
    connect(thread, &RenderThread::finished,
            this, &Window::resetUi);
//! [set up widgets and connections] //! [connecting signal with custom type]
    connect(thread, &RenderThread::sendBlock,
            this, &Window::addBlock);
//! [connecting signal with custom type]

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addLayout(buttonLayout);

//! [Window constructor finish]
    setWindowTitle(tr("Queued Custom Type"));
}
//! [Window constructor finish]

void Window::loadImage()
{
    QStringList formats;
    const QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    for (const QByteArray &format : supportedFormats)
        if (format.toLower() == format)
            formats.append(QLatin1String("*.") + QString::fromLatin1(format));

    const QString newPath = QFileDialog::getOpenFileName(this, tr("Open Image"),
        path, tr("Image files (%1)").arg(formats.join(' ')));

    if (newPath.isEmpty())
        return;

    const QImage image(newPath);
    if (!image.isNull()) {
        loadImage(image);
        path = newPath;
    }
}

void Window::loadImage(const QImage &image)
{
    QImage useImage;
    const QRect space = QGuiApplication::primaryScreen()->availableGeometry();
    if (image.width() > 0.75*space.width() || image.height() > 0.75*space.height())
        useImage = image.scaled(0.75*space.width(), 0.75*space.height(),
                                Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else
        useImage = image;

    pixmap = QPixmap(useImage.width(), useImage.height());
    pixmap.fill(qRgb(255, 255, 255));
    label->setPixmap(pixmap);
    loadButton->setEnabled(false);
    resetButton->setEnabled(true);
    thread->processImage(useImage);
}

//! [Adding blocks to the display]
void Window::addBlock(const Block &block)
{
    QColor color = block.color();
    color.setAlpha(64);

    QPainter painter;
    painter.begin(&pixmap);
    painter.fillRect(block.rect(), color);
    painter.end();
    label->setPixmap(pixmap);
}
//! [Adding blocks to the display]

void Window::resetUi()
{
    loadButton->setEnabled(true);
    resetButton->setEnabled(false);
}
