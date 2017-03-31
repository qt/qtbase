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

#include "qcocoamimetypes.h"
#include <QtClipboardSupport/private/qmacmime_p.h>
#include "qcocoahelpers.h"
#include <QtGui/private/qcoregraphics_p.h>

QT_BEGIN_NAMESPACE

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
    return QLatin1String("PlainText (traditional-mac-plain-text)");
}

QString QMacPasteboardMimeTraditionalMacPlainText::flavorFor(const QString &mime)
{
    if (mime == QLatin1String("text/plain"))
        return QLatin1String("com.apple.traditional-mac-plain-text");
    return QString();
}

QString QMacPasteboardMimeTraditionalMacPlainText::mimeFor(QString flav)
{
    if (flav == QLatin1String("com.apple.traditional-mac-plain-text"))
        return QLatin1String("text/plain");
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
    if (flavor == QLatin1String("com.apple.traditional-mac-plain-text")) {
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
    if (flavor == QLatin1String("com.apple.traditional-mac-plain-text"))
        ret.append(string.toLatin1());
    return ret;
}

void QCocoaMimeTypes::initializeMimeTypes()
{
    new QMacPasteboardMimeTraditionalMacPlainText;
}

QT_END_NAMESPACE
