// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:load-external-library-via-qlibrary

#include "qdbus_symbols_p.h"
#include <QtCore/qlatin1stringview.h>
#if QT_CONFIG(library)
#include <QtCore/qlibrary.h>
#include <QtCore/private/qlocking_p.h>
#endif
#include <QtCore/qmutex.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if !defined QT_LINKED_LIBDBUS

#if QT_CONFIG(library)
Q_CONSTINIT static QLibrary *qdbus_libdbus = nullptr;

void qdbus_unloadLibDBus()
{
    if (qdbus_libdbus) {
        if (qEnvironmentVariableIsSet("QDBUS_FORCE_SHUTDOWN"))
            qdbus_libdbus->resolve("dbus_shutdown")();
        qdbus_libdbus->unload();
    }
    delete qdbus_libdbus;
    qdbus_libdbus = nullptr;
}
#endif

bool qdbus_loadLibDBus()
{
#if QT_CONFIG(library)
#ifdef QT_BUILD_INTERNAL
    // this is to simulate a library load failure for our autotest suite.
    if (!qEnvironmentVariableIsEmpty("QT_SIMULATE_DBUS_LIBFAIL"))
        return false;
#endif

    Q_CONSTINIT static bool triedToLoadLibrary = false;
    Q_CONSTINIT static QBasicMutex mutex;
    const auto locker = qt_scoped_lock(mutex);

    QLibrary *&lib = qdbus_libdbus;
    if (triedToLoadLibrary)
        return lib && lib->isLoaded();

    lib = new QLibrary;
    lib->setLoadHints(QLibrary::ExportExternalSymbolsHint); // make libdbus symbols available for apps that need more advanced control over the dbus
    triedToLoadLibrary = true;

    static constexpr int majorversions[] = { 3, 2, -1 };
    const QString baseNames[] = {
#ifdef Q_OS_WIN
        "dbus-1"_L1,
#endif
        "libdbus-1"_L1
    };

    lib->unload();
    for (const int majorversion : majorversions) {
        for (const QString &baseName : baseNames) {
#ifdef Q_OS_WIN
            QString suffix;
            if (majorversion != -1)
                suffix = QString::number(- majorversion); // negative so it prepends the dash
            lib->setFileName(baseName + suffix);
#else
            lib->setFileNameAndVersion(baseName, majorversion);
#endif
            if (lib->load() && lib->resolve("dbus_connection_open_private"))
                return true;

            lib->unload();
        }
    }

    delete lib;
    lib = nullptr;
    return false;
#else
    return true;
#endif
}

QFunctionPointer qdbus_resolve_conditionally(const char *name)
{
#if QT_CONFIG(library)
    if (qdbus_loadLibDBus())
        return qdbus_libdbus->resolve(name);
#else
    Q_UNUSED(name);
#endif
    return nullptr;
}

QFunctionPointer qdbus_resolve_me(const char *name)
{
#if QT_CONFIG(library)
    if (Q_UNLIKELY(!qdbus_loadLibDBus()))
        qFatal("Cannot find libdbus-1 in your system to resolve symbol '%s'.", name);

    QFunctionPointer ptr = qdbus_libdbus->resolve(name);
    if (Q_UNLIKELY(!ptr))
        qFatal("Cannot resolve '%s' in your libdbus-1.", name);

    return ptr;
#else
    Q_UNUSED(name);
    return nullptr;
#endif
}

#else
static void qdbus_unloadLibDBus()
{
    if (qEnvironmentVariableIsSet("QDBUS_FORCE_SHUTDOWN"))
        dbus_shutdown();
}

#endif // !QT_LINKED_LIBDBUS

#if defined(QT_LINKED_LIBDBUS) || QT_CONFIG(library)
Q_DESTRUCTOR_FUNCTION(qdbus_unloadLibDBus)
#endif

QT_END_NAMESPACE

#endif // QT_NO_DBUS
