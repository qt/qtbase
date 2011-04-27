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

#include "qunixsocket_p.h"

// #define QUNIXSOCKET_DEBUG 1

#include <QtCore/qsocketnotifier.h>
#include <QtCore/qqueue.h>
#include <QtCore/qdatetime.h>
#include "private/qcore_unix_p.h" // overrides QT_OPEN

#ifdef QUNIXSOCKET_DEBUG
#include <QtCore/qdebug.h>
#endif

extern "C" {
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
};

#define UNIX_PATH_MAX 108 // From unix(7)

#ifdef QT_LINUXBASE
// LSB doesn't declare ucred
struct ucred
{
    pid_t pid;                    /* PID of sending process.  */
    uid_t uid;                    /* UID of sending process.  */
    gid_t gid;                    /* GID of sending process.  */
};

// LSB doesn't define the ones below
#ifndef SO_PASSCRED
#  define SO_PASSCRED   16
#endif
#ifndef SCM_CREDENTIALS
#  define SCM_CREDENTIALS 0x02
#endif
#ifndef MSG_DONTWAIT
#  define MSG_DONTWAIT 0x40
#endif
#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL 0x4000
#endif

#endif // QT_LINUXBASE

QT_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
// class QUnixSocketRights
///////////////////////////////////////////////////////////////////////////////
/*!
  \class QUnixSocketRights
  \internal

  \brief The QUnixSocketRights class encapsulates QUnixSocket rights data.
  \omit
  \ingroup Platform::DeviceSpecific
  \ingroup Platform::OS
  \ingroup Platform::Communications
  \endomit
  \ingroup qws

  \l QUnixSocket allows you to transfer Unix file descriptors between processes.
  A file descriptor is referred to as "rights data" as it allows one process to
  transfer its right to access a resource to another.

  The Unix system verifies resource permissions only when the resource is first
  opened.  For example, consider a file on disk readable only by the user "qt".
  A process running as user "qt" will be able to open this file for reading.
  If, while the process was still reading from the file, the ownership was
  changed from user "qt" to user "root", the process would be allowed to
  continue reading from the file, even though attempting to reopen the file
  would be denied.  Permissions are associated with special descriptors called
  file descriptors which are returned to a process after it initially opens a
  resource.

  File descriptors can be duplicated within a process through the dup(2) system
  call.  File descriptors can be passed between processes using the
  \l QUnixSocket class in the same way.  Even though the receiving process never
  opened the resource directly, it has the same permissions to access it as the
  process that did.

  \sa QUnixSocket
 */
struct QUnixSocketRightsPrivate : public QSharedData
{
    virtual ~QUnixSocketRightsPrivate() {
#ifdef QUNIXSOCKET_DEBUG
        int closerv =
#endif
            QT_CLOSE(fd);
#ifdef QUNIXSOCKET_DEBUG
        if(0 != closerv) {
            qDebug() << "QUnixSocketRightsPrivate: Unable to close managed"
                        " file descriptor (" << ::strerror(errno) << ')';
        }
#endif
    }

    int fd;
};

/*!
  Create a new QUnixSocketRights instance containing the file descriptor \a fd.
  \a fd will be dup(2)'d internally, so the application is free to close \a fd
  following this call.

  If the dup(2) fails, or you pass an invalid \a fd, an
  \l {QUnixSocketRights::isValid()}{invalid } object will be
  constructed.

  QUnixSocketRights instances are immutable and the internal file descriptor
  will be shared between any copies made of this object.  The system will
  close(2) the file descriptor once it is no longer needed.
  */
QUnixSocketRights::QUnixSocketRights(int fd)
{
    d = new QUnixSocketRightsPrivate();
    if(-1 == fd) {
        d->fd = -1;
    } else {
        d->fd = qt_safe_dup(fd);
#ifdef QUNIXSOCKET_DEBUG
        if(-1 == d->fd) {
            qDebug() << "QUnixSocketRights: Unable to duplicate fd "
                     << fd << " (" << ::strerror(errno) << ')';
        }
#endif
    }
}

/*!
  \internal

  Construct a QUnixSocketRights instance on \a fd without dup(2)'ing the file
  descriptor.
  */
QUnixSocketRights::QUnixSocketRights(int fd,int)
{
    Q_ASSERT(-1 != fd);
    d = new QUnixSocketRightsPrivate();
    d->fd = fd;
}

/*!
  Destroys the QUnixSocketRights instance.
  */
QUnixSocketRights::~QUnixSocketRights()
{
}

/*!
  Create a copy of \a other.
  */
QUnixSocketRights &
QUnixSocketRights::operator=(const QUnixSocketRights & other)
{
    d = other.d;
    return *this;
}

/*!
  Create a copy of \a other.
  */
QUnixSocketRights::QUnixSocketRights(const QUnixSocketRights & other)
: d(other.d)
{
}

/*!
  Returns true if this QUnixSocketRights instance is managing a valid file
  descriptor.  This method is equivalent to (-1 != peekFd()).

  \sa QUnixSocketRights::peekFd()
  */
bool QUnixSocketRights::isValid() const
{
    return d->fd != -1;
}

/*!
  Return a duplicate of the file descriptor contained in this object.  If this
  is an \l {QUnixSocketRights::isValid()}{invalid } object, or the
  dup(2) call fails, an invalid file descriptor (-1) will be returned.

  \sa QUnixSocketRights::peekFd()
  */
int QUnixSocketRights::dupFd() const
{
    if(-1 == d->fd) return -1;

    int rv = qt_safe_dup(d->fd);

#ifdef QUNIXSOCKET_DEBUG
    if(-1 == rv)
        qDebug() << "QUnixSocketRights: Unable to duplicate managed file "
                    "descriptor (" << ::strerror(errno) << ')';
#endif

    return rv;
}

/*!
  Returns the file descriptor contained in this object.  If this
  is an \l {QUnixSocketRights::isValid()}{invalid } object an invalid
  file descriptor (-1) will be returned.

  The lifetime of this file descriptor is tied to the lifetime of the
  QUnixSocketRights instance.  The file descriptor returned by this method
  \e may be close(2)'d when the QUnixSocketRights instance is destroyed.  If
  you want to continue to use the file descriptor use
  \l QUnixSocketRights::dupFd() instead.

  \sa QUnixSocketRights::dupFd()
  */
int QUnixSocketRights::peekFd() const
{
    return d->fd;
}

///////////////////////////////////////////////////////////////////////////////
// class QUnixSocketMessage
///////////////////////////////////////////////////////////////////////////////
struct QUnixSocketMessagePrivate : public QSharedData
{
    QUnixSocketMessagePrivate()
    : state(Default), vec(0), iovecLen(0), dataSize(0) {}
    QUnixSocketMessagePrivate(const QByteArray & b)
    : bytes(b), state(Default), vec(0), iovecLen(0), dataSize(0) {}
    QUnixSocketMessagePrivate(const QByteArray & b,
                              const QList<QUnixSocketRights> & r)
    : bytes(b), rights(r), state(Default), vec(0), iovecLen(0), dataSize(0) {}

    int size() const { return vec ? dataSize : bytes.size(); }
    void removeBytes( unsigned int );

    QByteArray bytes;
    QList<QUnixSocketRights> rights;

    enum AncillaryDataState {
        Default = 0x00,
        Truncated = 0x01,
        Credential = 0x02
    };
    AncillaryDataState state;

    pid_t pid;
    gid_t gid;
    uid_t uid;

    ::iovec *vec;
    int iovecLen;  // number of vectors in array
    int dataSize;  // total size of vectors = payload
};

/*!
  \internal
  Remove \a bytesToDequeue bytes from the front of this message
*/
void QUnixSocketMessagePrivate::removeBytes( unsigned int bytesToDequeue )
{
    if ( vec )
    {
        ::iovec *vecPtr = vec;
        if ( bytesToDequeue > (unsigned int)dataSize ) bytesToDequeue = dataSize;
        while ( bytesToDequeue > 0 && iovecLen > 0 )
        {
            if ( vecPtr->iov_len > bytesToDequeue )
            {
                // dequeue the bytes by taking them off the front of the
                // current vector.  since we don't own the iovec, its okay
                // to "leak" this away by pointing past it
                char **base = reinterpret_cast<char**>(&(vecPtr->iov_base));
                *base += bytesToDequeue;
                vecPtr->iov_len -= bytesToDequeue;
                bytesToDequeue = 0;
            }
            else
            {
                // dequeue bytes by skipping a whole vector.  again, its ok
                // to lose the pointers to this data
                bytesToDequeue -= vecPtr->iov_len;
                iovecLen--;
                vecPtr++;
            }
        }
        dataSize -= bytesToDequeue;
        if ( iovecLen == 0 ) vec = 0;
    }
    else
    {
        bytes.remove(0, bytesToDequeue );
    }
}


/*!
  \class QUnixSocketMessage
  \internal

  \brief The QUnixSocketMessage class encapsulates a message sent or received
  through the QUnixSocket class.
  \omit
  \ingroup Platform::DeviceSpecific
  \ingroup Platform::OS
  \ingroup Platform::Communications
  \endomit
  \ingroup qws

  In addition to transmitting regular byte stream data, messages sent over Unix
  domain sockets may have special ancillary properties.  QUnixSocketMessage
  instances allow programmers to retrieve and control these properties.

  Every QUnixSocketMessage sent has an associated set of credentials.  A
  message's credentials consist of the process id, the user id and the group id
  of the sending process.  Normally these credentials are set automatically for
  you by the QUnixSocketMessage class and can be queried by the receiving
  process using the \l QUnixSocketMessage::processId(),
  \l QUnixSocketMessage::userId() and \l QUnixSocketMessage::groupId() methods
  respectively.

  Advanced applications may wish to change the credentials that their message
  is sent with, and may do so though the \l QUnixSocketMessage::setProcessId(),
  \l QUnixSocketMessage::setUserId() and \l QUnixSocketMessage::setGroupId()
  methods.  The validity of these credentials is verified by the system kernel.
  Only the root user can send messages with credentials that are not his own.
  Sending of the message will fail for any non-root user who attempts to
  fabricate credentials.  Note that this failure is enforced by the system
  kernel - receivers can trust the accuracy of credential data!

  Unix domain socket messages may also be used to transmit Unix file descriptors
  between processes.  In this context, file descriptors are known as rights data
  and are encapsulated by the \l QUnixSocketRights class.  Senders can set the
  file descriptors to transmit using the \l QUnixSocketMessage::setRights() and
  receivers can retrieve this data through a call to
  \l QUnixSocketMessage::rights().  \l QUnixSocket and \l QUnixSocketRights
  discuss the specific copy and ordering semantic associated with rights data.

  QUnixSocketMessage messages are sent by the \l QUnixSocket::write() method.
  Like any normal network message, attempting to transmit an empty
  QUnixSocketMessage will succeed, but result in a no-op.  Limitations in the
  Unix domain protocol semantic will cause a transmission of a
  QUnixSocketMessage with rights data, but no byte data portion, to fail.

  \sa QUnixSocket QUnixSocketRights
  */

/*!
  Construct an empty QUnixSocketMessage.  This instance will have not data and
  no rights information.  The message's credentials will be set to the
  application's default credentials.
  */
QUnixSocketMessage::QUnixSocketMessage()
: d(new QUnixSocketMessagePrivate())
{
}

/*!
  Construct a QUnixSocketMessage with an initial data payload of \a bytes.  The
  message's credentials will be set to the application's default credentials.
  */
QUnixSocketMessage::QUnixSocketMessage(const QByteArray & bytes)
: d(new QUnixSocketMessagePrivate(bytes))
{
}

/*!
  Construct a QUnixSocketMessage with an initial data payload of \a bytes and
  an initial rights payload of \a rights.  The message's credentials will be set
  to the application's default credentials.

  A message with rights data but an empty data payload cannot be transmitted
  by the system.
  */
QUnixSocketMessage::QUnixSocketMessage(const QByteArray & bytes,
                                       const QList<QUnixSocketRights> & rights)
: d(new QUnixSocketMessagePrivate(bytes, rights))
{
}

/*!
  Create a copy of \a other.
  */
QUnixSocketMessage::QUnixSocketMessage(const QUnixSocketMessage & other)
: d(other.d)
{
}

/*!
  \fn  QUnixSocketMessage::QUnixSocketMessage(const iovec* data, int vecLen)

  Construct a QUnixSocketMessage with an initial data payload of \a
  data which points to an array of \a vecLen iovec structures.  The
  message's credentials will be set to the application's default
  credentials.

  This method can be used to avoid the overhead of copying buffers of data
  and will directly send the data pointed to by \a data on the socket.  It also
  avoids the syscall overhead of making a number of small socket write calls,
  if a number of data items can be delivered with one write.

  Caller must ensure the iovec * \a data remains valid until the message
  is flushed.  Caller retains ownership of the iovec structs.
  */
QUnixSocketMessage::QUnixSocketMessage(const ::iovec* data, int vecLen )
: d(new QUnixSocketMessagePrivate())
{
    for ( int v = 0; v < vecLen; v++ )
        d->dataSize += data[v].iov_len;
    d->vec = const_cast<iovec*>(data);
    d->iovecLen = vecLen;
}

/*!
  Assign the contents of \a other to this object.
  */
QUnixSocketMessage & QUnixSocketMessage::operator=(const QUnixSocketMessage & other)
{
    d = other.d;
    return *this;
}

/*!
  Destroy this instance.
  */
QUnixSocketMessage::~QUnixSocketMessage()
{
}

/*!
  Set the data portion of the message to \a bytes.

  \sa QUnixSocketMessage::bytes()
  */
void QUnixSocketMessage::setBytes(const QByteArray & bytes)
{
    d.detach();
    d->bytes = bytes;
}

/*!
  Set the rights portion of the message to \a rights.

  A message with rights data but an empty byte data payload cannot be
  transmitted by the system.

  \sa QUnixSocketMessage::rights()
  */
void QUnixSocketMessage::setRights(const QList<QUnixSocketRights> & rights)
{
    d.detach();
    d->rights = rights;
}

/*!
  Return the rights portion of the message.

  \sa QUnixSocketMessage::setRights()
  */
const QList<QUnixSocketRights> & QUnixSocketMessage::rights() const
{
    return d->rights;
}

/*!
  Returns true if the rights portion of the message was truncated on reception
  due to insufficient buffer size.  The rights buffer size can be adjusted
  through calls to the \l QUnixSocket::setRightsBufferSize() method.
  \l QUnixSocket contains a discussion of the buffering and truncation
  characteristics of the Unix domain protocol.

  \sa QUnixSocket QUnixSocket::setRightsBufferSize()
  */
bool QUnixSocketMessage::rightsWereTruncated() const
{
    return d->state & QUnixSocketMessagePrivate::Truncated;
}

/*!
  Return the data portion of the message.

  \sa QUnixSocketMessage::setBytes()
  */
const QByteArray & QUnixSocketMessage::bytes() const
{
    return d->bytes;
}

/*!
  Returns the process id credential associated with this message.

  \sa QUnixSocketMessage::setProcessId()
  */
pid_t QUnixSocketMessage::processId() const
{
    if(QUnixSocketMessagePrivate::Credential & d->state)
        return d->pid;
    else
        return ::getpid();
}

/*!
  Returns the user id credential associated with this message.

  \sa QUnixSocketMessage::setUserId()
  */
uid_t QUnixSocketMessage::userId() const
{
    if(QUnixSocketMessagePrivate::Credential & d->state)
        return d->uid;
    else
        return ::geteuid();
}

/*!
  Returns the group id credential associated with this message.

  \sa QUnixSocketMessage::setGroupId()
  */
gid_t QUnixSocketMessage::groupId() const
{
    if(QUnixSocketMessagePrivate::Credential & d->state)
        return d->gid;
    else
        return ::getegid();
}

/*!
  Set the process id credential associated with this message to \a pid.  Unless
  you are the root user, setting a fraudulant credential will cause this message
  to fail.

  \sa QUnixSocketMessage::processId()
 */
void QUnixSocketMessage::setProcessId(pid_t pid)
{
    if(!(d->state & QUnixSocketMessagePrivate::Credential)) {
        d->state = (QUnixSocketMessagePrivate::AncillaryDataState)( d->state | QUnixSocketMessagePrivate::Credential );
        d->uid = ::geteuid();
        d->gid = ::getegid();
    }
    d->pid = pid;
}

/*!
  Set the user id credential associated with this message to \a uid.  Unless
  you are the root user, setting a fraudulant credential will cause this message
  to fail.

  \sa QUnixSocketMessage::userId()
 */
void QUnixSocketMessage::setUserId(uid_t uid)
{
    if(!(d->state & QUnixSocketMessagePrivate::Credential)) {
        d->state = (QUnixSocketMessagePrivate::AncillaryDataState)( d->state | QUnixSocketMessagePrivate::Credential );
        d->pid = ::getpid();
        d->gid = ::getegid();
    }
    d->uid = uid;
}

/*!
  Set the group id credential associated with this message to \a gid.  Unless
  you are the root user, setting a fraudulant credential will cause this message
  to fail.

  \sa QUnixSocketMessage::groupId()
 */
void QUnixSocketMessage::setGroupId(gid_t gid)
{
    if(!(d->state & QUnixSocketMessagePrivate::Credential)) {
        d->state = (QUnixSocketMessagePrivate::AncillaryDataState)( d->state | QUnixSocketMessagePrivate::Credential );
        d->pid = ::getpid();
        d->uid = ::geteuid();
    }
    d->gid = gid;
}

/*!
  Return true if this message is valid.  A message with rights data but an empty
  byte data payload cannot be transmitted by the system and is marked as
  invalid.
  */
bool QUnixSocketMessage::isValid() const
{
    return d->rights.isEmpty() || !d->bytes.isEmpty();
}

///////////////////////////////////////////////////////////////////////////////
// class QUnixSocket
///////////////////////////////////////////////////////////////////////////////
#define QUNIXSOCKET_DEFAULT_READBUFFER 1024
#define QUNIXSOCKET_DEFAULT_ANCILLARYBUFFER 0

/*!
  \class QUnixSocket
  \internal

  \brief The QUnixSocket class provides a Unix domain socket.

  \omit
  \ingroup Platform::DeviceSpecific
  \ingroup Platform::OS
  \ingroup Platform::Communications
  \endomit
  \ingroup qws

  Unix domain sockets provide an efficient mechanism for communications between
  Unix processes on the same machine.  Unix domain sockets support a reliable,
  stream-oriented, connection-oriented transport protocol, much like TCP
  sockets.  Unlike IP based sockets, the connection endpoint of a Unix domain
  socket is a file on disk of type socket.

  In addition to transporting raw data bytes, Unix domain sockets are able to
  transmit special ancillary data.  The two types of ancillary data supported
  by the QUnixSocket class are:

  \list
  \o Credential Data - Allows a receiver
  to reliably identify the process sending each message.
  \o \l {QUnixSocketRights}{Rights Data } - Allows Unix file descriptors
  to be transmitted between processes.
  \endlist

  Because of the need to support ancillary data, QUnixSocket is not a QIODevice,
  like QTcpSocket and QUdpSocket.  Instead, QUnixSocket contains a number of
  read and write methods that clients must invoke directly.  Rather than
  returning raw data bytes, \l QUnixSocket::read() returns \l QUnixSocketMessage
  instances that encapsulate the message's byte data and any other ancillary
  data.

  Ancillary data is transmitted "out of band".  Every \l QUnixSocketMessage
  received will have credential data associated with it that the client can
  access through calls to \l QUnixSocketMessage::processId(),
  \l QUnixSocketMessage::groupId() and \l QUnixSocketMessage::userId().
  Likewise, message creators can set the credential data to send through calls
  to \l QUnixSocketMessage::setProcessId(), \l QUnixSocketMessage::setGroupId()
  and \l QUnixSocketMessage::setUserId() respectively.  The authenticity of the
  credential values is verified by the system kernel and cannot be fabricated
  by unprivileged processes.  Only processes running as the root user can
  specify credential data that does not match the sending process.

  Unix file descriptors, known as "rights data", transmitted between processes
  appear as though they had been dup(2)'d between the two.  As Unix
  domain sockets present a continuous stream of bytes to the receiver, the
  rights data - which is transmitted out of band - must be "slotted" in at some
  point.  The rights data is logically associated with the first byte - called
  the anchor byte - of the \l QUnixSocketMessage to which they are attached.
  Received rights data will be available from the
  \l QUnixSocketMessage::rights() method for the \l QUnixSocketMessage
  instance that contains the anchor byte.

  In addition to a \l QUnixSocket::write() that takes a \l QUnixSocketMessage
  instance - allowing a client to transmit both byte and rights data - a
  number of convenience overloads are provided for use when only transmitting
  simple byte data.  Unix requires that at least one byte of raw data be
  transmitted in order to send rights data.  A \l QUnixSocketMessage instance
  with rights data, but no byte data, cannot be transmitted.

  Unix sockets present a stream interface, such that, for example, a single
  six byte transmission might be received as two three byte messages.  Rights
  data, on the other hand, is conceptually transmitted as unfragmentable
  datagrams.  If the receiving buffer is not large enough to contain all the
  transmitted rights information, the data is truncated and irretreivably lost.
  Users should use the \l QUnixSocket::setRightsBufferSize() method to control
  the buffer size used for this data, and develop protocols that avoid the
  problem.  If the buffer size is too small and rights data is truncated,
  the \l QUnixSocketMessage::rightsWereTruncated() flag will be set.

  \sa QUnixSocketMessage QUnixSocketRights
*/

/*!
  \enum QUnixSocket::SocketError

  The SocketError enumeration represents the various errors that can occur on
  a Unix domain socket.  The most recent error for the socket is available
  through the \l QUnixSocket::error() method.

  \value NoError No error has occurred.
  \value InvalidPath An invalid path endpoint was passed to
         \l QUnixSocket::connect().  As defined by unix(7), invalid paths
         include an empty path, or what more than 107 characters long.
  \value ResourceError An error acquiring or manipulating the system's socket
         resources occurred.  For example, if the process runs out of available
         socket descriptors, a ResourceError will occur.
  \value NonexistentPath The endpoing passed to \l QUnixSocket::connect() does
         not refer to a Unix domain socket entity on disk.
  \value ConnectionRefused The connection to the specified endpoint was refused.
         Generally this means that there is no server listening on that
         endpoint.
  \value UnknownError An unknown error has occurred.
  \value ReadFailure An error occurred while reading bytes from the connection.
  \value WriteFailure An error occurred while writing bytes into the connection.
  */

/*!
  \enum QUnixSocket::SocketState

  The SocketState enumeration represents the connection state of a QUnixSocket
  instance.

  \value UnconnectedState The connection is not established.
  \value ConnectedState The connection is established.
  \value ClosingState The connection is being closed, following a call to
         \l QUnixSocket::close().  While closing, any pending data will be
         transmitted, but further writes by the application will be refused.
  */

/*
  \fn QUnixSocket::bytesWritten(qint64 bytes)

  This signal is emitted every time a payload of data has been written to the
  connection.  The \a bytes argument is set to the number of bytes that were
  written in this payload.

  \sa QUnixSocket::readyRead()
*/

/*
  \fn QUnixSocket::readyRead()

  This signal is emitted once every time new data is available for reading from
  the connection. It will only be emitted again once new data is available.

  \sa QUnixSocket::bytesWritten()
*/

/*!
  \fn QUnixSocket::stateChanged(SocketState socketState)

  This signal is emitted each time the socket changes connection state.
  \a socketState will be set to the socket's new state.
*/

class QUnixSocketPrivate : public QObject {
Q_OBJECT
public:
    QUnixSocketPrivate(QUnixSocket * _me)
    : me(_me), fd(-1), readNotifier(0), writeNotifier(0),
      state(QUnixSocket::UnconnectedState), error(QUnixSocket::NoError),
      writeQueueBytes(0), messageValid(false), dataBuffer(0),
      dataBufferLength(0), dataBufferCapacity(0), ancillaryBuffer(0),
      ancillaryBufferCount(0), closingTimer(0) {
          QObject::connect(this, SIGNAL(readyRead()), me, SIGNAL(readyRead()));
          QObject::connect(this, SIGNAL(bytesWritten(qint64)),
                           me, SIGNAL(bytesWritten(qint64)));
      }
    ~QUnixSocketPrivate()
    {
        if(dataBuffer)
            delete [] dataBuffer;
        if(ancillaryBuffer)
            delete [] ancillaryBuffer;
    }

    enum { CausedAbort = 0x70000000 };

    QUnixSocket * me;

    int fd;

    QSocketNotifier * readNotifier;
    QSocketNotifier * writeNotifier;

    QUnixSocket::SocketState state;
    QUnixSocket::SocketError error;

    QQueue<QUnixSocketMessage> writeQueue;
    unsigned int writeQueueBytes;

    bool messageValid;
    ::msghdr message;
    inline void flushAncillary()
    {
        if(!messageValid) return;
        ::cmsghdr * h = (::cmsghdr *)CMSG_FIRSTHDR(&(message));
        while(h) {

            if(SCM_RIGHTS == h->cmsg_type) {
                int * fds = (int *)CMSG_DATA(h);
                int numFds = (h->cmsg_len - CMSG_LEN(0)) / sizeof(int);

                for(int ii = 0; ii < numFds; ++ii)
                    QT_CLOSE(fds[ii]);
            }

            h = (::cmsghdr *)CMSG_NXTHDR(&(message), h);
        }

        messageValid = false;
    }


    char * dataBuffer;
    unsigned int dataBufferLength;
    unsigned int dataBufferCapacity;

    char * ancillaryBuffer;
    inline unsigned int ancillaryBufferCapacity()
    {
        return CMSG_SPACE(sizeof(::ucred)) + CMSG_SPACE(sizeof(int) * ancillaryBufferCount);
    }
    unsigned int ancillaryBufferCount;

    QByteArray address;

    int closingTimer;

    virtual void timerEvent(QTimerEvent *)
    {
        me->abort();
        killTimer(closingTimer);
        closingTimer = 0;
    }
signals:
    void readyRead();
    void bytesWritten(qint64);

public slots:
    void readActivated();
    qint64 writeActivated();
};

/*!
  Construct a QUnixSocket instance, with \a parent.

  The read buffer is initially set to 1024 bytes, and the rights buffer to 0
  entries.

  \sa QUnixSocket::readBufferSize() QUnixSocket::rightsBufferSize()
  */
QUnixSocket::QUnixSocket(QObject * parent)
: QIODevice(parent), d(new QUnixSocketPrivate(this))
{
    setOpenMode(QIODevice::NotOpen);
    setReadBufferSize(QUNIXSOCKET_DEFAULT_READBUFFER);
    setRightsBufferSize(QUNIXSOCKET_DEFAULT_ANCILLARYBUFFER);
}

/*!
  Construct a QUnixSocket instance, with \a parent.

  The read buffer is initially set to \a readBufferSize bytes, and the rights
  buffer to \a rightsBufferSize entries.

  \sa QUnixSocket::readBufferSize() QUnixSocket::rightsBufferSize()
  */
QUnixSocket::QUnixSocket(qint64 readBufferSize, qint64 rightsBufferSize,
                         QObject * parent)
: QIODevice(parent), d(new QUnixSocketPrivate(this))
{
    Q_ASSERT(readBufferSize > 0 && rightsBufferSize >= 0);

    setOpenMode(QIODevice::NotOpen);
    setReadBufferSize(readBufferSize);
    setRightsBufferSize(rightsBufferSize);
}

/*!
  Destroys the QUnixSocket instance.  Any unsent data is discarded.
  */
QUnixSocket::~QUnixSocket()
{
    abort();
    delete d;
}

/*!
  Attempt to connect to \a path.

  This method is synchronous and will return true if the connection succeeds and
  false otherwise.  In the case of failure, \l QUnixSocket::error() will be set
  accordingly.

  Any existing connection will be aborted, and all pending data will be
  discarded.

  \sa QUnixSocket::close() QUnixSocket::abort() QUnixSocket::error()
  */
bool QUnixSocket::connect(const QByteArray & path)
{
    int _true;
    int crv;
#ifdef QUNIXSOCKET_DEBUG
    qDebug() << "QUnixSocket: Connect requested to '"
             << path << '\'';
#endif

    abort(); // Reset any existing connection

    if(UnconnectedState != d->state) // abort() caused a signal and someone messed
                                 // with us.  We'll assume they know what
                                 // they're doing and bail.  Alternative is to
                                 // have a special "Connecting" state
        return false;


    if(path.isEmpty() || path.size() > UNIX_PATH_MAX) {
        d->error = InvalidPath;
        return false;
    }

    // Create the socket
    d->fd = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if(-1 == d->fd) {
#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: Unable to create socket ("
                 << strerror(errno) << ')';
#endif
        d->error = ResourceError;
        goto connect_error;
    }

    // Set socket options
    _true = 1;
    crv = ::setsockopt(d->fd, SOL_SOCKET, SO_PASSCRED, (void *)&_true,
                       sizeof(int));
    if(-1 == crv) {
#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: Unable to configure socket ("
                 << ::strerror(errno) << ')';
#endif
        d->error = ResourceError;

        goto connect_error;
    }

    // Construct our unix address
    struct ::sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    ::memcpy(addr.sun_path, path.data(), path.size());
    if(path.size() < UNIX_PATH_MAX)
        addr.sun_path[path.size()] = '\0';

    // Attempt the connect
    crv = ::connect(d->fd, (sockaddr *)&addr, sizeof(sockaddr_un));
    if(-1 == crv) {
#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: Unable to connect ("
                 << ::strerror(errno) << ')';
#endif
        if(ECONNREFUSED == errno)
            d->error = ConnectionRefused;
        else if(ENOENT == errno)
            d->error = NonexistentPath;
        else
            d->error = UnknownError;

        goto connect_error;
    }

    // We're connected!
    d->address = path;
    d->state = ConnectedState;
    d->readNotifier = new QSocketNotifier(d->fd, QSocketNotifier::Read, d);
    d->writeNotifier = new QSocketNotifier(d->fd, QSocketNotifier::Write, d);
    QObject::connect(d->readNotifier, SIGNAL(activated(int)),
                     d, SLOT(readActivated()));
    QObject::connect(d->writeNotifier, SIGNAL(activated(int)),
                     d, SLOT(writeActivated()));
    d->readNotifier->setEnabled(true);
    d->writeNotifier->setEnabled(false);
    setOpenMode(QIODevice::ReadWrite);
    emit stateChanged(ConnectedState);

#ifdef QUNIXSOCKET_DEBUG
    qDebug() << "QUnixSocket: Connected to " << path;
#endif
    return true;

connect_error: // Cleanup failed connection
    if(-1 != d->fd) {
#ifdef QUNIXSOCKET_DEBUG
        int closerv =
#endif
            QT_CLOSE(d->fd);
#ifdef QUNIXSOCKET_DEBUG
        if(0 != closerv) {
            qDebug() << "QUnixSocket: Unable to close file descriptor after "
                        "failed connect (" << ::strerror(errno) << ')';
        }
#endif
    }
    d->fd = -1;
    return false;
}

/*!
  Sets the socket descriptor to use to \a socketDescriptor, bypassing
  QUnixSocket's connection infrastructure, and return true on success and false
  on failure.  \a socketDescriptor must be in the connected state, and must be
  a Unix domain socket descriptor.  Following a successful call to this method,
  the QUnixSocket instance will be in the Connected state and will have assumed
  ownership of \a socketDescriptor.

  Any existing connection will be aborted, and all pending data will be
  discarded.

  \sa QUnixSocket::connect()
*/
bool QUnixSocket::setSocketDescriptor(int socketDescriptor)
{
    abort();

    if(UnconnectedState != state()) // See QUnixSocket::connect()
        return false;

    // Attempt to set the socket options
    if(-1 == socketDescriptor) {
#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: User provided socket is invalid";
#endif
        d->error = ResourceError;
        return false;
    }

    // Set socket options
    int _true = 1;
    int crv = ::setsockopt(socketDescriptor, SOL_SOCKET,
                           SO_PASSCRED, (void *)&_true, sizeof(int));
    if(-1 == crv) {
#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: Unable to configure client provided socket ("
                 << ::strerror(errno) << ')';
#endif
        d->error = ResourceError;

        return false;
    }

    d->fd = socketDescriptor;
    d->state = ConnectedState;
    d->address = QByteArray();
    setOpenMode(QIODevice::ReadWrite);
    d->readNotifier = new QSocketNotifier(d->fd, QSocketNotifier::Read, d);
    d->writeNotifier = new QSocketNotifier(d->fd, QSocketNotifier::Write, d);
    QObject::connect(d->readNotifier, SIGNAL(activated(int)),
                     d, SLOT(readActivated()));
    QObject::connect(d->writeNotifier, SIGNAL(activated(int)),
                     d, SLOT(writeActivated()));
    d->readNotifier->setEnabled(true);
    d->writeNotifier->setEnabled(false);
    emit stateChanged(d->state);

    return true;
}

/*!
  Returns the socket descriptor currently in use.  This method will return -1
  if the QUnixSocket instance is in the UnconnectedState \l {QUnixSocket::state()}{state. }

  \sa QUnixSocket::setSocketDescriptor()
  */
int QUnixSocket::socketDescriptor() const
{
    return d->fd;
}

/*!
  Abort the connection.  This will immediately disconnect (if connected) and
  discard any pending data.  Following a call to QUnixSocket::abort() the
  object will always be in the disconnected \link QUnixSocket::state() state.
  \endlink

  \sa QUnixSocket::close()
*/
void QUnixSocket::abort()
{
    setOpenMode(QIODevice::NotOpen);

    // We want to be able to use QUnixSocket::abort() to cleanup our state but
    // also preserve the error message that caused the abort.  It is not
    // possible to reorder code to do this:
    //        abort();
    //        d->error = SomeError
    // as QUnixSocket::abort() might emit a signal and we need the error to be
    // set within that signal.  So, if we want an error message to be preserved
    // across a *single* call to abort(), we set the
    // QUnixSocketPrivate::CausedAbort flag in the error.
    if(d->error & QUnixSocketPrivate::CausedAbort)
        d->error = (QUnixSocket::SocketError)(d->error &
                                              ~QUnixSocketPrivate::CausedAbort);
    else
        d->error = NoError;

    if( UnconnectedState == d->state) return;

#ifdef QUNIXSOCKET_DEBUG
    int closerv =
#endif
        ::close(d->fd);
#ifdef QUNIXSOCKET_DEBUG
    if(0 != closerv) {
        qDebug() << "QUnixSocket: Unable to close socket during abort ("
                 << strerror(errno) << ')';
    }
#endif

    // Reset variables
    d->fd = -1;
    d->state = UnconnectedState;
    d->dataBufferLength = 0;
    d->flushAncillary();
    d->address = QByteArray();
    if(d->readNotifier) {
        d->readNotifier->setEnabled(false);
        d->readNotifier->deleteLater();
    }
    if(d->writeNotifier) {
        d->writeNotifier->setEnabled(false);
        d->writeNotifier->deleteLater();
    }
    d->readNotifier = 0;
    d->writeNotifier = 0;
    d->writeQueue.clear();
    d->writeQueueBytes = 0;
    if(d->closingTimer) {
        d->killTimer(d->closingTimer);
    }
    d->closingTimer = 0;
    emit stateChanged(d->state);
}

/*!
  Close the connection.  The instance will enter the Closing
  \l {QUnixSocket::state()}{state } until all pending data has been
  transmitted, at which point it will enter the Unconnected state.

  Even if there is no pending data for transmission, the object will never
  jump directly to Disconnect without first passing through the
  Closing state.

  \sa QUnixSocket::abort()
  */
void QUnixSocket::close()
{
    if(ConnectedState != state()) return;

    d->state = ClosingState;
    if(d->writeQueue.isEmpty()) {
        d->closingTimer = d->startTimer(0); // Start a timer to "fake"
                                            // completing writes
    }
    emit stateChanged(d->state);
}

/*!
    This function writes as much as possible from the internal write buffer to
    the underlying socket, without blocking. If any data was written, this
    function returns true; otherwise false is returned.
*/
// Note! docs partially copied from QAbstractSocket::flush()
bool QUnixSocket::flush()
{
    // This needs to have the same semantics as QAbstractSocket, if it is to
    // be used interchangeably with that class.
    if (d->writeQueue.isEmpty())
        return false;

    d->writeActivated();
    return true;
}

/*!
  Returns the last error to have occurred on this object.  This method is not
  destructive, so multiple calls to QUnixSocket::error() will return the same
  value.  The error is only reset by a call to \l QUnixSocket::connect() or
  \l QUnixSocket::abort()
  */
QUnixSocket::SocketError QUnixSocket::error() const
{
    return (QUnixSocket::SocketError)
        (d->error & ~QUnixSocketPrivate::CausedAbort);
}

/*!
  Returns the connection state of this instance.
  */
QUnixSocket::SocketState QUnixSocket::state() const
{
    return d->state;
}

/*!
  Returns the Unix path address passed to \l QUnixSocket::connect().  This
  method will return an empty path if the object is in the Unconnected
  \l {QUnixSocket::state()}{state } or was connected through a call
  to \l QUnixSocket::setSocketDescriptor()

  \sa QUnixSocket::connect() QUnixSocket::setSocketDescriptor()
  */
QByteArray QUnixSocket::address() const
{
    return d->address;
}

/*!
  Returns the number of bytes available for immediate retrieval through a call
  to \l QUnixSocket::read().
  */
qint64 QUnixSocket::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + d->dataBufferLength;
}

/*!
  Returns the number of enqueued bytes still to be written to the socket.
  */
qint64 QUnixSocket::bytesToWrite() const
{
    return d->writeQueueBytes;
}

/*!
  Returns the size of the read buffer in bytes.  The read buffer size
  determines the amount of byte data that can be read from the socket in one go.
  The read buffer size caps the maximum value that can be returned by
  \l QUnixSocket::bytesAvailable() and will always be greater than zero.  By
  default, the read buffer size is 1024 bytes.

  The size of the read buffer is independent of the rights buffer, which can be
  queried by \l QUnixSocket::rightsBufferSize().

  \sa QUnixSocket::setReadBufferSize()
  */
qint64 QUnixSocket::readBufferSize() const
{
    return d->dataBufferCapacity;
}

/*!
  Sets the \a size of the socket's read buffer in bytes.

  The size of the read buffer is independent of the rights buffer, which can be
  set by \l QUnixSocket::setRightsBufferSize().

  Attempting to reduce the buffer size while bytes are available for reading
  (ie. while the buffer is in use) will fail.

  \sa QUnixSocket::readBufferSize()
  */
void QUnixSocket::setReadBufferSize(qint64 size)
{
    Q_ASSERT(size > 0);
    if(size == d->dataBufferCapacity || d->dataBufferLength) return;
    if(d->dataBuffer) delete [] d->dataBuffer;
    d->dataBuffer = new char[size];
    d->dataBufferCapacity = size;
}

/*!
  Returns the size of the rights buffer in rights entries.  The rights buffer
  size determines the number of rights transferences that can be received in
  any message.  Unlike byte stream data which can be fragmented into many
  smaller messages if the \link QUnixSocket::readBufferSize() read buffer
  \endlink is not large enough to contain all the available data, rights data
  is transmitted as unfragmentable datagrams.  If the rights buffer is not
  large enough to contain this unfragmentable datagram, the datagram will be
  truncated and rights data irretrievably lost.  If truncation occurs, the
  \l QUnixSocketMessage::rightsWereTruncated() flag will be set.  By default
  the rights buffer size is 0 entries - rights data cannot be received.

  The size of the rights buffer is independent of the read buffer, which can be
  queried by \l QUnixSocket::readBufferSize().

  \sa QUnixSocket::setRightsBufferSize()
  */
qint64 QUnixSocket::rightsBufferSize() const
{
    return d->ancillaryBufferCount;
}

/*!
  Sets the \a size of the socket's rights buffer in rights entries.

  The size of the rights buffer is independent of the read buffer, which can be
  set by \l QUnixSocket::setReadBufferSize().

  Attempting to reduce the buffer size while bytes are available for reading
  (ie. while the buffer is in use) will fail.

  \sa QUnixSocket::rightsBufferSize()
  */
void QUnixSocket::setRightsBufferSize(qint64 size)
{
    Q_ASSERT(size >= 0);

    if((size == d->ancillaryBufferCount || d->dataBufferLength) &&
            d->ancillaryBuffer)
        return;

    qint64 byteSize = CMSG_SPACE(sizeof(::ucred)) +
                      CMSG_SPACE(size * sizeof(int));

    if(d->ancillaryBuffer) delete [] d->ancillaryBuffer;
    d->ancillaryBuffer = new char[byteSize];
    d->ancillaryBufferCount = size;
}

/*!
  \overload

  Writes \a socketdata to the socket.  In addition to failing if the socket
  is not in the Connected state, writing will fail if \a socketdata is
  \l {QUnixSocketMessage::isValid()}{invalid. }

  Writes through the QUnixSocket class are asynchronous.  Rather than being
  written immediately, data is enqueued and written once the application
  reenters the Qt event loop and the socket becomes available for writing.
  Thus, this method will only fail if the socket is not in the Connected state
  - it is illegal to attempt a write on a Unconnected or Closing socket.

  Applications can monitor the progress of data writes through the
  \l QUnixSocket::bytesWritten() signal and \l QUnixSocket::bytesToWrite()
  method.

  \sa QUnixSocketMessage
  */
qint64 QUnixSocket::write(const QUnixSocketMessage & socketdata)
{
    if(ConnectedState != state() || !socketdata.isValid()) return -1;
    if(socketdata.d->size() == 0) return 0;

    d->writeQueue.enqueue(socketdata);
    d->writeQueueBytes += socketdata.d->size();
    d->writeNotifier->setEnabled(true);

    return socketdata.d->size();
}

/*!
  Return the next available message, or an empty message if none is available.

  To avoid retrieving empty messages, applications should connect to the
  \l QUnixSocket::readyRead() signal to be notified when new messages are
  available or periodically poll the \l QUnixSocket::bytesAvailable() method.

  \sa QUnixSocket::readyRead() QUnixSocket::bytesAvailable()
  */
QUnixSocketMessage QUnixSocket::read()
{
    QUnixSocketMessage data;
    if(!d->dataBufferLength)
        return data;

    data.d->state = QUnixSocketMessagePrivate::Credential;

    // Bytes are easy
    data.setBytes(QByteArray(d->dataBuffer, d->dataBufferLength));

    // Extract ancillary data
    QList<QUnixSocketRights> a;

    ::cmsghdr * h = (::cmsghdr *)CMSG_FIRSTHDR(&(d->message));
    while(h) {

        if(SCM_CREDENTIALS == h->cmsg_type) {
            ::ucred * cred = (::ucred *)CMSG_DATA(h);
#ifdef QUNIXSOCKET_DEBUG
            qDebug( "Credentials recd: pid %lu - gid %lu - uid %lu",
                    cred->pid, cred->gid, cred->uid );
#endif
            data.d->pid = cred->pid;
            data.d->gid = cred->gid;
            data.d->uid = cred->uid;

        } else if(SCM_RIGHTS == h->cmsg_type) {

            int * fds = (int *)CMSG_DATA(h);
            int numFds = (h->cmsg_len - CMSG_LEN(0)) / sizeof(int);

            for(int ii = 0; ii < numFds; ++ii) {
                QUnixSocketRights qusr(fds[ii], 0);
                a.append(qusr);
            }

        } else {

#ifdef QUNIXSOCKET_DEBUG
            qFatal("QUnixSocket: Unknown ancillary data type (%d) received.",
                   h->cmsg_type);
#endif

        }

        h = (::cmsghdr *)CMSG_NXTHDR(&(d->message), h);
    }

    if(d->message.msg_flags & MSG_CTRUNC) {
        data.d->state = (QUnixSocketMessagePrivate::AncillaryDataState)(QUnixSocketMessagePrivate::Truncated |
                               QUnixSocketMessagePrivate::Credential );
    }

    if(!a.isEmpty())
        data.d->rights = a;

    d->dataBufferLength = 0;
    d->messageValid = false;
    d->readNotifier->setEnabled(true);

    return data;
}

/*! \internal */
bool QUnixSocket::isSequential() const
{
    return true;
}

/*! \internal */
bool QUnixSocket::waitForReadyRead(int msecs)
{
    if(UnconnectedState == d->state)
        return false;

    if(d->messageValid) {
        return true;
    }

    Q_ASSERT(-1 != d->fd);

    int     timeout = msecs;
    struct  timeval tv;
    struct  timeval *ptrTv = 0;
    QTime   stopWatch;

    stopWatch.start();

    do
    {
        fd_set readset;

        FD_ZERO(&readset);
        FD_SET(d->fd, &readset);

        if(-1 != msecs) {
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            ptrTv = &tv;
        }

        int rv = ::select(d->fd + 1, &readset, 0, 0, ptrTv);
        switch(rv) {
            case 0:
                // timeout
                return false;
            case 1:
                // ok
                d->readActivated();
                return true;
            default:
                if (errno != EINTR)
                    abort();    // error
                break;
        }

        timeout = msecs - stopWatch.elapsed();
    }
    while (timeout > 0);

    return false;
}

bool QUnixSocket::waitForBytesWritten(int msecs)
{
    if(UnconnectedState == d->state)
        return false;

    Q_ASSERT(-1 != d->fd);

    if ( d->writeQueue.isEmpty() )
        return true;

    QTime stopWatch;
    stopWatch.start();

    while ( true )
    {
        fd_set fdwrite;
        FD_ZERO(&fdwrite);
        FD_SET(d->fd, &fdwrite);
        int timeout = msecs < 0 ? 0 : msecs - stopWatch.elapsed();
        struct timeval tv;
        struct timeval *ptrTv = 0;
        if ( -1 != msecs )
        {
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            ptrTv = &tv;
        }

        int rv = ::select(d->fd + 1, 0, &fdwrite, 0, ptrTv);
        switch ( rv )
        {
            case 0:
                // timeout
                return false;
            case 1:
            {
                // ok to write
                qint64 bytesWritten = d->writeActivated();
                if (bytesWritten == 0) {
                    // We need to retry
                    int delay = 1;
                    do {
                        if (-1 != msecs) {
                            timeout = msecs - stopWatch.elapsed();
                            if (timeout <= 0) {
                                // We have exceeded our allotted time
                                return false;
                            } else {
                                if (delay > timeout)
                                    delay = timeout;
                            }
                        }

                        // Pause before we make another attempt to send
                        ::usleep(delay * 1000);
                        if (delay < 1024)
                            delay *= 2;

                        bytesWritten = d->writeActivated();
                    } while (bytesWritten == 0);
                }
                return (bytesWritten != -1);
            }
            default:
                // error - or an uncaught signal!!!!!!!!!
                if ( rv == EINTR )
                    continue;
                abort();
                return false;
        }
    }
    return false; // fix warnings
}

/*! \internal */
bool QUnixSocket::canReadLine() const
{
    for(unsigned int ii = 0; ii < d->dataBufferLength; ++ii)
        if(d->dataBuffer[ii] == '\n') return true;
    return false;
}

/*! \internal */
qint64 QUnixSocket::readData(char * data, qint64 maxSize)
{
    Q_ASSERT(data);
    if(0 >= maxSize) return 0;
    if(!d->dataBufferLength) return 0;

    // Read data
    unsigned int size = d->dataBufferLength>maxSize?maxSize:d->dataBufferLength;
    memcpy(data, d->dataBuffer, size);
    if(size == d->dataBufferLength) {
        d->dataBufferLength = 0;
    } else {
        memmove(d->dataBuffer, d->dataBuffer + size, d->dataBufferLength - size);
        d->dataBufferLength -= size;
    }


    // Flush ancillary
    d->flushAncillary();

    if(0 == d->dataBufferLength)
        d->readNotifier->setEnabled(true);

    return size;
}

/*! \internal */
qint64 QUnixSocket::writeData (const char * data, qint64 maxSize)
{
    return write(QUnixSocketMessage(QByteArray(data, maxSize)));
}

qint64 QUnixSocketPrivate::writeActivated()
{
    writeNotifier->setEnabled(false);

    QUnixSocketMessage & m = writeQueue.head();
    const QList<QUnixSocketRights> & a = m.rights();

    //
    // Construct the message
    //
    ::iovec vec;
    if ( !m.d->vec ) // message does not already have an iovec
    {
        vec.iov_base = (void *)m.bytes().constData();
        vec.iov_len = m.bytes().size();
    }

    // Allocate the control buffer
    ::msghdr sendmessage;
    ::bzero(&sendmessage, sizeof(::msghdr));
    if ( m.d->vec )
    {
        sendmessage.msg_iov = m.d->vec;
        sendmessage.msg_iovlen = m.d->iovecLen;
    }
    else
    {
        sendmessage.msg_iov = &vec;
        sendmessage.msg_iovlen = 1;
    }
    unsigned int required = CMSG_SPACE(sizeof(::ucred)) +
                            a.size() * CMSG_SPACE(sizeof(int));
    sendmessage.msg_control = new char[required];
    ::bzero(sendmessage.msg_control, required);
    sendmessage.msg_controllen = required;

    // Create ancillary buffer
    ::cmsghdr * h = CMSG_FIRSTHDR(&sendmessage);

    if(m.d->state & QUnixSocketMessagePrivate::Credential) {
        h->cmsg_len = CMSG_LEN(sizeof(::ucred));
        h->cmsg_level = SOL_SOCKET;
        h->cmsg_type = SCM_CREDENTIALS;
        ((::ucred *)CMSG_DATA(h))->pid = m.d->pid;
        ((::ucred *)CMSG_DATA(h))->gid = m.d->gid;
        ((::ucred *)CMSG_DATA(h))->uid = m.d->uid;
        h = CMSG_NXTHDR(&sendmessage, h);
    } else {
        sendmessage.msg_controllen -= CMSG_SPACE(sizeof(::ucred));
    }

    for(int ii = 0; ii < a.count(); ++ii) {
        const QUnixSocketRights & r = a.at(ii);

        if(r.isValid()) {
            h->cmsg_len = CMSG_LEN(sizeof(int));
            h->cmsg_level = SOL_SOCKET;
            h->cmsg_type = SCM_RIGHTS;
            *((int *)CMSG_DATA(h)) = r.peekFd();
            h = CMSG_NXTHDR(&sendmessage, h);
        } else {
            sendmessage.msg_controllen -= CMSG_SPACE(sizeof(int));
        }
    }

#ifdef QUNIXSOCKET_DEBUG
    qDebug() << "QUnixSocket: Transmitting message (length" << m.d->size() << ')';
#endif
    ::ssize_t s = ::sendmsg(fd, &sendmessage, MSG_DONTWAIT | MSG_NOSIGNAL);
#ifdef QUNIXSOCKET_DEBUG
    qDebug() << "QUnixSocket: Transmitted message (" << s << ')';
#endif

    if(-1 == s) {
        if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno) {
            writeNotifier->setEnabled(true);
        } else if(EPIPE == errno) {
#ifdef QUNIXSOCKET_DEBUG
            qDebug() << "QUnixSocket: Remote side disconnected during transmit "
                        "(" << ::strerror(errno) << ')';
#endif
            me->abort();
        } else {
#ifdef QUNIXSOCKET_DEBUG
            qDebug() << "QUnixSocket: Unable to transmit data ("
                     << ::strerror(errno) << ')';
#endif
            error = (QUnixSocket::SocketError)(QUnixSocket::WriteFailure |
                    CausedAbort);
            me->abort();
        }
    } else if(s != m.d->size()) {

        // A partial transmission
        writeNotifier->setEnabled(true);
        delete [] (char *)sendmessage.msg_control;
        m.d->rights = QList<QUnixSocketRights>();
        m.d->removeBytes( s );
        writeQueueBytes -= s;
        emit bytesWritten(s);
        return s;

    } else {

        // Success!
        writeQueue.dequeue();
        Q_ASSERT(writeQueueBytes >= (unsigned)s);
        writeQueueBytes -= s;
        emit bytesWritten(s);

    }

    delete [] (char *)sendmessage.msg_control;
    if(-1 != s && !writeQueue.isEmpty())
        return writeActivated();
    else if(QUnixSocket::ClosingState == me->state() && writeQueue.isEmpty())
        me->abort();

    if((-1 == s) && (EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno))
        // Return zero bytes written to indicate retry may be required
        return 0;
    else
        return s;
}

void QUnixSocketPrivate::readActivated()
{
#ifdef QUNIXSOCKET_DEBUG
    qDebug() << "QUnixSocket: readActivated";
#endif
    readNotifier->setEnabled(false);

    ::iovec vec;
    vec.iov_base = dataBuffer;
    vec.iov_len = dataBufferCapacity;

    bzero(&message, sizeof(::msghdr));
    message.msg_iov = &vec;
    message.msg_iovlen = 1;
    message.msg_controllen = ancillaryBufferCapacity();
    message.msg_control = ancillaryBuffer;

    int flags = 0;
#ifdef MSG_CMSG_CLOEXEC
    flags = MSG_CMSG_CLOEXEC;
#endif

    int recvrv = ::recvmsg(fd, &message, flags);
#ifdef QUNIXSOCKET_DEBUG
    qDebug() << "QUnixSocket: Received message (" << recvrv << ')';
#endif
    if(-1 == recvrv) {
#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: Unable to receive data ("
                 << ::strerror(errno) << ')';
#endif
        error = (QUnixSocket::SocketError)(QUnixSocket::ReadFailure |
                                           CausedAbort);
        me->abort();
    } else if(0 == recvrv) {
        me->abort();
    } else {
        Q_ASSERT(recvrv);
        Q_ASSERT((unsigned)recvrv <= dataBufferCapacity);
        dataBufferLength = recvrv;
        messageValid = true;

#ifdef QUNIXSOCKET_DEBUG
        qDebug() << "QUnixSocket: readyRead() " << dataBufferLength;
#endif
        emit readyRead();
    }
}

QT_END_NAMESPACE

#include "qunixsocket.moc"
