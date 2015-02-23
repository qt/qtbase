/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#if !defined(QT_NO_CLIPBOARD)

#include "qhaikuclipboard.h"

#include <QMimeData>
#include <QThread>

#include <Clipboard.h>

QHaikuClipboard::QHaikuClipboard()
{
    if (be_clipboard)
        be_clipboard->StartWatching(BMessenger(this));
}

QHaikuClipboard::~QHaikuClipboard()
{
    if (be_clipboard)
        be_clipboard->StopWatching(BMessenger(this));
}

QMimeData *QHaikuClipboard::mimeData(QClipboard::Mode mode)
{
    QMimeData *mimeData = new QMimeData();

    if (mode != QClipboard::Clipboard)
        return mimeData;

    if (!be_clipboard->Lock())
        return mimeData;

    const BMessage *clipboard = be_clipboard->Data();
    if (clipboard) {
        char *name = Q_NULLPTR;
        uint32 type = 0;
        int32 count = 0;

        for (int i = 0; clipboard->GetInfo(B_MIME_TYPE, i, &name, &type, &count) == B_OK; i++) {
            const void *data = Q_NULLPTR;
            int32 dataLen = 0;

            const status_t status = clipboard->FindData(name, B_MIME_TYPE, &data, &dataLen);
            if (dataLen && (status == B_OK)) {
                const QString format = QString::fromLatin1(name);
                if (format == QStringLiteral("text/plain")) {
                    mimeData->setText(QString::fromLocal8Bit(reinterpret_cast<const char*>(data), dataLen));
                } else if (format == QStringLiteral("text/html")) {
                    mimeData->setHtml(QString::fromLocal8Bit(reinterpret_cast<const char*>(data), dataLen));
                } else {
                    mimeData->setData(format, QByteArray(reinterpret_cast<const char*>(data), dataLen));
                }
            }
        }
    }

    be_clipboard->Unlock();

    return mimeData;
}

void QHaikuClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return;

    if (!be_clipboard->Lock())
        return;

    be_clipboard->Clear();
    if (mimeData) {
        BMessage *clipboard = be_clipboard->Data();
        if (clipboard) {
            const QStringList formats = mimeData->formats();
            Q_FOREACH (const QString &format, formats) {
                const QByteArray data = mimeData->data(format).data();
                clipboard->AddData(format.toUtf8(), B_MIME_TYPE, data, data.count());
            }
        }
    }

    if (be_clipboard->Commit() != B_OK)
        qWarning("Unable to store mime data on clipboard");

    be_clipboard->Unlock();
}

bool QHaikuClipboard::supportsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard);
}

bool QHaikuClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);

    return false;
}

void QHaikuClipboard::MessageReceived(BMessage* message)
{
    if (message->what == B_CLIPBOARD_CHANGED)
        emitChanged(QClipboard::Clipboard);

    BHandler::MessageReceived(message);
}

#endif
