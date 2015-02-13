/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include <QtWidgets>
#include "window.h"

//! [Window constructor start]
Window::Window()
{
    thread = new RenderThread();
//! [Window constructor start] //! [set up widgets and connections]

    label = new QLabel();
    label->setAlignment(Qt::AlignCenter);

    loadButton = new QPushButton(tr("&Load image..."));
    resetButton = new QPushButton(tr("&Stop"));
    resetButton->setEnabled(false);

    connect(loadButton, SIGNAL(clicked()), this, SLOT(loadImage()));
    connect(resetButton, SIGNAL(clicked()), thread, SLOT(stopProcess()));
    connect(thread, SIGNAL(finished()), this, SLOT(resetUi()));
//! [set up widgets and connections] //! [connecting signal with custom type]
    connect(thread, SIGNAL(sendBlock(Block)), this, SLOT(addBlock(Block)));
//! [connecting signal with custom type]

    QHBoxLayout *buttonLayout = new QHBoxLayout();
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
    foreach (QByteArray format, QImageReader::supportedImageFormats())
        if (format.toLower() == format)
            formats.append("*." + format);

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
    QDesktopWidget desktop;
    QImage useImage;
    QRect space = desktop.availableGeometry();
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
