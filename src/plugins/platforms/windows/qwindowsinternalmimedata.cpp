// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsinternalmimedata.h"
#include "qwindowscontext.h"
#include "qwindowsmimeregistry.h"
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>

/*!
    \class QWindowsInternalMimeDataBase
    \brief Base for implementations of QInternalMimeData using a IDataObject COM object.

    In clipboard handling and Drag and drop, static instances
    of QInternalMimeData implementations are kept and passed to the client.

    QInternalMimeData provides virtuals that query the formats and retrieve
    mime data on demand when the client invokes functions like QMimeData::hasHtml(),
    QMimeData::html() on the instance returned. Otherwise, expensive
    construction of a new QMimeData object containing all possible
    formats would have to be done in each call to mimeData().

    The base class introduces new virtuals to obtain and release
    the instances IDataObject from the clipboard or Drag and Drop and
    does conversion using QWindowsMimeConverter classes.

    \sa QInternalMimeData, QWindowsMimeConverter, QWindowsMimeRegistry
    \internal
*/

bool QWindowsInternalMimeData::hasFormat_sys(const QString &mime) const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return false;

    const QWindowsMimeRegistry &mc = QWindowsContext::instance()->mimeConverter();
    const bool has = mc.converterToMime(mime, pDataObj) != nullptr;
    releaseDataObject(pDataObj);
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mime << has;
    return has;
}

QStringList QWindowsInternalMimeData::formats_sys() const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return QStringList();

    const QWindowsMimeRegistry &mc = QWindowsContext::instance()->mimeConverter();
    const QStringList fmts = mc.allMimesForFormats(pDataObj);
    releaseDataObject(pDataObj);
    qCDebug(lcQpaMime) << __FUNCTION__ <<  fmts;
    return fmts;
}

QVariant QWindowsInternalMimeData::retrieveData_sys(const QString &mimeType, QMetaType type) const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return QVariant();

    QVariant result;
    const QWindowsMimeRegistry &mc = QWindowsContext::instance()->mimeConverter();
    if (auto converter = mc.converterToMime(mimeType, pDataObj))
        result = converter->convertToMime(mimeType, pDataObj, type);
    releaseDataObject(pDataObj);
    if (QWindowsContext::verbose) {
        qCDebug(lcQpaMime) <<__FUNCTION__ << ' '  << mimeType << ' ' << type.name()
            << " returns " << result.metaType().name()
            << (result.metaType().id() != QMetaType::QByteArray ? result.toString() : QStringLiteral("<data>"));
    }
    return result;
}
