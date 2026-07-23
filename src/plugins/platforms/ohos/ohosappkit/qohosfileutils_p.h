// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSFILEUTILS_P_H
#define QOHOSFILEUTILS_P_H

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

#include <QtCore/qstring.h>
#include <QtGui/qwindow.h>
#include <QtOhosAppKit/private/qtohosappkitglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

Q_OHOSAPPKIT_EXPORT bool authorizeFilePath(QWindow *parentWindow, const QString &filePath);

}

QT_END_NAMESPACE

#endif // QOHOSFILEUTILS_P_H
