// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "httpwindow.h"

#include "ui_authenticationdialog.h"

#include <QtWidgets>
#include <QtNetwork>
#include <QUrl>

#include <algorithm>
#include <memory>

#if QT_CONFIG(ssl)
const char defaultUrl[] = "https://www.qt.io/";
#else
const char defaultUrl[] = "http://www.qt.io/";
#endif
const char defaultFileName[] = "index.html";

ProgressDialog::ProgressDialog(const QUrl &url, QWidget *parent)
  : QProgressDialog(parent)
{
    setWindowTitle(tr("Download Progress"));
    setLabelText(tr("Downloading %1.").arg(url.toDisplayString()));
    setMinimum(0);
    setValue(0);
    setMinimumDuration(0);
    setMinimumSize(QSize(400, 75));
}

void ProgressDialog::networkReplyProgress(qint64 bytesRead, qint64 totalBytes)
{
    setMaximum(totalBytes);
    setValue(bytesRead);
}

HttpWindow::HttpWindow(QWidget *parent)
    : QDialog(parent)
    , statusLabel(new QLabel(tr("Please enter the URL of a file you want to download.\n\n"), this))
    , urlLineEdit(new QLineEdit(defaultUrl))
    , downloadButton(new QPushButton(tr("Download")))
    , launchCheckBox(new QCheckBox(tr("Launch file")))
    , defaultFileLineEdit(new QLineEdit(defaultFileName))
    , downloadDirectoryLineEdit(new QLineEdit)
{
    setWindowTitle(tr("HTTP Client"));

    //! [qnam-auth-required-1]
    connect(&qnam, &QNetworkAccessManager::authenticationRequired,
            this, &HttpWindow::slotAuthenticationRequired);
    //! [qnam-auth-required-1]
#if QT_CONFIG(networkproxy)
    connect(&qnam, &QNetworkAccessManager::proxyAuthenticationRequired,
            this, &HttpWindow::slotProxyAuthenticationRequired);
#endif

    QFormLayout *formLayout = new QFormLayout;
    urlLineEdit->setClearButtonEnabled(true);
    connect(urlLineEdit, &QLineEdit::textChanged, this, &HttpWindow::enableDownloadButton);
    formLayout->addRow(tr("&URL:"), urlLineEdit);
    QString downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadDirectory.isEmpty() || !QFileInfo(downloadDirectory).isDir())
        downloadDirectory = QDir::currentPath();
    downloadDirectoryLineEdit->setText(QDir::toNativeSeparators(downloadDirectory));
    formLayout->addRow(tr("&Download directory:"), downloadDirectoryLineEdit);
    formLayout->addRow(tr("Default &file:"), defaultFileLineEdit);
    launchCheckBox->setChecked(true);
    formLayout->addRow(launchCheckBox);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);

    mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    downloadButton->setDefault(true);
    connect(downloadButton, &QAbstractButton::clicked, this, &HttpWindow::downloadFile);
    QPushButton *quitButton = new QPushButton(tr("Quit"));
    quitButton->setAutoDefault(false);
    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);
    mainLayout->addWidget(buttonBox);

    urlLineEdit->setFocus();
}
HttpWindow::~HttpWindow() = default;

void HttpWindow::startRequest(const QUrl &requestedUrl)
{
    url = requestedUrl;
    httpRequestAborted = false;

    //! [qnam-download]
    reply.reset(qnam.get(QNetworkRequest(url)));
    //! [qnam-download]
    //! [connecting-reply-to-slots]
    connect(reply.get(), &QNetworkReply::finished, this, &HttpWindow::httpFinished);
    //! [networkreply-readyread-1]
    connect(reply.get(), &QIODevice::readyRead, this, &HttpWindow::httpReadyRead);
    //! [networkreply-readyread-1]
#if QT_CONFIG(ssl)
    //! [sslerrors-1]
    connect(reply.get(), &QNetworkReply::sslErrors, this, &HttpWindow::sslErrors);
    //! [sslerrors-1]
#endif
    //! [connecting-reply-to-slots]

    ProgressDialog *progressDialog = new ProgressDialog(url, this);
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(progressDialog, &QProgressDialog::canceled, this, &HttpWindow::cancelDownload);
    connect(reply.get(), &QNetworkReply::downloadProgress,
            progressDialog, &ProgressDialog::networkReplyProgress);
    connect(reply.get(), &QNetworkReply::finished, progressDialog, &ProgressDialog::hide);
    progressDialog->show();

    statusLabel->setText(tr("Downloading %1...").arg(url.toString()));
}

void HttpWindow::downloadFile()
{
    const QString urlSpec = urlLineEdit->text().trimmed();
    if (urlSpec.isEmpty())
        return;

    const QUrl newUrl = QUrl::fromUserInput(urlSpec);
    if (!newUrl.isValid()) {
        QMessageBox::information(this, tr("Error"),
                                 tr("Invalid URL: %1: %2").arg(urlSpec, newUrl.errorString()));
        return;
    }

    QString fileName = newUrl.fileName();
    if (fileName.isEmpty())
        fileName = defaultFileLineEdit->text().trimmed();
    if (fileName.isEmpty())
        fileName = defaultFileName;
    QString downloadDirectory = QDir::cleanPath(downloadDirectoryLineEdit->text().trimmed());
    bool useDirectory = !downloadDirectory.isEmpty() && QFileInfo(downloadDirectory).isDir();
    if (useDirectory)
        fileName.prepend(downloadDirectory + '/');

    if (QFile::exists(fileName)) {
        QString alreadyExists = useDirectory
                ? tr("There already exists a file called %1. Overwrite?")
                : tr("There already exists a file called %1 in the current directory. "
                     "Overwrite?");
        QMessageBox::StandardButton response = QMessageBox::question(this,
                tr("Overwrite Existing File"),
                alreadyExists.arg(QDir::toNativeSeparators(fileName)),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (response == QMessageBox::No)
            return;
        QFile::remove(fileName);
    }

    file = openFileForWrite(fileName);
    if (!file)
        return;

    downloadButton->setEnabled(false);

    // schedule the request
    startRequest(newUrl);
}

std::unique_ptr<QFile> HttpWindow::openFileForWrite(const QString &fileName)
{
    std::unique_ptr<QFile> file = std::make_unique<QFile>(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Error"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(QDir::toNativeSeparators(fileName),
                                      file->errorString()));
        return nullptr;
    }
    return file;
}

void HttpWindow::cancelDownload()
{
    statusLabel->setText(tr("Download canceled."));
    httpRequestAborted = true;
    reply->abort();
    downloadButton->setEnabled(true);
}

void HttpWindow::httpFinished()
{
    QFileInfo fi;
    if (file) {
        fi.setFile(file->fileName());
        file->close();
        file.reset();
    }

    //! [networkreply-error-handling-1]
    QNetworkReply::NetworkError error = reply->error();
    const QString &errorString = reply->errorString();
    //! [networkreply-error-handling-1]
    reply.reset();
    //! [networkreply-error-handling-2]
    if (error != QNetworkReply::NoError) {
        QFile::remove(fi.absoluteFilePath());
        // For "request aborted" we handle the label and button in cancelDownload()
        if (!httpRequestAborted) {
            statusLabel->setText(tr("Download failed:\n%1.").arg(errorString));
            downloadButton->setEnabled(true);
        }
        return;
    }
    //! [networkreply-error-handling-2]

    statusLabel->setText(tr("Downloaded %1 bytes to %2\nin\n%3")
                                 .arg(fi.size())
                                 .arg(fi.fileName(), QDir::toNativeSeparators(fi.absolutePath())));
    if (launchCheckBox->isChecked())
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absoluteFilePath()));
    downloadButton->setEnabled(true);
}

//! [networkreply-readyread-2]
void HttpWindow::httpReadyRead()
{
    // This slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file)
        file->write(reply->readAll());
}
//! [networkreply-readyread-2]

void HttpWindow::enableDownloadButton()
{
    downloadButton->setEnabled(!urlLineEdit->text().isEmpty());
}

//! [qnam-auth-required-2]
void HttpWindow::slotAuthenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    QDialog authenticationDialog;
    Ui::Dialog ui;
    ui.setupUi(&authenticationDialog);
    authenticationDialog.adjustSize();
    ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm(), url.host()));

    // Did the URL have information? Fill the UI.
    // This is only relevant if the URL-supplied credentials were wrong
    ui.userEdit->setText(url.userName());
    ui.passwordEdit->setText(url.password());

    if (authenticationDialog.exec() == QDialog::Accepted) {
        authenticator->setUser(ui.userEdit->text());
        authenticator->setPassword(ui.passwordEdit->text());
    }
}
//! [qnam-auth-required-2]

#if QT_CONFIG(ssl)
//! [sslerrors-2]
void HttpWindow::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    if (QMessageBox::warning(this, tr("TLS Errors"),
                             tr("One or more TLS errors has occurred:\n%1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort)
        == QMessageBox::Ignore) {
        reply->ignoreSslErrors();
    }
}
//! [sslerrors-2]
#endif

#if QT_CONFIG(networkproxy)
void HttpWindow::slotProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    QDialog authenticationDialog;
    Ui::Dialog ui;
    ui.setupUi(&authenticationDialog);
    authenticationDialog.adjustSize();
    ui.siteDescription->setText(tr("A network proxy at %1 is requesting credentials for realm: %2")
                                        .arg(proxy.hostName(), authenticator->realm()));

    // If the user passed credentials in the URL to http_proxy or similar they may be available to
    // us. Otherwise this will just leave the fields empty
    ui.userEdit->setText(proxy.user());
    ui.passwordEdit->setText(proxy.password());

    if (authenticationDialog.exec() == QDialog::Accepted) {
        authenticator->setUser(ui.userEdit->text());
        authenticator->setPassword(ui.passwordEdit->text());
    }
}
#endif
