// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQNXCLIPBOARD_H
#define QQNXCLIPBOARD_H

#include <QtCore/qglobal.h>

#if !defined(QT_NO_CLIPBOARD)
#include <qpa/qplatformclipboard.h>

QT_BEGIN_NAMESPACE

class QQnxClipboard : public QPlatformClipboard
{
public:
    QQnxClipboard();
    ~QQnxClipboard();
    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;

private:
    class MimeData;
    MimeData *m_mimeData;
};

QT_END_NAMESPACE

#endif //QT_NO_CLIPBOARD
#endif //QQNXCLIPBOARD_H
