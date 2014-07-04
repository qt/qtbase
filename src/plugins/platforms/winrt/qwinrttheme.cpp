/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrttheme.h"
#include "qwinrtplatformmessagedialoghelper.h"

#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.ui.viewmanagement.h>
using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::ViewManagement;

QT_BEGIN_NAMESPACE

static IUISettings *uiSettings()
{
    static ComPtr<IUISettings> settings;
    if (!settings) {
        HRESULT hr;
        hr = RoActivateInstance(Wrappers::HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(),
                                &settings);
        Q_ASSERT_SUCCEEDED(hr);
    }
    return settings.Get();
}

QWinRTTheme::QWinRTTheme()
{
}

bool QWinRTTheme::usePlatformNativeDialog(DialogType type) const
{
    if (type == MessageDialog)
        return true;
    return false;
}

QPlatformDialogHelper *QWinRTTheme::createPlatformDialogHelper(DialogType type) const
{
    switch (type) {
    case MessageDialog:
        return new QWinRTPlatformMessageDialogHelper;
    default:
        break;
    }
    return QPlatformTheme::createPlatformDialogHelper(type);
}

QVariant QWinRTTheme::styleHint(QPlatformIntegration::StyleHint hint)
{
    HRESULT hr;
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime: {
        quint32 blinkRate;
        hr = uiSettings()->get_CaretBlinkRate(&blinkRate);
        RETURN_IF_FAILED("Failed to get caret blink rate", return defaultThemeHint(CursorFlashTime));
        return blinkRate;
    }
    case QPlatformIntegration::KeyboardInputInterval:
        return defaultThemeHint(KeyboardInputInterval);
    case QPlatformIntegration::MouseDoubleClickInterval: {
        quint32 doubleClickTime;
        hr = uiSettings()->get_DoubleClickTime(&doubleClickTime);
        RETURN_IF_FAILED("Failed to get double click time", return defaultThemeHint(MouseDoubleClickInterval));
        return doubleClickTime;
    }
    case QPlatformIntegration::StartDragDistance:
        return defaultThemeHint(StartDragDistance);
    case QPlatformIntegration::StartDragTime:
        return defaultThemeHint(StartDragTime);
    case QPlatformIntegration::KeyboardAutoRepeatRate:
        return defaultThemeHint(KeyboardAutoRepeatRate);
    case QPlatformIntegration::ShowIsFullScreen:
        return true;
    case QPlatformIntegration::PasswordMaskDelay:
        return defaultThemeHint(PasswordMaskDelay);
    case QPlatformIntegration::FontSmoothingGamma:
        return qreal(1.7);
    case QPlatformIntegration::StartDragVelocity:
        return defaultThemeHint(StartDragVelocity);
    case QPlatformIntegration::UseRtlExtensions:
        return false;
    case QPlatformIntegration::SynthesizeMouseFromTouchEvents:
        return true;
    case QPlatformIntegration::PasswordMaskCharacter:
        return defaultThemeHint(PasswordMaskCharacter);
    case QPlatformIntegration::SetFocusOnTouchRelease:
        return false;
    case QPlatformIntegration::ShowIsMaximized:
        return false;
    case MousePressAndHoldInterval:
        return defaultThemeHint(MousePressAndHoldInterval);
    default:
        break;
    }
    return QVariant();
}

QT_END_NAMESPACE
