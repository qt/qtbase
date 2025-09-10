// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include <QtCore/qurl.h>
#include <QtGui/private/qmacmimeregistry_p.h>
#include <QtGui/qutimimeconverter.h>
#include <QtCore/QMimeData>
#include <QtGui/QGuiApplication>

// --------------------------------------------------------------------

@interface QUIClipboard : NSObject
@end

@implementation QUIClipboard {
    QIOSClipboard *m_qiosClipboard;
    NSInteger m_changeCountClipboard;
}

- (instancetype)initWithQIOSClipboard:(QIOSClipboard *)qiosClipboard
{
    self = [super init];
    if (self) {
        m_qiosClipboard = qiosClipboard;
        m_changeCountClipboard = UIPasteboard.generalPasteboard.changeCount;

        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(updatePasteboardChanged:)
            name:UIPasteboardChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(updatePasteboardChanged:)
            name:UIPasteboardRemovedNotification object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(updatePasteboardChanged:)
            name:UIApplicationDidBecomeActiveNotification
            object:nil];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIPasteboardChangedNotification object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIPasteboardRemovedNotification object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIApplicationDidBecomeActiveNotification
        object:nil];
    [super dealloc];
}

- (void)updatePasteboardChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    NSInteger changeCountClipboard = UIPasteboard.generalPasteboard.changeCount;

    if (m_changeCountClipboard != changeCountClipboard) {
        m_changeCountClipboard = changeCountClipboard;
        m_qiosClipboard->emitChanged(QClipboard::Clipboard);
    }
}

@end

// --------------------------------------------------------------------

QT_BEGIN_NAMESPACE

class QIOSMimeData : public QMimeData {
public:
    QIOSMimeData() : QMimeData() { }
    ~QIOSMimeData() { }

    QStringList formats() const override;
    QVariant retrieveData(const QString &mimeType, QMetaType type) const override;
};

QStringList QIOSMimeData::formats() const
{
    QStringList foundMimeTypes;
    UIPasteboard *pb = UIPasteboard.generalPasteboard;
    NSArray<NSString *> *pasteboardTypes = [pb pasteboardTypes];

    for (NSUInteger i = 0; i < [pasteboardTypes count]; ++i) {
        const QString uti = QString::fromNSString([pasteboardTypes objectAtIndex:i]);
        const QString mimeType = QMacMimeRegistry::flavorToMime(QUtiMimeConverter::HandlerScopeFlag::All, uti);
        if (!mimeType.isEmpty() && !foundMimeTypes.contains(mimeType))
            foundMimeTypes << mimeType;
    }

    return foundMimeTypes;
}

QVariant QIOSMimeData::retrieveData(const QString &mimeType, QMetaType) const
{
    UIPasteboard *pb = UIPasteboard.generalPasteboard;
    NSArray<NSString *> *pasteboardTypes = [pb pasteboardTypes];

    const auto converters = QMacMimeRegistry::all(QUtiMimeConverter::HandlerScopeFlag::All);
    for (QUtiMimeConverter *converter : converters) {
        for (NSUInteger i = 0; i < [pasteboardTypes count]; ++i) {
            NSString *availableUtiNSString = [pasteboardTypes objectAtIndex:i];
            const QString availableUti = QString::fromNSString(availableUtiNSString);
            if (!converter->canConvert(mimeType, availableUti))
                continue;

            NSData *nsData = [pb dataForPasteboardType:availableUtiNSString];
            QList<QByteArray> dataList;
            dataList << QByteArray(reinterpret_cast<const char *>([nsData bytes]), [nsData length]);
            return converter->convertToMime(mimeType, dataList, availableUti);
        }
    }

    return QVariant();
}

// --------------------------------------------------------------------

QIOSClipboard::QIOSClipboard()
    : m_clipboard([[QUIClipboard alloc] initWithQIOSClipboard:this])
{
}

QIOSClipboard::~QIOSClipboard()
{
    qDeleteAll(m_mimeData);
}

QMimeData *QIOSClipboard::mimeData(QClipboard::Mode mode)
{
    Q_ASSERT(supportsMode(mode));
    if (!m_mimeData.contains(mode))
        return *m_mimeData.insert(mode, new QIOSMimeData);
    return m_mimeData[mode];
}

void QIOSClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    Q_ASSERT(supportsMode(mode));

    UIPasteboard *pb = UIPasteboard.generalPasteboard;
    if (!mimeData) {
        pb.items = [NSArray<NSDictionary<NSString *, id> *> array];
        return;
    }

    mimeData->deleteLater();
    NSMutableDictionary<NSString *, id> *pbItem = [NSMutableDictionary<NSString *, id> dictionaryWithCapacity:mimeData->formats().size()];

    const auto formats = mimeData->formats();
    for (const QString &mimeType : formats) {
        const auto converters = QMacMimeRegistry::all(QUtiMimeConverter::HandlerScopeFlag::All);
        for (const QUtiMimeConverter *converter : converters) {
            const QString uti = converter->utiForMime(mimeType);
            if (uti.isEmpty())
                continue;

            QVariant mimeDataAsVariant;
            if (mimeData->hasImage()) {
                mimeDataAsVariant = mimeData->imageData();
            } else if (mimeData->hasUrls()) {
                const auto urls = mimeData->urls();
                QVariantList urlList;
                urlList.reserve(urls.size());
                for (const QUrl& url : urls)
                    urlList << url;
                mimeDataAsVariant = QVariant(urlList);
            } else {
                mimeDataAsVariant = QVariant(mimeData->data(mimeType));
            }

            QByteArray byteArray = converter->convertFromMime(mimeType, mimeDataAsVariant, uti).constFirst();
            NSData *nsData = [NSData dataWithBytes:byteArray.constData() length:byteArray.size()];
            [pbItem setValue:nsData forKey:uti.toNSString()];
            break;
        }
    }

    pb.items = @[pbItem];
}

bool QIOSClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QIOSClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
