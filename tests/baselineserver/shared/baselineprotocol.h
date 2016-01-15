/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BASELINEPROTOCOL_H
#define BASELINEPROTOCOL_H

#include <QDataStream>
#include <QTcpSocket>
#include <QImage>
#include <QVector>
#include <QMap>
#include <QPointer>
#include <QStringList>

#define QLS QLatin1String
#define QLC QLatin1Char

#define FileFormat "png"

extern const QString PI_Project;
extern const QString PI_TestCase;
extern const QString PI_HostName;
extern const QString PI_HostAddress;
extern const QString PI_OSName;
extern const QString PI_OSVersion;
extern const QString PI_QtVersion;
extern const QString PI_QtBuildMode;
extern const QString PI_GitCommit;
extern const QString PI_QMakeSpec;
extern const QString PI_PulseGitBranch;
extern const QString PI_PulseTestrBranch;

class PlatformInfo : public QMap<QString, QString>
{
public:
    PlatformInfo();
    PlatformInfo(const PlatformInfo &other);
    ~PlatformInfo()
    {}
    PlatformInfo &operator=(const PlatformInfo &other);

    static PlatformInfo localHostInfo();

    void addOverride(const QString& key, const QString& value);
    QStringList overrides() const;
    bool isAdHocRun() const;
    void setAdHocRun(bool isAdHoc);

private:
    QStringList orides;
    bool adHoc;
    friend QDataStream & operator<< (QDataStream &stream, const PlatformInfo &pi);
    friend QDataStream & operator>> (QDataStream &stream, PlatformInfo& pi);
};
QDataStream & operator<< (QDataStream &stream, const PlatformInfo &pi);
QDataStream & operator>> (QDataStream &stream, PlatformInfo& pi);


struct ImageItem
{
public:
    ImageItem()
        : status(Ok), itemChecksum(0)
    {}
    ImageItem(const ImageItem &other)
    { *this = other; }
    ~ImageItem()
    {}
    ImageItem &operator=(const ImageItem &other);

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
    ItemStatus status;
    QImage image;
    QList<quint64> imageChecksums;
    quint16 itemChecksum;
    QByteArray misc;

    void writeImageToStream(QDataStream &stream) const;
    void readImageFromStream(QDataStream &stream);
};
QDataStream & operator<< (QDataStream &stream, const ImageItem &ii);
QDataStream & operator>> (QDataStream &stream, ImageItem& ii);

Q_DECLARE_METATYPE(ImageItem);

typedef QVector<ImageItem> ImageItemList;


class BaselineProtocol
{
public:
    BaselineProtocol();
    ~BaselineProtocol();

    static BaselineProtocol *instance(QObject *parent = 0);

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
    bool connect(const QString &testCase, bool *dryrun = 0, const PlatformInfo& clientInfo = PlatformInfo());
    bool disconnect();
    bool requestBaselineChecksums(const QString &testFunction, ImageItemList *itemList);
    bool submitMatch(const ImageItem &item, QByteArray *serverMsg);
    bool submitNewBaseline(const ImageItem &item, QByteArray *serverMsg);
    bool submitMismatch(const ImageItem &item, QByteArray *serverMsg, bool *fuzzyMatch = 0);

    // For server:
    bool acceptConnection(PlatformInfo *pi);

    QString errorMessage();

private:
    bool sendItem(Command cmd, const ImageItem &item);

    bool sendBlock(Command cmd, const QByteArray &block);
    bool receiveBlock(Command *cmd, QByteArray *block);
    void sysSleep(int ms);

    QString errMsg;
    QTcpSocket socket;

    friend class BaselineThread;
    friend class BaselineHandler;
};


#endif // BASELINEPROTOCOL_H
