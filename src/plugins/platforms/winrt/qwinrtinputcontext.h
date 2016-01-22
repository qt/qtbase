/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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

    QRectF keyboardRect() const;

    bool isInputPanelVisible() const;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    void showInputPanel();
    void hideInputPanel();
#endif

private:
    HRESULT onShowing(ABI::Windows::UI::ViewManagement::IInputPane *,
                      ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs *);
    HRESULT onHiding(ABI::Windows::UI::ViewManagement::IInputPane *,
                     ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs *);

    HRESULT handleVisibilityChange(ABI::Windows::UI::ViewManagement::IInputPane *);

    QWinRTScreen *m_screen;
    QRectF m_keyboardRect;
    bool m_isInputPanelVisible;
};

QT_END_NAMESPACE

#endif // QWINRTINPUTCONTEXT_H
