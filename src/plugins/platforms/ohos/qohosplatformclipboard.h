
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMCLIPBOARD_H
#define QOHOSPLATFORMCLIPBOARD_H

#include <QtCore/qmimedata.h>
#include <QtCore/private/qohoscommon_p.h>
#include <qohosclipboardobject.h>
#include <qpa/qplatformclipboard.h>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QMimeData;
class QOhosPlatformClipboard : public QPlatformClipboard
{
public:
    static void setInAppOnlyPasteboardShareOption(bool shareInAppOnly);

    QOhosPlatformClipboard();
    virtual ~QOhosPlatformClipboard();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *mimeData, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    std::shared_ptr<QMimeData> getPasteboardDataWithLazyFetchOrLocalIfOwner() const;

private:
    Q_REQUIRED_RESULT static std::optional<bool> &shareInAppOnlyFlagRef();

    std::shared_ptr<QMimeData> m_mimeData;
    bool m_mimeDataIsOurs = false;
    std::shared_ptr<QOhosClipboardObject> m_clipboardObject;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMCLIPBOARD_H
