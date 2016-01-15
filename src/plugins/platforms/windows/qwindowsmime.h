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

#ifndef QWINDOWSMIME_H
#define QWINDOWSMIME_H

#include "qtwindows_additional.h"

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QDebug;
class QMimeData;

class QWindowsMime
{
    Q_DISABLE_COPY(QWindowsMime)
public:
    QWindowsMime();
    virtual ~QWindowsMime();

    // for converting from Qt
    virtual bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const = 0;
    virtual bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const = 0;
    virtual QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const = 0;

    // for converting to Qt
    virtual bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const = 0;
    virtual QVariant convertToMime(const QString &mimeType, IDataObject *pDataObj, QVariant::Type preferredType) const = 0;
    virtual QString mimeForFormat(const FORMATETC &formatetc) const = 0;

    static int registerMimeType(const QString &mime);
};

class QWindowsMimeConverter
{
    Q_DISABLE_COPY(QWindowsMimeConverter)
public:
    QWindowsMimeConverter();
    ~QWindowsMimeConverter();

    QWindowsMime *converterToMime(const QString &mimeType, IDataObject *pDataObj) const;
    QStringList allMimesForFormats(IDataObject *pDataObj) const;
    QWindowsMime *converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const;
    QVector<FORMATETC> allFormatsForMime(const QMimeData *mimeData) const;

    // Convenience.
    QVariant convertToMime(const QStringList &mimeTypes, IDataObject *pDataObj, QVariant::Type preferredType,
                           QString *format = 0) const;

    void registerMime(QWindowsMime *mime);
    void unregisterMime(QWindowsMime *mime) { m_mimes.removeOne(mime); }

    static QString clipboardFormatName(int cf);

private:
    void ensureInitialized() const;

    mutable QList<QWindowsMime *> m_mimes;
    mutable int m_internalMimeCount;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const FORMATETC &);
QDebug operator<<(QDebug d, IDataObject *);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSMIME_H
