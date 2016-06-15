/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QStringList>
#include <QMainWindow>

#include "torrentclient.h"

QT_BEGIN_NAMESPACE
class QAction;
class QCloseEvent;
class QLabel;
class QProgressDialog;
class QSlider;
QT_END_NAMESPACE
class TorrentView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

    QSize sizeHint() const override;
    const TorrentClient *clientForRow(int row) const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void loadSettings();
    void saveSettings();

    bool addTorrent();
    void removeTorrent();
    void pauseTorrent();
    void moveTorrentUp();
    void moveTorrentDown();

    void torrentStopped();
    void torrentError(TorrentClient::Error error);

    void updateState(TorrentClient::State state);
    void updatePeerInfo();
    void updateProgress(int percent);
    void updateDownloadRate(int bytesPerSecond);
    void updateUploadRate(int bytesPerSecond);

    void setUploadLimit(int bytes);
    void setDownloadLimit(int bytes);

    void about();
    void setActionsEnabled();
    void acceptFileDrop(const QString &fileName);

private:
    int rowOfClient(TorrentClient *client) const;
    bool addTorrent(const QString &fileName, const QString &destinationFolder,
                    const QByteArray &resumeState = QByteArray());

    TorrentView *torrentView;
    QAction *pauseTorrentAction;
    QAction *removeTorrentAction;
    QAction *upActionTool;
    QAction *downActionTool;
    QSlider *uploadLimitSlider;
    QSlider *downloadLimitSlider;
    QLabel *uploadLimitLabel;
    QLabel *downloadLimitLabel;

    int uploadLimit;
    int downloadLimit;

    struct Job {
        TorrentClient *client;
        QString torrentFileName;
        QString destinationDirectory;
    };
    QList<Job> jobs;
    int jobsStopped;
    int jobsToStop;

    QString lastDirectory;
    QProgressDialog *quitDialog;

    bool saveChanges;
};

#endif
