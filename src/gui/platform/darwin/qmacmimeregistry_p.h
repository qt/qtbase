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
#include <QtGui/qutimimeconverter.h>

#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

namespace QMacMimeRegistry {
    Q_GUI_EXPORT void initializeMimeTypes();
    Q_GUI_EXPORT void destroyMimeTypes();

    Q_GUI_EXPORT void registerMimeConverter(QUtiMimeConverter *);
    Q_GUI_EXPORT void unregisterMimeConverter(QUtiMimeConverter *);

    Q_GUI_EXPORT QList<QUtiMimeConverter *> all(QUtiMimeConverter::HandlerScope scope);
    Q_GUI_EXPORT QString flavorToMime(QUtiMimeConverter::HandlerScope scope, const QString &flav);

    Q_GUI_EXPORT void registerDraggedTypes(const QStringList &types);
    Q_GUI_EXPORT const QStringList& enabledDraggedTypes();
};

QT_END_NAMESPACE

#endif // QMACMIMEREGISTRY_H
