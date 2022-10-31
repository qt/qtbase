// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACMIMEREGISTRY_H
#define QMACMIMEREGISTRY_H

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

#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

class QMacInternalPasteboardMime;

namespace QMacMimeRegistry {
    Q_GUI_EXPORT void initializeMimeTypes();
    Q_GUI_EXPORT void destroyMimeTypes();

    Q_GUI_EXPORT void registerMimeConverter(QMacInternalPasteboardMime *);
    Q_GUI_EXPORT void unregisterMimeConverter(QMacInternalPasteboardMime *);

    Q_GUI_EXPORT QList<QMacInternalPasteboardMime *> all(uchar);
    Q_GUI_EXPORT QMacInternalPasteboardMime *convertor(uchar, const QString &mime, QString flav);
    Q_GUI_EXPORT QString flavorToMime(uchar, QString flav);

    Q_GUI_EXPORT void registerDraggedTypes(const QStringList &types);
    Q_GUI_EXPORT const QStringList& enabledDraggedTypes();
};

QT_END_NAMESPACE

#endif // QMACMIMEREGISTRY_H
