// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QANDROIDPLATFORMCLIPBOARD_H
#define QANDROIDPLATFORMCLIPBOARD_H

#include <qpa/qplatformclipboard.h>
#include <QMimeData>

#ifndef QT_NO_CLIPBOARD
QT_BEGIN_NAMESPACE

class QAndroidPlatformClipboard : public QPlatformClipboard
{
public:
    QAndroidPlatformClipboard();
    ~QAndroidPlatformClipboard();
    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
private:
    QMimeData *data = nullptr;
};

QT_END_NAMESPACE
#endif // QT_NO_CLIPBOARD

#endif // QANDROIDPLATFORMCLIPBOARD_H
