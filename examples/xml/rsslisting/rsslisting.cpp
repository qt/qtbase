/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

/*
rsslisting.cpp

Provides a widget for displaying news items from RDF news sources.
RDF is an XML-based format for storing items of information (see
http://www.w3.org/RDF/ for details).

The widget itself provides a simple user interface for specifying
the URL of a news source, and controlling the downloading of news.

The widget downloads and parses the XML asynchronously, feeding the
data to an XML reader in pieces. This allows the user to interrupt
its operation, and also allows very large data sources to be read.
*/


#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "rsslisting.h"


/*
    Constructs an RSSListing widget with a simple user interface, and sets
    up the XML reader to use a custom handler class.

    The user interface consists of a line edit, a push button, and a
    list view widget. The line edit is used for entering the URLs of news
    sources; the push button starts the process of reading the
    news.
*/

RSSListing::RSSListing(QWidget *parent)
    : QWidget(parent), currentReply(0)
{
    lineEdit = new QLineEdit(this);
    lineEdit->setText("http://labs.qt.nokia.com/blogs/feed");

    fetchButton = new QPushButton(tr("Fetch"), this);

    treeWidget = new QTreeWidget(this);
    connect(treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(itemActivated(QTreeWidgetItem*)));
    QStringList headerLabels;
    headerLabels << tr("Title") << tr("Link");
    treeWidget->setHeaderLabels(headerLabels);
    treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);

    connect(&manager, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(finished(QNetworkReply*)));

    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(fetch()));
    connect(fetchButton, SIGNAL(clicked()), this, SLOT(fetch()));

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *hboxLayout = new QHBoxLayout;

    hboxLayout->addWidget(lineEdit);
    hboxLayout->addWidget(fetchButton);

    layout->addLayout(hboxLayout);
    layout->addWidget(treeWidget);

    setWindowTitle(tr("RSS listing example"));
    resize(640,480);
}

/*
    Starts the network request and connects the needed signals
*/
void RSSListing::get(const QUrl &url)
{
    QNetworkRequest request(url);
    if (currentReply) {
        currentReply->disconnect(this);
        currentReply->deleteLater();
    }
    currentReply = manager.get(request);
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

/*
    Starts fetching data from a news source specified in the line
    edit widget.

    The line edit is made read only to prevent the user from modifying its
    contents during the fetch; this is only for cosmetic purposes.
    The fetch button is disabled, the list view is cleared, and we
    define the last list view item to be 0, meaning that there are no
    existing items in the list.

    A URL is created with the raw contents of the line edit and
    a get is initiated.
*/

void RSSListing::fetch()
{
    lineEdit->setReadOnly(true);
    fetchButton->setEnabled(false);
    treeWidget->clear();

    xml.clear();

    QUrl url(lineEdit->text());
    get(url);
}

void RSSListing::metaDataChanged()
{
    QUrl redirectionTarget = currentReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectionTarget.isValid()) {
        get(redirectionTarget);
    }
}

/*
    Reads data received from the RDF source.

    We read all the available data, and pass it to the XML
    stream reader. Then we call the XML parsing function.
*/

void RSSListing::readyRead()
{
    int statusCode = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 200 && statusCode < 300) {
        QByteArray data = currentReply->readAll();
        xml.addData(data);
        parseXml();
    }
}

/*
    Finishes processing an HTTP request.

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
*/

void RSSListing::finished(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    lineEdit->setReadOnly(false);
    fetchButton->setEnabled(true);
}


/*
    Parses the XML data and creates treeWidget items accordingly.
*/
void RSSListing::parseXml()
{
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "item")
                linkString = xml.attributes().value("rss:about").toString();
            currentTag = xml.name().toString();
        } else if (xml.isEndElement()) {
            if (xml.name() == "item") {

                QTreeWidgetItem *item = new QTreeWidgetItem;
                item->setText(0, titleString);
                item->setText(1, linkString);
                treeWidget->addTopLevelItem(item);

                titleString.clear();
                linkString.clear();
            }

        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentTag == "title")
                titleString += xml.text().toString();
            else if (currentTag == "link")
                linkString += xml.text().toString();
        }
    }
    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
    }
}

/*
    Open the link in the browser
*/
void RSSListing::itemActivated(QTreeWidgetItem * item)
{
    QDesktopServices::openUrl(QUrl(item->text(1)));
}

void RSSListing::error(QNetworkReply::NetworkError)
{
    qWarning("error retrieving RSS feed");
    currentReply->disconnect(this);
    currentReply->deleteLater();
    currentReply = 0;
}
