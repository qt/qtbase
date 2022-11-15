// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMIMEREGISTRY_H
#define QWINDOWSMIMEREGISTRY_H


#include <QtCore/qt_windows.h>

#include <QtGui/qwindowsmimeconverter.h>
#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QMimeData;

class QWindowsMimeRegistry
{
    Q_DISABLE_COPY_MOVE(QWindowsMimeRegistry)
public:
    using QWindowsMimeConverter = QWindowsMimeConverter;

    QWindowsMimeRegistry();
    ~QWindowsMimeRegistry();

    QWindowsMimeConverter *converterToMime(const QString &mimeType, IDataObject *pDataObj) const;
    QStringList allMimesForFormats(IDataObject *pDataObj) const;
    QWindowsMimeConverter *converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const;
    QList<FORMATETC> allFormatsForMime(const QMimeData *mimeData) const;

    // Convenience.
    QVariant convertToMime(const QStringList &mimeTypes, IDataObject *pDataObj, QMetaType preferredType,
                           QString *format = nullptr) const;

    void registerMime(QWindowsMimeConverter *mime);
    void unregisterMime(QWindowsMimeConverter *mime) { m_mimes.removeOne(mime); }

    static int registerMimeType(const QString &mime);

    static QString clipboardFormatName(int cf);

private:
    void ensureInitialized() const;

    mutable QList<QWindowsMimeConverter *> m_mimes;
    mutable int m_internalMimeCount = 0;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const FORMATETC &);
QDebug operator<<(QDebug d, IDataObject *);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSMIMEREGISTRY_H
