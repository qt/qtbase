// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#if !defined(QT_NO_CLIPBOARD)

#include "qhaikuclipboard.h"

#include <QMimeData>
#include <QThread>

#include <Clipboard.h>

using namespace Qt::StringLiterals;

QHaikuClipboard::QHaikuClipboard()
    : m_systemMimeData(nullptr)
    , m_userMimeData(nullptr)
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
        char *name = nullptr;
        uint32 type = 0;
        int32 count = 0;

        for (int i = 0; clipboard->GetInfo(B_MIME_TYPE, i, &name, &type, &count) == B_OK; i++) {
            const void *data = nullptr;
            int32 dataLen = 0;

            const status_t status = clipboard->FindData(name, B_MIME_TYPE, &data, &dataLen);
            if (dataLen && (status == B_OK)) {
                const QLatin1StringView format(name);
                if (format == "text/plain"_L1) {
                    m_systemMimeData->setText(QString::fromLocal8Bit(reinterpret_cast<const char*>(data), dataLen));
                } else if (format == "text/html"_L1) {
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
        m_userMimeData = nullptr;

        emitChanged(QClipboard::Clipboard);
    }

    BHandler::MessageReceived(message);
}

#endif
