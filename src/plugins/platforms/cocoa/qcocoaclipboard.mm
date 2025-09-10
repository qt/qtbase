// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcocoaclipboard.h"

#include <QtGui/qutimimeconverter.h>

#ifndef QT_NO_CLIPBOARD

QT_BEGIN_NAMESPACE

QCocoaClipboard::QCocoaClipboard()
    :m_clipboard(new QMacPasteboard(kPasteboardClipboard, QUtiMimeConverter::HandlerScopeFlag::Clipboard))
    ,m_find(new QMacPasteboard(kPasteboardFind, QUtiMimeConverter::HandlerScopeFlag::Clipboard))
{
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &QCocoaClipboard::handleApplicationStateChanged);
}

QMimeData *QCocoaClipboard::mimeData(QClipboard::Mode mode)
{
    if (QMacPasteboard *pasteBoard = pasteboardForMode(mode)) {
        pasteBoard->sync();
        return pasteBoard->mimeData();
    }
    return nullptr;
}

void QCocoaClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (QMacPasteboard *pasteBoard = pasteboardForMode(mode)) {
        if (!data) {
            pasteBoard->clear();
        }

        pasteBoard->sync();
        pasteBoard->setMimeData(data, QMacPasteboard::LazyRequest);
        emitChanged(mode);
    }
}

bool QCocoaClipboard::supportsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard || mode == QClipboard::FindBuffer);
}

bool QCocoaClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

QMacPasteboard *QCocoaClipboard::pasteboardForMode(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Clipboard)
        return m_clipboard.data();
    else if (mode == QClipboard::FindBuffer)
        return m_find.data();
    else
        return nullptr;
}

void QCocoaClipboard::handleApplicationStateChanged(Qt::ApplicationState state)
{
    if (state != Qt::ApplicationActive)
        return;

    if (m_clipboard->sync())
        emitChanged(QClipboard::Clipboard);
    if (m_find->sync())
        emitChanged(QClipboard::FindBuffer);
}

QT_END_NAMESPACE

#include "moc_qcocoaclipboard.cpp"

#endif // QT_NO_CLIPBOARD
