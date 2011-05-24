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

#ifndef QSOUNDQSS_QWS_H
#define QSOUNDQSS_QWS_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_SOUND

#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtGui/qwssocket_qws.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if defined(QT_NO_NETWORK) || defined(QT_NO_DNS)
#define QT_NO_QWS_SOUNDSERVER
#endif

#ifndef Q_OS_MAC

class QWSSoundServerPrivate;

class Q_GUI_EXPORT QWSSoundServer : public QObject {
    Q_OBJECT
public:
    explicit QWSSoundServer(QObject *parent=0);
    ~QWSSoundServer();
    void playFile( int id, const QString& filename );
    void stopFile( int id );
    void pauseFile( int id );
    void resumeFile( int id );
    
Q_SIGNALS:
    void soundCompleted( int );
    
private Q_SLOTS:
    void translateSoundCompleted( int, int );

private:
    QWSSoundServerPrivate* d;
};

#ifndef QT_NO_QWS_SOUNDSERVER
class Q_GUI_EXPORT QWSSoundClient : public QWSSocket {
    Q_OBJECT
public:

    enum SoundFlags {
	Priority = 0x01,
	Streaming = 0x02  // currently ignored, but but could set up so both Raw and non raw can be done streaming or not.
    };
    enum DeviceErrors {
	ErrOpeningAudioDevice = 0x01,
	ErrOpeningFile = 0x02,
	ErrReadingFile = 0x04
    };
    explicit QWSSoundClient(QObject* parent=0);
    ~QWSSoundClient( );
    void reconnect();
    void play( int id, const QString& filename );
    void play( int id, const QString& filename, int volume, int flags = 0 );
    void playRaw( int id, const QString&, int, int, int, int flags = 0 );

    void pause( int id );
    void stop( int id );
    void resume( int id );
    void setVolume( int id, int left, int right );
    void setMute( int id, bool m );
    
    // to be used by server only, to protect phone conversation/rings.
    void playPriorityOnly(bool);

    // If silent, tell sound server to release audio device
    // Otherwise, allow sound server to regain audio device
    void setSilent(bool);
    
Q_SIGNALS:
    void soundCompleted(int);
    void deviceReady(int id);
    void deviceError(int id, QWSSoundClient::DeviceErrors);

private Q_SLOTS:
    void tryReadCommand();
    void emitConnectionRefused();
    
private:
    void sendServerMessage(QString msg);
};

class QWSSoundServerSocket : public QWSServerSocket {
    Q_OBJECT

public:
    explicit QWSSoundServerSocket(QObject *parent=0);
public Q_SLOTS:    
    void newConnection();

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QWSSoundServerSocket(QObject *parent, const char *name);
#endif

Q_SIGNALS:
    void playFile(int, int, const QString&);
    void playFile(int, int, const QString&, int, int);
    void playRawFile(int, int, const QString&, int, int, int, int);
    void pauseFile(int, int);
    void stopFile(int, int);
    void resumeFile(int, int);
    void setVolume(int, int, int, int);
    void setMute(int, int, bool);

    void stopAll(int);

    void playPriorityOnly(bool);

    void setSilent(bool);

    void soundFileCompleted(int, int);
    void deviceReady(int, int);
    void deviceError(int, int, int);
};
#endif

#endif // Q_OS_MAC

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_SOUND

#endif // QSOUNDQSS_QWS_H
