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

#include "qunixsocketserver_p.h"

// #define QUNIXSOCKETSERVER_DEBUG

#ifdef QUNIXSOCKETSERVER_DEBUG
#include <QDebug>
#endif

#include <QtCore/qsocketnotifier.h>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
};

#define UNIX_PATH_MAX 108 // From unix(7)

QT_BEGIN_NAMESPACE

class QUnixSocketServerPrivate : public QObject
{
Q_OBJECT
public:
    QUnixSocketServerPrivate(QUnixSocketServer * parent)
    : QObject(), me(parent), fd(-1), maxConns(30),
      error(QUnixSocketServer::NoError), acceptNotifier(0)
    {}

    QUnixSocketServer * me;
    int fd;
    int maxConns;
    QByteArray address;
    QUnixSocketServer::ServerError error;
    QSocketNotifier * acceptNotifier;
public slots:
    void acceptActivated();
};

/*!
  \class QUnixSocketServer
  \internal

  \brief The QUnixSocketServer class provides a Unix domain socket based server.
  \omit
  \ingroup Platform::DeviceSpecific
  \ingroup Platform::OS
  \ingroup Platform::Communications
  \endomit
  \ingroup qws

  This class makes it possible to accept incoming Unix domain socket
  connections.  Call \l QUnixSocketServer::listen() to have the server listen
  for incoming connections on a specified path.  The pure virtual
  \l QUnixSocketServer::incomingConnection() is called each time a new
  connection is established.  Users must inherit from QUnixSocketServer and
  implement this method.

  If an error occurs, \l QUnixSocketServer::serverError() returns the type of
  error.  Errors can only occur during server establishment - that is, during a
  call to \l QUnixSocketServer::listen().  Calling \l QUnixSocketServer::close()
  causes QUnixSocketServer to stop listening for connections and reset its
  state.

  QUnixSocketServer is often used in conjunction with the \l QUnixSocket class.

  \sa QUnixSocket
*/

/*!
  \enum QUnixSocketServer::ServerError

  The ServerError enumeration represents the errors that can occur during server
  establishment.  The most recent error can be retrieved through a call to
  \l QUnixSocketServer::serverError().

  \value NoError No error has occurred.
  \value InvalidPath An invalid path endpoint was passed to
         \l QUnixSocketServer::listen().  As defined by unix(7), invalid paths
         include an empty path, or what more than 107 characters long.
  \value ResourceError An error acquiring or manipulating the system's socket
         resources occurred.  For example, if the process runs out of available
         socket descriptors, a ResourceError will occur.
  \value BindError The server was unable to bind to the specified path.
  \value ListenError The server was unable to listen on the specified path for
         incoming connections.
  */

/*!
  Create a new Unix socket server with the given \a parent.
  */
QUnixSocketServer::QUnixSocketServer(QObject *parent)
: QObject(parent), d(0)
{
}

/*!
  Stops listening for incoming connection and destroys the Unix socket server.
  */
QUnixSocketServer::~QUnixSocketServer()
{
    close();
    if(d)
        delete d;
}

/*!
  Stop listening for incoming connections and resets the Unix socket server's
  state.  Calling this method while \l {QUnixSocketServer::isListening()}{not listening } for incoming connections is a no-op.

  \sa QUnixSocketServer::listen()
  */
void QUnixSocketServer::close()
{
    if(!d)
        return;

    if(d->acceptNotifier) {
        d->acceptNotifier->setEnabled(false);
        delete d->acceptNotifier;
    }
    d->acceptNotifier = 0;

    if(-1 != d->fd) {
#ifdef QUNIXSOCKET_DEBUG
        int closerv =
#endif
            ::close(d->fd);
#ifdef QUNIXSOCKET_DEBUG
        if(0 != closerv) {
            qDebug() << "QUnixSocketServer: Unable to close socket ("
                     << strerror(errno) << ')';
        }
#endif
    }
    d->fd = -1;
    d->address = QByteArray();
    d->error = NoError;
}

/*!
  Returns the last server error.  Errors may only occur within a call to
  \l QUnixSocketServer::listen(), and only when such a call fails.

  This method is not destructive, so multiple calls to
  QUnixSocketServer::serverError() will return the same value.  The error is
  only reset by an explicit call to \l QUnixSocketServer::close() or
  by further calls to \l QUnixSocketServer::listen().
  */
QUnixSocketServer::ServerError QUnixSocketServer::serverError() const
{
    if(!d)
        return NoError;

    return d->error;
}

/*!
  Returns true if this server is listening for incoming connections, false
  otherwise.

  \sa QUnixSocketServer::listen()
  */
bool QUnixSocketServer::isListening() const
{
    if(!d)
        return false;

    return (-1 != d->fd);
}

/*!
  Tells the server to listen for incoming connections on \a path.  Returns true
  if it successfully initializes, false otherwise.  In the case of failure, the
  \l QUnixSocketServer::serverError() error status is set accordingly.

  Calling this method while the server is already running will result in the
  server begin reset, and then attempting to listen on \a path.  This will not
  affect connections established prior to the server being reset, but further
  incoming connections on the previous path will be refused.

  The server can be explicitly reset by a call to \l QUnixSocketServer::close().

  \sa QUnixSocketServer::close()
  */
bool QUnixSocketServer::listen(const QByteArray & path)
{
    if(d) {
        close(); // Any existing server is destroyed
    } else {
        d = new QUnixSocketServerPrivate(this);
    }

    if(path.isEmpty() || path.size() > UNIX_PATH_MAX) {
        d->error = InvalidPath;
        return false;
    }
    unlink( path );  // ok if this fails

    // Create the socket
    d->fd = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if(-1 == d->fd) {
#ifdef QUNIXSOCKETSERVER_DEBUG
        qDebug() << "QUnixSocketServer: Unable to create socket ("
                 << strerror(errno) << ')';
#endif
        close();
        d->error = ResourceError;
        return false;
    }

    // Construct our unix address
    struct ::sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    ::memcpy(addr.sun_path, path.data(), path.size());
    if(path.size() < UNIX_PATH_MAX)
        addr.sun_path[path.size()] = '\0';

    // Attempt to bind
    if(-1 == ::bind(d->fd, (sockaddr *)&addr, sizeof(sockaddr_un))) {
#ifdef QUNIXSOCKETSERVER_DEBUG
        qDebug() << "QUnixSocketServer: Unable to bind socket ("
                 << strerror(errno) << ')';
#endif
        close();
        d->error = BindError;
        return false;
    }

    // Listen to socket
    if(-1 == ::listen(d->fd, d->maxConns)) {
#ifdef QUNIXSOCKETSERVER_DEBUG
        qDebug() << "QUnixSocketServer: Unable to listen socket ("
                 << strerror(errno) << ')';
#endif
        close();
        d->error = ListenError;
        return false;
    }

    // Success!
    d->address = path;
    d->acceptNotifier = new QSocketNotifier(d->fd, QSocketNotifier::Read, d);
    d->acceptNotifier->setEnabled(true);
    QObject::connect(d->acceptNotifier, SIGNAL(activated(int)),
                     d, SLOT(acceptActivated()));

    return true;
}

/*!
  Returns the Unix path on which this server is listening.  If this server is
  not listening, and empty address will be returned.
  */
QByteArray QUnixSocketServer::serverAddress() const
{
    if(!d)
        return QByteArray();
    return d->address;
}

int QUnixSocketServer::socketDescriptor() const
{
    if (!d)
        return -1;
    return d->fd;
}


/*!
  Returns the maximum length the queue of pending connections may grow to.  That
  is, the maximum number of clients attempting to connect for which the Unix
  socket server has not yet accepted and passed to
  \l QUnixSocketServer::incomingConnection().  If a connection request arrives
  with the queue full, the client may receive a connection refused notification.

  By default a queue length of 30 is used.

  \sa QUnixSocketServer::setMaxPendingConnections()
  */
int QUnixSocketServer::maxPendingConnections() const
{
    if(!d)
        return 30;

    return d->maxConns;
}

/*!
  Sets the maximum length the queue of pending connections may grow to
  \a numConnections.  This value will only apply to
  \l QUnixSocketServer::listen() calls made following the value change - it will
  not be retroactively applied.

  \sa QUnixSocketServer::maxPendingConnections()
  */
void QUnixSocketServer::setMaxPendingConnections(int numConnections)
{
    Q_ASSERT(numConnections >= 1);
    if(!d)
        d = new QUnixSocketServerPrivate(this);

    d->maxConns = numConnections;
}

/*!
  \fn void QUnixSocketServer::incomingConnection(int socketDescriptor)

  This method is invoked each time a new incoming connection is established with
  the server.  Clients must reimplement this function in their QUnixSocketServer
  derived class to handle the connection.

  A common approach to handling the connection is to pass \a socketDescriptor to
  a QUnixSocket instance.

  \sa QUnixSocket
  */

void QUnixSocketServerPrivate::acceptActivated()
{
    ::sockaddr_un r;
    socklen_t len = sizeof(sockaddr_un);
    int connsock = ::accept(fd, (sockaddr *)&r, &len);
#ifdef QUNIXSOCKETSERVER_DEBUG
    qDebug() << "QUnixSocketServer: Accept connection " << connsock;
#endif
    if(-1 != connsock)
        me->incomingConnection(connsock);
}

QT_END_NAMESPACE

#include "qunixsocketserver.moc"
