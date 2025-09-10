// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAMESSAGEDIALOG_H
#define QCOCOAMESSAGEDIALOG_H

#include <qpa/qplatformdialoghelper.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSAlert);
typedef long NSInteger;
typedef NSInteger NSModalResponse;

QT_BEGIN_NAMESPACE

class QEventLoop;

class QCocoaMessageDialog : public QPlatformMessageDialogHelper
{
public:
    QCocoaMessageDialog() = default;
    ~QCocoaMessageDialog();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

private:
    Qt::WindowModality modality() const;
    NSAlert *m_alert = nullptr;
    QEventLoop *m_eventLoop = nullptr;
    NSModalResponse runModal() const;
    void processResponse(NSModalResponse response);
};

QT_END_NAMESPACE

#endif // QCOCOAMESSAGEDIALOG_H

