/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "downloaddialog.h"

#include <QNetworkReply>

#include <qmath.h>

#include <functional>

Images::Images(QWidget *parent) : QWidget(parent), downloadDialog(new DownloadDialog())
{
    setWindowTitle(tr("Image downloading and scaling example"));
    resize(800, 600);

    addUrlsButton = new QPushButton(tr("Add URLs"));
//! [1]
    connect(addUrlsButton, &QPushButton::clicked, this, &Images::process);
//! [1]

    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setEnabled(false);
//! [2]
    connect(cancelButton, &QPushButton::clicked, this, &Images::cancel);
//! [2]

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addUrlsButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    statusBar = new QStatusBar();

    imagesLayout = new QGridLayout();

    mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(statusBar);
    setLayout(mainLayout);
}

Images::~Images()
{
    cancel();
}

//! [3]
void Images::process()
{
    // Clean previous state
    replies.clear();

    if (downloadDialog->exec() == QDialog::Accepted) {

        const auto urls = downloadDialog->getUrls();
        if (urls.empty())
            return;

        cancelButton->setEnabled(true);

        initLayout(urls.size());

        downloadFuture = download(urls);
        statusBar->showMessage(tr("Downloading..."));
//! [3]

//! [4]
        downloadFuture.then([this](auto) { cancelButton->setEnabled(false); })
                .then(QtFuture::Launch::Async,
                      [this] {
                          updateStatus(tr("Scaling..."));
                          return scaled();
                      })
//! [4]
//! [5]
                .then([this](const QList<QImage> &scaled) {
                    QMetaObject::invokeMethod(this, [this, scaled] { showImages(scaled); });
                    updateStatus(tr("Finished"));
                })
//! [5]
//! [6]
                .onCanceled([this] { updateStatus(tr("Download has been canceled.")); })
                .onFailed([this](QNetworkReply::NetworkError error) {
                    const auto msg = QString("Download finished with error: %1").arg(error);
                    updateStatus(tr(msg.toStdString().c_str()));

                    // Abort all pending requests
                    QMetaObject::invokeMethod(this, &Images::abortDownload);
                })
                .onFailed([this](const std::exception& ex) {
                    updateStatus(tr(ex.what()));
                });
//! [6]
    }
}

//! [7]
void Images::cancel()
{
    statusBar->showMessage(tr("Canceling..."));

    downloadFuture.cancel();
    abortDownload();
}
//! [7]

//! [8]
QFuture<QByteArray> Images::download(const QList<QUrl> &urls)
//! [8]
{
//! [9]
    QSharedPointer<QPromise<QByteArray>> promise(new QPromise<QByteArray>());
    promise->start();
//! [9]

//! [10]
    for (auto url : urls) {
        QSharedPointer<QNetworkReply> reply(qnam.get(QNetworkRequest(url)));
        replies.push_back(reply);
//! [10]

//! [11]
        QtFuture::connect(reply.get(), &QNetworkReply::finished).then([=] {
            if (promise->isCanceled()) {
                if (!promise->future().isFinished())
                    promise->finish();
                return;
            }

            if (reply->error() != QNetworkReply::NoError) {
                if (!promise->future().isFinished())
                    throw reply->error();
            }
//! [12]
            promise->addResult(reply->readAll());

            // Report finished on the last download
            if (promise->future().resultCount() == urls.size()) {
                promise->finish();
            }
//! [12]
        }).onFailed([=] (QNetworkReply::NetworkError error) {
            promise->setException(std::make_exception_ptr(error));
            promise->finish();
        }).onFailed([=] {
            const auto ex = std::make_exception_ptr(
                        std::runtime_error("Unknown error occurred while downloading."));
            promise->setException(ex);
            promise->finish();
        });
    }
//! [11]

//! [13]
    return promise->future();
}
//! [13]

//! [14]
QList<QImage> Images::scaled() const
{
    QList<QImage> scaled;
    const auto data = downloadFuture.results();
    for (auto imgData : data) {
        QImage image;
        image.loadFromData(imgData);
        if (image.isNull())
            throw std::runtime_error("Failed to load image.");

        scaled.push_back(image.scaled(100, 100, Qt::KeepAspectRatio));
    }

    return scaled;
}
//! [14]

void Images::showImages(const QList<QImage> &images)
{
    for (int i = 0; i < images.size(); ++i) {
        labels[i]->setAlignment(Qt::AlignCenter);
        labels[i]->setPixmap(QPixmap::fromImage(images[i]));
    }
}

void Images::initLayout(qsizetype count)
{
    // Clean old images
    QLayoutItem *child;
    while ((child = imagesLayout->takeAt(0)) != nullptr) {
        child->widget()->setParent(nullptr);
        delete child;
    }
    labels.clear();

    // Init the images layout for the new images
    const auto dim = int(qSqrt(qreal(count))) + 1;
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            QLabel *imageLabel = new QLabel;
            imageLabel->setFixedSize(100, 100);
            imagesLayout->addWidget(imageLabel, i, j);
            labels.append(imageLabel);
        }
    }
}

void Images::updateStatus(const QString &msg)
{
    QMetaObject::invokeMethod(this, [this, msg] { statusBar->showMessage(msg); });
}

void Images::abortDownload()
{
    for (auto reply : replies)
        reply->abort();
}
