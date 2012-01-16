/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "sessionwidget.h"
#include "qnetworkconfigmanager.h"

SessionWidget::SessionWidget(const QNetworkConfiguration &config, QWidget *parent)
:   QWidget(parent), statsTimer(-1)
{
    setupUi(this);

#ifdef QT_NO_NETWORKINTERFACE
    interfaceName->setVisible(false);
    interfaceNameLabel->setVisible(false);
    interfaceGuid->setVisible(false);
    interfaceGuidLabel->setVisible(false);
#endif

    session = new QNetworkSession(config, this);

    connect(session, SIGNAL(stateChanged(QNetworkSession::State)),
            this, SLOT(updateSession()));
    connect(session, SIGNAL(error(QNetworkSession::SessionError)),
            this, SLOT(updateSessionError(QNetworkSession::SessionError)));

    updateSession();

    sessionId->setText(QString("0x%1").arg(qulonglong(session), 8, 16, QChar('0')));

    configuration->setText(session->configuration().name());

    connect(openSessionButton, SIGNAL(clicked()),
            this, SLOT(openSession()));
    connect(openSyncSessionButton, SIGNAL(clicked()),
            this, SLOT(openSyncSession()));
    connect(closeSessionButton, SIGNAL(clicked()),
            this, SLOT(closeSession()));
    connect(stopSessionButton, SIGNAL(clicked()),
            this, SLOT(stopSession()));
#ifdef MAEMO_UI
    connect(deleteSessionButton, SIGNAL(clicked()),
            this, SLOT(deleteSession()));
#endif
}

SessionWidget::~SessionWidget()
{
    delete session;
}

void SessionWidget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == statsTimer) {
        rxData->setText(QString::number(session->bytesReceived()));
        txData->setText(QString::number(session->bytesWritten()));
        activeTime->setText(QString::number(session->activeTime()));
    }
}

#ifdef MAEMO_UI
void SessionWidget::deleteSession()
{
    delete this;
}
#endif

void SessionWidget::updateSession()
{
    updateSessionState(session->state());

    if (session->state() == QNetworkSession::Connected)
        statsTimer = startTimer(1000);
    else if (statsTimer != -1)
        killTimer(statsTimer);

    if (session->configuration().type() == QNetworkConfiguration::InternetAccessPoint)
        bearer->setText(session->configuration().bearerTypeName());
    else {
        QNetworkConfigurationManager mgr;
        QNetworkConfiguration c = mgr.configurationFromIdentifier(session->sessionProperty("ActiveConfiguration").toString());
        bearer->setText(c.bearerTypeName());
    }

#ifndef QT_NO_NETWORKINTERFACE
    interfaceName->setText(session->interface().humanReadableName());
    interfaceGuid->setText(session->interface().name());
#endif
}

void SessionWidget::openSession()
{
    clearError();
    session->open();
    updateSession();
}

void SessionWidget::openSyncSession()
{
    clearError();
    session->open();
    session->waitForOpened();
    updateSession();
}

void SessionWidget::closeSession()
{
    clearError();
    session->close();
    updateSession();
}

void SessionWidget::stopSession()
{
    clearError();
    session->stop();
    updateSession();
}

void SessionWidget::updateSessionState(QNetworkSession::State state)
{
    QString s = tr("%1 (%2)");

    switch (state) {
    case QNetworkSession::Invalid:
        s = s.arg(tr("Invalid"));
        break;
    case QNetworkSession::NotAvailable:
        s = s.arg(tr("Not Available"));
        break;
    case QNetworkSession::Connecting:
        s = s.arg(tr("Connecting"));
        break;
    case QNetworkSession::Connected:
        s = s.arg(tr("Connected"));
        break;
    case QNetworkSession::Closing:
        s = s.arg(tr("Closing"));
        break;
    case QNetworkSession::Disconnected:
        s = s.arg(tr("Disconnected"));
        break;
    case QNetworkSession::Roaming:
        s = s.arg(tr("Roaming"));
        break;
    default:
        s = s.arg(tr("Unknown"));
    }

    if (session->isOpen())
        s = s.arg(tr("Open"));
    else
        s = s.arg(tr("Closed"));

    sessionState->setText(s);
}

void SessionWidget::updateSessionError(QNetworkSession::SessionError error)
{
    lastError->setText(QString::number(error));
    errorString->setText(session->errorString());
}

void SessionWidget::clearError()
{
    lastError->clear();
    errorString->clear();
}
