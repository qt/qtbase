/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
