/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QWINDOWSMIME_H
#define QWINDOWSMIME_H

#include <QtGui/private/qwindowsmime_p.h>

#include <QtCore/qt_windows.h>

#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QMimeData;

class QWindowsMimeConverter
{
    Q_DISABLE_COPY_MOVE(QWindowsMimeConverter)
public:
    using QWindowsMime = QNativeInterface::Private::QWindowsMime;

    QWindowsMimeConverter();
    ~QWindowsMimeConverter();

    QWindowsMime *converterToMime(const QString &mimeType, IDataObject *pDataObj) const;
    QStringList allMimesForFormats(IDataObject *pDataObj) const;
    QWindowsMime *converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const;
    QList<FORMATETC> allFormatsForMime(const QMimeData *mimeData) const;

    // Convenience.
    QVariant convertToMime(const QStringList &mimeTypes, IDataObject *pDataObj, QMetaType preferredType,
                           QString *format = nullptr) const;

    void registerMime(QWindowsMime *mime);
    void unregisterMime(QWindowsMime *mime) { m_mimes.removeOne(mime); }

    static int registerMimeType(const QString &mime);

    static QString clipboardFormatName(int cf);

private:
    void ensureInitialized() const;

    mutable QList<QWindowsMime *> m_mimes;
    mutable int m_internalMimeCount = 0;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const FORMATETC &);
QDebug operator<<(QDebug d, IDataObject *);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSMIME_H
