/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef NO_KEYWORDS_H
#define NO_KEYWORDS_H

#define QT_NO_KEYWORDS
#undef signals
#undef slots
#undef emit
#define signals FooBar
#define slots Baz
#define emit Yoyodyne

#include <QtCore/QtCore>

#ifdef QT_CONCURRENT_LIB
#include <QtConcurrent/QtConcurrent>
#endif
#ifdef QT_NETWORK_LIB
#include <QtNetwork/QtNetwork>
#endif
#ifdef QT_SQL_LIB
#include <QtSql/QtSql>
#endif
#ifdef QT_DBUS_LIB
#include <QtDBus/QtDBus>
#endif

#undef signals
#undef slots
#undef emit

class MyBooooooostishClass : public QObject
{
    Q_OBJECT
public:
    inline MyBooooooostishClass() {}

Q_SIGNALS:
    void mySignal();

public Q_SLOTS:
    inline void mySlot() { mySignal(); }

private:
    int signals;
    double slots;
};

#define signals Q_SIGNALS
#define slots Q_SLOTS
#define emit Q_EMIT
#undef QT_NO_KEYWORDS

#endif // NO_KEYWORDS_H
