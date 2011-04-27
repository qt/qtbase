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

#ifndef BEARERMONITOR_H
#define BEARERMONITOR_H

#include <qnetworkconfigmanager.h>
#include <qnetworksession.h>
#if defined (Q_OS_SYMBIAN) || defined(Q_OS_WINCE)	
#include "ui_bearermonitor_240_320.h"
#elif defined(MAEMO_UI)
#include "ui_bearermonitor_maemo.h"
#else
#include "ui_bearermonitor_640_480.h"
#endif

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

#ifdef Q_OS_WIN
    void registerNetwork();
    void unregisterNetwork();
#endif

    void showConfigurationFor(QTreeWidgetItem *item);

    void createSessionFor(QTreeWidgetItem *item);
    void createNewSession();
#ifndef MAEMO_UI
    void deleteSession();
#endif
    void performScan();

private:
    QNetworkConfigurationManager manager;
    QList<SessionWidget *> sessionWidgets;
};

#endif //BEARERMONITOR_H
