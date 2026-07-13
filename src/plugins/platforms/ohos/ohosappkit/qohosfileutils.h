// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSFILEUTILS_H
#define QOHOSFILEUTILS_H

#include <QtCore/qstring.h>
#include <QtGui/qwindow.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

Q_OHOSAPPKIT_EXPORT bool authorizeFilePath(QWindow *parentWindow, const QString &filePath);

}

QT_END_NAMESPACE

#endif // QOHOSFILEUTILS_H
