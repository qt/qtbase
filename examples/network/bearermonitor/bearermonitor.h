/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#ifndef BEARERMONITOR_H
#define BEARERMONITOR_H

#include <qnetworkconfigmanager.h>
#include <qnetworksession.h>
#include "ui_bearermonitor_640_480.h"

QT_USE_NAMESPACE

class SessionWidget;

class BearerMonitor : public QWidget, public Ui_BearerMonitor
{
    Q_OBJECT

public:
    BearerMonitor(QWidget *parent = 0);
    ~BearerMonitor();

private slots:
    void configurationAdded(const QNetworkConfiguration &config, QTreeWidgetItem *parent = 0);
    void configurationRemoved(const QNetworkConfiguration &config);
    void configurationChanged(const QNetworkConfiguration &config);
    void updateSnapConfiguration(QTreeWidgetItem *parent, const QNetworkConfiguration &snap);
    void updateConfigurations();

    void onlineStateChanged(bool isOnline);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void registerNetwork();
    void unregisterNetwork();
#endif // Q_OS_WIN && !Q_OS_WINRT

    void showConfigurationFor(QTreeWidgetItem *item);

    void createSessionFor(QTreeWidgetItem *item);
    void createNewSession();
    void deleteSession();
    void performScan();

private:
    QNetworkConfigurationManager manager;
    QList<SessionWidget *> sessionWidgets;
};

#endif //BEARERMONITOR_H
