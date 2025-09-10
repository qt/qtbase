// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOACLIPBOARD_H
#define QCOCOACLIPBOARD_H

#include <qpa/qplatformclipboard.h>

#ifndef QT_NO_CLIPBOARD

#include "qmacclipboard.h"
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE

class QCocoaClipboard : public QObject, public QPlatformClipboard
{
    Q_OBJECT

public:
    QCocoaClipboard();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

private Q_SLOTS:
    void handleApplicationStateChanged(Qt::ApplicationState state);

protected:
    QMacPasteboard *pasteboardForMode(QClipboard::Mode mode) const;

private:
    QScopedPointer<QMacPasteboard> m_clipboard;
    QScopedPointer<QMacPasteboard> m_find;
};

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD

#endif // QCOCOACLIPBOARD_H
