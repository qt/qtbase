// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include <QtCore/qurl.h>
#include <QtGui/private/qmacmimeregistry_p.h>
#include <QtGui/qutimimeconverter.h>
#include <QtCore/QMimeData>
#include <QtGui/QGuiApplication>

@interface UIPasteboard (QUIPasteboard)
+ (instancetype)pasteboardWithQClipboardMode:(QClipboard::Mode)mode;
@end

@implementation UIPasteboard (QUIPasteboard)
+ (instancetype)pasteboardWithQClipboardMode:(QClipboard::Mode)mode
{
    NSString *name = (mode == QClipboard::Clipboard) ? UIPasteboardNameGeneral : UIPasteboardNameFind;
    return [UIPasteboard pasteboardWithName:name create:NO];
}
@end

// --------------------------------------------------------------------

@interface QUIClipboard : NSObject
@end

@implementation QUIClipboard {
    QIOSClipboard *m_qiosClipboard;
    NSInteger m_changeCountClipboard;
    NSInteger m_changeCountFindBuffer;
}

- (instancetype)initWithQIOSClipboard:(QIOSClipboard *)qiosClipboard
{
    self = [super init];
    if (self) {
        m_qiosClipboard = qiosClipboard;
        m_changeCountClipboard = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::Clipboard].changeCount;
        m_changeCountFindBuffer = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::FindBuffer].changeCount;

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
    NSInteger changeCountClipboard = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::Clipboard].changeCount;
    NSInteger changeCountFindBuffer = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::FindBuffer].changeCount;

    if (m_changeCountClipboard != changeCountClipboard) {
        m_changeCountClipboard = changeCountClipboard;
        m_qiosClipboard->emitChanged(QClipboard::Clipboard);
    }

    if (m_changeCountFindBuffer != changeCountFindBuffer) {
        m_changeCountFindBuffer = changeCountFindBuffer;
        m_qiosClipboard->emitChanged(QClipboard::FindBuffer);
    }
}

@end

// --------------------------------------------------------------------

QT_BEGIN_NAMESPACE

class QIOSMimeData : public QMimeData {
public:
    QIOSMimeData(QClipboard::Mode mode) : QMimeData(), m_mode(mode) { }
    ~QIOSMimeData() { }

    QStringList formats() const override;
    QVariant retrieveData(const QString &mimeType, QMetaType type) const override;

private:
    const QClipboard::Mode m_mode;
};

QStringList QIOSMimeData::formats() const
{
    QStringList foundMimeTypes;
    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:m_mode];
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
    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:m_mode];
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
        return *m_mimeData.insert(mode, new QIOSMimeData(mode));
    return m_mimeData[mode];
}

void QIOSClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    Q_ASSERT(supportsMode(mode));

    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:mode];
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
                QVariantList urlList;
                for (QUrl url : mimeData->urls())
                    urlList << url;
                mimeDataAsVariant = QVariant(urlList);
            } else {
                mimeDataAsVariant = QVariant(mimeData->data(mimeType));
            }

            QByteArray byteArray = converter->convertFromMime(mimeType, mimeDataAsVariant, uti).first();
            NSData *nsData = [NSData dataWithBytes:byteArray.constData() length:byteArray.size()];
            [pbItem setValue:nsData forKey:uti.toNSString()];
            break;
        }
    }

    pb.items = @[pbItem];
}

bool QIOSClipboard::supportsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard || mode == QClipboard::FindBuffer);
}

bool QIOSClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
