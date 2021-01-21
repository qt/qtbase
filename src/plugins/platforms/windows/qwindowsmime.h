/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QWINDOWSMIME_H
#define QWINDOWSMIME_H

#include <QtCore/qt_windows.h>

#include <QtCore/qvector.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QMimeData;

class QWindowsMime
{
    Q_DISABLE_COPY_MOVE(QWindowsMime)
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
    Q_DISABLE_COPY_MOVE(QWindowsMimeConverter)
public:
    QWindowsMimeConverter();
    ~QWindowsMimeConverter();

    QWindowsMime *converterToMime(const QString &mimeType, IDataObject *pDataObj) const;
    QStringList allMimesForFormats(IDataObject *pDataObj) const;
    QWindowsMime *converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const;
    QVector<FORMATETC> allFormatsForMime(const QMimeData *mimeData) const;

    // Convenience.
    QVariant convertToMime(const QStringList &mimeTypes, IDataObject *pDataObj, QVariant::Type preferredType,
                           QString *format = nullptr) const;

    void registerMime(QWindowsMime *mime);
    void unregisterMime(QWindowsMime *mime) { m_mimes.removeOne(mime); }

    static QString clipboardFormatName(int cf);

private:
    void ensureInitialized() const;

    mutable QVector<QWindowsMime *> m_mimes;
    mutable int m_internalMimeCount = 0;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const FORMATETC &);
QDebug operator<<(QDebug d, IDataObject *);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSMIME_H
