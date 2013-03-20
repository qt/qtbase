/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bearerex.h"
#include "datatransferer.h"

#include <QtNetwork>
#include <QtWidgets>

BearerEx::BearerEx(QWidget* parent)
     : QMainWindow(parent)
{
    setupUi(this);

    createMenus();

    connect(&m_NetworkConfigurationManager, SIGNAL(updateCompleted()), this, SLOT(configurationsUpdateCompleted()));
    connect(&m_NetworkConfigurationManager, SIGNAL(configurationAdded(QNetworkConfiguration)),
            this, SLOT(configurationAdded(QNetworkConfiguration)));
    connect(&m_NetworkConfigurationManager, SIGNAL(configurationRemoved(QNetworkConfiguration)),
            this, SLOT(configurationRemoved(QNetworkConfiguration)));
    connect(&m_NetworkConfigurationManager, SIGNAL(onlineStateChanged(bool)),
            this, SLOT(onlineStateChanged(bool)));
    connect(&m_NetworkConfigurationManager, SIGNAL(configurationChanged(QNetworkConfiguration)),
            this, SLOT(configurationChanged(QNetworkConfiguration)));
    showConfigurations();
}

void BearerEx::createMenus()
{
    QAction* act1 = new QAction(tr("Show Details"), this);
    menuBar()->addAction(act1);
    connect(act1, SIGNAL(triggered()), this, SLOT(on_showDetailsButton_clicked()));

    QAction* exitAct = new QAction(tr("Exit"), this);
    menuBar()->addAction(exitAct);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
}

void BearerEx::showConfigurations()
{
    listWidget->clear();
    QListWidgetItem* listItem;

    QNetworkConfiguration defaultConfig = m_NetworkConfigurationManager.defaultConfiguration();
    if (defaultConfig.type() == QNetworkConfiguration::UserChoice) {
        listItem = new QListWidgetItem();
        QFont font = listItem->font();
        font.setBold(true);
        font.setUnderline(true);
        listItem->setFont(font);
        listItem->setText("       UserChoice");
        listItem->setData(Qt::UserRole, QVariant::fromValue(defaultConfig));
        listWidget->addItem(listItem);
    }

    QList<QNetworkConfiguration> configurations = m_NetworkConfigurationManager.allConfigurations();
    for (int i=0; i<configurations.count(); i++)
    {
        listItem = new QListWidgetItem();
        QString text;
        if (configurations[i].type() == QNetworkConfiguration::InternetAccessPoint) {
            text.append("(IAP,");
        } else if (configurations[i].type() == QNetworkConfiguration::ServiceNetwork) {
            text.append("(SNAP,");
        }

        if ((configurations[i].state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
            text.append("Act) ");
        } else if ((configurations[i].state() & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered) {
            text.append("Disc) ");
        } else {
            text.append("Def) ");
        }
        text.append(configurations[i].name());

        if (defaultConfig.isValid() && defaultConfig == configurations[i]) {
            QFont font = listItem->font();
            font.setBold(true);
            font.setUnderline(true);
            listItem->setFont(font);
        }
        listItem->setText(text);
        listItem->setData(Qt::UserRole, QVariant::fromValue(configurations[i]));
        listWidget->addItem(listItem);
    }
}

void BearerEx::on_updateConfigurationsButton_clicked()
{
    m_NetworkConfigurationManager.updateConfigurations();
}

void BearerEx::on_updateListButton_clicked()
{
    showConfigurations();
}

void BearerEx::on_showDetailsButton_clicked()
{
    QListWidgetItem* item = listWidget->currentItem();
    if (!item) {
        return;
    }

    QNetworkConfiguration networkConfiguration = qvariant_cast<QNetworkConfiguration>(item->data(Qt::UserRole));
	DetailedInfoDialog infoDialog(&networkConfiguration,this);
	infoDialog.exec();
}

void BearerEx::on_createSessionButton_clicked()
{
    QListWidgetItem* item = listWidget->currentItem();
    if (!item) {
        return;
    }
    QNetworkConfiguration networkConfiguration = qvariant_cast<QNetworkConfiguration>(item->data(Qt::UserRole));
    int newTabIndex = mainTabWidget->count();
    SessionTab* newTab = new SessionTab(&networkConfiguration,&m_NetworkConfigurationManager,eventListWidget,newTabIndex-1);
    QString label = QString("S")+QString::number(newTabIndex-1);
    mainTabWidget->insertTab(newTabIndex,newTab,label);
    mainTabWidget->setCurrentIndex(newTabIndex);
}

void BearerEx::on_clearEventListButton_clicked()
{
    eventListWidget->clear();
}

void BearerEx::configurationAdded(const QNetworkConfiguration& config)
{
    QListWidgetItem* listItem = new QListWidgetItem();
    listItem->setText(QString("Added: ")+config.name());
    eventListWidget->addItem(listItem);
}

void BearerEx::configurationRemoved(const QNetworkConfiguration& config)
{
    QListWidgetItem* listItem = new QListWidgetItem();
    listItem->setText(QString("Removed: ")+config.name());
    eventListWidget->addItem(listItem);
}

void BearerEx::onlineStateChanged(bool isOnline)
{
    QListWidgetItem* listItem = new QListWidgetItem();
    QFont font = listItem->font();
    font.setBold(true);
    listItem->setFont(font);
    if (isOnline) {
        listItem->setText(QString("> Online"));
    } else {
        listItem->setText(QString("< Offline"));
    }
    eventListWidget->addItem(listItem);
}

void BearerEx::configurationChanged(const QNetworkConfiguration & config)
{
    QListWidgetItem* listItem = new QListWidgetItem();
    QString state;
    switch (config.state())
    {
        case QNetworkConfiguration::Undefined:
            state = "Undef : ";
            break;
        case QNetworkConfiguration::Defined:
            state = "Def : ";
            break;
        case QNetworkConfiguration::Discovered:
            state = "Disc : ";
            break;
        case QNetworkConfiguration::Active:
            state = "Act : ";
            break;
    }
    listItem->setText(state+config.name());
    eventListWidget->addItem(listItem);
}

void BearerEx::configurationsUpdateCompleted()
{
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.setText("Configurations update completed.");
    msgBox.exec();
}

DetailedInfoDialog::DetailedInfoDialog(QNetworkConfiguration* apNetworkConfiguration, QWidget * parent)
    : QDialog(parent)
{
    setupUi(this);

    tableWidget->setColumnCount(2);
    int rowCount = 2;

    if (apNetworkConfiguration->type() == QNetworkConfiguration::ServiceNetwork) {
        rowCount = rowCount + apNetworkConfiguration->children().count();
    }

	tableWidget->setRowCount(rowCount);
	tableWidget->setColumnWidth(1,250);
	tableWidget->setItem(0, 0, new QTableWidgetItem(tr("Name")));
	tableWidget->setItem(0, 1, new QTableWidgetItem(apNetworkConfiguration->name()));
	tableWidget->setItem(1, 0, new QTableWidgetItem(tr("Id")));
	tableWidget->setItem(1, 1, new QTableWidgetItem(apNetworkConfiguration->identifier()));
    if (apNetworkConfiguration->type() == QNetworkConfiguration::ServiceNetwork) {
        for (int i=0; i<apNetworkConfiguration->children().count(); i++) {
            tableWidget->setItem(i+2, 0, new QTableWidgetItem(QString("IAP")+QString::number(i+1)));
            tableWidget->setItem(i+2, 1, new QTableWidgetItem(apNetworkConfiguration->children()[i].name()));
        }
    }

    tableWidget->setFocusPolicy(Qt::NoFocus);
}

SessionTab::SessionTab(QNetworkConfiguration* apNetworkConfiguration,
                       QNetworkConfigurationManager* configManager,
                       QListWidget* eventListWidget,
                       int index,
                       BearerEx * parent)
    : QWidget(parent), m_dataTransferer(0), m_eventListWidget(eventListWidget),
     m_index(index), m_alrEnabled (false)
{
    setupUi(this);

    m_ConfigManager = configManager;
    m_NetworkSession = new QNetworkSession(*apNetworkConfiguration);

    // Update initial Session state to UI
    newState(m_NetworkSession->state());

    connect(m_NetworkSession, SIGNAL(newConfigurationActivated()), this, SLOT(newConfigurationActivated()));
    connect(m_NetworkSession, SIGNAL(stateChanged(QNetworkSession::State)),
            this, SLOT(stateChanged(QNetworkSession::State)));
    connect(m_NetworkSession, SIGNAL(opened()), this, SLOT(opened()));
    connect(m_NetworkSession, SIGNAL(closed()), this, SLOT(closed()));
    connect(m_NetworkSession, SIGNAL(error(QNetworkSession::SessionError)), this, SLOT(error(QNetworkSession::SessionError)));

    if (apNetworkConfiguration->type() == QNetworkConfiguration::InternetAccessPoint) {
        snapLabel->hide();
        snapLineEdit->hide();
        alrButton->hide();
        iapLineEdit->setText(apNetworkConfiguration->name()+" ("+apNetworkConfiguration->identifier()+")");
    } else if (apNetworkConfiguration->type() == QNetworkConfiguration::ServiceNetwork) {
        snapLineEdit->setText(apNetworkConfiguration->name()+" ("+apNetworkConfiguration->identifier()+")");
    }
    bearerLineEdit->setText(apNetworkConfiguration->bearerTypeName());
    sentRecDataLineEdit->setText(QString::number(m_NetworkSession->bytesWritten())+
                                 QString(" / ")+
                                 QString::number(m_NetworkSession->bytesReceived()));
    snapLineEdit->setFocusPolicy(Qt::NoFocus);
    iapLineEdit->setFocusPolicy(Qt::NoFocus);
    bearerLineEdit->setFocusPolicy(Qt::NoFocus);
    sentRecDataLineEdit->setFocusPolicy(Qt::NoFocus);
    stateLineEdit->setFocusPolicy(Qt::NoFocus);
}

SessionTab::~SessionTab()
{
    delete m_NetworkSession; m_NetworkSession = 0;
    delete m_dataTransferer; m_dataTransferer = 0;
}

void SessionTab::on_createQNetworkAccessManagerButton_clicked()
{
    if (m_dataTransferer) {
        disconnect(m_dataTransferer, 0, 0, 0);
        delete m_dataTransferer;
        m_dataTransferer = 0;
    }
    // Create new object according to current selection
    QString type(comboBox->currentText());
    if (type == "QNAM") {
        m_dataTransferer = new DataTransfererQNam(this);
    } else if (type == "QTcpSocket") {
        m_dataTransferer = new DataTransfererQTcp(this);
    } else {
        qDebug("BearerEx Warning, unknown data transfer object requested, not creating anything.");
        return;
    }
    createQNetworkAccessManagerButton->setText("Recreate");
    connect(m_dataTransferer, SIGNAL(finished(quint32,qint64,QString)), this, SLOT(finished(quint32,qint64,QString)));
}

void SessionTab::on_sendRequestButton_clicked()
{
    if (m_dataTransferer) {
        if (!m_dataTransferer->transferData()) {
            QMessageBox msgBox;
            msgBox.setStandardButtons(QMessageBox::Close);
            msgBox.setText("Data transfer not started. \nVery likely data transfer ongoing.");
            msgBox.exec();
        }
    } else {
        QMessageBox msgBox;
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.setText("Data object not created.\nCreate data object first.");
        msgBox.exec();
    }
}

void SessionTab::on_openSessionButton_clicked()
{
    m_NetworkSession->open();
    if (m_NetworkSession->isOpen()) {
        newState(m_NetworkSession->state());
    }
}

void SessionTab::on_closeSessionButton_clicked()
{
    m_NetworkSession->close();
    if (!m_NetworkSession->isOpen()) {
        newState(m_NetworkSession->state());
    }
}

void SessionTab::on_stopConnectionButton_clicked()
{
    m_NetworkSession->stop();
}

void SessionTab::on_alrButton_clicked()
{
    if (!m_alrEnabled) {
        connect(m_NetworkSession, SIGNAL(preferredConfigurationChanged(QNetworkConfiguration,bool)),
                this, SLOT(preferredConfigurationChanged(QNetworkConfiguration,bool)));
        alrButton->setText("Disable ALR");
        m_alrEnabled = true;
    } else {
        disconnect(m_NetworkSession, SIGNAL(preferredConfigurationChanged(QNetworkConfiguration,bool)), 0, 0);
        alrButton->setText("Enable ALR");
        m_alrEnabled = false;
    }
}

void SessionTab::on_deleteSessionButton_clicked()
{
    setWindowTitle("Bearer Example");
    delete this;
}

void SessionTab::newConfigurationActivated()
{
    QMessageBox msgBox;
    msgBox.setText("New configuration activated.");
    msgBox.setInformativeText("Do you want to accept new configuration?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if (msgBox.exec() == QMessageBox::Yes) {
        m_NetworkSession->accept();
        iapLineEdit->setText(m_config.name()+" ("+m_config.identifier()+")");
    } else {
        m_NetworkSession->reject();
    }
}

void SessionTab::preferredConfigurationChanged(const QNetworkConfiguration& config, bool /*isSeamless*/)
{
    m_config =  config;
    QMessageBox msgBox;
    msgBox.setText("Roaming to new configuration.");
    msgBox.setInformativeText("Do you want to migrate to "+config.name()+"?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if (msgBox.exec() == QMessageBox::Yes) {
        m_NetworkSession->migrate();
    } else {
        m_NetworkSession->ignore();
    }
}

void SessionTab::opened()
{
    QListWidgetItem* listItem = new QListWidgetItem();
    QFont font = listItem->font();
    font.setBold(true);
    listItem->setFont(font);
    listItem->setText(QString("S")+QString::number(m_index)+QString(" - ")+QString("Opened"));
    m_eventListWidget->addItem(listItem);

    QVariant identifier = m_NetworkSession->sessionProperty("ActiveConfiguration");
    if (!identifier.isNull()) {
        QString configId = identifier.toString();
        QNetworkConfiguration config = m_ConfigManager->configurationFromIdentifier(configId);
        if (config.isValid()) {
            iapLineEdit->setText(config.name()+" ("+config.identifier()+")");
        }
    }
    newState(m_NetworkSession->state()); // Update the "(open)"

    if (m_NetworkSession->configuration().type() == QNetworkConfiguration::UserChoice) {
        QVariant identifier = m_NetworkSession->sessionProperty("UserChoiceConfiguration");
        if (!identifier.isNull()) {
            QString configId = identifier.toString();
            QNetworkConfiguration config = m_ConfigManager->configurationFromIdentifier(configId);
            if (config.isValid() && (config.type() == QNetworkConfiguration::ServiceNetwork)) {
                snapLineEdit->setText(config.name());
            }
        }
    }
}

void SessionTab::closed()
{
    QListWidgetItem* listItem = new QListWidgetItem();
    QFont font = listItem->font();
    font.setBold(true);
    listItem->setFont(font);
    listItem->setText(QString("S")+QString::number(m_index)+QString(" - ")+QString("Closed"));
    m_eventListWidget->addItem(listItem);
}

QString SessionTab::stateString(QNetworkSession::State state)
{
    QString stateString;
    switch (state)
    {
        case QNetworkSession::Invalid:
            stateString = "Invalid";
            break;
        case QNetworkSession::NotAvailable:
            stateString = "NotAvailable";
            break;
        case QNetworkSession::Connecting:
            stateString = "Connecting";
            break;
        case QNetworkSession::Connected:
            stateString = "Connected";
            break;
        case QNetworkSession::Closing:
            stateString = "Closing";
            break;
        case QNetworkSession::Disconnected:
            stateString = "Disconnected";
            break;
        case QNetworkSession::Roaming:
            stateString = "Roaming";
            break;
    }
    return stateString;
}

void SessionTab::on_dataObjectChanged(const QString &newObjectType)
{
    qDebug() << "BearerEx SessionTab dataObjectChanged to: " << newObjectType;
    if (m_dataTransferer) {
        disconnect(m_dataTransferer, 0, 0, 0);
        delete m_dataTransferer; m_dataTransferer = 0;
        qDebug() << "BearerEx SessionTab, previous data object deleted.";
    }
    createQNetworkAccessManagerButton->setText("Create");
}


void SessionTab::stateChanged(QNetworkSession::State state)
{
    newState(state);

    QListWidgetItem* listItem = new QListWidgetItem();
    listItem->setText(QString("S")+QString::number(m_index)+QString(" - ")+stateString(state));
    m_eventListWidget->addItem(listItem);
}

void SessionTab::newState(QNetworkSession::State state)
{
    QVariant identifier = m_NetworkSession->sessionProperty("ActiveConfiguration");
    if (state == QNetworkSession::Connected && !identifier.isNull()) {
        QString configId = identifier.toString();
        QNetworkConfiguration config = m_ConfigManager->configurationFromIdentifier(configId);
        if (config.isValid()) {
            iapLineEdit->setText(config.name()+" ("+config.identifier()+")");
            bearerLineEdit->setText(config.bearerTypeName());
        }
    } else {
        bearerLineEdit->setText(m_NetworkSession->configuration().bearerTypeName());
    }

    QString active;
    if (m_NetworkSession->isOpen()) {
        active = " (open)";
    }
    stateLineEdit->setText(stateString(state)+active);
}

void SessionTab::error(QNetworkSession::SessionError error)
{
    QListWidgetItem* listItem = new QListWidgetItem();
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Close);

    QString errorString;
    switch (error)
    {
        case QNetworkSession::UnknownSessionError:
            errorString = "UnknownSessionError";
            break;
        case QNetworkSession::SessionAbortedError:
            errorString = "SessionAbortedError";
            break;
        case QNetworkSession::RoamingError:
            errorString = "RoamingError";
            break;
        case QNetworkSession::OperationNotSupportedError:
            errorString = "OperationNotSupportedError";
            break;
        case QNetworkSession::InvalidConfigurationError:
            errorString = "InvalidConfigurationError";
            break;
    }
    listItem->setText(QString("S")+QString::number(m_index)+QString(" - ")+errorString);
    m_eventListWidget->addItem(listItem);

    msgBox.setText(errorString);
    msgBox.exec();
}

void SessionTab::finished(quint32 errorCode, qint64 dataReceived, QString errorType)
{
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.setText(QString("Data transfer completed. \nError code: ") +
                   QString::number(int(errorCode)) +
                   "\nError type: " + errorType +
                   "\nBytes received: " +
                   QString::number(dataReceived));
    msgBox.exec();
    // Check if the networksession still exists - it may have gone after returning from
    // the modal dialog (in the case that app has been closed, and deleting QHttp will
    // trigger the done() invocation).
    if (m_NetworkSession) {
        sentRecDataLineEdit->setText(QString::number(m_NetworkSession->bytesWritten())+
                                     QString(" / ")+
                                     QString::number(m_NetworkSession->bytesReceived()));
    } else {
        sentRecDataLineEdit->setText("Data amounts not available.");
    }
}

// End of file

