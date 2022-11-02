// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACCLIPBOARD_H
#define QMACCLIPBOARD_H

#include <QtGui>
#include <QtGui/private/qmacmime_p.h>

#include <ApplicationServices/ApplicationServices.h>

QT_BEGIN_NAMESPACE

class QMacMimeData;
class QMacMime;

class QMacPasteboard
{
public:
    enum DataRequestType { EagerRequest, LazyRequest };
private:
    struct Promise {
        Promise() : itemId(0), converter(nullptr) { }

        static Promise eagerPromise(int itemId, QMacMime *c, QString m, QMacMimeData *d, int o = 0);
        static Promise lazyPromise(int itemId, QMacMime *c, QString m, QMacMimeData *d, int o = 0);
        Promise(int itemId, QMacMime *c, QString m, QMacMimeData *md, int o, DataRequestType drt);

        int itemId, offset;
        QMacMime *converter;
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
    const QMacMime::HandlerScope scope;
    mutable QPointer<QMimeData> mime;
    mutable bool mac_mime_source;
    bool resolvingBeforeDestruction;
    static OSStatus promiseKeeper(PasteboardRef, PasteboardItemID, CFStringRef, void *);
    void clear_helper();
public:
    QMacPasteboard(PasteboardRef p, QMacMime::HandlerScope scope = QMacMime::HandlerScope::All);
    QMacPasteboard(QMacMime::HandlerScope scope);
    QMacPasteboard(CFStringRef name=nullptr, QMacMime::HandlerScope scope = QMacMime::HandlerScope::All);
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
