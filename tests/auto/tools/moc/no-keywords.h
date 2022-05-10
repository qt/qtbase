// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Q_DECL_UNUSED_MEMBER int signals;
    Q_DECL_UNUSED_MEMBER double slots;
};

#define signals Q_SIGNALS
#define slots Q_SLOTS
#define emit Q_EMIT
#undef QT_NO_KEYWORDS

#endif // NO_KEYWORDS_H
