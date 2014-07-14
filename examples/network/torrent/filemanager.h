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

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QBitArray>
#include <QList>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "metainfo.h"

QT_BEGIN_NAMESPACE
class QByteArray;
class QFile;
class QTimerEvent;
QT_END_NAMESPACE

class FileManager : public QThread
{
    Q_OBJECT

public:
    FileManager(QObject *parent = 0);
    virtual ~FileManager();

    inline void setMetaInfo(const MetaInfo &info) { metaInfo = info; }
    inline void setDestinationFolder(const QString &directory) { destinationPath = directory; }

    int read(int pieceIndex, int offset, int length);
    void write(int pieceIndex, int offset, const QByteArray &data);
    void verifyPiece(int pieceIndex);
    inline qint64 totalSize() const { return totalLength; }

    inline int pieceCount() const { return numPieces; }
    int pieceLengthAt(int pieceIndex) const;

    QBitArray completedPieces() const;
    void setCompletedPieces(const QBitArray &pieces);

    QString errorString() const;

public slots:
    void startDataVerification();

signals:
    void dataRead(int id, int pieceIndex, int offset, const QByteArray &data);
    void error();
    void verificationProgress(int percent);
    void verificationDone();
    void pieceVerified(int pieceIndex, bool verified);

protected:
    void run() Q_DECL_OVERRIDE;

private slots:
    bool verifySinglePiece(int pieceIndex);
    void wakeUp();

private:
    bool generateFiles();
    QByteArray readBlock(int pieceIndex, int offset, int length);
    bool writeBlock(int pieceIndex, int offset, const QByteArray &data);
    void verifyFileContents();

    struct WriteRequest {
        int pieceIndex;
        int offset;
        QByteArray data;
    };
    struct ReadRequest {
        int pieceIndex;
        int offset;
        int length;
        int id;
    };

    QString errString;
    QString destinationPath;
    MetaInfo metaInfo;
    QList<QFile *> files;
    QList<QByteArray> sha1s;
    QBitArray verifiedPieces;

    bool newFile;
    int pieceLength;
    qint64 totalLength;
    int numPieces;
    int readId;
    bool startVerification;
    bool quit;
    bool wokeUp;

    QList<WriteRequest> writeRequests;
    QList<ReadRequest> readRequests;
    QList<int> pendingVerificationRequests;
    QList<int> newPendingVerificationRequests;
    QList<qint64> fileSizes;

    mutable QMutex mutex;
    mutable QWaitCondition cond;
};

#endif
