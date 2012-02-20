/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QGET_H
#define QGET_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

class DownloadItem : public QObject
{
    Q_OBJECT
public:
    DownloadItem(QNetworkReply* r, QNetworkAccessManager& nam);
    ~DownloadItem();

    QNetworkReply* networkReply() { return reply; }

signals:
    void downloadFinished(DownloadItem *self);
private slots:
    void readyRead();
    void finished();
private:
    QNetworkAccessManager& nam;
    QNetworkReply* reply;
    QFile file;
    QList<QUrl> redirects;
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager();
    ~DownloadManager();
    void get(const QUrl& url);
    void setProxy(const QNetworkProxy& proxy) { nam.setProxy(proxy); }
    void setHttpUser(const QString& user) { httpUser = user; }
    void setHttpPassword(const QString& password) { httpPassword = password; }
    void setProxyUser(const QString& user) { proxyUser = user; }
    void setProxyPassword(const QString& password) { proxyPassword = password; }

public slots:
    void checkForAllDone();

private slots:
    void finished(QNetworkReply* reply);
    void authenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
#endif
    void downloadFinished(DownloadItem *item);
private:
    QNetworkAccessManager nam;
    QList<DownloadItem*> downloads;
    QString httpUser;
    QString httpPassword;
    QString proxyUser;
    QString proxyPassword;
};

#endif // QGET_H
