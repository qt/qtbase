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

#include "qcopchannel_qws.h"

#ifndef QT_NO_COP

#include "qwsdisplay_qws.h"
#include "qwscommand_qws_p.h"
#include "qwindowsystem_qws.h"
#include "qwindowsystem_p.h"
#include "qlist.h"
#include "qmap.h"
#include "qdatastream.h"
#include "qpointer.h"
#include "qmutex.h"

#include "qdebug.h"

QT_BEGIN_NAMESPACE

typedef QMap<QString, QList<QWSClient*> > QCopServerMap;
static QCopServerMap *qcopServerMap = 0;

class QCopServerRegexp
{
public:
    QCopServerRegexp( const QString& channel, QWSClient *client );
    QCopServerRegexp( const QCopServerRegexp& other );

    QString channel;
    QWSClient *client;
    QRegExp regexp;
};

QCopServerRegexp::QCopServerRegexp( const QString& channel, QWSClient *client )
{
    this->channel = channel;
    this->client = client;
    this->regexp = QRegExp( channel, Qt::CaseSensitive, QRegExp::Wildcard );
}

QCopServerRegexp::QCopServerRegexp( const QCopServerRegexp& other )
{
    channel = other.channel;
    client = other.client;
    regexp = other.regexp;
}

typedef QList<QCopServerRegexp> QCopServerRegexpList;
static QCopServerRegexpList *qcopServerRegexpList = 0;

typedef QMap<QString, QList< QPointer<QCopChannel> > > QCopClientMap;
static QCopClientMap *qcopClientMap = 0;

Q_GLOBAL_STATIC(QMutex, qcopClientMapMutex)

// Determine if a channel name contains wildcard characters.
static bool containsWildcards( const QString& channel )
{
    return channel.contains(QLatin1Char('*'));
}

class QCopChannelPrivate
{
public:
    QString channel;
};

/*!
    \class QCopChannel
    \ingroup qws

    \brief The QCopChannel class provides communication capabilities
    between clients in \l{Qt for Embedded Linux}.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    The Qt COmmunication Protocol (QCOP) is a many-to-many protocol
    for transferring messages across registered channels. A channel is
    registered by name, and anyone who wants to can listen to the
    channel as well as send messages through it. The QCOP protocol
    allows clients to communicate both within the same address space
    and between different processes.

    To send messages to a given channel, QCopChannel provides the
    static send() function. Using this function alone, the messages
    are queued until Qt re-enters the event loop. To immediately flush
    all queued messages to the registered listeners, call the static
    flush() function.

    To listen to the traffic on a given channel, you typically
    instantiate a QCopChannel object for the given channel and connect
    to its received() signal that is emitted whenever there is
    incoming data.  Use the static isRegistered() function to query
    the server for the existence of a given channel. QCopChannel
    provides the channel() function returning the name of this
    QCopChannel object's channel.

    In additon, QCopChannel provides the virtual receive() function
    that can be reimplemented to filter the incoming messages and
    data. The default implementation simply emits the received()
    signal.

    \sa QWSServer, QWSClient, {Qt for Embedded Linux Architecture}
*/

/*!
    Constructs a QCopChannel object for the specified \a channel, with
    the given \a parent. Once created, the channel is registered by
    the server.

    \sa isRegistered(), channel()
*/

QCopChannel::QCopChannel(const QString& channel, QObject *parent) :
    QObject(parent)
{
    init(channel);
}

#ifdef QT3_SUPPORT
/*!
    Use the two argument overload instead, and call the
    QObject::setObjectName() function to \a name the instance.
*/
QCopChannel::QCopChannel(const QString& channel, QObject *parent, const char *name) :
    QObject(parent)
{
    setObjectName(QString::fromAscii(name));
    init(channel);
}
#endif

void QCopChannel::init(const QString& channel)
{
    d = new QCopChannelPrivate;
    d->channel = channel;

    if (!qt_fbdpy) {
        qFatal("QCopChannel: Must construct a QApplication "
                "before QCopChannel");
        return;
    }

    {
	QMutexLocker locker(qcopClientMapMutex());

	if (!qcopClientMap)
	    qcopClientMap = new QCopClientMap;

	// do we need a new channel list ?
	QCopClientMap::Iterator it = qcopClientMap->find(channel);
	if (it != qcopClientMap->end()) {
	    it.value().append(this);
	    return;
	}

	it = qcopClientMap->insert(channel, QList< QPointer<QCopChannel> >());
	it.value().append(QPointer<QCopChannel>(this));
    }

    // inform server about this channel
    qt_fbdpy->registerChannel(channel);
}

/*!
  \internal

  Resend all channel registrations
  */
void QCopChannel::reregisterAll()
{
    if(qcopClientMap)
        for(QCopClientMap::Iterator iter = qcopClientMap->begin();
            iter != qcopClientMap->end();
            ++iter)
            qt_fbdpy->registerChannel(iter.key());
}

/*!
    Destroys this QCopChannel object.

    The server is notified that this particular listener has closed
    its connection. The server will keep the channel open until the
    last registered listener detaches.

    \sa isRegistered(), channel()
*/

QCopChannel::~QCopChannel()
{
    QMutexLocker locker(qcopClientMapMutex());
    QCopClientMap::Iterator it = qcopClientMap->find(d->channel);
    Q_ASSERT(it != qcopClientMap->end());
    it.value().removeAll(this);
    // still any clients connected locally ?
    if (it.value().isEmpty()) {
        QByteArray data;
        QDataStream s(&data, QIODevice::WriteOnly);
        s << d->channel;
        if (qt_fbdpy)
            send(QLatin1String(""), QLatin1String("detach()"), data);
        qcopClientMap->remove(d->channel);
    }

    delete d;
}

/*!
    Returns the name of this object's channel.

    \sa isRegistered()
*/

QString QCopChannel::channel() const
{
    return d->channel;
}

/*!
    \fn void QCopChannel::receive(const QString& message, const QByteArray &data)

    Processes the incoming \a message and \a data.

    This function is called by the server when this object's channel
    receives new messages. Note that the default implementation simply
    emits the received() signal; reimplement this function to process
    the incoming \a message and \a data.

    Note that the format of the given \a data has to be well defined
    in order to extract the information it contains. In addition, it
    is recommended to use the DCOP convention. This is not a
    requirement, but you must ensure that the sender and receiver
    agree on the argument types. For example:

    \snippet doc/src/snippets/code/src_gui_embedded_qcopchannel_qws.cpp 0

    The above code assumes that the \c message is a DCOP-style
    function signature and the \c data contains the function's
    arguments.

    \sa send(), channel(), received()
 */
void QCopChannel::receive(const QString& msg, const QByteArray &data)
{
    emit received(msg, data);
}

/*!
    \fn void QCopChannel::received(const QString& message, const QByteArray &data)

    This signal is emitted whenever this object's channel receives new
    messages (i.e., it is emitted by the receive() function), passing
    the incoming \a message and \a data as parameters.

    \sa receive(), channel()
*/

/*!
    Queries the server for the existence of the given \a channel. Returns true
    if the channel is registered; otherwise returns false.

    \sa channel(), send()
*/

bool QCopChannel::isRegistered(const QString&  channel)
{
    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    s << channel;
    if (!send(QLatin1String(""), QLatin1String("isRegistered()"), data))
        return false;

    QWSQCopMessageEvent *e = qt_fbdpy->waitForQCopResponse();
    bool known = e->message == "known";
    delete e;
    return known;
}

/*!
    \fn bool QCopChannel::send(const QString& channel, const QString& message)
    \overload
*/

bool QCopChannel::send(const QString& channel, const QString& msg)
{
    QByteArray data;
    return send(channel, msg, data);
}

/*!
    \fn bool QCopChannel::send(const QString& channel, const QString& message,
                       const QByteArray &data)

    Sends the given \a message on the specified \a channel with the
    given \a data. The message will be distributed to all clients
    subscribed to the channel. Returns true if the message is sent
    successfully; otherwise returns false.

    It is recommended to use the DCOP convention. This is not a
    requirement, but you must ensure that the sender and receiver
    agree on the argument types.

    Note that QDataStream provides a convenient way to fill the byte
    array with auxiliary data. For example:

    \snippet doc/src/snippets/code/src_gui_embedded_qcopchannel_qws.cpp 1

    In the code above the channel is \c "System/Shell". The \c message
    is an arbitrary string, but in the example we've used the DCOP
    convention of passing a function signature. Such a signature is
    formatted as \c "functionname(types)" where \c types is a list of
    zero or more comma-separated type names, with no whitespace, no
    consts and no pointer or reference marks, i.e. no "*" or "&".

    \sa receive(), isRegistered()
*/

bool QCopChannel::send(const QString& channel, const QString& msg,
                       const QByteArray &data)
{
    if (!qt_fbdpy) {
        qFatal("QCopChannel::send: Must construct a QApplication "
                "before using QCopChannel");
        return false;
    }

    qt_fbdpy->sendMessage(channel, msg, data);

    return true;
}

/*!
    \since 4.2

    Flushes all queued messages to the registered listeners.

    Note that this function returns false if no QApplication has been
    constructed, otherwise it returns true.

    \sa send()

*/
bool QCopChannel::flush()
{
    if (!qt_fbdpy) {
        qFatal("QCopChannel::flush: Must construct a QApplication "
                "before using QCopChannel");
        return false;
    }

    qt_fbdpy->flushCommands();

    return true;
}

class QWSServerSignalBridge : public QObject {
  Q_OBJECT

public:
  void emitNewChannel(const QString& channel);
  void emitRemovedChannel(const QString& channel);

  signals:
  void newChannel(const QString& channel);
  void removedChannel(const QString& channel);
};

void QWSServerSignalBridge::emitNewChannel(const QString& channel){
  emit newChannel(channel);
}

void QWSServerSignalBridge::emitRemovedChannel(const QString& channel) {
  emit removedChannel(channel);
}

/*!
    \internal
    Server side: subscribe client \a cl on channel \a ch.
*/

void QCopChannel::registerChannel(const QString& ch, QWSClient *cl)
{
    if (!qcopServerMap)
        qcopServerMap = new QCopServerMap;

    // do we need a new channel list ?
    QCopServerMap::Iterator it = qcopServerMap->find(ch);
    if (it == qcopServerMap->end())
      it = qcopServerMap->insert(ch, QList<QWSClient*>());

    // If the channel name contains wildcard characters, then we also
    // register it on the server regexp matching list.
    if (containsWildcards( ch )) {
	QCopServerRegexp item(ch, cl);
	if (!qcopServerRegexpList)
	    qcopServerRegexpList = new QCopServerRegexpList;
	qcopServerRegexpList->append( item );
    }

    // If this is the first client in the channel, announce the channel as being created.
    if (it.value().count() == 0) {
      QWSServerSignalBridge* qwsBridge = new QWSServerSignalBridge();
      connect(qwsBridge, SIGNAL(newChannel(QString)), qwsServer, SIGNAL(newChannel(QString)));
      qwsBridge->emitNewChannel(ch);
      delete qwsBridge;
    }

    it.value().append(cl);
}

/*!
    \internal
    Server side: unsubscribe \a cl from all channels.
*/

void QCopChannel::detach(QWSClient *cl)
{
    if (!qcopServerMap)
        return;

    QCopServerMap::Iterator it = qcopServerMap->begin();
    for (; it != qcopServerMap->end(); ++it) {
      if (it.value().contains(cl)) {
        it.value().removeAll(cl);
        // If this was the last client in the channel, announce the channel as dead.
        if (it.value().count() == 0) {
          QWSServerSignalBridge* qwsBridge = new QWSServerSignalBridge();
          connect(qwsBridge, SIGNAL(removedChannel(QString)), qwsServer, SIGNAL(removedChannel(QString)));
          qwsBridge->emitRemovedChannel(it.key());
          delete qwsBridge;
        }
      }
    }

    if (!qcopServerRegexpList)
	return;

    QCopServerRegexpList::Iterator it2 = qcopServerRegexpList->begin();
    while(it2 != qcopServerRegexpList->end()) {
	if ((*it2).client == cl)
	    it2 = qcopServerRegexpList->erase(it2);
	else
	    ++it2;
    }
}

/*!
    \internal
    Server side: transmit the message to all clients registered to the
    specified channel.
*/

void QCopChannel::answer(QWSClient *cl, const QString& ch,
                          const QString& msg, const QByteArray &data)
{
    // internal commands
    if (ch.isEmpty()) {
        if (msg == QLatin1String("isRegistered()")) {
            QString c;
            QDataStream s(data);
            s >> c;
            bool known = qcopServerMap && qcopServerMap->contains(c)
                        && !((*qcopServerMap)[c]).isEmpty();
            // Yes, it's a typo, it's not user-visible, and we choose not to fix it for compatibility
            QLatin1String ans = QLatin1String(known ? "known" : "unknown");
            QWSServerPrivate::sendQCopEvent(cl, QLatin1String(""),
                                            ans, data, true);
            return;
        } else if (msg == QLatin1String("detach()")) {
            QString c;
            QDataStream s(data);
            s >> c;
            Q_ASSERT(qcopServerMap);
            QCopServerMap::Iterator it = qcopServerMap->find(c);
            if (it != qcopServerMap->end()) {
                //Q_ASSERT(it.value().contains(cl));
                it.value().removeAll(cl);
                if (it.value().isEmpty()) {
                  // If this was the last client in the channel, announce the channel as dead
                  QWSServerSignalBridge* qwsBridge = new QWSServerSignalBridge();
                  connect(qwsBridge, SIGNAL(removedChannel(QString)), qwsServer, SIGNAL(removedChannel(QString)));
                  qwsBridge->emitRemovedChannel(it.key());
                  delete qwsBridge;
                  qcopServerMap->erase(it);
                }
            }
	    if (qcopServerRegexpList && containsWildcards(c)) {
		// Remove references to a wildcarded channel.
		QCopServerRegexpList::Iterator it
		    = qcopServerRegexpList->begin();
		while(it != qcopServerRegexpList->end()) {
		    if ((*it).client == cl && (*it).channel == c)
			it = qcopServerRegexpList->erase(it);
		    else
			++it;
		}
	    }
            return;
        }
        qWarning("QCopChannel: unknown internal command %s", qPrintable(msg));
        QWSServerPrivate::sendQCopEvent(cl, QLatin1String(""),
                                        QLatin1String("bad"), data);
        return;
    }

    if (qcopServerMap) {
        QList<QWSClient*> clist = qcopServerMap->value(ch);
        for (int i=0; i < clist.size(); ++i) {
            QWSClient *c = clist.at(i);
            QWSServerPrivate::sendQCopEvent(c, ch, msg, data);
        }
    }

    if(qcopServerRegexpList && !containsWildcards(ch)) {
	// Search for wildcard matches and forward the message on.
	QCopServerRegexpList::ConstIterator it = qcopServerRegexpList->constBegin();
	for (; it != qcopServerRegexpList->constEnd(); ++it) {
	    if ((*it).regexp.exactMatch(ch)) {
		QByteArray newData;
		{
		    QDataStream stream
			(&newData, QIODevice::WriteOnly | QIODevice::Append);
		    stream << ch;
		    stream << msg;
		    stream << data;
		    // Stream is flushed and closed at this point.
		}
		QWSServerPrivate::sendQCopEvent
		    ((*it).client, (*it).channel,
		     QLatin1String("forwardedMessage(QString,QString,QByteArray)"),
		     newData);
	    }
	}
    }
}

/*!
    \internal
    Client side: distribute received event to the QCop instance managing the
    channel.
*/
void QCopChannel::sendLocally(const QString& ch, const QString& msg,
                                const QByteArray &data)
{
    Q_ASSERT(qcopClientMap);

    // filter out internal events
    if (ch.isEmpty())
        return;

    // feed local clients with received data
    QList< QPointer<QCopChannel> > clients;
    {
	QMutexLocker locker(qcopClientMapMutex());
	clients = (*qcopClientMap)[ch];
    }
    for (int i = 0; i < clients.size(); ++i) {
	QCopChannel *channel = (QCopChannel *)clients.at(i);
	if ( channel )
	    channel->receive(msg, data);
    }
}

QT_END_NAMESPACE

#include "qcopchannel_qws.moc"

#endif
