// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "qget.h"

#include <QNetworkProxy>
#include <QDebug>
#include <QCoreApplication>
#include <QList>
#include <QStringList>

void printShortUsage()
{
    qDebug() << QCoreApplication::applicationName() << " [options] [list of urls]" << Qt::endl
             << "Get one or more urls using QNetworkAccessManager" << Qt::endl
             << "--help to display detailed usage" << Qt::endl;
}

void printUsage()
{
    qDebug() << QCoreApplication::applicationName() << " [options] [list of urls]" << Qt::endl
             << "Get one or more urls using QNetworkAccessManager" << Qt::endl
             << "Options:"
             << "--help                             This message" << Qt::endl
             << "--user=<username>                  Set username to use for authentication"
             << Qt::endl
             << "--password=<password>              Set password to use for authentication"
             << Qt::endl
             << "--proxy-user=<username>            Set username to use for proxy authentication"
             << Qt::endl
             << "--proxy-password=<password>        Set password to use for proxy authentication"
             << Qt::endl
             << "--proxy=on                         Use system proxy (default)" << Qt::endl
             << "--proxy=off                        Don't use system proxy" << Qt::endl
             << "--proxy=<host:port>[,type]         Use specified proxy" << Qt::endl
             << "                   ,http           HTTP proxy (default)" << Qt::endl
             << "                   ,socks          SOCKS5 proxy" << Qt::endl
             << "                   ,ftp            FTP proxy" << Qt::endl
             << "                   ,httpcaching    HTTP caching proxy (no CONNECT method)"
             << Qt::endl
             << "--headers=filename                 Set request headers from file contents"
             << Qt::endl
             << "--post=filename                    upload the file to the next url using HTTP POST"
             << Qt::endl
             << "--put=filename                     upload the file to the next url using HTTP PUT"
             << Qt::endl
             << "--content-type=<MIME>              set content-type header for upload" << Qt::endl
             << "--serial                           don't run requests in parallel" << Qt::endl;
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
    QString uploadFileName;
    QString contentType;
    QString httpUser;
    QString httpPassword;
    QString headersFile;
    TransferItem::Method method = TransferItem::Get;
    //arguments match wget where possible
    foreach (QString str, app.arguments().mid(1)) {
        if (str == "--help")
            printUsage();
        else if (str.startsWith("--user="))
            httpUser = str.mid(7);
        else if (str.startsWith("--password="))
            httpPassword = str.mid(11);
        else if (str.startsWith("--proxy-user="))
            dl.setProxyUser(str.mid(13));
        else if (str.startsWith("--proxy-password="))
            dl.setProxyPassword(str.mid(17));
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
        else if (str.startsWith("--put=")) {
            method = TransferItem::Put;
            uploadFileName = str.mid(6);
        }
        else if (str.startsWith("--post=")) {
            method = TransferItem::Post;
            uploadFileName = str.mid(7);
        }
        else if (str.startsWith("--content-type="))
            contentType=str.mid(15);
        else if (str.startsWith("--headers="))
            headersFile=str.mid(10);
        else if (str == "--serial")
            dl.setQueueMode(DownloadManager::Serial);
        else if (str.startsWith(QLatin1Char('-')))
            qDebug() << "unsupported option" << str;
        else {
            QUrl url(QUrl::fromUserInput(str));
            QNetworkRequest request(url);
            //set headers
            if (!headersFile.isEmpty()) {
                QFile f(headersFile);
                if (!f.open(QFile::ReadOnly | QFile::Text)) {
                    qDebug() << "can't open headers file: " << headersFile;
                } else {
                    while (!f.atEnd()) {
                        QByteArray line = f.readLine().trimmed();
                        if (line.isEmpty()) break;
                        int colon = line.indexOf(':');
                        qDebug() << line;
                        if (colon > 0 && colon < line.length() - 2) {
                            request.setRawHeader(line.left(colon), line.mid(colon+2));
                        }
                    }
                    f.close();
                }
            }
            if (!contentType.isEmpty())
                request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);

            switch (method) {
            case TransferItem::Put:
            case TransferItem::Post:
                dl.upload(request, httpUser, httpPassword, uploadFileName, method);
                break;
            case TransferItem::Get:
                dl.get(request, httpUser, httpPassword);
                break;
            }
            method = TransferItem::Get; //default for urls without a request type before it
        }
    }
    QMetaObject::invokeMethod(&dl, "checkForAllDone", Qt::QueuedConnection);
    return app.exec();
}
