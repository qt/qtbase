// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qmacclipboard.h"
#include <QtGui/private/qmacmimeregistry_p.h>
#include <QtGui/qutimimeconverter.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qbitmap.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qdebug.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>
#include <QtCore/qurl.h>
#include <stdlib.h>
#include <string.h>
#include "qcocoahelpers.h"
#include <type_traits>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*****************************************************************************
   QMacPasteboard code
*****************************************************************************/

namespace
{
OSStatus PasteboardGetItemCountSafe(PasteboardRef paste, ItemCount *cnt)
{
    Q_ASSERT(paste);
    Q_ASSERT(cnt);
    const OSStatus result = PasteboardGetItemCount(paste, cnt);
    // Despite being declared unsigned, this API can return -1
    if (std::make_signed<ItemCount>::type(*cnt) < 0)
        *cnt = 0;
    return result;
}
} // namespace

// Ensure we don't call the broken one later on
#define PasteboardGetItemCount

class QMacMimeData : public QMimeData
{
public:
    QVariant variantData(const QString &mime) { return retrieveData(mime, QMetaType()); }
private:
    QMacMimeData();
};

QMacPasteboard::Promise::Promise(int itemId, const QUtiMimeConverter *c, const QString &m, QMimeData *md, int o, DataRequestType drt)
    : itemId(itemId), offset(o), converter(c), mime(m), dataRequestType(drt)
{
    // Request the data from the application immediately for eager requests.
    if (dataRequestType == QMacPasteboard::EagerRequest) {
        variantData = static_cast<QMacMimeData *>(md)->variantData(m);
        isPixmap = variantData.metaType().id() == QMetaType::QPixmap;
        mimeData = nullptr;
    } else {
        mimeData = md;
        if (md->hasImage())
            isPixmap = md->imageData().metaType().id() == QMetaType::QPixmap;
    }
}

QMacPasteboard::QMacPasteboard(PasteboardRef p, QUtiMimeConverter::HandlerScope scope)
    : scope(scope)
{
    mac_mime_source = false;
    paste = p;
    CFRetain(paste);
    resolvingBeforeDestruction = false;
}

QMacPasteboard::QMacPasteboard(QUtiMimeConverter::HandlerScope scope)
    : scope(scope)
{
    mac_mime_source = false;
    paste = nullptr;
    OSStatus err = PasteboardCreate(nullptr, &paste);
    if (err == noErr)
        PasteboardSetPromiseKeeper(paste, promiseKeeper, this);
    else
        qDebug("PasteBoard: Error creating pasteboard: [%d]", (int)err);
    resolvingBeforeDestruction = false;
}

QMacPasteboard::QMacPasteboard(CFStringRef name, QUtiMimeConverter::HandlerScope scope)
    : scope(scope)
{
    mac_mime_source = false;
    paste = nullptr;
    OSStatus err = PasteboardCreate(name, &paste);
    if (err == noErr) {
        PasteboardSetPromiseKeeper(paste, promiseKeeper, this);
    } else {
        qDebug("PasteBoard: Error creating pasteboard: %s [%d]", QString::fromCFString(name).toLatin1().constData(), (int)err);
    }
    resolvingBeforeDestruction = false;
}

QMacPasteboard::~QMacPasteboard()
{
    /*
        Commit all promises for paste when shutting down,
        unless we are the stack-allocated clipboard used by QCocoaDrag.
    */
    if (scope == QUtiMimeConverter::HandlerScopeFlag::DnD)
        resolvingBeforeDestruction = true;
    PasteboardResolvePromises(paste);
    if (paste)
        CFRelease(paste);
}

PasteboardRef QMacPasteboard::pasteBoard() const
{
    return paste;
}

OSStatus QMacPasteboard::promiseKeeper(PasteboardRef paste, PasteboardItemID id,
                                       CFStringRef uti, void *_qpaste)
{
    QMacPasteboard *qpaste = (QMacPasteboard*)_qpaste;
    const long promise_id = (long)id;

    // Find the kept promise
    const QList<QUtiMimeConverter*> availableConverters = QMacMimeRegistry::all(QUtiMimeConverter::HandlerScopeFlag::All);
    const QString utiAsQString = QString::fromCFString(uti);
    QMacPasteboard::Promise promise;
    for (int i = 0; i < qpaste->promises.size(); i++){
        const QMacPasteboard::Promise tmp = qpaste->promises[i];
        if (!availableConverters.contains(tmp.converter)) {
            // promise.converter is a pointer initialized by the value found
            // in QUtiMimeConverter's global list of QMacMimes.
            // We add pointers to this list in QUtiMimeConverter's ctor;
            // we remove these pointers in QUtiMimeConverter's dtor.
            // If tmp.converter was not found in this list, we probably have a
            // dangling pointer so let's skip it.
            continue;
        }

        if (tmp.itemId == promise_id && tmp.converter->canConvert(tmp.mime, utiAsQString)) {
            promise = tmp;
            break;
        }
    }

    if (!promise.itemId && utiAsQString == "com.trolltech.qt.MimeTypeName"_L1) {
        // we have promised this data, but won't be able to convert, so return null data.
        // This helps in making the application/x-qt-mime-type-name hidden from normal use.
        QByteArray ba;
        const QCFType<CFDataRef> data = CFDataCreate(nullptr, (UInt8*)ba.constData(), ba.size());
        PasteboardPutItemFlavor(paste, id, uti, data, kPasteboardFlavorNoFlags);
        return noErr;
    }

    if (!promise.itemId) {
        // There was no promise that could deliver data for the
        // given id and uti. This should not happen.
        qDebug("Pasteboard: %d: Request for %ld, %s, but no promise found!", __LINE__, promise_id, qPrintable(utiAsQString));
        return cantGetFlavorErr;
    }

    qCDebug(lcQpaClipboard, "PasteBoard: Calling in promise for %s[%ld] [%s] [%d]", qPrintable(promise.mime), promise_id,
           qPrintable(utiAsQString), promise.offset);

    // Get the promise data. If this is a "lazy" promise call variantData()
    // to request the data from the application.
    QVariant promiseData;
    if (promise.dataRequestType == LazyRequest) {
        if (!qpaste->resolvingBeforeDestruction && !promise.mimeData.isNull()) {
            if (promise.isPixmap && !QGuiApplication::instance()) {
                qCWarning(lcQpaClipboard,
                          "Cannot keep promise, data contains QPixmap and requires livining QGuiApplication");
                return cantGetFlavorErr;
            }
            promiseData = static_cast<QMacMimeData *>(promise.mimeData.data())->variantData(promise.mime);
        }
    } else {
        promiseData = promise.variantData;
    }

    const QList<QByteArray> md = promise.converter->convertFromMime(promise.mime, promiseData, utiAsQString);
    if (md.size() <= promise.offset)
        return cantGetFlavorErr;
    const QByteArray &ba = md[promise.offset];
    const QCFType<CFDataRef> data = CFDataCreate(nullptr, (UInt8*)ba.constData(), ba.size());
    PasteboardPutItemFlavor(paste, id, uti, data, kPasteboardFlavorNoFlags);
    return noErr;
}

bool QMacPasteboard::hasUti(const QString &uti) const
{
    if (!paste)
        return false;

    sync();

    ItemCount cnt = 0;
    if (PasteboardGetItemCountSafe(paste, &cnt) || !cnt)
        return false;

    qCDebug(lcQpaClipboard, "PasteBoard: hasUti [%s]", qPrintable(uti));
    const QCFString c_uti(uti);
    for (uint index = 1; index <= cnt; ++index) {

        PasteboardItemID id;
        if (PasteboardGetItemIdentifier(paste, index, &id) != noErr)
            return false;

        PasteboardFlavorFlags flags;
        if (PasteboardGetItemFlavorFlags(paste, id, c_uti, &flags) == noErr) {
            qCDebug(lcQpaClipboard, "  - Found!");
            return true;
        }
    }
    qCDebug(lcQpaClipboard, "  - NotFound!");
    return false;
}

class QMacPasteboardMimeSource : public QMimeData
{
    const QMacPasteboard *paste;
public:
    QMacPasteboardMimeSource(const QMacPasteboard *p) : QMimeData(), paste(p) { }
    ~QMacPasteboardMimeSource() { }
    QStringList formats() const override { return paste->formats(); }
    QVariant retrieveData(const QString &format, QMetaType) const override
    {
        return paste->retrieveData(format);
    }
};

QMimeData *QMacPasteboard::mimeData() const
{
    if (!mime) {
        mac_mime_source = true;
        mime = new QMacPasteboardMimeSource(this);

    }
    return mime;
}

void QMacPasteboard::setMimeData(QMimeData *mime_src, DataRequestType dataRequestType)
{
    if (!paste)
        return;

    if (mime == mime_src || (!mime_src && mime && mac_mime_source))
        return;
    mac_mime_source = false;
    delete mime;
    mime = mime_src;

    const QList<QUtiMimeConverter*> availableConverters = QMacMimeRegistry::all(scope);
    if (mime != nullptr) {
        clear_helper();
        QStringList formats = mime_src->formats();

        // QMimeData sub classes reimplementing the formats() might not expose the
        // temporary "application/x-qt-mime-type-name" mimetype. So check the existence
        // of this mime type while doing drag and drop.
        QString dummyMimeType("application/x-qt-mime-type-name"_L1);
        if (!formats.contains(dummyMimeType)) {
            QByteArray dummyType = mime_src->data(dummyMimeType);
            if (!dummyType.isEmpty())
                formats.append(dummyMimeType);
        }
        for (const auto &mimeType : formats) {
            for (const auto *c : availableConverters) {
                // Hack: The Rtf handler converts incoming Rtf to Html. We do
                // not want to convert outgoing Html to Rtf but instead keep
                // posting it as Html. Skip the Rtf handler here.
                if (c->utiForMime("text/html"_L1) == "public.rtf"_L1)
                    continue;
                const QString uti(c->utiForMime(mimeType));
                if (!uti.isEmpty()) {

                    const int numItems = c->count(mime_src);
                    for (int item = 0; item < numItems; ++item) {
                        const NSInteger itemID = item + 1; //id starts at 1
                        //QMacPasteboard::Promise promise = (dataRequestType == QMacPasteboard::EagerRequest) ?
                        //    QMacPasteboard::Promise::eagerPromise(itemID, c, mimeType, mimeData, item) :
                        //    QMacPasteboard::Promise::lazyPromise(itemID, c, mimeType, mimeData, item);

                        const QMacPasteboard::Promise promise(itemID, c, mimeType, mime_src, item, dataRequestType);
                        promises.append(promise);
                        PasteboardPutItemFlavor(paste, reinterpret_cast<PasteboardItemID>(itemID), QCFString(uti), 0, kPasteboardFlavorNoFlags);
                        qCDebug(lcQpaClipboard, " -  adding %ld %s [%s] [%d]",
                               itemID, qPrintable(mimeType), qPrintable(uti), item);
                    }
                }
            }
        }
    }
}

QStringList QMacPasteboard::formats() const
{
    if (!paste)
        return QStringList();

    sync();

    QStringList ret;
    ItemCount cnt = 0;
    if (PasteboardGetItemCountSafe(paste, &cnt) || !cnt)
        return ret;

    qCDebug(lcQpaClipboard, "PasteBoard: Formats [%d]", (int)cnt);
    for (uint index = 1; index <= cnt; ++index) {

        PasteboardItemID id;
        if (PasteboardGetItemIdentifier(paste, index, &id) != noErr)
            continue;

        QCFType<CFArrayRef> types;
        if (PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
            continue;

        const int type_count = CFArrayGetCount(types);
        for (int i = 0; i < type_count; ++i) {
            const QString uti = QString::fromCFString((CFStringRef)CFArrayGetValueAtIndex(types, i));
            qCDebug(lcQpaClipboard, " -%s", qPrintable(QString(uti)));
            const QString mimeType = QMacMimeRegistry::flavorToMime(scope, uti);
            if (!mimeType.isEmpty() && !ret.contains(mimeType)) {
                qCDebug(lcQpaClipboard, "   -<%lld> %s [%s]", ret.size(), qPrintable(mimeType), qPrintable(QString(uti)));
                ret << mimeType;
            }
        }
    }
    return ret;
}

bool QMacPasteboard::hasFormat(const QString &format) const
{
    if (!paste)
        return false;

    sync();

    ItemCount cnt = 0;
    if (PasteboardGetItemCountSafe(paste, &cnt) || !cnt)
        return false;

    qCDebug(lcQpaClipboard, "PasteBoard: hasFormat [%s]", qPrintable(format));
    for (uint index = 1; index <= cnt; ++index) {

        PasteboardItemID id;
        if (PasteboardGetItemIdentifier(paste, index, &id) != noErr)
            continue;

        QCFType<CFArrayRef> types;
        if (PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
            continue;

        const int type_count = CFArrayGetCount(types);
        for (int i = 0; i < type_count; ++i) {
            const QString uti = QString::fromCFString((CFStringRef)CFArrayGetValueAtIndex(types, i));
            qCDebug(lcQpaClipboard, " -%s [0x%x]", qPrintable(uti), uchar(scope));
            QString mimeType = QMacMimeRegistry::flavorToMime(scope, uti);
            if (!mimeType.isEmpty())
                qCDebug(lcQpaClipboard, "   - %s", qPrintable(mimeType));
            if (mimeType == format)
                return true;
        }
    }
    return false;
}

QVariant QMacPasteboard::retrieveData(const QString &format) const
{
    if (!paste)
        return QVariant();

    sync();

    ItemCount cnt = 0;
    if (PasteboardGetItemCountSafe(paste, &cnt) || !cnt)
        return QByteArray();

    qCDebug(lcQpaClipboard, "Pasteboard: retrieveData [%s]", qPrintable(format));
    const QList<QUtiMimeConverter *> availableConverters = QMacMimeRegistry::all(scope);
    for (const auto *c : availableConverters) {
        const QString c_uti = c->utiForMime(format);
        if (!c_uti.isEmpty()) {
            // Converting via PasteboardCopyItemFlavorData below will for some UITs result
            // in newlines mapping to '\r' instead of '\n'. To work around this we shortcut
            // the conversion via NSPasteboard's NSStringPboardType if possible.
            if (c_uti == "com.apple.traditional-mac-plain-text"_L1
             || c_uti == "public.utf8-plain-text"_L1
             || c_uti == "public.utf16-plain-text"_L1) {
                const QString str = qt_mac_get_pasteboardString(paste);
                if (!str.isEmpty())
                    return str;
            }

            QVariant ret;
            QList<QByteArray> retList;
            for (uint index = 1; index <= cnt; ++index) {
                PasteboardItemID id;
                if (PasteboardGetItemIdentifier(paste, index, &id) != noErr)
                    continue;

                QCFType<CFArrayRef> types;
                if (PasteboardCopyItemFlavors(paste, id, &types ) != noErr)
                    continue;

                const int type_count = CFArrayGetCount(types);
                for (int i = 0; i < type_count; ++i) {
                    const CFStringRef uti = static_cast<CFStringRef>(CFArrayGetValueAtIndex(types, i));
                    if (c_uti == QString::fromCFString(uti)) {
                        QCFType<CFDataRef> macBuffer;
                        if (PasteboardCopyItemFlavorData(paste, id, uti, &macBuffer) == noErr) {
                            QByteArray buffer((const char *)CFDataGetBytePtr(macBuffer),
                                              CFDataGetLength(macBuffer));
                            if (!buffer.isEmpty()) {
                                qCDebug(lcQpaClipboard, "  - %s [%s]", qPrintable(format),
                                                                       qPrintable(c_uti));
                                buffer.detach(); //detach since we release the macBuffer
                                retList.append(buffer);
                                break; //skip to next element
                            }
                        }
                    } else {
                        qCDebug(lcQpaClipboard, "  - NoMatch %s [%s]", qPrintable(c_uti),
                                                qPrintable(QString::fromCFString(uti)));
                    }
                }
            }

            if (!retList.isEmpty()) {
                ret = c->convertToMime(format, retList, c_uti);
                return ret;
            }
        }
    }
    return QVariant();
}

void QMacPasteboard::clear_helper()
{
    if (paste)
        PasteboardClear(paste);
    promises.clear();
}

void QMacPasteboard::clear()
{
    qCDebug(lcQpaClipboard, "PasteBoard: clear!");
    clear_helper();
}

bool QMacPasteboard::sync() const
{
    if (!paste)
        return false;
    const bool fromGlobal = PasteboardSynchronize(paste) & kPasteboardModified;

    if (fromGlobal)
        const_cast<QMacPasteboard *>(this)->setMimeData(nullptr);

    if (fromGlobal)
        qCDebug(lcQpaClipboard, "Pasteboard: Synchronize!");
    return fromGlobal;
}


QString qt_mac_get_pasteboardString(PasteboardRef paste)
{
    QMacAutoReleasePool pool;
    NSPasteboard *pb = nil;
    CFStringRef pbname;
    if (PasteboardCopyName(paste, &pbname) == noErr) {
        pb = [NSPasteboard pasteboardWithName:const_cast<NSString *>(reinterpret_cast<const NSString *>(pbname))];
        CFRelease(pbname);
    } else {
        pb = [NSPasteboard generalPasteboard];
    }
    if (pb) {
        NSString *text = [pb stringForType:NSPasteboardTypeString];
        if (text)
            return QString::fromNSString(text);
    }
    return QString();
}

QT_END_NAMESPACE
