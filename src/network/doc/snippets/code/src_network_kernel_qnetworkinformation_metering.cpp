// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![file]
#include <QCoreApplication>
#include <QNetworkInformation>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDebug>
#include <QTimer>

//! [0]
void uploadLogFile()
{
//! [0]
    // Hardcoded log file path (can be replaced with config or env variable)
    QString logFilePath = QCoreApplication::applicationDirPath() + "/log.txt";

    QFile *file = new QFile(logFilePath);
    if (!file->exists()) {
        qWarning() << "Log file does not exist:" << logFilePath;
        return;
    }

    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open log file for reading:" << logFilePath;
        return;
    }

    QNetworkRequest request(QUrl("http://localhost:8080/upload")); // Replace it with an actual upload URL
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    QNetworkAccessManager *manager = new QNetworkAccessManager(file);
    QNetworkReply *reply = manager->post(request, file);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError)
            qDebug() << "Log file upload successful.";
        else
            qWarning() << "Log file upload failed:" << reply->errorString();

        file->deleteLater();
        reply->deleteLater();
        QCoreApplication::quit();
    });
//! [1]
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
//! [1]

    if (!QNetworkInformation::loadDefaultBackend()) {
        qWarning() << "Failed to load QNetworkInformation backend. Exiting.";
        return 1;
    }

    QNetworkInformation *netInfo = QNetworkInformation::instance();

    QTimer::singleShot(0, [&]() {
//! [2]
        if (netInfo->isMetered()) {
            qWarning() << "Log upload skipped: Current network is metered.";
            app.quit();
        } else {
            uploadLogFile();
        }
//! [2]
    });

    return app.exec();
//! [3]
}
//! [3]
//![file]
