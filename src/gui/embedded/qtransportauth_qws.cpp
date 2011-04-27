/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qtransportauth_qws.h"
#include "qtransportauth_qws_p.h"

#ifndef QT_NO_SXE

#include "../../3rdparty/md5/md5.h"
#include "../../3rdparty/md5/md5.cpp"
#include "qwsutils_qws.h"
#include "qwssocket_qws.h"
#include "qwscommand_qws_p.h"
#include "qwindowsystem_qws.h"
#include "qbuffer.h"
#include "qthread.h"
#include "qabstractsocket.h"
#include "qlibraryinfo.h"
#include "qfile.h"
#include "qdebug.h"
#include <private/qcore_unix_p.h> // overrides QT_OPEN

#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <QtCore/qcache.h>

#define BUF_SIZE 512

QT_BEGIN_NAMESPACE

/*!
  \internal
  memset for security purposes, guaranteed not to be optimized away
  http://www.faqs.org/docs/Linux-HOWTO/Secure-Programs-HOWTO.html
*/
Q_GUI_EXPORT void *guaranteed_memset(void *v,int c,size_t n)
{
    volatile char *p = (char *)v; while (n--) *p++=c; return v;
}

/*!
  \class QTransportAuth
  \internal

  \brief Authenticate a message transport.

  For performance reasons, message authentication is tied to an individual
  message transport instance.  For example in connection oriented transports
  the authentication cookie can be cached against the connection avoiding
  the overhead of authentication on every message.

  For each process there is one instance of the QTransportAuth object.
  For server processes it can determine the \link secure-exe-environ.html SXE
  Program Identity \endlink and provide access to policy data to determine if
  the message should be forwarded for action.  If not actioned, the message
  may be treated as being from a flawed or malicious process.

  Retrieve the instance with the getInstance() method.  The constructor is
  disabled and instances of QTransportAuth should never be constructed by
  calling classes.

  To make the Authentication easier to use a proxied QIODevice is provided
  which uses an internal QBuffer.

  In the server code first get a pointer to a QTransportAuth::Data object
  using the connectTransport() method:

  \snippet doc/src/snippets/code/src_gui_embedded_qtransportauth_qws.cpp 0

  Here it is asserted that the transport is trusted.  See the assumptions
  listed in the \link secure-exe-environ.html SXE documentation \endlink

  Then proxy in the authentication device:

  \snippet doc/src/snippets/code/src_gui_embedded_qtransportauth_qws.cpp 1

  In the client code it is similar.  Use the connectTransport() method
  just the same then proxy in the authentication device instead of the
  socket in write calls:

  \snippet doc/src/snippets/code/src_gui_embedded_qtransportauth_qws.cpp 2
*/

static int hmac_md5(
        unsigned char*  text,         /* pointer to data stream */
        int             text_length,  /* length of data stream */
        const unsigned char*  key,    /* pointer to authentication key */
        int             key_length,   /* length of authentication key */
        unsigned char * digest        /* caller digest to be filled in */
        );



#define KEY_CACHE_SIZE 30

const char * const errorStrings[] = {
    "pending identity verification",
    "message too small to carry auth data",
    "cache miss on connection oriented transport",
    "no magic bytes on message",
    "key not found for prog id",
    "authorization key match failed",
    "key out of date"
};

const char *QTransportAuth::errorString( const Data &d )
{
    if (( d.status & ErrMask ) == Success )
        return "success";
    int e = d.status & ErrMask;
    if ( e > OutOfDate )
        return "unknown";
    return errorStrings[e];
}

SxeRegistryLocker::SxeRegistryLocker( QObject *reg )
    : m_success( false )
    , m_reg( 0 )
{
    if ( reg )
        if ( !QMetaObject::invokeMethod( reg, "lockManifest", Q_RETURN_ARG(bool, m_success) ))
            m_success = false;
    m_reg = reg;
}

SxeRegistryLocker::~SxeRegistryLocker()
{
    if ( m_success )
        QMetaObject::invokeMethod( m_reg, "unlockManifest" );
}


QTransportAuthPrivate::QTransportAuthPrivate()
    : keyInitialised(false)
    , m_packageRegistry( 0 )
{
}

QTransportAuthPrivate::~QTransportAuthPrivate()
{
}

/*!
  \internal
  Construct a new QTransportAuth
*/
QTransportAuth::QTransportAuth() : QObject(*new QTransportAuthPrivate)
{
    // qDebug( "creating transport auth" );
}

/*!
  \internal
  Destructor
*/
QTransportAuth::~QTransportAuth()
{
    // qDebug( "deleting transport auth" );
}

/*!
  Set the process key for this currently running Qt Extended process to
  the \a authdata.  \a authdata should be sizeof(struct AuthCookie)
  in length and contain the key and program id.  Use this method
  when setting or changing the SXE identity of the current program.
*/
void QTransportAuth::setProcessKey( const char *authdata )
{
    Q_D(QTransportAuth);
    ::memcpy(&d->authKey, authdata, sizeof(struct AuthCookie));
    QFile proc_key( QLatin1String("/proc/self/lids_key") );
    // where proc key exists use that instead
    if ( proc_key.open( QIODevice::ReadOnly ))
    {
        qint64 kb = proc_key.read( (char*)&d->authKey.key, QSXE_KEY_LEN );
#ifdef QTRANSPORTAUTH_DEBUG
        qDebug( "Using %li bytes of /proc/%i/lids_key\n", (long int)kb, getpid() );
#else
        Q_UNUSED( kb );
#endif
    }
    d->keyInitialised = true;
}


/*!
  Apply \a key as the process key for the currently running application.

  \a prog is current ignored

  Deprecated function
*/
void QTransportAuth::setProcessKey( const char *key, const char *prog )
{
    Q_UNUSED(prog);
    setProcessKey( key );
#ifdef QTRANSPORTAUTH_DEBUG
    char displaybuf[QSXE_KEY_LEN*2+1];
    hexstring( displaybuf, (const unsigned char *)key, QSXE_KEY_LEN );
    qDebug() << "key" << displaybuf << "set";
#endif
}

/*!
  Register \a pr as a policy handler object.  The object pointed to
  by \a pr should have a slot as follows
  \snippet doc/src/snippets/code/src_gui_embedded_qtransportauth_qws.cpp 3
  All requests received by this server will then generate a call to
  this slot, and may be processed for policy compliance.
*/
void QTransportAuth::registerPolicyReceiver( QObject *pr )
{
    // not every policy receiver needs setup - no error if this fails
    QMetaObject::invokeMethod( pr, "setupPolicyCheck" );

    connect( this, SIGNAL(policyCheck(QTransportAuth::Data&,QString)),
             pr, SLOT(policyCheck(QTransportAuth::Data&,QString)), Qt::DirectConnection );
}

/*!
  Unregister the \a pr from being a policy handler.  No more policyCheck signals
  are received by this object.
*/
void QTransportAuth::unregisterPolicyReceiver( QObject *pr )
{
    disconnect( pr );
    // not every policy receiver needs tear down - no error if this fails
    QMetaObject::invokeMethod( pr, "teardownPolicyCheck" );
}

/*!
  Record a new transport connection with \a properties and \a descriptor.

  The calling code is responsible for destroying the returned data when the
  tranport connection is closed.
*/
QTransportAuth::Data *QTransportAuth::connectTransport( unsigned char properties, int descriptor )
{
    Data *data = new Data(properties, descriptor);
    data->status = Pending;
    return data;
}

/*!
  Is the transport trusted.  This is true iff data written into the
  transport medium cannot be intercepted or modified by another process.
  This is for example true for Unix Domain Sockets, but not for shared
  memory or UDP sockets.

  There is of course an underlying assumption that the kernel implementing
  the transport is sound, ie it cannot be compromised by writing to
  /dev/kmem or loading untrusted modules
*/
inline bool QTransportAuth::Data::trusted() const
{
    return (bool)(properties & Trusted);
}

/*!
  Assert that the transport is trusted.

  For example with respect to shared memory, if it is ensured that no untrusted
  root processes are running, and that unix permissions have been set such that
  any untrusted non-root processes do not have access rights, then a shared
  memory transport could be asserted to be trusted.

  \sa trusted()
*/
inline void QTransportAuth::Data::setTrusted( bool t )
{
    properties = t ? properties | Trusted : properties & ~Trusted;
}

/*!
  Is the transport connection oriented.  This is true iff once a connection
  has been accepted, and state established, then further messages over the
  transport are guaranteed to have come from the original connecting entity.
  This is for example true for Unix Domain Sockets, but not
  for shared memory or UDP sockets.

  By extension if the transport is not trusted() then it should not be
  assumed to be connection oriented, since spoofed connection information
  could be created.  For example if we assume the TCP/IP transport is
  trusted, it can be treated as connection oriented; but this is only the
  case if intervening routers are trusted.

  Connection oriented transports have authorization cached against the
  connection, and thus authorization is only done at connect time.
*/
inline bool QTransportAuth::Data::connection() const
{
    return (bool)(properties & Connection);
}

/*!
  Assert that the transport is connection oriented.

  \sa connection()
*/
inline void QTransportAuth::Data::setConnection( bool t )
{
    properties = t ? properties | Connection : properties & ~Connection;
}

/*!
  Return a pointer to the instance of this process's QTransportAuth object
*/
QTransportAuth *QTransportAuth::getInstance()
{
    static QTransportAuth theInstance;

    return &theInstance;
}

/*!
  Set the full path to the key file

  Since this is normally relative to Qtopia::qpeDir() this needs to be
  set within the Qt Extended framework.

  The keyfile should be protected by file permissions or by MAC rules
  such that it can only be read/written by the "qpe" server process
*/
void QTransportAuth::setKeyFilePath( const QString &path )
{
    Q_D(QTransportAuth);
    d->m_keyFilePath = path;
}

QString QTransportAuth::keyFilePath() const
{
    Q_D(const QTransportAuth);
    return d->m_keyFilePath;
}

void QTransportAuth::setLogFilePath( const QString &path )
{
    Q_D(QTransportAuth);
    d->m_logFilePath = path;
}

QString QTransportAuth::logFilePath() const
{
    Q_D(const QTransportAuth);
    return d->m_logFilePath;
}

void QTransportAuth::setPackageRegistry( QObject *registry )
{
    Q_D(QTransportAuth);
    d->m_packageRegistry = registry;
}

bool QTransportAuth::isDiscoveryMode() const
{
#if defined(SXE_DISCOVERY)
    static bool checked = false;
    static bool yesItIs = false;

    if ( checked ) return yesItIs;

    yesItIs = ( getenv( "SXE_DISCOVERY_MODE" ) != 0 );
    if ( yesItIs )
    {
        qWarning("SXE Discovery mode on, ALLOWING ALL requests and logging to %s",
                 qPrintable(logFilePath()));
        QFile::remove( logFilePath() );
    }
    checked = true;
    return yesItIs;
#else
    return false;
#endif
}

/*!
  \internal
  Return the authorizer device mapped to this client.  Note that this
  could probably all be void* instead of QWSClient* for generality.
  Until the need for that rears its head its QWSClient* to save the casts.

  #### OK the need has arrived, but the public API is frozen.
*/
QIODevice *QTransportAuth::passThroughByClient( QWSClient *client ) const
{
    Q_D(const QTransportAuth);

    if ( client == 0 ) return 0;
    if ( d->buffersByClient.contains( client ))
    {
        return d->buffersByClient[client];
    }
    // qWarning( "buffer not found for client %p", client );
    return 0;
}

/*!
  \internal
  Return a QIODevice pointer (to an internal QBuffer) which can be used
  to receive data after authorization on transport \a d.

  The return QIODevice will act as a pass-through.

  The data will be consumed from \a iod and forwarded on to the returned
  QIODevice which can be connected to readyRead() signal handlers in
  place of the original QIODevice \a iod.

  This will be called in the server process to handle incoming
  authenticated requests.

  The returned QIODevice will take ownership of \a data which will be deleted
  when the QIODevice is delected.

  \sa setTargetDevice()
*/
QAuthDevice *QTransportAuth::recvBuf( QTransportAuth::Data *data, QIODevice *iod )
{
    return new QAuthDevice( iod, data, QAuthDevice::Receive );
}

/*!
  Return a QIODevice pointer (to an internal QBuffer) which can be used
  to write data onto, for authorization on transport \a d.

  The return QIODevice will act as a pass-through.

  The data written to the return QIODevice will be forwarded on to the
  returned QIODevice.  In the case of a QTcpSocket, this will cause it
  to send out the data with the authentication information on it.

  This will be called in the client process to generate outgoing
  authenticated requests.

  The returned QIODevice will take ownership of \a data which will be deleted
  when the QIODevice is delected.

  \sa setTargetDevice()
*/
QAuthDevice *QTransportAuth::authBuf( QTransportAuth::Data *data, QIODevice *iod )
{
    return new QAuthDevice( iod, data, QAuthDevice::Send );
}

const unsigned char *QTransportAuth::getClientKey( unsigned char progId )
{
    Q_D(QTransportAuth);
    return d->getClientKey( progId );
}

void QTransportAuth::invalidateClientKeyCache()
{
    Q_D(QTransportAuth);
    d->invalidateClientKeyCache();
}

QMutex *QTransportAuth::getKeyFileMutex()
{
    Q_D(QTransportAuth);
    return &d->keyfileMutex;
}

/*
   \internal
   Respond to the destroyed(QObject*) signal of the QAuthDevice's
   client object and remove it from the buffersByClient lookup hash.
*/
void QTransportAuth::bufferDestroyed( QObject *cli )
{
    Q_D(QTransportAuth);
    if ( cli == NULL ) return;

    if ( d->buffersByClient.contains( cli ))
    {
        d->buffersByClient.remove( cli );
        // qDebug( "@@@@@@@ client %p removed @@@@@@@@@", cli );
    }
    // qDebug( "           client count %d", d->buffersByClient.count() );
}

bool QTransportAuth::authorizeRequest( QTransportAuth::Data &d, const QString &request )
{
    bool isAuthorized = true;

    if ( !request.isEmpty() && request != QLatin1String("Unknown") )
    {
        d.status &= QTransportAuth::ErrMask;  // clear the status
        emit policyCheck( d, request );
        isAuthorized = (( d.status & QTransportAuth::StatusMask ) == QTransportAuth::Allow );
    }
#if defined(SXE_DISCOVERY)
    if (isDiscoveryMode()) {
#ifndef QT_NO_TEXTSTREAM
        if (!logFilePath().isEmpty()) {
            QFile log( logFilePath() );
            if (!log.open(QIODevice::WriteOnly | QIODevice::Append)) {
                qWarning("Could not write to log in discovery mode: %s",
                         qPrintable(logFilePath()));
            } else {
                QTextStream ts( &log );
                ts << d.progId << '\t' << ( isAuthorized ? "Allow" : "Deny" ) << '\t' << request << endl;
            }
        }
#endif
        isAuthorized = true;
    }
#endif
    if ( !isAuthorized )
    {
        qWarning( "%s - denied: for Program Id %u [PID %d]"
                , qPrintable(request), d.progId, d.processId );

        char linkTarget[BUF_SIZE]="";
        char exeLink[BUF_SIZE]="";
        char cmdlinePath[BUF_SIZE]="";
        char cmdline[BUF_SIZE]="";

        //get executable from /proc/pid/exe
        snprintf( exeLink, BUF_SIZE, "/proc/%d/exe", d.processId );
        if ( -1 == ::readlink( exeLink, linkTarget, BUF_SIZE - 1 ) )
        {
            qWarning( "SXE:- Error encountered in retrieving executable link target from /proc/%u/exe : %s",
                d.processId, strerror(errno) );
            snprintf( linkTarget, BUF_SIZE, "%s", linkTarget );
        }

        //get cmdline from proc/pid/cmdline
        snprintf( cmdlinePath, BUF_SIZE, "/proc/%d/cmdline", d.processId );
        int  cmdlineFd = QT_OPEN( cmdlinePath, O_RDONLY );
        if ( cmdlineFd == -1 )
        {
            qWarning( "SXE:- Error encountered in opening /proc/%u/cmdline: %s",
                d.processId, strerror(errno) );
            snprintf( cmdline, BUF_SIZE, "%s", "Unknown" );
        }
        else
        {
            if ( -1 == QT_READ(cmdlineFd, cmdline, BUF_SIZE - 1 ) )
            {
                qWarning( "SXE:- Error encountered in reading /proc/%u/cmdline : %s",
                    d.processId, strerror(errno) );
                snprintf( cmdline, BUF_SIZE, "%s", "Unknown" );
            }
            QT_CLOSE( cmdlineFd );
        }

        syslog( LOG_ERR | LOG_LOCAL6, "%s // PID:%u // ProgId:%u // Exe:%s // Request:%s // Cmdline:%s",
                "<SXE Breach>", d.processId, d.progId, linkTarget, qPrintable(request), cmdline);
    }

    return isAuthorized;
}

inline bool __fileOpen( QFile *f )
{
#ifdef QTRANSPORTAUTH_DEBUG
    if ( f->open( QIODevice::ReadOnly ))
    {
        qDebug( "Opened file: %s\n", qPrintable( f->fileName() ));
        return true;
    }
    else
    {
        qWarning( "Could not open file: %s\n", qPrintable( f->fileName() ));
        return false;
    }
#else
    return ( f->open( QIODevice::ReadOnly ));
#endif
}

/*!
  \internal
  Find client keys for the \a progId.  If it is cached should be very
  fast, otherwise requires a read of the secret key file

  In the success case a pointer to the keys is returned.  The pointer is
  to storage allocated for the internal cache and must be used asap.

  The list returned is a sequence of one or more keys which match the
  progId.  There is no separator, each 16 byte sequence represents a key.
  The sequence is followed by two iterations of the SXE magic
  bytes,eg  0xBA, 0xD4, 0xD4, 0xBA, 0xBA, 0xD4, 0xD4, 0xBA

  NULL is returned in the following cases:
  \list
    \o the keyfiles could not be accessed - error condition
    \o there was no key for the supplied program id - key auth failed
  \endlist

  Note that for the keyfiles, there is multi-thread and multi-process
  concurrency issues: they can be read by the qpe process when
  QTransportAuth calls getClientKey to verify a request, and they can be
  read or written by the packagemanager when updating package data.

  To protect against this, the keyfileMutex & SxeRegistryLocker is used.

  The sxe_installer tool can also update inode and device numbers in
  the manifest file, but this only occurs outside of normal operation,
  so qpe and packagemanager are never running when this occurs.
*/
const unsigned char *QTransportAuthPrivate::getClientKey(unsigned char progId)
{
    int manifestMatchCount = 0;
    struct IdBlock mr;
    int total_size = 0;
    char *result = 0;
    char *result_ptr;
    int keysFound = 0;
    bool foundKey;
    int keysRead = 0;
    struct usr_key_entry keys_list[128];

    if ( keyCache.contains( progId ))
        return (const unsigned char *)keyCache[progId];

    SxeRegistryLocker rlock( m_packageRegistry );

    // ### Qt 4.3: this is hacky - see documentation for setKeyFilePath
    QString manifestPath = m_keyFilePath + QLatin1String("/manifest");
    QString actualKeyPath = QLatin1String("/proc/lids/keys");
    bool noFailOnKeyMissing = true;
    if ( !QFile::exists( actualKeyPath )) {
        actualKeyPath = m_keyFilePath + QLatin1String( "/" QSXE_KEYFILE );
    }
    QFile kf( actualKeyPath );
    QFile mn( manifestPath );
    if ( !__fileOpen( &mn ))
        goto key_not_found;
    // first find how much storage is needed
    while ( mn.read( (char*)&mr, sizeof(struct IdBlock)) > 0 )
        if ( mr.progId == progId )
            manifestMatchCount++;
    if ( manifestMatchCount == 0 )
        goto key_not_found;
    if ( !__fileOpen( &kf ))
    {
        noFailOnKeyMissing = false;
        goto key_not_found;
    }
    total_size = 2 * QSXE_MAGIC_BYTES + manifestMatchCount * QSXE_KEY_LEN;
    result = (char*)malloc( total_size );
    Q_CHECK_PTR( result );
    mn.seek( 0 );
    result_ptr = result;
    /* reading whole key array in is much more efficient, 99% case is this loop only
       executes once, should not have more than 128 keyed items */
    while (( keysRead = kf.read( (char*)keys_list, sizeof(struct usr_key_entry)*128 )) > 0 )
    {
        /* qDebug("PID %d: getClientKey() - read %d bytes = %d keys from %s", getpid(), keysRead,
                keysRead/sizeof(struct usr_key_entry), qPrintable(actualKeyPath)); */
        keysRead /= sizeof(struct usr_key_entry);
        while ( mn.read( (char*)&mr, sizeof(struct IdBlock)) > 0 )
        {
            if ( mr.progId == progId )
            {
                foundKey = false;
                for ( int i = 0; i < keysRead; ++i )
                {
                    /* if ( i == 0 )
                        qDebug() << "         pid" << getpid() << "looking for device"  << (dev_t)mr.device << "inode" << (ino_t)mr.inode;
                    qDebug() << "         pid" << getpid() << "trying device" <<  keys_list[i].dev << "inode" <<  keys_list[i].ino; */
                    if ( keys_list[i].ino == (ino_t)mr.inode && keys_list[i].dev == (dev_t)mr.device )
                    {
                        memcpy( result_ptr, keys_list[i].key, QSXE_KEY_LEN );
                        result_ptr += QSXE_KEY_LEN;
                        foundKey = true;
                        break;
                    }
                }
                if ( foundKey )
                {
                    keysFound++;
                    if ( keysFound == manifestMatchCount )
                        break;
                }
            }
        }
    }
    if ( result_ptr == result ) // nothing found!
        goto key_not_found;
    // 2 x magic bytes sentinel at end of sequence
    for ( int i = 0; i < 2; ++i )
        for ( int j = 0; j < QSXE_MAGIC_BYTES; ++j )
            *result_ptr++ = magic[j];
    keyCache.insert( progId, result, total_size / 10 );
    /* qDebug( "PID %d : Found %d client keys for prog %u", getpid(), keysFound, progId ); */
    goto success_out;

key_not_found:
    if ( noFailOnKeyMissing )  // return an "empty" set of keys in this case
    {
        if ( result == 0 )
        {
            result = (char*)malloc( 2 * QSXE_MAGIC_BYTES );
            Q_CHECK_PTR( result );
        }
        result_ptr = result;
        for ( int i = 0; i < 2; ++i )
            for ( int j = 0; j < QSXE_MAGIC_BYTES; ++j )
                *result_ptr++ = magic[j];
        return (unsigned char *)result;
    }
    qWarning( "PID %d : Not found client key for prog %u", getpid(), progId );
    if ( result )
    {
        free( result );
        result = 0;
    }
success_out:
    if ( mn.isOpen() )
        mn.close();
    if ( kf.isOpen() )
        kf.close();
    return (unsigned char *)result;
}

void QTransportAuthPrivate::invalidateClientKeyCache()
{
    keyfileMutex.lock();
    keyCache.clear();
    keyfileMutex.unlock();
}

////////////////////////////////////////////////////////////////////////
////
////  RequestAnalyzer definition
////


RequestAnalyzer::RequestAnalyzer()
    : moreData( false )
    , dataSize( 0 )
{
}

RequestAnalyzer::~RequestAnalyzer()
{
}

/*!
  Analzye the data in the\a msgQueue according to some protocol
  and produce a request string for policy analysis.

  If enough data is in the queue for analysis of a complete message,
  return a non-null string, and set a flag so requireMoreData() will
  return false; otherwise return a null string and requireMoreData()
  return true.

  The amount of bytes analyzed is then available via bytesAnalyzed().

  A null string is also returned in the case where the message was
  corrupt and could not be analyzed.  In this case requireMoreData()
  returns false.

Note: this method will modify the msgQueue and pull off the data
  deemed to be corrupt, in the case of corrupt data.

  In all other cases the msgQueue is left alone.  The calling code
  should then pull off the analyzed data.  Use bytesAnalzyed() to
  find how much data to pull off the queue.
*/
QString RequestAnalyzer::analyze( QByteArray *msgQueue )
{
#ifdef Q_WS_QWS
    dataSize = 0;
    moreData = false;
    QBuffer cmdBuf( msgQueue );
    cmdBuf.open( QIODevice::ReadOnly | QIODevice::Unbuffered );
    QWSCommand::Type command_type = (QWSCommand::Type)(qws_read_uint( &cmdBuf ));
    QWSCommand *command = QWSCommand::factory(command_type);
    // if NULL, factory will have already printed warning for bogus
    // command_type just purge the bad stuff and attempt to recover
    if ( command == NULL )
    {
        *msgQueue = msgQueue->mid( sizeof(int) );
        return QString();
    }
    QString request = QLatin1String(qws_getCommandTypeString(command_type));
#ifndef QT_NO_COP
    if ( !command->read( &cmdBuf ))
    {
        // not all command arrived yet - come back later
        delete command;
        moreData = true;
        return QString();
    }
    if ( command_type == QWSCommand::QCopSend )
    {
        QWSQCopSendCommand *sendCommand = static_cast<QWSQCopSendCommand*>(command);
        request += QString::fromLatin1("/QCop/%1/%2").arg( sendCommand->channel ).arg( sendCommand->message );
    }
    if ( command_type == QWSCommand::QCopRegisterChannel )
    {
        QWSQCopRegisterChannelCommand *registerCommand = static_cast<QWSQCopRegisterChannelCommand*>(command);
        request += QString::fromLatin1("/QCop/RegisterChannel/%1").arg( registerCommand->channel );
    }
#endif
    dataSize = QWS_PROTOCOL_ITEM_SIZE( *command );
    delete command;
    return request;
#else
    Q_UNUSED(msgQueue);
    return QString();
#endif
}

////////////////////////////////////////////////////////////////////////
////
////  AuthDevice definition
////

/*!
  Constructs a new auth device for the transport \a data and I/O device \a parent.

  Incoming or outgoing data will be authenticated according to the auth direction \a dir.

  The auth device will take ownership of the transport \a data and delete it when the device
  is destroyed.
*/
QAuthDevice::QAuthDevice( QIODevice *parent, QTransportAuth::Data *data, AuthDirection dir )
    : QIODevice( parent )
    , d( data )
    , way( dir )
    , m_target( parent )
    , m_client( 0 )
    , m_bytesAvailable( 0 )
    , m_skipWritten( 0 )
    , analyzer( 0 )
{
    if ( dir == Receive ) // server side
    {
        connect( m_target, SIGNAL(readyRead()),
                this, SLOT(recvReadyRead()));
    } else {
        connect( m_target, SIGNAL(readyRead()),
                this, SIGNAL(readyRead()));
    }
    connect( m_target, SIGNAL(bytesWritten(qint64)),
            this, SLOT(targetBytesWritten(qint64)) );
    open( QIODevice::ReadWrite | QIODevice::Unbuffered );
}

QAuthDevice::~QAuthDevice()
{
    if ( analyzer )
        delete analyzer;
    delete d;
}

/*!
  \internal
  Store a pointer to the related device or instance which this
  authorizer is proxying for
*/
void QAuthDevice::setClient( QObject *cli )
{
    m_client = cli;
    QTransportAuth::getInstance()->d_func()->buffersByClient[cli] = this;
    QObject::connect( cli, SIGNAL(destroyed(QObject*)),
            QTransportAuth::getInstance(), SLOT(bufferDestroyed(QObject*)) );
    // qDebug( "@@@@@@@@@@@@ client set %p @@@@@@@@@", cli );
    // qDebug( "           client count %d", QTransportAuth::getInstance()->d_func()->buffersByClient.count() );
}

QObject *QAuthDevice::client() const
{
    return m_client;
}

/*
  \fn void QAuthDevice::authViolation(QTransportAuth::Data &)

  This signal is emitted if an authorization failure is generated, as
  described in checkAuth();

  \sa checkAuth()
*/


/*
  \fn void QAuthDevice::policyCheck(QTransportAuth::Data &transport, const QString &request )

  This signal is emitted when a transport successfully delivers a request
  and gives the opportunity to either deny or accept the request.

  This signal must be connected in the same thread, ie it cannot be queued.

  As soon as all handlers connected to this signal are processed the Allow or
  Deny state on the \a transport is checked, and the request is allowed or denied
  accordingly.

  \sa checkAuth()
*/

/*!
  \internal
  Reimplement QIODevice writeData method.

  For client end, when the device is written to the incoming data is
  processed and an authentication header calculated.  This is pushed
  into the target device, followed by the actual incoming data (the
  payload).

  For server end, it is a fatal error to write to the device.
*/
qint64 QAuthDevice::writeData(const char *data, qint64 len)
{
    if ( way == Receive )  // server
        return m_target->write( data, len );
    // client
#ifdef QTRANSPORTAUTH_DEBUG
    char displaybuf[1024];
#endif
    char header[QSXE_HEADER_LEN];
    ::memset( header, 0, QSXE_HEADER_LEN );
    qint64 bytes = 0;
    if ( QTransportAuth::getInstance()->authToMessage( *d, header, data, len ))
    {
        m_target->write( header, QSXE_HEADER_LEN );
#ifdef QTRANSPORTAUTH_DEBUG
        hexstring( displaybuf, (const unsigned char *)header, QSXE_HEADER_LEN );
        qDebug( "%d QAuthDevice::writeData - CLIENT: Header written: %s", getpid(), displaybuf );
#endif
        m_skipWritten += QSXE_HEADER_LEN;
    }
    m_target->write( data, len );
    bytes += len;
#ifdef QTRANSPORTAUTH_DEBUG
    int bytesToDisplay = bytes;
    const unsigned char *dataptr = (const unsigned char *)data;
    while ( bytesToDisplay > 0 )
    {
        int amt = bytes < 500 ? bytes : 500;
        hexstring( displaybuf, dataptr, amt );
        qDebug( "%d QAuthDevice::writeData - CLIENT: %s", getpid(), bytes > 0 ? displaybuf : "(null)" );
        dataptr += 500;
        bytesToDisplay -= 500;
    }
#endif
    if ( m_target->inherits( "QAbstractSocket" ))
        static_cast<QAbstractSocket*>(m_target)->flush();
    return bytes;
}

/*!
  Reimplement from QIODevice

  Read data out of the internal message queue, reduce the queue by the amount
  read.  Note that the amount available is only ever the size of a command
  (although a command can be very big) since we need to check at command
  boundaries for new authentication headers.
*/
qint64 QAuthDevice::readData( char *data, qint64 maxSize )
{
    if ( way == Send )  // client
        return m_target->read( data, maxSize );
    if ( msgQueue.size() == 0 )
        return 0;
#ifdef QTRANSPORTAUTH_DEBUG
    char displaybuf[1024];
    hexstring( displaybuf, reinterpret_cast<const unsigned char *>(msgQueue.constData()),
            msgQueue.size() > 500 ? 500 : msgQueue.size() );
    qDebug() << getpid() << "QAuthDevice::readData() buffered/requested/avail"
            << msgQueue.size() << maxSize << m_bytesAvailable << displaybuf;
#endif
    Q_ASSERT( m_bytesAvailable <= msgQueue.size() );
    qint64 bytes = ( maxSize > m_bytesAvailable ) ? m_bytesAvailable : maxSize;
    ::memcpy( data, msgQueue.constData(), bytes );
    msgQueue = msgQueue.mid( bytes );
    m_bytesAvailable -= bytes;
    return bytes;
}

/*!
  \internal
  Receive readyRead signal from the target recv device.  In response
  authorize the data, and write results out to the recvBuf() device
  for processing by the application.  Trigger the readyRead signal.

  Authorizing involves first checking the transport is valid, ie the
  handshake has either already been done and is cached on a trusted
  transport, or was valid with this message; then second passing the
  string representation of the service request up to any policyReceivers

  If either of these fail, the message is denied.  In discovery mode
  denied messages are allowed, but the message is logged.
*/
void QAuthDevice::recvReadyRead()
{
    qint64 bytes = m_target->bytesAvailable();
    if ( bytes <= 0 ) return;
    open( QIODevice::ReadWrite | QIODevice::Unbuffered );
    QUnixSocket *usock = static_cast<QUnixSocket*>(m_target);
    QUnixSocketMessage msg = usock->read();
    msgQueue.append( msg.bytes() );
    d->processId = msg.processId();
    // if "fragmented" packet 1/2 way through start of a command, ie
    // in the QWS msg type, cant do anything, come back later when
    // there's more of the packet
    if ( msgQueue.size() < (int)sizeof(int) )
    {
        // qDebug() << "returning: msg size too small" << msgQueue.size();
        return;
    }
#ifdef QTRANSPORTAUTH_DEBUG
    char displaybuf[1024];
    hexstring( displaybuf, reinterpret_cast<const unsigned char *>(msgQueue.constData()),
            msgQueue.size() > 500 ? 500 : msgQueue.size() );
    qDebug( "%d ***** SERVER read %lli bytes - msg %s", getpid(), bytes, displaybuf );
#endif

    bool bufHasMessages = msgQueue.size() >= (int)sizeof(int);
    while ( bufHasMessages )
    {
        unsigned char saveStatus = d->status;
        if (( d->status & QTransportAuth::ErrMask ) == QTransportAuth::NoSuchKey )
        {
            QTransportAuth::getInstance()->authorizeRequest( *d, QLatin1String("NoSuchKey") );
            break;
        }
        if ( !QTransportAuth::getInstance()->authFromMessage( *d, msgQueue, msgQueue.size() ))
        {
            // not all arrived yet?  come back later
            if (( d->status & QTransportAuth::ErrMask ) == QTransportAuth::TooSmall )
            {
                d->status = saveStatus;
                return;
            }
        }
        if (( d->status & QTransportAuth::ErrMask ) == QTransportAuth::NoMagic )
        {
            // no msg auth header, don't change the success status for connections
            if ( d->connection() )
                d->status = saveStatus;
        }
        else
        {
            // msg auth header detected and auth determined, remove hdr
            msgQueue = msgQueue.mid( QSXE_HEADER_LEN );
        }
        if ( !authorizeMessage() )
            break;
        bufHasMessages = msgQueue.size() >= (int)sizeof(int);
    }
}

/**
  \internal
  Handle bytesWritten signals from the underlying target device.
  We adjust the target's value for bytes that are part of auth packets.
*/
void QAuthDevice::targetBytesWritten( qint64 bytes )
{
    if ( m_skipWritten >= bytes ) {
        m_skipWritten -= bytes;
        bytes = 0;
    } else if ( m_skipWritten > 0 ) {
        bytes -= m_skipWritten;
        m_skipWritten = 0;
    }
    if ( bytes > 0 ) {
        emit bytesWritten( bytes );
    }
}

/**
  \internal
  Pre-process the message to determine what QWS command it is.  This
  information is used as the "request" for the purposes of authorization.

  The request and other data on the connection (id, PID, etc.) are forwarded
  to all policy listeners by emitting a signal.

  The signal must be processed synchronously because on return the allow/deny
  status is used immediately to either drop or continue processing the message.
*/
bool QAuthDevice::authorizeMessage()
{
    if ( analyzer == NULL )
        analyzer = new RequestAnalyzer();
    QString request = (*analyzer)( &msgQueue );
    if ( analyzer->requireMoreData() )
        return false;
    bool isAuthorized = true;

    if ( !request.isEmpty() && request != QLatin1String("Unknown") )
    {
        isAuthorized = QTransportAuth::getInstance()->authorizeRequest( *d, request );
    }

    bool moreToProcess = ( msgQueue.size() - analyzer->bytesAnalyzed() ) > (int)sizeof(int);
    if ( isAuthorized )
    {
#ifdef QTRANSPORTAUTH_DEBUG
        qDebug() << getpid() << "SERVER authorized: releasing" << analyzer->bytesAnalyzed() << "byte command" << request;
#endif
        m_bytesAvailable = analyzer->bytesAnalyzed();
        emit QIODevice::readyRead();
        return moreToProcess;
    }
    else
    {
        msgQueue = msgQueue.mid( analyzer->bytesAnalyzed() );
    }

    return true;
}

void QAuthDevice::setRequestAnalyzer( RequestAnalyzer *ra )
{
    Q_ASSERT( ra );
    if ( analyzer )
        delete analyzer;
    analyzer = ra;
}

/*!
  \internal
   Add authentication header to the beginning of a message

   Note that the per-process auth cookie is used.  This key should be rewritten in
   the binary image of the executable at install time to make it unique.

   For this to be secure some mechanism (eg MAC kernel or other
   permissions) must prevent other processes from reading the key.

   The buffer must have AUTH_SPACE(0) bytes spare at the beginning for the
   authentication header to be added.

   Returns true if header successfully added.  Will fail if the
   per-process key has not yet been set with setProcessKey()
*/
bool QTransportAuth::authToMessage( QTransportAuth::Data &d, char *hdr, const char *msg, int msgLen )
{
    // qDebug( "authToMessage(): prog id %u", d.progId );
    // only authorize connection oriented transports once, unless key has changed
    if ( d.connection() && ((d.status & QTransportAuth::ErrMask) != QTransportAuth::Pending) &&
        d_func()->authKey.progId == d.progId )
        return false;
    d.progId = d_func()->authKey.progId;
    // If Unix socket credentials are being used the key wont be set
    if ( !d_func()->keyInitialised )
        return false;
    unsigned char digest[QSXE_KEY_LEN];
    char *msgPtr = hdr;
    // magic always goes on the beginning
    for ( int m = 0; m < QSXE_MAGIC_BYTES; ++m )
        *msgPtr++ = magic[m];
    hdr[ QSXE_LEN_IDX ] = (unsigned char)msgLen;
    if ( !d.trusted())
    {
        // Use HMAC
        int rc = hmac_md5( (unsigned char *)msg, msgLen, d_func()->authKey.key, QSXE_KEY_LEN, digest );
        if ( rc == -1 )
            return false;
        memcpy( hdr + QSXE_KEY_IDX, digest, QSXE_KEY_LEN );
    }
    else
    {
        memcpy( hdr + QSXE_KEY_IDX, d_func()->authKey.key, QSXE_KEY_LEN );
    }

    hdr[ QSXE_PROG_IDX ] = d_func()->authKey.progId;

#ifdef QTRANSPORTAUTH_DEBUG
    char keydisplay[QSXE_KEY_LEN*2+1];
    hexstring( keydisplay, d_func()->authKey.key, QSXE_KEY_LEN );

    qDebug( "%d CLIENT Auth to message %s against prog id %u and key %s\n",
            getpid(), msg, d_func()->authKey.progId, keydisplay );
#endif

    // TODO implement sequence to prevent replay attack, not required
    // for trusted transports
    hdr[ QSXE_SEQ_IDX ] = 1;  // dummy sequence

    d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::Success;
    return true;
}


/*!
  Check authorization on the \a msg, which must be of size \a msgLen,
  for the transport \a d.

  If able to determine authorization, return the program identity of
  the message source in the reference \a progId, and return true.

  Otherwise return false.

  If data is being received on a socket, it may be that more data is yet
  needed before authentication can proceed.

  Also the message may not be an authenticated at all.

  In these cases the method returns false to indicate authorization could
  not be determined:
  \list
    \i The message is too small to carry the authentication data
       (status TooSmall is set on the \a d transport )
    \i The 4 magic bytes are missing from the message start
       (status NoMagic is set on the \a d transport )
    \i The message is too small to carry the auth + claimed payload
       (status TooSmall is set on the \a d transport )
  \endlist

  If however the authentication header (preceded by the magic bytes) and
  any authenticated payload is received the method will determine the
  authentication status, and return true.

  In the following cases as well as returning true it will also emit
  an authViolation():
  \list
    \i If the program id claimed by the message is not found in the key file
       (status NoSuchKey is set on the \a d transport )
    \i The authentication token failed against the claimed program id:
        \list
            \i in the case of trusted transports, the secret did not match
            \i in the case of untrusted transports the HMAC code did not match
        \endlist
       (status FailMatch is set on the \a d transport )
    \endlist

  In these cases the authViolation( QTransportAuth::Data d ) signal is emitted
  and the error string can be obtained from the status like this:
  \snippet doc/src/snippets/code/src_gui_embedded_qtransportauth_qws.cpp 4
*/
bool QTransportAuth::authFromMessage( QTransportAuth::Data &d, const char *msg, int msgLen )
{
    if ( msgLen < QSXE_MAGIC_BYTES )
    {
        d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::TooSmall;
        return false;
    }
    // if no magic bytes, exit straight away
    int m;
    const unsigned char *mptr = reinterpret_cast<const unsigned char *>(msg);
    for ( m = 0; m < QSXE_MAGIC_BYTES; ++m )
    {
        if ( *mptr++ != magic[m] )
        {
            d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::NoMagic;
            return false;
        }
    }

    if ( msgLen < AUTH_SPACE(1) )
    {
        d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::TooSmall;
        return false;
    }

    // At this point we know the header is at least long enough to contain valid auth
    // data, however the data may be spoofed.  If it is not verified then the status will
    // be set to uncertified so the spoofed data will not be relied on.  However we want to
    // know the program id which is being reported (even if it might be spoofed) for
    // policy debugging purposes.  So set it here, rather than after verification.
    d.progId = msg[QSXE_PROG_IDX];

#ifdef QTRANSPORTAUTH_DEBUG
    char authhdr[QSXE_HEADER_LEN*2+1];
    hexstring( authhdr, reinterpret_cast<const unsigned char *>(msg), QSXE_HEADER_LEN );
    qDebug( "%d SERVER authFromMessage(): message header is %s",
            getpid(), authhdr );
#endif

    unsigned char authLen = (unsigned char)(msg[ QSXE_LEN_IDX ]);

    if ( msgLen < AUTH_SPACE(authLen) )
    {
        d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::TooSmall;
        return false;
    }

    bool isCached = d_func()->keyCache.contains( d.progId );
    const unsigned char *clientKey = d_func()->getClientKey( d.progId );
    if ( clientKey == NULL )
    {
        d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::NoSuchKey;
        return false;
    }

#ifdef QTRANSPORTAUTH_DEBUG
    char keydisplay[QSXE_KEY_LEN*2+1];
    hexstring( keydisplay, clientKey, QSXE_KEY_LEN );
    qDebug( "\t\tauthFromMessage(): message %s against prog id %u and key %s\n",
            AUTH_DATA(msg), ((unsigned int)d.progId), keydisplay );
#endif

    const unsigned char *auth_tok;
    unsigned char digest[QSXE_KEY_LEN];
    bool multi_tok = false;

    bool need_to_recheck=false;
    do
    {
        if ( !d.trusted())
        {
            hmac_md5( AUTH_DATA(msg), authLen, clientKey, QSXE_KEY_LEN, digest );
            auth_tok = digest;
        }
        else
        {
            auth_tok = clientKey;
            multi_tok = true;  // 1 or more keys are in the clientKey
        }
        while( true )
        {
            if ( memcmp( auth_tok, magic, QSXE_MAGIC_BYTES ) == 0
                    && memcmp( auth_tok + QSXE_MAGIC_BYTES, magic, QSXE_MAGIC_BYTES ) == 0 )
                break;
            if ( memcmp( msg + QSXE_KEY_IDX, auth_tok, QSXE_KEY_LEN ) == 0 )
            {
                d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::Success;
                return true;
            }
            if ( !multi_tok )
                break;
            auth_tok += QSXE_KEY_LEN;
        }
        //the keys cached on d.progId may not contain the binary key because the cache entry was made
        //before the binary had first started, must search for client key again.
        if ( isCached )
        {
            d_func()->keyCache.remove(d.progId);
            isCached = false;

#ifdef QTRANSPORTAUTH_DEBUG
            qDebug() << "QTransportAuth::authFromMessage(): key not found in set of keys cached"
                     << "against prog Id =" << d.progId << ". Re-obtaining client key. ";
#endif
            clientKey = d_func()->getClientKey( d.progId );
            if ( clientKey == NULL )
            {
                d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::NoSuchKey;
                return false;
            }
            need_to_recheck = true;
        }
        else
        {
            need_to_recheck = false;
        }
    } while( need_to_recheck );

    d.status = ( d.status & QTransportAuth::StatusMask ) | QTransportAuth::FailMatch;
    qWarning() << "QTransportAuth::authFromMessage():failed authentication";
    FAREnforcer::getInstance()->logAuthAttempt( QDateTime::currentDateTime() );
    emit authViolation( d );
    return false;
}


#ifdef QTRANSPORTAUTH_DEBUG
/*!
  sprintf into hex - dest \a buf, src \a key, \a key_len is length of key.

  The target buf should be [ key_len * 2 + 1 ] in size
*/
void hexstring( char *buf, const unsigned char* key, size_t key_len )
{
    unsigned int i, p;
    for ( i = 0, p = 0; i < key_len; i++, p+=2 )
    {
        unsigned char lo_nibble = key[i] & 0x0f;
        unsigned char hi_nibble = key[i] >> 4;
        buf[p] = (int)hi_nibble > 9 ? hi_nibble-10 + 'A' : hi_nibble + '0';
        buf[p+1] = (int)lo_nibble > 9 ? lo_nibble-10 + 'A' : lo_nibble + '0';
    }
    buf[p] = '\0';
}
#endif

/*
  HMAC MD5 as listed in RFC 2104

  This code is taken from:

      http://www.faqs.org/rfcs/rfc2104.html

  with the allowance for keys other than length 16 removed, but otherwise
  a straight cut-and-paste.

  The HMAC_MD5 transform looks like:

  \snippet doc/src/snippets/code/src.gui.embedded.qtransportauth_qws.cpp 5

  \list
    \i where K is an n byte key
    \i ipad is the byte 0x36 repeated 64 times
    \i opad is the byte 0x5c repeated 64 times
    \i and text is the data being protected
  \endlist

  Hardware is available with accelerated implementations of HMAC-MD5 and
  HMAC-SHA1.  Where this hardware is available, this routine should be
  replaced with a call into the accelerated version.
*/

static int hmac_md5(
        unsigned char*  text,         /* pointer to data stream */
        int             text_length,  /* length of data stream */
        const unsigned char*  key,    /* pointer to authentication key */
        int             key_length,   /* length of authentication key */
        unsigned char * digest        /* caller digest to be filled in */
        )
{
        MD5Context context;
        unsigned char k_ipad[65];    /* inner padding - * key XORd with ipad */
        unsigned char k_opad[65];    /* outer padding - * key XORd with opad */
        int i;

        /* in this implementation key_length == 16 */
        if ( key_length != 16 )
        {
            fprintf( stderr, "Key length was %d - must be 16 bytes", key_length );
            return 0;
        }

        /* start out by storing key in pads */
        memset( k_ipad, 0, sizeof k_ipad );
        memset( k_opad, 0, sizeof k_opad );
        memcpy( k_ipad, key, key_length );
        memcpy( k_opad, key, key_length );

        /* XOR key with ipad and opad values */
        for (i=0; i<64; i++) {
                k_ipad[i] ^= 0x36;
                k_opad[i] ^= 0x5c;
        }

        /* perform inner MD5 */
        MD5Init(&context);                   /* init context for 1st pass */
        MD5Update(&context, k_ipad, 64);     /* start with inner pad */
        MD5Update(&context, text, text_length); /* then text of datagram */
        MD5Final(&context, digest);          /* finish up 1st pass */

        /* perform outer MD5 */
        MD5Init(&context);                   /* init context for 2nd pass */
        MD5Update(&context, k_opad, 64);     /* start with outer pad */
        MD5Update(&context, digest, 16);     /* then results of 1st * hash */
        MD5Final(&context, digest);          /* finish up 2nd pass */
        return 1;
}


const int FAREnforcer::minutelyRate = 4; //allowed number of false authentication attempts per minute
const QString FAREnforcer::FARMessage = QLatin1String("FAR_Exceeded");
const QString FAREnforcer::SxeTag = QLatin1String("<SXE Breach>");
const int FAREnforcer::minute = 60;

FAREnforcer::FAREnforcer():authAttempts()
{
    QDateTime nullDateTime = QDateTime();
    for (int i = 0; i < minutelyRate; i++ )
        authAttempts << nullDateTime;
}


FAREnforcer *FAREnforcer::getInstance()
{
    static FAREnforcer theInstance;
    return &theInstance;
}

void FAREnforcer::logAuthAttempt( QDateTime time )
{
    QDateTime dt =  authAttempts.takeFirst();

    authAttempts.append( time );
    if ( dt.secsTo( authAttempts.last() ) <= minute )
    {
#if defined(SXE_DISCOVERY)
        if ( QTransportAuth::getInstance()->isDiscoveryMode() ) {
            static QBasicAtomicInt reported = Q_BASIC_ATOMIC_INITIALIZER(0);
            if ( reported.testAndSetRelaxed(0,1) ) {
#ifndef QT_NO_TEXTSTREAM
                QString logFilePath = QTransportAuth::getInstance()->logFilePath();
                if ( !logFilePath.isEmpty() ) {
                      QFile log( logFilePath );
                    if ( !log.open(QIODevice::WriteOnly | QIODevice::Append) ) {
                        qWarning("Could not write to log in discovery mode: %s",
                                 qPrintable(logFilePath) );
                    } else {
                        QTextStream ts( &log );
                        ts << "\t\tWarning: False Authentication Rate of " <<  minutelyRate << "\n"
                           << "\t\tserver connections/authentications per minute has been exceeded,\n"
                           << "\t\tno further warnings will be issued\n";
                    }
                }
            }
#endif
            reset();
            return;
        }
#endif
        syslog( LOG_ERR | LOG_LOCAL6, "%s %s",
                qPrintable( FAREnforcer::SxeTag ), 
                qPrintable( FAREnforcer::FARMessage ) );
        reset();
    }
}

void FAREnforcer::reset()
{
    QDateTime nullDateTime = QDateTime();
    for (int i = 0; i < minutelyRate; i++ )
        authAttempts[i] = nullDateTime;
}

QT_END_NAMESPACE

#include "moc_qtransportauth_qws_p.cpp"

#endif // QT_NO_SXE
