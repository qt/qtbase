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

#ifndef QWINRTMESSAGEDIALOGHELPER_H
#define QWINRTMESSAGEDIALOGHELPER_H

#include <qpa/qplatformdialoghelper.h>
#include <QtCore/qt_windows.h>

namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Popups {
                struct IUICommand;
            }
        }
        namespace Foundation {
            enum class AsyncStatus;
            template <typename T> struct IAsyncOperation;
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTTheme;

class QWinRTMessageDialogHelperPrivate;
class QWinRTMessageDialogHelper : public QPlatformMessageDialogHelper
{
    Q_OBJECT
public:
    explicit QWinRTMessageDialogHelper(const QWinRTTheme *theme);
    ~QWinRTMessageDialogHelper() override;

    void exec() override;
    bool show(Qt::WindowFlags windowFlags,
              Qt::WindowModality windowModality,
              QWindow *parent) override;
    void hide() override;

private:
    HRESULT onCompleted(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::UI::Popups::IUICommand *> *asyncInfo,
                        ABI::Windows::Foundation::AsyncStatus status);

    QScopedPointer<QWinRTMessageDialogHelperPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTMessageDialogHelper)
};

QT_END_NAMESPACE

#endif // QWINRTMESSAGEDIALOGHELPER_H
