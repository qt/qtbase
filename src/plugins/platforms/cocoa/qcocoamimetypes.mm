// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoamimetypes.h"
#include <QtGui/qutimimeconverter.h>
#include "qcocoahelpers.h"
#include <QtGui/private/qcoregraphics_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QMacMimeTraditionalMacPlainText : public QUtiMimeConverter {
public:
    QString utiForMime(const QString &mime) const override;
    QString mimeForUti(const QString &uti) const override;
    QVariant convertToMime(const QString &mime, const QList<QByteArray> &data,
                           const QString &uti) const override;
    QList<QByteArray> convertFromMime(const QString &mime, const QVariant &data,
                                      const QString &uti) const override;
};

QString QMacMimeTraditionalMacPlainText::utiForMime(const QString &mime) const
{
    if (mime == "text/plain"_L1)
        return "com.apple.traditional-mac-plain-text"_L1;
    return QString();
}

QString QMacMimeTraditionalMacPlainText::mimeForUti(const QString &uti) const
{
    if (uti == "com.apple.traditional-mac-plain-text"_L1)
        return "text/plain"_L1;
    return QString();
}

QVariant
QMacMimeTraditionalMacPlainText::convertToMime(const QString &mimetype,
                                                         const QList<QByteArray> &data,
                                                         const QString &uti) const
{
    if (data.count() > 1)
        qWarning("QMacMimeTraditionalMacPlainText: Cannot handle multiple member data");
    const QByteArray &firstData = data.first();
    QVariant ret;
    if (uti == "com.apple.traditional-mac-plain-text"_L1) {
        return QString(QCFString(CFStringCreateWithBytes(kCFAllocatorDefault,
                                             reinterpret_cast<const UInt8 *>(firstData.constData()),
                                             firstData.size(), CFStringGetSystemEncoding(), false)));
    } else {
        qWarning("QMime::convertToMime: unhandled mimetype: %s", qPrintable(mimetype));
    }
    return ret;
}

QList<QByteArray>
QMacMimeTraditionalMacPlainText::convertFromMime(const QString &,
                                                           const QVariant &data,
                                                           const QString &uti) const
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (uti == "com.apple.traditional-mac-plain-text"_L1)
        ret.append(string.toLatin1());
    return ret;
}

void QCocoaMimeTypes::initializeMimeTypes()
{
    new QMacMimeTraditionalMacPlainText;
}

QT_END_NAMESPACE
