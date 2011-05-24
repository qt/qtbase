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

#ifndef QUNIXSOCKETSERVER_P_H
#define QUNIXSOCKETSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QUnixSocketServerPrivate;
class Q_GUI_EXPORT QUnixSocketServer : public QObject
{
    Q_OBJECT
public:
    enum ServerError { NoError, InvalidPath, ResourceError, BindError,
                       ListenError };

    QUnixSocketServer(QObject *parent=0);
    virtual ~QUnixSocketServer();

    void close();

    ServerError serverError() const;

    bool isListening() const;
    bool listen(const QByteArray & path);

    int socketDescriptor() const;
    QByteArray serverAddress() const;
   
    int maxPendingConnections() const; 
    void setMaxPendingConnections(int numConnections);

protected:
    virtual void incomingConnection(int socketDescriptor) = 0;

private:
    QUnixSocketServer(const QUnixSocketServer &);
    QUnixSocketServer & operator=(const QUnixSocketServer &);

    friend class QUnixSocketServerPrivate;
    QUnixSocketServerPrivate * d;
};


QT_END_NAMESPACE
#endif // QUNIXSOCKETSERVER_P_H

