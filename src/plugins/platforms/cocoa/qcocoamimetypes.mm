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
    QString convertorName();

    QString flavorFor(const QString &mime);
    QString mimeFor(QString flav);
    bool canConvert(const QString &mime, QString flav);
    QVariant convertToMime(const QString &mime, QList<QByteArray> data, QString flav);
    QList<QByteArray> convertFromMime(const QString &mime, QVariant data, QString flav);
};

QString QMacPasteboardMimeTraditionalMacPlainText::convertorName()
{
    return "PlainText (traditional-mac-plain-text)"_L1;
}

QString QMacPasteboardMimeTraditionalMacPlainText::flavorFor(const QString &mime)
{
    if (mime == "text/plain"_L1)
        return "com.apple.traditional-mac-plain-text"_L1;
    return QString();
}

QString QMacPasteboardMimeTraditionalMacPlainText::mimeFor(QString flav)
{
    if (flav == "com.apple.traditional-mac-plain-text"_L1)
        return "text/plain"_L1;
    return QString();
}

bool QMacPasteboardMimeTraditionalMacPlainText::canConvert(const QString &mime, QString flav)
{
    return flavorFor(mime) == flav;
}

QVariant QMacPasteboardMimeTraditionalMacPlainText::convertToMime(const QString &mimetype, QList<QByteArray> data, QString flavor)
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

QList<QByteArray> QMacPasteboardMimeTraditionalMacPlainText::convertFromMime(const QString &, QVariant data, QString flavor)
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
