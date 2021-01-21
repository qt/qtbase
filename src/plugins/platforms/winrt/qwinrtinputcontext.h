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

#ifndef QWINRTINPUTCONTEXT_H
#define QWINRTINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <QtCore/QRectF>
#include <QtCore/QLoggingCategory>

#include <wrl.h>

namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Core {
                struct ICoreWindow;
            }
            namespace ViewManagement {
                struct IInputPane;
                struct IInputPaneVisibilityEventArgs;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods)

class QWinRTScreen;
class QWinRTInputContext : public QPlatformInputContext
{
public:
    explicit QWinRTInputContext(QWinRTScreen *);

    QRectF keyboardRect() const override;

    bool isInputPanelVisible() const override;

    void showInputPanel() override;
    void hideInputPanel() override;

private slots:
    void updateScreenCursorRect();

private:
    HRESULT onShowing(ABI::Windows::UI::ViewManagement::IInputPane *,
                      ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs *);
    HRESULT onHiding(ABI::Windows::UI::ViewManagement::IInputPane *,
                     ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs *);

    HRESULT handleVisibilityChange(ABI::Windows::UI::ViewManagement::IInputPane *);

    QWinRTScreen *m_screen;
    QRectF m_keyboardRect;
    QRectF m_cursorRect;
    bool m_isInputPanelVisible;
};

QT_END_NAMESPACE

#endif // QWINRTINPUTCONTEXT_H
