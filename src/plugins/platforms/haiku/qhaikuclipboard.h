// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHAIKUCLIPBOARD_H
#define QHAIKUCLIPBOARD_H

#if !defined(QT_NO_CLIPBOARD)

#include <qpa/qplatformclipboard.h>

#include <Handler.h>

QT_BEGIN_NAMESPACE

class QHaikuClipboard : public QPlatformClipboard, public BHandler
{
public:
    QHaikuClipboard();
    ~QHaikuClipboard();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    // override from BHandler to catch change notifications from Haiku clipboard
    void MessageReceived(BMessage* message) override;

private:
    QMimeData *m_systemMimeData;
    QMimeData *m_userMimeData;
};

QT_END_NAMESPACE

#endif

#endif
