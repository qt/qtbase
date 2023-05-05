// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rsslisting.h"

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

//! [setup]
RSSListing::RSSListing(const QString &url, QWidget *parent)
    : QWidget(parent), currentReply(0)
{
    connect(&manager, &QNetworkAccessManager::finished, this, &RSSListing::finished);

    lineEdit = new QLineEdit(this);
    lineEdit->setText(url);
    connect(lineEdit, &QLineEdit::returnPressed, this, &RSSListing::fetch);

    fetchButton = new QPushButton(tr("Fetch"), this);
    connect(fetchButton, &QPushButton::clicked, this, &RSSListing::fetch);

    treeWidget = new QTreeWidget(this);
    connect(treeWidget, &QTreeWidget::itemActivated,
            // Open the link in the browser:
            this, [](QTreeWidgetItem *item) { QDesktopServices::openUrl(QUrl(item->text(1))); });
    treeWidget->setHeaderLabels(QStringList { tr("Title"), tr("Link") });
    treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->addWidget(lineEdit);
    hboxLayout->addWidget(fetchButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(hboxLayout);
    layout->addWidget(treeWidget);

    setWindowTitle(tr("RSS listing example"));
    resize(640, 480);
}
//! [setup]

//! [slots]
void RSSListing::fetch()
{
    lineEdit->setReadOnly(true);
    fetchButton->setEnabled(false);
    treeWidget->clear();

    get(QUrl(lineEdit->text()));
}

void RSSListing::consumeData()
{
    int statusCode = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 200 && statusCode < 300)
        parseXml();
}

void RSSListing::error(QNetworkReply::NetworkError)
{
    qWarning("error retrieving RSS feed");
    xml.clear();
    currentReply->disconnect(this);
    currentReply->deleteLater();
    currentReply = nullptr;
}

void RSSListing::finished(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    lineEdit->setReadOnly(false);
    fetchButton->setEnabled(true);
}
//! [slots]

// Private methods

//! [get]
void RSSListing::get(const QUrl &url)
{
    if (currentReply) {
        currentReply->disconnect(this);
        currentReply->deleteLater();
    }
    currentReply = url.isValid() ? manager.get(QNetworkRequest(url)) : nullptr;
    if (currentReply) {
        connect(currentReply, &QNetworkReply::readyRead, this, &RSSListing::consumeData);
        connect(currentReply, &QNetworkReply::errorOccurred, this, &RSSListing::error);

    }
    xml.setDevice(currentReply); // Equivalent to clear() if currentReply is null.
}
//! [get]

// TODO: this is a candidate for showing how to use coroutines, once available.
//! [parse]
void RSSListing::parseXml()
{
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == u"item") {
                linkString = xml.attributes().value("rss:about").toString();
                titleString.clear();
            }
            currentTag = xml.name().toString();
        } else if (xml.isEndElement()) {
            if (xml.name() == u"item") {

                QTreeWidgetItem *item = new QTreeWidgetItem;
                item->setText(0, titleString);
                item->setText(1, linkString);
                treeWidget->addTopLevelItem(item);
            }
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentTag == "title")
                titleString += xml.text();
            else if (currentTag == "link")
                linkString += xml.text();
        }
    }
    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
}
//! [parse]
