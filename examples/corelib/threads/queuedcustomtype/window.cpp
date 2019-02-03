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
#include <QtWidgets>

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
            thread, &RenderThread::stopProcess);
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

    QString newPath = QFileDialog::getOpenFileName(this, tr("Open Image"),
        path, tr("Image files (%1)").arg(formats.join(' ')));

    if (newPath.isEmpty())
        return;

    QImage image(newPath);
    if (!image.isNull()) {
        loadImage(image);
        path = newPath;
    }
}

void Window::loadImage(const QImage &image)
{
    QImage useImage;
    QRect space = QGuiApplication::primaryScreen()->availableGeometry();
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
