// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RSSLISTING_H
#define RSSLISTING_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWidget>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QUrl;
QT_END_NAMESPACE

//! [0]
class RSSListing : public QWidget
{
    Q_OBJECT
public:
    explicit RSSListing(const QString &url = QString(), QWidget *widget = nullptr);

public slots:
    void fetch();
    void finished(QNetworkReply *reply);
    void consumeData();
    void error(QNetworkReply::NetworkError);

private:
    void parseXml();
    void get(const QUrl &url);

    // Parser state:
    QXmlStreamReader xml;
    QString currentTag;
    QString linkString;
    QString titleString;

    // Network state:
    QNetworkAccessManager manager;
    QNetworkReply *currentReply;

    // UI elements:
    QLineEdit *lineEdit;
    QTreeWidget *treeWidget;
    QPushButton *fetchButton;
};
//! [0]

#endif
