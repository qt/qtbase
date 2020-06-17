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
#include "imagescaling.h"

#include <qmath.h>

#include <functional>

Images::Images(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Image loading and scaling example"));
    resize(800, 600);

    imageScaling = new QFutureWatcher<QImage>(this);
    connect(imageScaling, &QFutureWatcher<QImage>::resultReadyAt, this, &Images::showImage);
    connect(imageScaling, &QFutureWatcher<QImage>::finished, this, &Images::finished);

    openButton = new QPushButton(tr("Open Images"));
    connect(openButton, &QPushButton::clicked, this, &Images::open);

    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setEnabled(false);
    connect(cancelButton, &QPushButton::clicked, imageScaling, &QFutureWatcher<QImage>::cancel);

    pauseButton = new QPushButton(tr("Pause/Resume"));
    pauseButton->setEnabled(false);
    connect(pauseButton, &QPushButton::clicked, imageScaling, &QFutureWatcher<QImage>::togglePaused);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(pauseButton);
    buttonLayout->addStretch();

    imagesLayout = new QGridLayout();

    mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addStretch();
    setLayout(mainLayout);
}

Images::~Images()
{
    imageScaling->cancel();
    imageScaling->waitForFinished();
}

void Images::open()
{
    // Cancel and wait if we are already loading images.
    if (imageScaling->isRunning()) {
        imageScaling->cancel();
        imageScaling->waitForFinished();
    }

    // Show a file open dialog at QStandardPaths::PicturesLocation.
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Images"),
                            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                            "*.jpg *.png");

    if (files.isEmpty())
        return;

    const int imageSize = 100;

    // Do a simple layout.
    qDeleteAll(labels);
    labels.clear();

    int dim = qSqrt(qreal(files.count())) + 1;
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            QLabel *imageLabel = new QLabel;
            imageLabel->setFixedSize(imageSize,imageSize);
            imagesLayout->addWidget(imageLabel,i,j);
            labels.append(imageLabel);
        }
    }

    std::function<QImage(const QString&)> scale = [imageSize](const QString &imageFileName) {
        QImage image(imageFileName);
        return image.scaled(QSize(imageSize, imageSize), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    };

    // Use mapped to run the thread safe scale function on the files.
    imageScaling->setFuture(QtConcurrent::mapped(files, scale));

    openButton->setEnabled(false);
    cancelButton->setEnabled(true);
    pauseButton->setEnabled(true);
}

void Images::showImage(int num)
{
    labels[num]->setPixmap(QPixmap::fromImage(imageScaling->resultAt(num)));
}

void Images::finished()
{
    openButton->setEnabled(true);
    cancelButton->setEnabled(false);
    pauseButton->setEnabled(false);
}
