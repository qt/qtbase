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

#include "qiosclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include <QtClipboardSupport/private/qmacmime_p.h>
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
    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const override;

private:
    const QClipboard::Mode m_mode;
};

QStringList QIOSMimeData::formats() const
{
    QStringList foundMimeTypes;
    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:m_mode];
    NSArray<NSString *> *pasteboardTypes = [pb pasteboardTypes];

    for (NSUInteger i = 0; i < [pasteboardTypes count]; ++i) {
        QString uti = QString::fromNSString([pasteboardTypes objectAtIndex:i]);
        QString mimeType = QMacInternalPasteboardMime::flavorToMime(QMacInternalPasteboardMime::MIME_ALL, uti);
        if (!mimeType.isEmpty() && !foundMimeTypes.contains(mimeType))
            foundMimeTypes << mimeType;
    }

    return foundMimeTypes;
}

QVariant QIOSMimeData::retrieveData(const QString &mimeType, QVariant::Type) const
{
    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:m_mode];
    NSArray<NSString *> *pasteboardTypes = [pb pasteboardTypes];

    foreach (QMacInternalPasteboardMime *converter,
             QMacInternalPasteboardMime::all(QMacInternalPasteboardMime::MIME_ALL)) {
        if (!converter->canConvert(mimeType, converter->flavorFor(mimeType)))
            continue;

        for (NSUInteger i = 0; i < [pasteboardTypes count]; ++i) {
            NSString *availableUtiNSString = [pasteboardTypes objectAtIndex:i];
            QString availableUti = QString::fromNSString(availableUtiNSString);
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

    foreach (const QString &mimeType, mimeData->formats()) {
        foreach (QMacInternalPasteboardMime *converter,
                 QMacInternalPasteboardMime::all(QMacInternalPasteboardMime::MIME_ALL)) {
            QString uti = converter->flavorFor(mimeType);
            if (uti.isEmpty() || !converter->canConvert(mimeType, uti))
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
