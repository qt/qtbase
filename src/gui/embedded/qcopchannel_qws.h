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

#ifndef QCOPCHANNEL_QWS_H
#define QCOPCHANNEL_QWS_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_COP

class QWSClient;
class QCopChannelPrivate;

class Q_GUI_EXPORT QCopChannel : public QObject
{
    Q_OBJECT
public:
    explicit QCopChannel(const QString& channel, QObject *parent=0);
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QCopChannel(const QString& channel, QObject *parent, const char *name);
#endif
    virtual ~QCopChannel();

    QString channel() const;

    static bool isRegistered(const QString&  channel);
    static bool send(const QString& channel, const QString& msg);
    static bool send(const QString& channel, const QString& msg,
                      const QByteArray &data);

    static bool flush();

    static void sendLocally( const QString& ch, const QString& msg,
                               const QByteArray &data);
    static void reregisterAll();

    virtual void receive(const QString& msg, const QByteArray &data);

Q_SIGNALS:
    void received(const QString& msg, const QByteArray &data);

private:
    void init(const QString& channel);

    // server side
    static void registerChannel(const QString& ch, QWSClient *cl);
    static void detach(QWSClient *cl);
    static void answer(QWSClient *cl, const QString& ch,
                        const QString& msg, const QByteArray &data);
    // client side
    QCopChannelPrivate* d;

    friend class QWSServer;
    friend class QWSServerPrivate;
    friend class QApplication;
};

#endif // QT_NO_COP

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOPCHANNEL_QWS_H
