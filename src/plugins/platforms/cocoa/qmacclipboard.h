// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACCLIPBOARD_H
#define QMACCLIPBOARD_H

#include <QtGui>

#include <ApplicationServices/ApplicationServices.h>

QT_BEGIN_NAMESPACE

class QMacMimeData;
class QMacInternalPasteboardMime;

class QMacPasteboard
{
public:
    enum DataRequestType { EagerRequest, LazyRequest };
private:
    struct Promise {
        Promise() : itemId(0), convertor(nullptr) { }

        static Promise eagerPromise(int itemId, QMacInternalPasteboardMime *c, QString m, QMacMimeData *d, int o = 0);
        static Promise lazyPromise(int itemId, QMacInternalPasteboardMime *c, QString m, QMacMimeData *d, int o = 0);
        Promise(int itemId, QMacInternalPasteboardMime *c, QString m, QMacMimeData *md, int o, DataRequestType drt);

        int itemId, offset;
        QMacInternalPasteboardMime *convertor;
        QString mime;
        QPointer<QMacMimeData> mimeData;
        QVariant variantData;
        DataRequestType dataRequestType;
        // QMimeData can be set from QVariant, holding
        // QPixmap. When converting, this triggers
        // QPixmap's ctor which in turn requires QGuiApplication
        // to exist and thus will abort the application
        // abnormally if not.
        bool isPixmap = false;
    };
    QList<Promise> promises;

    PasteboardRef paste;
    uchar mime_type;
    mutable QPointer<QMimeData> mime;
    mutable bool mac_mime_source;
    bool resolvingBeforeDestruction;
    static OSStatus promiseKeeper(PasteboardRef, PasteboardItemID, CFStringRef, void *);
    void clear_helper();
public:
    QMacPasteboard(PasteboardRef p, uchar mime_type=0);
    QMacPasteboard(uchar mime_type);
    QMacPasteboard(CFStringRef name=nullptr, uchar mime_type=0);
    ~QMacPasteboard();

    bool hasFlavor(QString flavor) const;
    bool hasOSType(int c_flavor) const;

    PasteboardRef pasteBoard() const;
    QMimeData *mimeData() const;

    void setMimeData(QMimeData *mime, DataRequestType dataRequestType = EagerRequest);

    QStringList formats() const;
    bool hasFormat(const QString &format) const;
    QVariant retrieveData(const QString &format, QMetaType) const;

    void clear();
    bool sync() const;
};

QString qt_mac_get_pasteboardString(PasteboardRef paste);

QT_END_NAMESPACE

#endif
