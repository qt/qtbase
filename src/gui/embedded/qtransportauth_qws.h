/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTRANSPORTAUTH_QWS_H
#define QTRANSPORTAUTH_QWS_H

#include <QtCore/qglobal.h>

#if !defined(QT_NO_SXE) || defined(SXE_INSTALLER)

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qpointer.h>

#include <sys/types.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QAuthDevice;
class QWSClient;
class QIODevice;
class QTransportAuthPrivate;
class QMutex;

class Q_GUI_EXPORT QTransportAuth : public QObject
{
    Q_OBJECT
public:
    static QTransportAuth *getInstance();

    enum Result {
        // Error codes
        Pending = 0x00,
        TooSmall = 0x01,
        CacheMiss = 0x02,
        NoMagic = 0x03,
        NoSuchKey = 0x04,
        FailMatch = 0x05,
        OutOfDate = 0x06,
        // reserved for expansion
        Success = 0x1e,
        ErrMask = 0x1f,

        // Verification codes
        Allow = 0x20,
        Deny = 0x40,
        Ask = 0x60,
        // reserved
        StatusMask = 0xe0
    };

    enum Properties {
        Trusted = 0x01,
        Connection = 0x02,
        UnixStreamSock = 0x04,
        SharedMemory = 0x08,
        MessageQueue = 0x10,
        UDP = 0x20,
        TCP = 0x40,
        UserDefined = 0x80,
        TransportType = 0xfc
    };

    struct Data
    {
        Data() { processId = -1; }
        Data( unsigned char p, int d )
            : properties( p )
            , descriptor( d )
            , processId( -1 )
        {
            if (( properties & TransportType ) == TCP ||
                ( properties & TransportType ) == UnixStreamSock )
                properties |= Connection;
        }

        unsigned char properties;
        unsigned char progId;
        unsigned char status;
        unsigned int descriptor;   // socket fd or shmget key
        pid_t processId;

        bool trusted() const;
        void setTrusted( bool );
        bool connection() const;
        void setConnection( bool );
    };

    static const char *errorString( const QTransportAuth::Data & );

    QTransportAuth::Data *connectTransport( unsigned char, int );

    QAuthDevice *authBuf( QTransportAuth::Data *, QIODevice * );
    QAuthDevice *recvBuf( QTransportAuth::Data *, QIODevice * );
    QIODevice *passThroughByClient( QWSClient * ) const;

    void setKeyFilePath( const QString & );
    QString keyFilePath() const;
    const unsigned char *getClientKey( unsigned char progId );
    void invalidateClientKeyCache();
    QMutex *getKeyFileMutex();
    void setLogFilePath( const QString & );
    QString logFilePath() const;
    void setPackageRegistry( QObject *registry );
    bool isDiscoveryMode() const;
    void setProcessKey( const char * );
    void setProcessKey( const char *, const char * );
    void registerPolicyReceiver( QObject * );
    void unregisterPolicyReceiver( QObject * );

    bool authToMessage( QTransportAuth::Data &d, char *hdr, const char *msg, int msgLen );
    bool authFromMessage( QTransportAuth::Data &d, const char *msg, int msgLen );

    bool authorizeRequest( QTransportAuth::Data &d, const QString &request );

Q_SIGNALS:
    void policyCheck( QTransportAuth::Data &, const QString & );
    void authViolation( QTransportAuth::Data & );
private Q_SLOTS:
    void bufferDestroyed( QObject * );

private:
    // users should never construct their own
    QTransportAuth();
    ~QTransportAuth();

    friend class QAuthDevice;
    Q_DECLARE_PRIVATE(QTransportAuth)
};

class Q_GUI_EXPORT RequestAnalyzer
{
public:
    RequestAnalyzer();
    virtual ~RequestAnalyzer();
    QString operator()( QByteArray *data ) { return analyze( data ); }
    bool requireMoreData() const { return moreData; }
    qint64 bytesAnalyzed() const { return dataSize; }
protected:
    virtual QString analyze( QByteArray * );
    bool moreData;
    qint64 dataSize;
};

/*!
  \internal
  \class QAuthDevice

  \brief Pass-through QIODevice sub-class for authentication.

   Use this class to forward on or receive forwarded data over a real
   device for authentication.
*/
class Q_GUI_EXPORT QAuthDevice : public QIODevice
{
    Q_OBJECT
public:
    enum AuthDirection {
        Receive,
        Send
    };
    QAuthDevice( QIODevice *, QTransportAuth::Data *, AuthDirection );
    ~QAuthDevice();
    void setTarget( QIODevice *t ) { m_target = t; }
    QIODevice *target() const { return m_target; }
    void setClient( QObject* );
    QObject *client() const;
    void setRequestAnalyzer( RequestAnalyzer * );
    bool isSequential() const;
    bool atEnd() const;
    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;
    bool seek( qint64 );
    QByteArray & buffer();

protected:
    qint64 readData( char *, qint64 );
    qint64 writeData(const char *, qint64 );
private Q_SLOTS:
    void recvReadyRead();
    void targetBytesWritten( qint64 );
private:
    bool authorizeMessage();

    QTransportAuth::Data *d;
    AuthDirection way;
    QIODevice *m_target;
    QObject *m_client;
    QByteArray msgQueue;
    qint64 m_bytesAvailable;
    qint64 m_skipWritten;

    RequestAnalyzer *analyzer;
};

inline bool QAuthDevice::isSequential() const
{
    return true;
}

inline bool QAuthDevice::seek( qint64 )
{
    return false;
}

inline bool QAuthDevice::atEnd() const
{
    return msgQueue.isEmpty();
}

inline qint64 QAuthDevice::bytesAvailable() const
{
    if ( way == Receive )
        return m_bytesAvailable;
    else
        return ( m_target ? m_target->bytesAvailable() : 0 );
}

inline qint64 QAuthDevice::bytesToWrite() const
{
    return msgQueue.size();
}

inline QByteArray &QAuthDevice::buffer()
{
    return msgQueue;
}




QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_SXE
#endif // QTRANSPORTAUTH_QWS_H
