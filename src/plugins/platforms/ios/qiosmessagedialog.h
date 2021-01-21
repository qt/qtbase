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

