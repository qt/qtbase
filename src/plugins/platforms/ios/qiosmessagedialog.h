// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSMESSAGEDIALOG_H
#define QIOSMESSAGEDIALOG_H

#include <QtCore/qeventloop.h>
#include <qpa/qplatformdialoghelper.h>

Q_FORWARD_DECLARE_OBJC_CLASS(UIAlertController);
Q_FORWARD_DECLARE_OBJC_CLASS(UIAlertAction);

QT_BEGIN_NAMESPACE

class QIOSMessageDialog : public QPlatformMessageDialogHelper
{
public:
    QIOSMessageDialog();
    ~QIOSMessageDialog();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

private:
    QEventLoop m_eventLoop;
    UIAlertController *m_alertController;
    QString messageTextPlain();
    UIAlertAction *createAction(StandardButton button);
    UIAlertAction *createAction(const QMessageDialogOptions::CustomButton &customButton);
};

QT_END_NAMESPACE

#endif // QIOSMESSAGEDIALOG_H

