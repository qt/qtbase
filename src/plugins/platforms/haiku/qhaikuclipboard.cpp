/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#if !defined(QT_NO_CLIPBOARD)

#include "qhaikuclipboard.h"

#include <QMimeData>
#include <QThread>

#include <Clipboard.h>

QHaikuClipboard::QHaikuClipboard()
    : m_systemMimeData(Q_NULLPTR)
    , m_userMimeData(Q_NULLPTR)
{
    if (be_clipboard)
        be_clipboard->StartWatching(BMessenger(this));
}

QHaikuClipboard::~QHaikuClipboard()
{
    if (be_clipboard)
        be_clipboard->StopWatching(BMessenger(this));

    delete m_userMimeData;
    delete m_systemMimeData;
}

QMimeData *QHaikuClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return 0;

    if (m_userMimeData)
        return m_userMimeData;

    if (!be_clipboard->Lock())
        return 0;

    if (!m_systemMimeData)
        m_systemMimeData = new QMimeData();
    else
        m_systemMimeData->clear();

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
                const QLatin1String format(name);
                if (format == QLatin1String("text/plain")) {
                    m_systemMimeData->setText(QString::fromLocal8Bit(reinterpret_cast<const char*>(data), dataLen));
                } else if (format == QLatin1String("text/html")) {
                    m_systemMimeData->setHtml(QString::fromLocal8Bit(reinterpret_cast<const char*>(data), dataLen));
                } else {
                    m_systemMimeData->setData(format, QByteArray(reinterpret_cast<const char*>(data), dataLen));
                }
            }
        }
    }

    be_clipboard->Unlock();

    return m_systemMimeData;
}

void QHaikuClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return;

    if (mimeData) {
        if (m_systemMimeData == mimeData)
            return;

        if (m_userMimeData == mimeData)
            return;
    }

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

    m_userMimeData = mimeData;

    emitChanged(QClipboard::Clipboard);
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
    if (message->what == B_CLIPBOARD_CHANGED) {
        delete m_userMimeData;
        m_userMimeData = Q_NULLPTR;

        emitChanged(QClipboard::Clipboard);
    }

    BHandler::MessageReceived(message);
}

#endif
