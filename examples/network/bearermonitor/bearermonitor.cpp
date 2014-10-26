/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include "bearermonitor.h"
#include "sessionwidget.h"

#include <QtCore/QDebug>

#ifdef Q_OS_WIN
#include <winsock2.h>
#undef interface

#ifndef NS_NLA
#define NS_NLA 15
#endif
#endif

BearerMonitor::BearerMonitor(QWidget *parent)
:   QWidget(parent)
{
    setupUi(this);
#ifdef MAEMO_UI
    newSessionButton->hide();
    deleteSessionButton->hide();
#else
    delete tabWidget->currentWidget();
    sessionGroup->hide();
#endif
    updateConfigurations();
    onlineStateChanged(!manager.allConfigurations(QNetworkConfiguration::Active).isEmpty());
    QNetworkConfiguration defaultConfiguration = manager.defaultConfiguration();
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);

        if (item->data(0, Qt::UserRole).toString() == defaultConfiguration.identifier()) {
            treeWidget->setCurrentItem(item);
            showConfigurationFor(item);
            break;
        }
    }
    connect(&manager, SIGNAL(onlineStateChanged(bool)), this ,SLOT(onlineStateChanged(bool)));
    connect(&manager, SIGNAL(configurationAdded(const QNetworkConfiguration&)),
            this, SLOT(configurationAdded(const QNetworkConfiguration&)));
    connect(&manager, SIGNAL(configurationRemoved(const QNetworkConfiguration&)),
            this, SLOT(configurationRemoved(const QNetworkConfiguration&)));
    connect(&manager, SIGNAL(configurationChanged(const QNetworkConfiguration&)),
            this, SLOT(configurationChanged(const QNetworkConfiguration)));
    connect(&manager, SIGNAL(updateCompleted()), this, SLOT(updateConfigurations()));

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    connect(registerButton, SIGNAL(clicked()), this, SLOT(registerNetwork()));
    connect(unregisterButton, SIGNAL(clicked()), this, SLOT(unregisterNetwork()));
#else // Q_OS_WIN && !Q_OS_WINRT
    nlaGroup->hide();
#endif

    connect(treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(createSessionFor(QTreeWidgetItem*)));

    connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(showConfigurationFor(QTreeWidgetItem*)));

    connect(newSessionButton, SIGNAL(clicked()),
            this, SLOT(createNewSession()));
#ifndef MAEMO_UI
    connect(deleteSessionButton, SIGNAL(clicked()),
            this, SLOT(deleteSession()));
#endif
    connect(scanButton, SIGNAL(clicked()),
            this, SLOT(performScan()));

    // Just in case update all configurations so that all
    // configurations are up to date.
    manager.updateConfigurations();
}

BearerMonitor::~BearerMonitor()
{
}

static void updateItem(QTreeWidgetItem *item, const QNetworkConfiguration &config)
{
    item->setText(0, config.name());
    item->setData(0, Qt::UserRole, config.identifier());

    QFont font = item->font(1);
    font.setBold((config.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active);
    item->setFont(0, font);
}

void BearerMonitor::configurationAdded(const QNetworkConfiguration &config, QTreeWidgetItem *parent)
{
    if (!config.isValid())
        return;

    QTreeWidgetItem *item = new QTreeWidgetItem;
    updateItem(item, config);

    if (parent)
        parent->addChild(item);
    else
        treeWidget->addTopLevelItem(item);

    if (config.type() == QNetworkConfiguration::ServiceNetwork) {
        foreach (const QNetworkConfiguration &child, config.children())
            configurationAdded(child, item);
    }
}

void BearerMonitor::configurationRemoved(const QNetworkConfiguration &config)
{
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);

        if (item->data(0, Qt::UserRole).toString() == config.identifier()) {
            delete item;
            break;
        }
    }
}

void BearerMonitor::configurationChanged(const QNetworkConfiguration &config)
{
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);

        if (item->data(0, Qt::UserRole).toString() == config.identifier()) {
            updateItem(item, config);

            if (config.type() == QNetworkConfiguration::ServiceNetwork)
                updateSnapConfiguration(item, config);

            if (item == treeWidget->currentItem())
                showConfigurationFor(item);

            break;
        }
    }
}

void BearerMonitor::updateSnapConfiguration(QTreeWidgetItem *parent, const QNetworkConfiguration &snap)
{
    QMap<QString, QTreeWidgetItem *> itemMap;
    foreach (QTreeWidgetItem *item, parent->takeChildren())
        itemMap.insert(item->data(0, Qt::UserRole).toString(), item);

    QList<QNetworkConfiguration> allConfigurations = snap.children();

    while (!allConfigurations.isEmpty()) {
        QNetworkConfiguration config = allConfigurations.takeFirst();

        QTreeWidgetItem *item = itemMap.take(config.identifier());
        if (item) {
            updateItem(item, config);

            parent->addChild(item);

            if (config.type() == QNetworkConfiguration::ServiceNetwork)
                updateSnapConfiguration(item, config);
        } else {
            configurationAdded(config, parent);
        }
    }

    qDeleteAll(itemMap);
}

void BearerMonitor::updateConfigurations()
{
    progressBar->hide();
    scanButton->show();

    // Just in case update online state, on Symbian platform
    // WLAN scan needs to be triggered initially to have their true state.
    onlineStateChanged(manager.isOnline());

    QList<QTreeWidgetItem *> items = treeWidget->findItems(QLatin1String("*"), Qt::MatchWildcard);
    QMap<QString, QTreeWidgetItem *> itemMap;
    while (!items.isEmpty()) {
        QTreeWidgetItem *item = items.takeFirst();
        itemMap.insert(item->data(0, Qt::UserRole).toString(), item);
    }

    QNetworkConfiguration defaultConfiguration = manager.defaultConfiguration();
    QTreeWidgetItem *defaultItem = itemMap.take(defaultConfiguration.identifier());

    if (defaultItem) {
        updateItem(defaultItem, defaultConfiguration);

        if (defaultConfiguration.type() == QNetworkConfiguration::ServiceNetwork)
            updateSnapConfiguration(defaultItem, defaultConfiguration);
    } else {
        configurationAdded(defaultConfiguration);
    }

    QList<QNetworkConfiguration> allConfigurations = manager.allConfigurations();

    while (!allConfigurations.isEmpty()) {
        QNetworkConfiguration config = allConfigurations.takeFirst();

        if (config.identifier() == defaultConfiguration.identifier())
            continue;

        QTreeWidgetItem *item = itemMap.take(config.identifier());
        if (item) {
            updateItem(item, config);

            if (config.type() == QNetworkConfiguration::ServiceNetwork)
                updateSnapConfiguration(item, config);
        } else {
            configurationAdded(config);
        }
    }

    qDeleteAll(itemMap);
}

void BearerMonitor::onlineStateChanged(bool isOnline)
{
    if (isOnline)
        onlineState->setText(tr("Online"));
    else
        onlineState->setText(tr("Offline"));
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
void BearerMonitor::registerNetwork()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return;

    QNetworkConfiguration configuration =
        manager.configurationFromIdentifier(item->data(0, Qt::UserRole).toString());

    const QString name = configuration.name();

    qDebug() << "Registering" << name << "with system";

    WSAQUERYSET networkInfo;
    memset(&networkInfo, 0, sizeof(networkInfo));
    networkInfo.dwSize = sizeof(networkInfo);
    networkInfo.lpszServiceInstanceName = (LPWSTR)name.utf16();
    networkInfo.dwNameSpace = NS_NLA;

    if (WSASetService(&networkInfo, RNRSERVICE_REGISTER, 0) == SOCKET_ERROR)
        qDebug() << "WSASetService(RNRSERVICE_REGISTER) returned" << WSAGetLastError();
}

void BearerMonitor::unregisterNetwork()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return;

    QNetworkConfiguration configuration =
        manager.configurationFromIdentifier(item->data(0, Qt::UserRole).toString());

    const QString name = configuration.name();

    qDebug() << "Unregistering" << name << "with system";

    WSAQUERYSET networkInfo;
    memset(&networkInfo, 0, sizeof(networkInfo));
    networkInfo.dwSize = sizeof(networkInfo);
    networkInfo.lpszServiceInstanceName = (LPWSTR)name.utf16();
    networkInfo.dwNameSpace = NS_NLA;

    if (WSASetService(&networkInfo, RNRSERVICE_DELETE, 0) == SOCKET_ERROR)
        qDebug() << "WSASetService(RNRSERVICE_DELETE) returned" << WSAGetLastError();
}
#endif // Q_OS_WIN && !Q_OS_WINRT

void BearerMonitor::showConfigurationFor(QTreeWidgetItem *item)
{
    QString identifier;

    if (item)
        identifier = item->data(0, Qt::UserRole).toString();

    QNetworkConfiguration conf = manager.configurationFromIdentifier(identifier);

    switch (conf.state()) {
    case QNetworkConfiguration::Active:
        configurationState->setText(tr("Active"));
        break;
    case QNetworkConfiguration::Discovered:
        configurationState->setText(tr("Discovered"));
        break;
    case QNetworkConfiguration::Defined:
        configurationState->setText(tr("Defined"));
        break;
    case QNetworkConfiguration::Undefined:
        configurationState->setText(tr("Undefined"));
        break;
    default:
        configurationState->setText(QString());
    }

    switch (conf.type()) {
    case QNetworkConfiguration::InternetAccessPoint:
        configurationType->setText(tr("Internet Access Point"));
        break;
    case QNetworkConfiguration::ServiceNetwork:
        configurationType->setText(tr("Service Network"));
        break;
    case QNetworkConfiguration::UserChoice:
        configurationType->setText(tr("User Choice"));
        break;
    case QNetworkConfiguration::Invalid:
        configurationType->setText(tr("Invalid"));
        break;
    default:
        configurationType->setText(QString());
    }

    switch (conf.purpose()) {
    case QNetworkConfiguration::UnknownPurpose:
        configurationPurpose->setText(tr("Unknown"));
        break;
    case QNetworkConfiguration::PublicPurpose:
        configurationPurpose->setText(tr("Public"));
        break;
    case QNetworkConfiguration::PrivatePurpose:
        configurationPurpose->setText(tr("Private"));
        break;
    case QNetworkConfiguration::ServiceSpecificPurpose:
        configurationPurpose->setText(tr("Service Specific"));
        break;
    default:
        configurationPurpose->setText(QString());
    }

    configurationIdentifier->setText(conf.identifier());

    configurationRoaming->setText(conf.isRoamingAvailable() ? tr("Available") : tr("Not available"));

    configurationChildren->setText(QString::number(conf.children().count()));

    configurationName->setText(conf.name());
}

void BearerMonitor::createSessionFor(QTreeWidgetItem *item)
{
    const QString identifier = item->data(0, Qt::UserRole).toString();

    QNetworkConfiguration conf = manager.configurationFromIdentifier(identifier);

    SessionWidget *session = new SessionWidget(conf);

    tabWidget->addTab(session, conf.name());

#ifndef MAEMO_UI
    sessionGroup->show();
#endif

    sessionWidgets.append(session);
}

void BearerMonitor::createNewSession()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return;

    createSessionFor(item);
}

#ifndef MAEMO_UI
void BearerMonitor::deleteSession()
{
    SessionWidget *session = qobject_cast<SessionWidget *>(tabWidget->currentWidget());
    if (session) {
        sessionWidgets.removeAll(session);

        delete session;

        if (tabWidget->count() == 0)
            sessionGroup->hide();
    }
}
#endif

void BearerMonitor::performScan()
{
    scanButton->hide();
    progressBar->show();
    manager.updateConfigurations();
}
