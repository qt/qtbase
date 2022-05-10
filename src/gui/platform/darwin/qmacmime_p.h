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

// Duplicate of QMacPasteboardMime in QtMacExtras. Keep in sync!
class Q_GUI_EXPORT QMacInternalPasteboardMime {
    char type;
public:
    enum QMacPasteboardMimeType { MIME_DND=0x01,
        MIME_CLIP=0x02,
        MIME_QT_CONVERTOR=0x04,
        MIME_QT3_CONVERTOR=0x08,
        MIME_ALL=MIME_DND|MIME_CLIP
    };
    explicit QMacInternalPasteboardMime(char);
    virtual ~QMacInternalPasteboardMime();

    static void initializeMimeTypes();
    static void destroyMimeTypes();

    static QList<QMacInternalPasteboardMime*> all(uchar);
    static QMacInternalPasteboardMime *convertor(uchar, const QString &mime, QString flav);
    static QString flavorToMime(uchar, QString flav);

    virtual QString convertorName() = 0;

    virtual bool canConvert(const QString &mime, QString flav) = 0;
    virtual QString mimeFor(QString flav) = 0;
    virtual QString flavorFor(const QString &mime) = 0;
    virtual QVariant convertToMime(const QString &mime, QList<QByteArray> data, QString flav) = 0;
    virtual QList<QByteArray> convertFromMime(const QString &mime, QVariant data, QString flav) = 0;
    virtual int count(QMimeData *mimeData);
};

Q_GUI_EXPORT void qt_mac_addToGlobalMimeList(QMacInternalPasteboardMime *macMime);
Q_GUI_EXPORT void qt_mac_removeFromGlobalMimeList(QMacInternalPasteboardMime *macMime);
Q_GUI_EXPORT void qt_mac_registerDraggedTypes(const QStringList &types);
Q_GUI_EXPORT const QStringList& qt_mac_enabledDraggedTypes();

QT_END_NAMESPACE

#endif

