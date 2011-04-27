/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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


#ifndef DBUSDISPATCHER_H
#define DBUSDISPATCHER_H

#include <QObject>
#include <QVariant>

namespace Maemo {

class DBusDispatcherPrivate;
class DBusDispatcher : public QObject
{
    Q_OBJECT

public:
    DBusDispatcher(const QString& service,
                   const QString& path,
                   const QString& interface,
                   QObject *parent = 0);
    DBusDispatcher(const QString& service,
                   const QString& path,
                   const QString& interface,
                   const QString& signalPath,
                   QObject *parent = 0);
    ~DBusDispatcher();

    QList<QVariant> call(const QString& method, 
                         const QVariant& arg1 = QVariant(),
                         const QVariant& arg2 = QVariant(),
                         const QVariant& arg3 = QVariant(),
                         const QVariant& arg4 = QVariant(),
                         const QVariant& arg5 = QVariant(),
                         const QVariant& arg6 = QVariant(),
                         const QVariant& arg7 = QVariant(),
                         const QVariant& arg8 = QVariant());
    bool callAsynchronous(const QString& method, 
                          const QVariant& arg1 = QVariant(),
                          const QVariant& arg2 = QVariant(),
                          const QVariant& arg3 = QVariant(),
                          const QVariant& arg4 = QVariant(),
                          const QVariant& arg5 = QVariant(),
                          const QVariant& arg6 = QVariant(),
                          const QVariant& arg7 = QVariant(),
                          const QVariant& arg8 = QVariant());
    void emitSignalReceived(const QString& interface, 
                            const QString& signal,
                            const QList<QVariant>& args);
    void emitCallReply(const QString& method,
                       const QList<QVariant>& args,
                       const QString& error = "");
    void synchronousDispatch(int timeout_ms);

Q_SIGNALS:
    void signalReceived(const QString& interface, 
                        const QString& signal,
                        const QList<QVariant>& args);
    void callReply(const QString& method,
                   const QList<QVariant>& args,
                   const QString& error);

protected:
    void setupDBus();

private:
    DBusDispatcherPrivate *d_ptr;
};

}  // Maemo namespace

#endif
