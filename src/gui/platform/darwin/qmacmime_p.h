// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACMIME_H
#define QMACMIME_H

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

class Q_GUI_EXPORT QMacInternalPasteboardMime
{
public:
    enum QMacPasteboardMimeType
    {
        MIME_DND            = 0x01,
        MIME_CLIP           = 0x02,
        MIME_QT_CONVERTOR   = 0x04,
        MIME_QT3_CONVERTOR  = 0x08,
        MIME_ALL            = MIME_DND|MIME_CLIP,
        MIME_ALL_COMPATIBLE = MIME_ALL|MIME_QT_CONVERTOR
    };

    explicit QMacInternalPasteboardMime(QMacPasteboardMimeType);
    virtual ~QMacInternalPasteboardMime();

    char type() const { return m_type; }

    virtual bool canConvert(const QString &mime, const QString &flav) const = 0;
    virtual QString mimeFor(const QString &flav) const = 0;
    virtual QString flavorFor(const QString &mime) const = 0;
    virtual QVariant convertToMime(const QString &mime, const QList<QByteArray> &data, const QString &flav) const = 0;
    virtual QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data, const QString &flav) const = 0;
    virtual int count(const QMimeData *mimeData) const;

private:
    const QMacPasteboardMimeType m_type;
};

QT_END_NAMESPACE

#endif

