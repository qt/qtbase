/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QLIBRARY_P_H
#define QLIBRARY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qlibrary.h"
#include "QtCore/qmutex.h"
#include "QtCore/qpointer.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qplugin.h"
#ifdef Q_OS_WIN
#  include "QtCore/qt_windows.h"
#endif

QT_REQUIRE_CONFIG(library);

QT_BEGIN_NAMESPACE

bool qt_debug_component();

class QLibraryStore;
class QLibraryPrivate
{
public:
#ifdef Q_OS_WIN
    using Handle = HINSTANCE;
#else
    using Handle = void *;
#endif
    enum UnloadFlag { UnloadSys, NoUnloadSys };

    const QString fileName;
    const QString fullVersion;

    bool load();
    QtPluginInstanceFunction loadPlugin(); // loads and resolves instance
    bool unload(UnloadFlag flag = UnloadSys);
    void release();
    QFunctionPointer resolve(const char *);

    QLibrary::LoadHints loadHints() const
    { return QLibrary::LoadHints(loadHintsInt.loadRelaxed()); }
    void setLoadHints(QLibrary::LoadHints lh);
    QObject *pluginInstance();

    static QLibraryPrivate *findOrCreate(const QString &fileName, const QString &version = QString(),
                                         QLibrary::LoadHints loadHints = { });
    static QStringList suffixes_sys(const QString &fullVersion);
    static QStringList prefixes_sys();

    QAtomicPointer<std::remove_pointer<QtPluginInstanceFunction>::type> instanceFactory;
    QAtomicPointer<std::remove_pointer<Handle>::type> pHnd;

    // the mutex protects the fields below
    QMutex mutex;
    QPointer<QObject> inst;         // used by QFactoryLoader
    QJsonObject metaData;
    QString errorString;
    QString qualifiedFileName;

    void updatePluginState();
    bool isPlugin();

private:
    explicit QLibraryPrivate(const QString &canonicalFileName, const QString &version, QLibrary::LoadHints loadHints);
    ~QLibraryPrivate();
    void mergeLoadHints(QLibrary::LoadHints loadHints);

    bool load_sys();
    bool unload_sys();
    QFunctionPointer resolve_sys(const char *);

    QAtomicInt loadHintsInt;

    /// counts how many QLibrary or QPluginLoader are attached to us, plus 1 if it's loaded
    QAtomicInt libraryRefCount;
    /// counts how many times load() or loadPlugin() were called
    QAtomicInt libraryUnloadCount;

    enum { IsAPlugin, IsNotAPlugin, MightBeAPlugin } pluginState;
    friend class QLibraryStore;
};

QT_END_NAMESPACE

#endif // QLIBRARY_P_H
