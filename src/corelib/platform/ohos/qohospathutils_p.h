// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPATHUTILS_H
#define QOHOSPATHUTILS_H

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

#include <QtCore/qglobal.h>
#include <optional>
#include <string>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT std::optional<std::string> tryMapPathToOhosFileUri(const std::string &path);
Q_CORE_EXPORT std::optional<std::string> tryMapOhosFileUriToPath(const std::string &ohosFileUri);

QT_END_NAMESPACE

#endif // QOHOSPATHUTILS_H
