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

#include <QtGui>
#include <QtNetwork>

#include "ftpwindow.h"

FtpWindow::FtpWindow(QWidget *parent)
    : QDialog(parent), ftp(0), networkSession(0)
{
    ftpServerLabel = new QLabel(tr("Ftp &server:"));
    ftpServerLineEdit = new QLineEdit("ftp.qt.nokia.com");
    ftpServerLabel->setBuddy(ftpServerLineEdit);

    statusLabel = new QLabel(tr("Please enter the name of an FTP server."));
#ifdef Q_OS_SYMBIAN
    // Use word wrapping to fit the text on screen
    statusLabel->setWordWrap( true );
#endif

    fileList = new QTreeWidget;
    fileList->setEnabled(false);
    fileList->setRootIsDecorated(false);
    fileList->setHeaderLabels(QStringList() << tr("Name") << tr("Size") << tr("Owner") << tr("Group") << tr("Time"));
    fileList->header()->setStretchLastSection(false);

    connectButton = new QPushButton(tr("Connect"));
    connectButton->setDefault(true);

    cdToParentButton = new QPushButton;
    cdToParentButton->setIcon(QPixmap(":/images/cdtoparent.png"));
    cdToParentButton->setEnabled(false);

    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setEnabled(false);

    quitButton = new QPushButton(tr("Quit"));

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    progressDialog = new QProgressDialog(this);

    connect(fileList, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(processItem(QTreeWidgetItem*,int)));
    connect(fileList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(enableDownloadButton()));
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectOrDisconnect()));
    connect(cdToParentButton, SIGNAL(clicked()), this, SLOT(cdToParent()));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(ftpServerLabel);
    topLayout->addWidget(ftpServerLineEdit);
#ifndef Q_OS_SYMBIAN
    topLayout->addWidget(cdToParentButton);
    topLayout->addWidget(connectButton);
#else
    // Make app better lookin on small screen
    QHBoxLayout *topLayout2 = new QHBoxLayout;
    topLayout2->addWidget(cdToParentButton);
    topLayout2->addWidget(connectButton);
#endif

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
#ifdef Q_OS_SYMBIAN
    // Make app better lookin on small screen
    mainLayout->addLayout(topLayout2);
#endif
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, SIGNAL(opened()), this, SLOT(enableConnectButton()));

        connectButton->setEnabled(false);
        statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    }

    setWindowTitle(tr("FTP"));
}

QSize FtpWindow::sizeHint() const
{
    return QSize(500, 300);
}

//![0]
void FtpWindow::connectOrDisconnect()
{
    if (ftp) {
        ftp->abort();
        ftp->deleteLater();
        ftp = 0;
//![0]
        fileList->setEnabled(false);
        cdToParentButton->setEnabled(false);
        downloadButton->setEnabled(false);
        connectButton->setEnabled(true);
        connectButton->setText(tr("Connect"));
#ifndef QT_NO_CURSOR
        setCursor(Qt::ArrowCursor);
#endif
        statusLabel->setText(tr("Please enter the name of an FTP server."));
        return;
    }

#ifndef QT_NO_CURSOR
    setCursor(Qt::WaitCursor);
#endif

//![1]
    ftp = new QFtp(this);
    connect(ftp, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));
    connect(ftp, SIGNAL(listInfo(QUrlInfo)),
            this, SLOT(addToList(QUrlInfo)));
    connect(ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            this, SLOT(updateDataTransferProgress(qint64,qint64)));

    fileList->clear();
    currentPath.clear();
    isDirectory.clear();
//![1]

//![2]
    QUrl url(ftpServerLineEdit->text());
    if (!url.isValid() || url.scheme().toLower() != QLatin1String("ftp")) {
        ftp->connectToHost(ftpServerLineEdit->text(), 21);
        ftp->login();
    } else {
        ftp->connectToHost(url.host(), url.port(21));

        if (!url.userName().isEmpty())
            ftp->login(QUrl::fromPercentEncoding(url.userName().toLatin1()), url.password());
        else
            ftp->login();
        if (!url.path().isEmpty())
            ftp->cd(url.path());
    }
//![2]

    fileList->setEnabled(true);
    connectButton->setEnabled(false);
    connectButton->setText(tr("Disconnect"));
    statusLabel->setText(tr("Connecting to FTP server %1...")
                         .arg(ftpServerLineEdit->text()));
}

//![3]
void FtpWindow::downloadFile()
{
    QString fileName = fileList->currentItem()->text(0);
//![3]
//
    if (QFile::exists(fileName)) {
        QMessageBox::information(this, tr("FTP"),
                                 tr("There already exists a file called %1 in "
                                    "the current directory.")
                                 .arg(fileName));
        return;
    }

//![4]
    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("FTP"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        delete file;
        return;
    }

    ftp->get(fileList->currentItem()->text(0), file);

    progressDialog->setLabelText(tr("Downloading %1...").arg(fileName));
    downloadButton->setEnabled(false);
    progressDialog->exec();
}
//![4]

//![5]
void FtpWindow::cancelDownload()
{
    ftp->abort();
}
//![5]

//![6]
void FtpWindow::ftpCommandFinished(int, bool error)
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif

    if (ftp->currentCommand() == QFtp::ConnectToHost) {
        if (error) {
            QMessageBox::information(this, tr("FTP"),
                                     tr("Unable to connect to the FTP server "
                                        "at %1. Please check that the host "
                                        "name is correct.")
                                     .arg(ftpServerLineEdit->text()));
            connectOrDisconnect();
            return;
        }
        statusLabel->setText(tr("Logged onto %1.")
                             .arg(ftpServerLineEdit->text()));
        fileList->setFocus();
        downloadButton->setDefault(true);
        connectButton->setEnabled(true);
        return;
    }
//![6]

//![7]
    if (ftp->currentCommand() == QFtp::Login)
        ftp->list();
//![7]

//![8]
    if (ftp->currentCommand() == QFtp::Get) {
        if (error) {
            statusLabel->setText(tr("Canceled download of %1.")
                                 .arg(file->fileName()));
            file->close();
            file->remove();
        } else {
            statusLabel->setText(tr("Downloaded %1 to current directory.")
                                 .arg(file->fileName()));
            file->close();
        }
        delete file;
        enableDownloadButton();
        progressDialog->hide();
//![8]
//![9]
    } else if (ftp->currentCommand() == QFtp::List) {
        if (isDirectory.isEmpty()) {
            fileList->addTopLevelItem(new QTreeWidgetItem(QStringList() << tr("<empty>")));
            fileList->setEnabled(false);
        }
    }
//![9]
}

//![10]
void FtpWindow::addToList(const QUrlInfo &urlInfo)
{
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, urlInfo.name());
    item->setText(1, QString::number(urlInfo.size()));
    item->setText(2, urlInfo.owner());
    item->setText(3, urlInfo.group());
    item->setText(4, urlInfo.lastModified().toString("MMM dd yyyy"));

    QPixmap pixmap(urlInfo.isDir() ? ":/images/dir.png" : ":/images/file.png");
    item->setIcon(0, pixmap);

    isDirectory[urlInfo.name()] = urlInfo.isDir();
    fileList->addTopLevelItem(item);
    if (!fileList->currentItem()) {
        fileList->setCurrentItem(fileList->topLevelItem(0));
        fileList->setEnabled(true);
    }
}
//![10]

//![11]
void FtpWindow::processItem(QTreeWidgetItem *item, int /*column*/)
{
    QString name = item->text(0);
    if (isDirectory.value(name)) {
        fileList->clear();
        isDirectory.clear();
        currentPath += '/';
        currentPath += name;
        ftp->cd(name);
        ftp->list();
        cdToParentButton->setEnabled(true);
#ifndef QT_NO_CURSOR
        setCursor(Qt::WaitCursor);
#endif
        return;
    }
}
//![11]

//![12]
void FtpWindow::cdToParent()
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::WaitCursor);
#endif
    fileList->clear();
    isDirectory.clear();
    currentPath = currentPath.left(currentPath.lastIndexOf('/'));
    if (currentPath.isEmpty()) {
        cdToParentButton->setEnabled(false);
        ftp->cd("/");
    } else {
        ftp->cd(currentPath);
    }
    ftp->list();
}
//![12]

//![13]
void FtpWindow::updateDataTransferProgress(qint64 readBytes,
                                           qint64 totalBytes)
{
    progressDialog->setMaximum(totalBytes);
    progressDialog->setValue(readBytes);
}
//![13]

//![14]
void FtpWindow::enableDownloadButton()
{
    QTreeWidgetItem *current = fileList->currentItem();
    if (current) {
        QString currentFile = current->text(0);
        downloadButton->setEnabled(!isDirectory.value(currentFile));
    } else {
        downloadButton->setEnabled(false);
    }
}
//![14]

void FtpWindow::enableConnectButton()
{
    // Save the used configuration
    QNetworkConfiguration config = networkSession->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice)
        id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    else
        id = config.identifier();

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    connectButton->setEnabled(networkSession->isOpen());
    statusLabel->setText(tr("Please enter the name of an FTP server."));
}

