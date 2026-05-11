// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAPPCONTEXT_H
#define QOHOSAPPCONTEXT_H

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

#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

namespace QOhosAppContext {
enum class Type {
    bundleCodeDir,
    cacheDir,
    filesDir,
    preferencesDir,
    tempDir,
    databaseDir,
    distributedFilesDir,
    resourceDir,
};

Q_CORE_EXPORT void init(QMap<Type, QString> appcontext);
Q_CORE_EXPORT QString getProperty(Type prop);
Q_CORE_EXPORT QMap<Type, QString> getAllProperties();
}

QT_END_NAMESPACE

#endif // QOHOSAPPCONTEXT_H
