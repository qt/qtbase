// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACCLIPBOARD_H
#define QMACCLIPBOARD_H

#include <QtGui/qutimimeconverter.h>

#include <QtCore/qpointer.h>
#include <QtCore/qvariant.h>

#include <ApplicationServices/ApplicationServices.h>

QT_BEGIN_NAMESPACE

class QUtiMimeConverter;

class QMacPasteboard
{
public:
    enum DataRequestType { EagerRequest, LazyRequest };
private:
    struct Promise {
        Promise() : itemId(0), converter(nullptr) { }

        static Promise eagerPromise(int itemId, const QUtiMimeConverter *c, const QString &m, QMimeData *d, int o = 0);
        static Promise lazyPromise(int itemId, const QUtiMimeConverter *c, const QString &m, QMimeData *d, int o = 0);
        Promise(int itemId, const QUtiMimeConverter *c, const QString &m, QMimeData *md, int o, DataRequestType drt);

        int itemId, offset;
        const QUtiMimeConverter *converter;
        QString mime;
        QPointer<QMimeData> mimeData;
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
    const QUtiMimeConverter::HandlerScope scope;
    mutable QPointer<QMimeData> mime;
    mutable bool mac_mime_source;
    bool resolvingBeforeDestruction;
    static OSStatus promiseKeeper(PasteboardRef, PasteboardItemID, CFStringRef, void *);
    void clear_helper();
public:
    QMacPasteboard(PasteboardRef p, QUtiMimeConverter::HandlerScope scope = QUtiMimeConverter::HandlerScopeFlag::All);
    QMacPasteboard(QUtiMimeConverter::HandlerScope scope);
    QMacPasteboard(CFStringRef name=nullptr, QUtiMimeConverter::HandlerScope scope = QUtiMimeConverter::HandlerScopeFlag::All);
    ~QMacPasteboard();

    bool hasUti(const QString &uti) const;

    PasteboardRef pasteBoard() const;
    QMimeData *mimeData() const;

    void setMimeData(QMimeData *mime, DataRequestType dataRequestType = EagerRequest);

    QStringList formats() const;
    bool hasFormat(const QString &format) const;
    QVariant retrieveData(const QString &format) const;

    void clear();
    bool sync() const;
};

QString qt_mac_get_pasteboardString(PasteboardRef paste);

QT_END_NAMESPACE

#endif
