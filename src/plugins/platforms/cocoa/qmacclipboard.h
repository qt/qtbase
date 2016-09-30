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

#ifndef QMACCLIPBOARD_H
#define QMACCLIPBOARD_H

#include <QtGui>
#include <QtClipboardSupport/private/qmacmime_p.h>

#import <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

class QMacMimeData;
class QMacPasteboard
{
public:
    enum DataRequestType { EagerRequest, LazyRequest };
private:
    struct Promise {
        Promise() : itemId(0), convertor(0) { }

        static Promise eagerPromise(int itemId, QMacInternalPasteboardMime *c, QString m, QMacMimeData *d, int o = 0);
        static Promise lazyPromise(int itemId, QMacInternalPasteboardMime *c, QString m, QMacMimeData *d, int o = 0);
        Promise(int itemId, QMacInternalPasteboardMime *c, QString m, QMacMimeData *md, int o, DataRequestType drt);

        int itemId, offset;
        QMacInternalPasteboardMime *convertor;
        QString mime;
        QPointer<QMacMimeData> mimeData;
        QVariant variantData;
        DataRequestType dataRequestType;
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
    QMacPasteboard(CFStringRef name=0, uchar mime_type=0);
    ~QMacPasteboard();

    bool hasFlavor(QString flavor) const;
    bool hasOSType(int c_flavor) const;

    PasteboardRef pasteBoard() const;
    QMimeData *mimeData() const;

    void setMimeData(QMimeData *mime, DataRequestType dataRequestType = EagerRequest);

    QStringList formats() const;
    bool hasFormat(const QString &format) const;
    QVariant retrieveData(const QString &format, QVariant::Type) const;

    void clear();
    bool sync() const;
};

QString qt_mac_get_pasteboardString(PasteboardRef paste);

QT_END_NAMESPACE

#endif
