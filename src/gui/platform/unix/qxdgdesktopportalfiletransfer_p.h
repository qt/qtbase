// Copyright (C) 2026 David Redondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXDGDESKTOPPORTALFILETRANSFER_P_H
#define QXDGDESKTOPPORTALFILETRANSFER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qloggingcategory.h>

QT_REQUIRE_CONFIG(xdg_desktop_portal_file_transfer);

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaPortalFileTransfer);

namespace QXdgDesktopPortalFileTransfer {
[[nodiscard]] Q_GUI_EXPORT QLatin1StringView fileTransferMimeType();
[[nodiscard]] Q_GUI_EXPORT QString exportFiles(const QList<QUrl> &urls);
[[nodiscard]] Q_GUI_EXPORT QList<QString> retrieveFiles(const QString &key);
} // namespace QXdgDesktopPortalFileTransfer

QT_END_NAMESPACE

#endif
