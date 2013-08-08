/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDATETIME_P_H
#define QDATETIME_P_H

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

#include "qplatformdefs.h"
#include "QtCore/qatomic.h"
#include "QtCore/qdatetime.h"

QT_BEGIN_NAMESPACE

class QDateTimePrivate : public QSharedData
{
public:
    enum Spec { LocalUnknown = -1, LocalStandard = 0, LocalDST = 1, UTC = 2, OffsetFromUTC = 3};

    QDateTimePrivate() : spec(LocalUnknown), m_offsetFromUtc(0) {}
    QDateTimePrivate(const QDate &toDate, const QTime &toTime, Qt::TimeSpec toSpec,
                     int offsetSeconds);
    QDateTimePrivate(const QDateTimePrivate &other)
        : QSharedData(other), date(other.date), time(other.time), spec(other.spec),
          m_offsetFromUtc(other.m_offsetFromUtc)
    {}

    QDate date;
    QTime time;
    Spec spec;
    int m_offsetFromUtc;

    // Get current date/time in LocalTime and put result in outDate and outTime
    Spec getLocal(QDate &outDate, QTime &outTime) const;
    // Get current date/time in UTC and put result in outDate and outTime
    void getUTC(QDate &outDate, QTime &outTime) const;

    // Add msecs to given datetime and return result
    static QDateTime addMSecs(const QDateTime &dt, qint64 msecs);
    // Add msecs to given datetime and put result in utcDate and utcTime
    static void addMSecs(QDate &utcDate, QTime &utcTime, qint64 msecs);

    static inline qint64 minJd() { return QDate::minJd(); }
    static inline qint64 maxJd() { return QDate::maxJd(); }
};

QT_END_NAMESPACE

#endif // QDATETIME_P_H
