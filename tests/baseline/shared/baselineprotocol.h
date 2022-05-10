// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BASELINEPROTOCOL_H
#define BASELINEPROTOCOL_H

#include <QDataStream>
#include <QTcpSocket>
#include <QImage>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QStringList>

#define QLS QLatin1String
#define QLC QLatin1Char

#define FileFormat "png"

extern const QString PI_Project;
extern const QString PI_ProjectImageKeys;
extern const QString PI_TestCase;
extern const QString PI_HostName;
extern const QString PI_HostAddress;
extern const QString PI_OSName;
extern const QString PI_OSVersion;
extern const QString PI_QtVersion;
extern const QString PI_QtBuildMode;
extern const QString PI_GitCommit;
extern const QString PI_GitBranch;

class PlatformInfo : public QMap<QString, QString>
{
public:
    static PlatformInfo localHostInfo();

    void addOverride(const QString& key, const QString& value);
    QStringList overrides() const;
    bool isAdHocRun() const;
    void setAdHocRun(bool isAdHoc);

private:
    QStringList orides;
    bool adHoc = true;
    friend QDataStream & operator<< (QDataStream &stream, const PlatformInfo &pi);
    friend QDataStream & operator>> (QDataStream &stream, PlatformInfo& pi);
};
QDataStream & operator<< (QDataStream &stream, const PlatformInfo &pi);
QDataStream & operator>> (QDataStream &stream, PlatformInfo& pi);


struct ImageItem
{
    static quint64 computeChecksum(const QImage& image);

    enum ItemStatus {
        Ok = 0,
        BaselineNotFound = 1,
        IgnoreItem = 2,
        Mismatch = 3,
        FuzzyMatch = 4,
        Error = 5
    };

    QString testFunction;
    QString itemName;
    ItemStatus status = Ok;
    QImage image;
    QList<quint64> imageChecksums;
    quint16 itemChecksum = 0;
    QByteArray misc;

    void writeImageToStream(QDataStream &stream) const;
    void readImageFromStream(QDataStream &stream);
};
QDataStream & operator<< (QDataStream &stream, const ImageItem &ii);
QDataStream & operator>> (QDataStream &stream, ImageItem& ii);

Q_DECLARE_METATYPE(ImageItem);

typedef QList<ImageItem> ImageItemList;

class BaselineProtocol
{
public:
    BaselineProtocol();
    ~BaselineProtocol();

    static BaselineProtocol *instance(QObject *parent = nullptr);

    // ****************************************************
    // Important constants here
    // ****************************************************
    enum Constant {
        ProtocolVersion = 5,
        ServerPort = 54129,
        Timeout = 15000
    };

    enum Command {
        UnknownError = 0,
        // Queries
        AcceptPlatformInfo = 1,
        RequestBaselineChecksums = 2,
        AcceptMatch = 3,
        AcceptNewBaseline = 4,
        AcceptMismatch = 5,
        // Responses
        Ack = 128,
        Abort = 129,
        DoDryRun = 130,
        FuzzyMatch = 131
    };

    // For client:

    // For advanced client:
    bool connect(const QString &testCase, bool *dryrun = nullptr, const PlatformInfo& clientInfo = PlatformInfo());
    bool disconnect();
    bool requestBaselineChecksums(const QString &testFunction, ImageItemList *itemList);
    bool submitMatch(const ImageItem &item, QByteArray *serverMsg);
    bool submitNewBaseline(const ImageItem &item, QByteArray *serverMsg);
    bool submitMismatch(const ImageItem &item, QByteArray *serverMsg, bool *fuzzyMatch = nullptr);

    // For server:
    bool acceptConnection(PlatformInfo *pi);

    QString errorMessage();

private:
    bool sendItem(Command cmd, const ImageItem &item);

    bool sendBlock(Command cmd, const QByteArray &block);
    bool receiveBlock(Command *cmd, QByteArray *block);

    QString errMsg;
    QTcpSocket socket;

    friend class BaselineThread;
    friend class BaselineHandler;
};


#endif // BASELINEPROTOCOL_H
