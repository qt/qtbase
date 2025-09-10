// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMIMECONVERTER_P_H
#define QWINDOWSMIMECONVERTER_P_H

#include <QtGui/qtguiglobal.h>

struct tagFORMATETC;
using FORMATETC = tagFORMATETC;
struct tagSTGMEDIUM;
using STGMEDIUM = tagSTGMEDIUM;
struct IDataObject;

QT_BEGIN_NAMESPACE

class QMetaType;
class QMimeData;
class QVariant;

class Q_GUI_EXPORT QWindowsMimeConverter
{
    Q_DISABLE_COPY(QWindowsMimeConverter)
public:
    QWindowsMimeConverter();
    virtual ~QWindowsMimeConverter();

    static int registerMimeType(const QString &mimeType);

    // for converting from Qt
    virtual bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const = 0;
    virtual bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const = 0;
    virtual QList<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const = 0;

    // for converting to Qt
    virtual bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const = 0;
    virtual QVariant convertToMime(const QString &mimeType, IDataObject *pDataObj, QMetaType preferredType) const = 0;
    virtual QString mimeForFormat(const FORMATETC &formatetc) const = 0;
};

QT_END_NAMESPACE

#endif // QWINDOWSMIMECONVERTER_H
