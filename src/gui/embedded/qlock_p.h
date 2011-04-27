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

#ifndef QLOCK_P_H
#define QLOCK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include "QtCore/qstring.h"

QT_BEGIN_NAMESPACE

class QLockData;

class Q_GUI_EXPORT QLock
{
public:
    QLock(const QString &filename, char id, bool create = false);
    ~QLock();

    enum Type { Read, Write };

    bool isValid() const;
    void lock(Type type);
    void unlock();
    bool locked() const;

private:
    Type type;
    QLockData *data;
};


// Nice class for ensuring the lock is released.
// Just create one on the stack and the lock is automatically released
// when QLockHandle is destructed.
class Q_GUI_EXPORT QLockHandle
{
public:
    QLockHandle(QLock *l, QLock::Type type) : qlock(l) { qlock->lock(type); }
    ~QLockHandle() { if (locked()) qlock->unlock(); }

    void lock(QLock::Type type) { qlock->lock(type); }
    void unlock() { qlock->unlock(); }
    bool locked() const { return qlock->locked(); }

private:
    QLock *qlock;
};

QT_END_NAMESPACE

#endif // QLOCK_P_H
