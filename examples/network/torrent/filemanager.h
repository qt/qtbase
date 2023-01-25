// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    FileManager(QObject *parent = nullptr);
    virtual ~FileManager();

    inline void setMetaInfo(const MetaInfo &info) { metaInfo = info; }
    inline void setDestinationFolder(const QString &directory) { destinationPath = directory; }

    qint32 read(qint32 pieceIndex, qint32 offset, qint32 length);
    void write(qint32 pieceIndex, qint32 offset, const QByteArray &data);
    void verifyPiece(qint32 pieceIndex);
    inline qint64 totalSize() const { return totalLength; }

    inline qint32 pieceCount() const { return numPieces; }
    qint32 pieceLengthAt(qint32 pieceIndex) const;

    QBitArray completedPieces() const;
    void setCompletedPieces(const QBitArray &pieces);

    QString errorString() const;

public slots:
    void startDataVerification();

signals:
    void dataRead(qint32 id, qint32 pieceIndex, qint32 offset, const QByteArray &data);
    void error();
    void verificationProgress(int percent);
    void verificationDone();
    void pieceVerified(qint32 pieceIndex, bool verified);

protected:
    void run() override;

private slots:
    bool verifySinglePiece(qint32 pieceIndex);
    void wakeUp();

private:
    bool generateFiles();
    QByteArray readBlock(qint32 pieceIndex, qint32 offset, qint32 length);
    bool writeBlock(qint32 pieceIndex, qint32 offset, const QByteArray &data);
    void verifyFileContents();

    struct WriteRequest {
        qint32 pieceIndex;
        qint32 offset;
        QByteArray data;
    };
    struct ReadRequest {
        qint32 pieceIndex;
        qint32 offset;
        qint32 length;
        qint32 id;
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
