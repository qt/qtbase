/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qget.h"

#include <QNetworkProxy>
#include <QDebug>
#include <QCoreApplication>
#include <QList>
#include <QStringList>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>

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
             << "--user=<username>                  Set username to use for authentication" << endl
             << "--password=<password>              Set password to use for authentication" << endl
             << "--proxy-user=<username>            Set username to use for proxy authentication" << endl
             << "--proxy-password=<password>        Set password to use for proxy authentication" << endl
             << "--proxy=on                         Use system proxy (default)" << endl
             << "--proxy=off                        Don't use system proxy" << endl
             << "--proxy=<host:port>[,type]         Use specified proxy" << endl
             << "                   ,http           HTTP proxy (default)" << endl
             << "                   ,socks          SOCKS5 proxy" << endl
             << "                   ,ftp            FTP proxy" << endl
             << "                   ,httpcaching    HTTP caching proxy (no CONNECT method)" << endl
             << "--headers=filename                 Set request headers from file contents" << endl
             << "--post=filename                    upload the file to the next url using HTTP POST" << endl
             << "--put=filename                     upload the file to the next url using HTTP PUT" << endl
             << "--content-type=<MIME>              set content-type header for upload" << endl
             << "--serial                           don't run requests in parallel" << endl;
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
        else if (str.startsWith("-"))
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
