/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qget.h"

#include <QSslError>
#include <QNetworkProxy>
#include <QAuthenticator>
#include <QDebug>
#include <QCoreApplication>
#include <QList>
#include <QStringList>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>

DownloadManager::DownloadManager()
{
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));
    connect(&nam, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(&nam, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)), this, SLOT(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
    connect(&nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)), this, SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));
}

DownloadManager::~DownloadManager()
{

}

void DownloadManager::get(const QUrl& url)
{
    //currently multiple downloads are processed in parallel.
    //could add an option for serial using the downloads list as a queue
    //which would require DownloadItem to hold a request rather than a reply
    QNetworkReply* reply = nam.get(QNetworkRequest(url));
    DownloadItem *dl = new DownloadItem(reply, nam);
    downloads.append(dl);
    connect(dl, SIGNAL(downloadFinished(DownloadItem*)), SLOT(downloadFinished(DownloadItem*)));
}

void DownloadManager::finished(QNetworkReply* reply)
{
}

void DownloadManager::downloadFinished(DownloadItem *item)
{
    downloads.removeOne(item);
    item->deleteLater();
    checkForAllDone();
}

void DownloadManager::checkForAllDone()
{
    if (downloads.isEmpty()) {
        qDebug() << "All Done.";
        QCoreApplication::quit();
    }
}

void DownloadManager::authenticationRequired(QNetworkReply* reply, QAuthenticator* auth)
{
    //provide the credentials exactly once, so that it fails if credentials are incorrect.
    if (!httpUser.isEmpty() || !httpPassword.isEmpty()) {
        auth->setUser(httpUser);
        auth->setPassword(httpPassword);
        httpUser.clear();
        httpPassword.clear();
    }
}

void DownloadManager::proxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* auth)
{
    //provide the credentials exactly once, so that it fails if credentials are incorrect.
    if (!proxyUser.isEmpty() || !proxyPassword.isEmpty()) {
        auth->setUser(proxyUser);
        auth->setPassword(proxyPassword);
        proxyUser.clear();
        proxyPassword.clear();
    }
}

void DownloadManager::sslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    qDebug() << "sslErrors";
    foreach (const QSslError& error, errors) {
        qDebug() << error.errorString();
        qDebug() << error.certificate().toPem();
    }
}

DownloadItem::DownloadItem(QNetworkReply* r, QNetworkAccessManager& manager) : reply(r), nam(manager)
{
    reply->setParent(this);
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

DownloadItem::~DownloadItem()
{
}

void DownloadItem::readyRead()
{
    if (!file.isOpen()) {
        qDebug() << reply->header(QNetworkRequest::ContentTypeHeader) << reply->header(QNetworkRequest::ContentLengthHeader);
        QString path = reply->url().path();
        path = path.mid(path.lastIndexOf('/') + 1);
        if (path.isEmpty())
            path = QLatin1String("index.html");
        file.setFileName(path);
        for (int i=1;i<1000;i++) {
            if (!file.exists() && file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                break;
            file.setFileName(QString(QLatin1String("%1.%2")).arg(path).arg(i));
        }
        if (!file.isOpen()) {
            qDebug() << "couldn't open output file";
            reply->abort();
            return;
        }
        qDebug() << reply->url() << " -> " << file.fileName();
    }
    file.write(reply->readAll());
}

void DownloadItem::finished()
{
    if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        QUrl url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        url = reply->url().resolved(url);
        qDebug() << reply->url() << "redirected to " << url;
        if (redirects.contains(url)) {
            qDebug() << "redirect loop detected";
        } else if (redirects.count() > 10) {
            qDebug() << "too many redirects";
        } else {
            //follow redirect
            if (file.isOpen()) {
                if (!file.seek(0) || !file.resize(0)) {
                    file.close();
                    file.remove();
                }
            }
            reply->deleteLater();
            reply = nam.get(QNetworkRequest(url));
            reply->setParent(this);
            connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
            connect(reply, SIGNAL(finished()), this, SLOT(finished()));
            redirects.append(url);
            return;
        }
    }
    if (file.isOpen()) {
        file.write(reply->readAll());
        file.close();
    }
    qDebug() << "finished " << reply->url() << " with http status: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "and error: " << reply->error() << reply->errorString();
    emit downloadFinished(this);
}

void printShortUsage()
{
    qDebug() << QCoreApplication::applicationName() << " [options] [list of urls]" << endl
             << "Get one or more urls using QNetworkAccessManager" << endl
             << "--help to display detailed usage" << endl;
}

void printUsage()
{
    qDebug() << QCoreApplication::applicationName() << " [options] [list of urls]" << endl
             << "Get one or more urls using QNetworkAccessManager" << endl
             << "Options:"
             << "--help                             This message" << endl
             << "--http-user=<username>             Set username to use for http 401 challenges" << endl
             << "--http-password=<password>         Set password to use for http 401 challenges" << endl
             << "--proxy-user=<username>            Set username to use for proxy authentication" << endl
             << "--proxy-password=<password>        Set password to use for proxy authentication" << endl
             << "--proxy=on                         Use system proxy (default)" << endl
             << "--proxy=off                        Don't use system proxy" << endl
             << "--proxy=<host:port>[,type]         Use specified proxy" << endl
             << "                   ,http           HTTP proxy (default)" << endl
             << "                   ,socks          SOCKS5 proxy" << endl
             << "                   ,ftp            FTP proxy" << endl
             << "                   ,httpcaching    HTTP caching proxy (no CONNECT method)" << endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        printShortUsage();
        return EXIT_FAILURE;
    }

    //use system proxy (by default)
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    DownloadManager dl;
    //arguments match wget where possible
    foreach (QString str, app.arguments().mid(1)) {
        if (str == "--help")
            printUsage();
        else if (str.startsWith("--http-user="))
            dl.setHttpUser(str.mid(12));
        else if (str.startsWith("--http-passwd="))
            dl.setHttpPassword(str.mid(14));
        else if (str.startsWith("--proxy-user="))
            dl.setProxyUser(str.mid(13));
        else if (str.startsWith("--proxy-passwd="))
            dl.setProxyPassword(str.mid(15));
        else if (str == "--proxy=off")
            QNetworkProxyFactory::setUseSystemConfiguration(false);
        else if (str == "--proxy=on")
            QNetworkProxyFactory::setUseSystemConfiguration(true);
        else if (str.startsWith("--proxy=")) {
            //parse "--proxy=host:port[,type]"
            QNetworkProxy proxy;
            str = str.mid(8);
            int sep = str.indexOf(':');
            proxy.setHostName(str.left(sep));
            str = str.mid(sep + 1);
            sep = str.indexOf(',');
            QString port;
            if (sep < 0) {
                port = str;
                proxy.setType(QNetworkProxy::HttpProxy);
            } else {
                port = str.left(sep);
                str = str.mid(sep + 1);
                if (str == "socks")
                    proxy.setType(QNetworkProxy::Socks5Proxy);
                else if (str == "ftp")
                    proxy.setType(QNetworkProxy::FtpCachingProxy);
                else if (str == "httpcaching")
                    proxy.setType(QNetworkProxy::HttpCachingProxy);
                else if (str == "http")
                    proxy.setType(QNetworkProxy::HttpProxy);
                else {
                    qDebug() << "unknown proxy type";
                    return EXIT_FAILURE;
                }
            }
            bool ok;
            quint16 p = port.toUShort(&ok);
            if (!ok) {
                qDebug() << "couldn't parse proxy";
                return EXIT_FAILURE;
            }
            proxy.setPort(p);
            qDebug() << "proxy:" << proxy.hostName() << proxy.port() << proxy.type();
            dl.setProxy(proxy);
        }
        else if (str.startsWith("-"))
            qDebug() << "unsupported option" << str;
        else
            dl.get(QUrl::fromUserInput(str));
    }
    QMetaObject::invokeMethod(&dl, "checkForAllDone", Qt::QueuedConnection);
    return app.exec();
}
