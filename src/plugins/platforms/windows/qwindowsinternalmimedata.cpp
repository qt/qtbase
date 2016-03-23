/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsinternalmimedata.h"
#include "qwindowscontext.h"
#include "qwindowsmime.h"
#include <QDebug>
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
    does conversion using QWindowsMime classes.

    \sa QInternalMimeData, QWindowsMime, QWindowsMimeConverter
    \internal
    \ingroup qt-lighthouse-win
*/

bool QWindowsInternalMimeData::hasFormat_sys(const QString &mime) const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return false;

    const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
    const bool has = mc.converterToMime(mime, pDataObj) != 0;
    releaseDataObject(pDataObj);
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mime << has;
    return has;
}

QStringList QWindowsInternalMimeData::formats_sys() const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return QStringList();

    const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
    const QStringList fmts = mc.allMimesForFormats(pDataObj);
    releaseDataObject(pDataObj);
    qCDebug(lcQpaMime) << __FUNCTION__ <<  fmts;
    return fmts;
}

QVariant QWindowsInternalMimeData::retrieveData_sys(const QString &mimeType,
                                                        QVariant::Type type) const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return QVariant();

    QVariant result;
    const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
    if (const QWindowsMime *converter = mc.converterToMime(mimeType, pDataObj))
        result = converter->convertToMime(mimeType, pDataObj, type);
    releaseDataObject(pDataObj);
    if (QWindowsContext::verbose) {
        qCDebug(lcQpaMime) <<__FUNCTION__ << ' '  << mimeType << ' ' << type
            << " returns " << result.type()
            << (result.type() != QVariant::ByteArray ? result.toString() : QStringLiteral("<data>"));
    }
    return result;
}
