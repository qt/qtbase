/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QString>
#ifdef QT_NETWORK_LIB
#include <QtNetwork/QHostInfo>
#endif


#ifdef Q_OS_SYMBIAN
#include <e32base.h>
#include <sys/socket.h>
#include <net/if.h>
#include <QSharedPointer>
#include <QHash>
#endif
#if defined(Q_OS_SYMBIAN)
#if defined(Q_CC_NOKIAX86)
// In emulator we use WINSOCK connectivity by default. Unfortunately winsock
// does not work very well with UDP sockets. This defines skips some test
// cases which have known problems.

// NOTE: Prefer to use WINPCAP based connectivity in S60 emulator when running
// network tests. WINPCAP connectivity uses Symbian OS IP stack,
// correspondingly as HW does. When using WINPCAP disable this define
//#define SYMBIAN_WINSOCK_CONNECTIVITY
#endif // Q_CC_NOKIAX86

// FIXME: any reason we do this for symbian only, and not other platforms?
class QtNetworkSettingsRecord {
public:
    QtNetworkSettingsRecord() { }

    QtNetworkSettingsRecord(const QString& recName, const QString& recVal)
        : strRecordName(recName), strRecordValue(recVal) { }

    QtNetworkSettingsRecord(const QtNetworkSettingsRecord & other)
         : strRecordName(other.strRecordName), strRecordValue(other.strRecordValue) { }

    ~QtNetworkSettingsRecord() { }

    const QString& recordName() const { return strRecordName; }
    const QString& recordValue() const { return strRecordValue; }

private:
    QString strRecordName;
    QString strRecordValue;
};

#endif // Q_OS_SYMBIAN

class QtNetworkSettings
{
public:

    static QString serverLocalName()
    {
#ifdef Q_OS_SYMBIAN
        loadTestSettings();

        if(QtNetworkSettings::entries.contains("server.localname")) {
            QtNetworkSettingsRecord* entry = entries["server.localname"];
            return entry->recordValue();
        }
#endif
        return QString("qt-test-server");
    }
    static QString serverDomainName()
    {
#ifdef Q_OS_SYMBIAN
        loadTestSettings();

        if(QtNetworkSettings::entries.contains("server.domainname")) {
            QtNetworkSettingsRecord* entry = entries["server.domainname"];
            return entry->recordValue();
        }
#endif
        return QString("qt-test-net");
    }
    static QString serverName()
    {
#ifdef Q_OS_SYMBIAN
        loadTestSettings();
#endif
        return serverLocalName() + "." + serverDomainName();
    }
    static QString winServerName()
    {
        return serverName();
    }
    static QString wildcardServerName()
    {
        return "qt-test-server.wildcard.dev." + serverDomainName();
    }

#ifdef QT_NETWORK_LIB
    static QHostAddress serverIP()
    {
#ifdef Q_OS_SYMBIAN
        loadTestSettings();

        if(QtNetworkSettings::entries.contains("server.ip")) {
            QtNetworkSettingsRecord* entry = entries["server.ip"];
            if(serverIp.isNull()) {
                serverIp = entry->recordValue().toAscii();
            }
            return QHostAddress(serverIp.data());
        }
#endif // Q_OS_SYMBIAN
    return QHostInfo::fromName(serverName()).addresses().first();
    }
#endif

    static bool compareReplyIMAP(QByteArray const& actual)
    {
        QList<QByteArray> expected;

#ifdef Q_OS_SYMBIAN
        loadTestSettings();

        if(QtNetworkSettings::entries.contains("imap.expectedreply")) {
            QtNetworkSettingsRecord* entry = entries["imap.expectedreply"];
            if(imapExpectedReply.isNull()) {
                imapExpectedReply = entry->recordValue().toAscii();
                imapExpectedReply.append('\r').append('\n');
            }
            expected << imapExpectedReply.data();
        }
#endif

        // Mandriva; old test server
        expected << QByteArray( "* OK [CAPABILITY IMAP4 IMAP4rev1 LITERAL+ ID STARTTLS LOGINDISABLED] " )
            .append(QtNetworkSettings::serverName().toAscii())
            .append(" Cyrus IMAP4 v2.3.11-Mandriva-RPM-2.3.11-6mdv2008.1 server ready\r\n");

        // Ubuntu 10.04; new test server
        expected << QByteArray( "* OK " )
            .append(QtNetworkSettings::serverLocalName().toAscii())
            .append(" Cyrus IMAP4 v2.2.13-Debian-2.2.13-19 server ready\r\n");

        // Feel free to add more as needed

        Q_FOREACH (QByteArray const& ba, expected) {
            if (ba == actual) {
                return true;
            }
        }

        return false;
    }

    static bool compareReplyIMAPSSL(QByteArray const& actual)
    {
        QList<QByteArray> expected;

#ifdef Q_OS_SYMBIAN
        loadTestSettings();

        if(QtNetworkSettings::entries.contains("imap.expectedreplyssl")) {
            QtNetworkSettingsRecord* entry = entries["imap.expectedreplyssl"];
            if(imapExpectedReplySsl.isNull()) {
                imapExpectedReplySsl = entry->recordValue().toAscii();
                imapExpectedReplySsl.append('\r').append('\n');
            }
            expected << imapExpectedReplySsl.data();
        }
#endif
        // Mandriva; old test server
        expected << QByteArray( "* OK [CAPABILITY IMAP4 IMAP4rev1 LITERAL+ ID AUTH=PLAIN SASL-IR] " )
            .append(QtNetworkSettings::serverName().toAscii())
            .append(" Cyrus IMAP4 v2.3.11-Mandriva-RPM-2.3.11-6mdv2008.1 server ready\r\n");

        // Ubuntu 10.04; new test server
        expected << QByteArray( "* OK " )
            .append(QtNetworkSettings::serverLocalName().toAscii())
            .append(" Cyrus IMAP4 v2.2.13-Debian-2.2.13-19 server ready\r\n");

        // Feel free to add more as needed

        Q_FOREACH (QByteArray const& ba, expected) {
            if (ba == actual) {
                return true;
            }
        }

        return false;
    }

    static bool compareReplyFtp(QByteArray const& actual)
    {
        QList<QByteArray> expected;

        // A few different vsFTPd versions.
        // Feel free to add more as needed
        expected << QByteArray( "220 (vsFTPd 2.0.5)\r\n221 Goodbye.\r\n" );
        expected << QByteArray( "220 (vsFTPd 2.2.2)\r\n221 Goodbye.\r\n" );

        Q_FOREACH (QByteArray const& ba, expected) {
            if (ba == actual) {
                return true;
            }
        }

        return false;
    }

#ifdef Q_OS_SYMBIAN
    static void setDefaultIap()
    {
        loadDefaultIap();

        struct ifreq ifReq;
        if(entries.contains("iap.default")) {
            QtNetworkSettingsRecord* entry = entries["iap.default"];
            QByteArray tmp(entry->recordValue().toAscii());
            strcpy( ifReq.ifr_name, tmp.data());
        }
        else // some default value
            strcpy( ifReq.ifr_name, "Lab");

        int err = setdefaultif( &ifReq );
        if(err)
            printf("Setting default IAP - '%s' failed: %d\n", ifReq.ifr_name, err);
        else
            printf("'%s' used as an default IAP\n", ifReq.ifr_name);
    }
#endif

private:

#ifdef Q_OS_SYMBIAN

    static  QHash<QString, QtNetworkSettingsRecord* > entries;
    static bool bDefaultIapLoaded;
    static bool bTestSettingsLoaded;
    static QString iapFileFullPath;
    static QByteArray serverIp;
    static QByteArray imapExpectedReply;
    static QByteArray imapExpectedReplySsl;

    static bool loadDefaultIap() {
        if(bDefaultIapLoaded)
            return true;

        QFile iapCfgFile(iapFileFullPath);

        bool bFoundDefaultIapTag = false;

        if (iapCfgFile.open(QFile::ReadOnly)) {
            QTextStream input(&iapCfgFile);
            QString line;
            do {
                line = input.readLine().trimmed();
                if(line.startsWith(QString("#")))
                    continue; // comment found

                if(line.contains(QString("[DEFAULT]"))) {
                    bFoundDefaultIapTag = true;
                } else if(line.contains(QString("[")) && bFoundDefaultIapTag) {
                    break;
                }

                if(bFoundDefaultIapTag && line.contains("name")) {
                    int position = line.indexOf(QString("="));
                    position += QString("=").length();

                    //create record
                    QtNetworkSettingsRecord *entry =
                        new QtNetworkSettingsRecord( QString("iap.default"), line.mid(position).trimmed() );
                    entries.insert(entry->recordName(), entry);
                    break;
                }
            } while (!line.isNull());
        }

        return bDefaultIapLoaded = bFoundDefaultIapTag;
    }

    static bool loadTestSettings() {
        if(bTestSettingsLoaded)
            return true;

        QFile cfgFile(iapFileFullPath);
        bool bFoundTestTag = false;

        if (cfgFile.open(QFile::ReadOnly)) {
            QTextStream input(&cfgFile);
            QString line;
            do {
                line = input.readLine().trimmed();

                if(line.startsWith(QString("#")) || line.length() == 0)
                    continue; // comment or empty line found

                if(line.contains(QString("[TEST]"))) {
                    bFoundTestTag = true;
                } else if(line.startsWith(QString("[")) && bFoundTestTag) {
                    bFoundTestTag = false;
                    break; // finished with test tag
                }

                if(bFoundTestTag) { // non-empty line
                    int position = line.indexOf(QString("="));

                    if(position <= 0) // not found
                        continue;

                    // found - extract

                    QString recname = line.mid(0, position - QString("=").length()).trimmed();
                    QString recval = line.mid(position + QString("=").length()).trimmed();

                    //create record
                    QtNetworkSettingsRecord *entry = new QtNetworkSettingsRecord(recname, recval);
                    entries.insert(entry->recordName(), entry);
                }
            } while (!line.isNull());
        }

        return bTestSettingsLoaded = true;
    }
#endif


};
#ifdef Q_OS_SYMBIAN
QHash<QString, QtNetworkSettingsRecord* > QtNetworkSettings::entries = QHash<QString, QtNetworkSettingsRecord* > ();
bool QtNetworkSettings::bDefaultIapLoaded = false;
bool QtNetworkSettings::bTestSettingsLoaded = false;
QString QtNetworkSettings::iapFileFullPath = QString("C:\\Data\\iap.txt");
QByteArray QtNetworkSettings::serverIp;
QByteArray QtNetworkSettings::imapExpectedReply;
QByteArray QtNetworkSettings::imapExpectedReplySsl;
#endif

#ifdef Q_OS_SYMBIAN
#define Q_SET_DEFAULT_IAP QtNetworkSettings::setDefaultIap();
#else
#define Q_SET_DEFAULT_IAP
#endif

#ifdef QT_NETWORK_LIB
class QtNetworkSettingsInitializerCode {
public:
    QtNetworkSettingsInitializerCode() {
#ifdef Q_OS_SYMBIAN
#ifdef Q_CC_NOKIAX86
        // We have a non-trivial constructor in global static.
        // The QtNetworkSettings::serverName() uses native API which assumes
        // Cleanup-stack to exist. That's why we create it here and install
        // top level TRAP harness.
        CTrapCleanup *cleanupStack = q_check_ptr(CTrapCleanup::New());
        TRAPD(err,
            QHostInfo testServerResult = QHostInfo::fromName(QtNetworkSettings::serverName());
            if (testServerResult.error() != QHostInfo::NoError) {
                qWarning() << "Could not lookup" << QtNetworkSettings::serverName();
                qWarning() << "Please configure the test environment!";
                qWarning() << "See /etc/hosts or network-settings.h";
                qFatal("Exiting");
            }
        )
        delete cleanupStack;
//#else
        // In Symbian HW there is no sense to run this check since global statics are
        // initialized before QTestLib initializes the output channel for QWarnigns.
        // So if there is problem network setup, also all QtCore etc tests whcih have
        // QtNetwork dependency will crash with panic "0 - Exiciting"
#endif

#else
        QHostInfo testServerResult = QHostInfo::fromName(QtNetworkSettings::serverName());
        if (testServerResult.error() != QHostInfo::NoError) {
            qWarning() << "Could not lookup" << QtNetworkSettings::serverName();
            qWarning() << "Please configure the test environment!";
            qWarning() << "See /etc/hosts or network-settings.h";
            qFatal("Exiting");
        }
#endif
    }
};
QtNetworkSettingsInitializerCode qtNetworkSettingsInitializer;
#endif
