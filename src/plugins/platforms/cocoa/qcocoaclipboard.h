/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
