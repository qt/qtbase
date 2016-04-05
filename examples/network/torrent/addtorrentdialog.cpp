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

#include "addtorrentdialog.h"
#include "metainfo.h"

#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMetaObject>

static QString stringNumber(qint64 number)
{
    if (number > (1024 * 1024 * 1024))
        return QString::asprintf("%.2fGB", number / (1024.0 * 1024.0 * 1024.0));
    else if (number > (1024 * 1024))
        return QString::asprintf("%.2fMB", number / (1024.0 * 1024.0));
    else if (number > (1024))
        return QString::asprintf("%.2fKB", number / (1024.0));
    else
        return QString::asprintf("%d bytes", int(number));
}

AddTorrentDialog::AddTorrentDialog(QWidget *parent)
  : QDialog(parent, Qt::Sheet)
{
    ui.setupUi(this);

    connect(ui.browseTorrents, SIGNAL(clicked()),
            this, SLOT(selectTorrent()));
    connect(ui.browseDestination, SIGNAL(clicked()),
            this, SLOT(selectDestination()));
    connect(ui.torrentFile, SIGNAL(textChanged(QString)),
            this, SLOT(setTorrent(QString)));

    ui.destinationFolder->setText(destinationDirectory = QDir::current().path());
    ui.torrentFile->setFocus();
}

void AddTorrentDialog::selectTorrent()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a torrent file"),
                                                    lastDirectory,
                                                    tr("Torrents (*.torrent);; All files (*.*)"));
    if (fileName.isEmpty())
        return;
    lastDirectory = QFileInfo(fileName).absolutePath();
    setTorrent(fileName);
}

void AddTorrentDialog::selectDestination()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a destination directory"),
                                                    lastDestinationDirectory);
    if (dir.isEmpty())
        return;
    lastDestinationDirectory = destinationDirectory = dir;
    ui.destinationFolder->setText(destinationDirectory);
    enableOkButton();
}

void AddTorrentDialog::enableOkButton()
{
    ui.okButton->setEnabled(!ui.destinationFolder->text().isEmpty()
                            && !ui.torrentFile->text().isEmpty());
}

void AddTorrentDialog::setTorrent(const QString &torrentFile)
{
    if (torrentFile.isEmpty()) {
        enableOkButton();
        return;
    }

    ui.torrentFile->setText(torrentFile);
    if (!torrentFile.isEmpty())
        lastDirectory = QFileInfo(torrentFile).absolutePath();

    if (lastDestinationDirectory.isEmpty())
        lastDestinationDirectory = lastDirectory;

    MetaInfo metaInfo;
    QFile torrent(torrentFile);
    if (!torrent.open(QFile::ReadOnly) || !metaInfo.parse(torrent.readAll())) {
        enableOkButton();
        return;
    }

    ui.torrentFile->setText(torrentFile);
    ui.announceUrl->setText(metaInfo.announceUrl());
    if (metaInfo.comment().isEmpty())
        ui.commentLabel->setText("<unknown>");
    else
        ui.commentLabel->setText(metaInfo.comment());
    if (metaInfo.createdBy().isEmpty())
        ui.creatorLabel->setText("<unknown>");
    else
        ui.creatorLabel->setText(metaInfo.createdBy());
    ui.sizeLabel->setText(stringNumber(metaInfo.totalSize()));
    if (metaInfo.fileForm() == MetaInfo::SingleFileForm) {
        ui.torrentContents->setHtml(metaInfo.singleFile().name);
    } else {
        QString html;
        foreach (MetaInfoMultiFile file, metaInfo.multiFiles()) {
            QString name = metaInfo.name();
            if (!name.isEmpty()) {
                html += name;
                if (!name.endsWith('/'))
                    html += '/';
            }
            html += file.path + "<br>";
        }
        ui.torrentContents->setHtml(html);
    }

    QFileInfo info(torrentFile);
    ui.destinationFolder->setText(info.absolutePath());

    enableOkButton();
}

QString AddTorrentDialog::torrentFileName() const
{
    return ui.torrentFile->text();
}

QString AddTorrentDialog::destinationFolder() const
{
    return ui.destinationFolder->text();
}
