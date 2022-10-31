// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoamimetypes.h"
#include <QtGui/private/qmacmime_p.h>
#include "qcocoahelpers.h"
#include <QtGui/private/qcoregraphics_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QMacPasteboardMimeTraditionalMacPlainText : public QMacInternalPasteboardMime {
public:
    QMacPasteboardMimeTraditionalMacPlainText() : QMacInternalPasteboardMime(MIME_ALL) { }

    QString flavorFor(const QString &mime) const override;
    QString mimeFor(const QString &flav) const override;
    bool canConvert(const QString &mime, const QString &flav) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &flav) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &flav) const override;
};

QString QMacPasteboardMimeTraditionalMacPlainText::flavorFor(const QString &mime) const
{
    if (mime == "text/plain"_L1)
        return "com.apple.traditional-mac-plain-text"_L1;
    return QString();
}

QString QMacPasteboardMimeTraditionalMacPlainText::mimeFor(const QString &flav) const
{
    if (flav == "com.apple.traditional-mac-plain-text"_L1)
        return "text/plain"_L1;
    return QString();
}

bool QMacPasteboardMimeTraditionalMacPlainText::canConvert(const QString &mime,
                                                           const QString &flav) const
{
    return flavorFor(mime) == flav;
}

QVariant
QMacPasteboardMimeTraditionalMacPlainText::convertToMime(const QString &mimetype,
                                                         const QList<QByteArray> &data,
                                                         const QString &flavor) const
{
    if (data.count() > 1)
        qWarning("QMacPasteboardMimeTraditionalMacPlainText: Cannot handle multiple member data");
    const QByteArray &firstData = data.first();
    QVariant ret;
    if (flavor == "com.apple.traditional-mac-plain-text"_L1) {
        return QString(QCFString(CFStringCreateWithBytes(kCFAllocatorDefault,
                                             reinterpret_cast<const UInt8 *>(firstData.constData()),
                                             firstData.size(), CFStringGetSystemEncoding(), false)));
    } else {
        qWarning("QMime::convertToMime: unhandled mimetype: %s", qPrintable(mimetype));
    }
    return ret;
}

QList<QByteArray>
QMacPasteboardMimeTraditionalMacPlainText::convertFromMime(const QString &,
                                                           const QVariant &data,
                                                           const QString &flavor) const
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (flavor == "com.apple.traditional-mac-plain-text"_L1)
        ret.append(string.toLatin1());
    return ret;
}

void QCocoaMimeTypes::initializeMimeTypes()
{
    new QMacPasteboardMimeTraditionalMacPlainText;
}

QT_END_NAMESPACE
