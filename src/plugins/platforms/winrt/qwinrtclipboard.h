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

#ifndef QWINRTPLATFORMCLIPBOARD_H
#define QWINRTPLATFORMCLIPBOARD_H

#include <qpa/qplatformclipboard.h>
#include <QMimeData>

#include <wrl.h>

namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            namespace DataTransfer {
                struct IClipboardStatics;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTClipboard: public QPlatformClipboard
{
public:
    QWinRTClipboard();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;

    HRESULT onContentChanged(IInspectable *, IInspectable *);
private:
    Microsoft::WRL::ComPtr<ABI::Windows::ApplicationModel::DataTransfer::IClipboardStatics> m_nativeClipBoard;
    QMimeData *m_mimeData;
};

QT_END_NAMESPACE

#endif // QWINRTPLATFORMCLIPBOARD_H
