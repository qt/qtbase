// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMIME_P_H
#define QWINDOWSMIME_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qt_windows.h>
#include <QtCore/qvariant.h>

#include <QtGui/qtguiglobal.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QMimeData;

namespace QNativeInterface::Private {

class Q_GUI_EXPORT QWindowsMime
{
public:
    virtual ~QWindowsMime() = default;

    // for converting from Qt
    virtual bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const = 0;
    virtual bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const = 0;
    virtual QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const = 0;

    // for converting to Qt
    virtual bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const = 0;
    virtual QVariant convertToMime(const QString &mimeType, IDataObject *pDataObj, QMetaType preferredType) const = 0;
    virtual QString mimeForFormat(const FORMATETC &formatetc) const = 0;
};

} // QNativeInterface::Private

QT_END_NAMESPACE

#endif // QWINDOWSMIME_P_H
