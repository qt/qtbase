/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qmutex.h>
#include <private/qmutexpool_p.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

void *qdbus_resolve_me(const char *name);

#if !defined QT_LINKED_LIBDBUS

static QLibrary *qdbus_libdbus = 0;

void qdbus_unloadLibDBus()
{
    delete qdbus_libdbus;
    qdbus_libdbus = 0;
}

bool qdbus_loadLibDBus()
{
    static volatile bool triedToLoadLibrary = false;
#ifndef QT_NO_THREAD
    QMutexLocker locker(QMutexPool::globalInstanceGet((void *)&qdbus_resolve_me));
#endif
    QLibrary *&lib = qdbus_libdbus;
    if (triedToLoadLibrary)
        return lib && lib->isLoaded();

    lib = new QLibrary;
    triedToLoadLibrary = true;

    static int majorversions[] = { 3, 2, -1 };
    lib->unload();
    lib->setFileName(QLatin1String("dbus-1"));
    for (uint i = 0; i < sizeof(majorversions) / sizeof(majorversions[0]); ++i) {
        lib->setFileNameAndVersion(lib->fileName(), majorversions[i]);
        if (lib->load() && lib->resolve("dbus_connection_open_private"))
            return true;

        lib->unload();
    }

    delete lib;
    lib = 0;
    return false;
}

void *qdbus_resolve_conditionally(const char *name)
{
    if (qdbus_loadLibDBus())
        return qdbus_libdbus->resolve(name);
    return 0;
}

void *qdbus_resolve_me(const char *name)
{
    void *ptr = 0;
    if (!qdbus_loadLibDBus())
        qFatal("Cannot find libdbus-1 in your system to resolve symbol '%s'.", name);

    ptr = qdbus_libdbus->resolve(name);
    if (!ptr)
        qFatal("Cannot resolve '%s' in your libdbus-1.", name);

    return ptr;
}

Q_DESTRUCTOR_FUNCTION(qdbus_unloadLibDBus)

#endif // QT_LINKED_LIBDBUS

QT_END_NAMESPACE

#endif // QT_NO_DBUS
