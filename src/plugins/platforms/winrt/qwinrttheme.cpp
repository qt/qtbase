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

#include "qwinrttheme.h"
#include "qwinrtmessagedialoghelper.h"
#include "qwinrtfiledialoghelper.h"

#include <QtCore/qfunctions_winrt.h>
#include <QtGui/QPalette>

#include <wrl.h>
#include <windows.ui.h>
#include <windows.ui.viewmanagement.h>
#if _MSC_VER >= 1900
#include <windows.foundation.metadata.h>
using namespace ABI::Windows::Foundation::Metadata;
#endif

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::ViewManagement;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaTheme, "qt.qpa.theme")

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

class QWinRTThemePrivate
{
public:
    QPalette palette;
};

static inline QColor fromColor(const Color &color)
{
    return QColor(color.R, color.G, color.B, color.A);
}

#if _MSC_VER >= 1900
static bool uiColorSettings(const wchar_t *value, UIElementType type, Color *color)
{
    static ComPtr<IApiInformationStatics> apiInformationStatics;
    HRESULT hr;
    if (!apiInformationStatics) {
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
                                    IID_PPV_ARGS(&apiInformationStatics));
        RETURN_FALSE_IF_FAILED("Could not get ApiInformationStatics");
    }

    static const HStringReference enumRef(L"Windows.UI.ViewManagement.UIElementType");
    HStringReference valueRef(value);

    boolean exists;
    hr = apiInformationStatics->IsEnumNamedValuePresent(enumRef.Get(), valueRef.Get(), &exists);

    if (hr != S_OK || !exists)
        return false;

    return SUCCEEDED(uiSettings()->UIElementColor(type, color));
}

static void nativeColorSettings(QPalette &p)
{
    Color color;

    if (uiColorSettings(L"ActiveCaption", UIElementType_ActiveCaption, &color))
        p.setColor(QPalette::ToolTipBase, fromColor(color));

    if (uiColorSettings(L"Background", UIElementType_Background, &color))
        p.setColor(QPalette::AlternateBase, fromColor(color));

    if (uiColorSettings(L"ButtonFace", UIElementType_ButtonFace, &color)) {
        p.setColor(QPalette::Button, fromColor(color));
        p.setColor(QPalette::Midlight, fromColor(color).lighter(110));
        p.setColor(QPalette::Light, fromColor(color).lighter(150));
        p.setColor(QPalette::Mid, fromColor(color).dark(130));
        p.setColor(QPalette::Dark, fromColor(color).dark(150));
    }

    if (uiColorSettings(L"ButtonText", UIElementType_ButtonText, &color)) {
        p.setColor(QPalette::ButtonText, fromColor(color));
        p.setColor(QPalette::Text, fromColor(color));
    }

    if (uiColorSettings(L"CaptionText", UIElementType_CaptionText, &color))
        p.setColor(QPalette::ToolTipText, fromColor(color));

    if (uiColorSettings(L"Highlight", UIElementType_Highlight, &color))
        p.setColor(QPalette::Highlight, fromColor(color));

    if (uiColorSettings(L"HighlightText", UIElementType_HighlightText, &color))
        p.setColor(QPalette::HighlightedText, fromColor(color));

    if (uiColorSettings(L"Window", UIElementType_Window, &color)) {
        p.setColor(QPalette::Window, fromColor(color));
        p.setColor(QPalette::Base, fromColor(color));
    }

    if (uiColorSettings(L"Hotlight", UIElementType_Hotlight, &color))
        p.setColor(QPalette::BrightText, fromColor(color));

    //Phone related
    if (uiColorSettings(L"PopupBackground", UIElementType_PopupBackground, &color)) {
        p.setColor(QPalette::ToolTipBase, fromColor(color));
        p.setColor(QPalette::AlternateBase, fromColor(color));
    }

    if (uiColorSettings(L"NonTextMedium", UIElementType_NonTextMedium, &color))
        p.setColor(QPalette::Button, fromColor(color));

    if (uiColorSettings(L"NonTextMediumHigh", UIElementType_NonTextMediumHigh, &color))
        p.setColor(QPalette::Midlight, fromColor(color));

    if (uiColorSettings(L"NonTextHigh", UIElementType_NonTextHigh, &color))
        p.setColor(QPalette::Light, fromColor(color));

    if (uiColorSettings(L"NonTextMediumLow", UIElementType_NonTextMediumLow, &color))
        p.setColor(QPalette::Mid, fromColor(color));

    if (uiColorSettings(L"NonTextLow", UIElementType_NonTextLow, &color))
        p.setColor(QPalette::Dark, fromColor(color));

    if (uiColorSettings(L"TextHigh", UIElementType_TextHigh, &color)) {
        p.setColor(QPalette::ButtonText, fromColor(color));
        p.setColor(QPalette::Text, fromColor(color));
        p.setColor(QPalette::WindowText, fromColor(color));
    }

    if (uiColorSettings(L"TextMedium", UIElementType_TextMedium, &color))
        p.setColor(QPalette::ToolTipText, fromColor(color));

    if (uiColorSettings(L"AccentColor", UIElementType_AccentColor, &color))
        p.setColor(QPalette::Highlight, fromColor(color));

    if (uiColorSettings(L"PageBackground", UIElementType_PageBackground, &color)) {
        p.setColor(QPalette::Window, fromColor(color));
        p.setColor(QPalette::Base, fromColor(color));
    }

    if (uiColorSettings(L"TextContrastWithHigh", UIElementType_TextContrastWithHigh, &color))
        p.setColor(QPalette::BrightText, fromColor(color));
}

#else // _MSC_VER >= 1900

static void nativeColorSettings(QPalette &p)
{
    HRESULT hr;
    Color color;

#ifdef Q_OS_WINPHONE
    hr = uiSettings()->UIElementColor(UIElementType_PopupBackground, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::ToolTipBase, fromColor(color));
    p.setColor(QPalette::AlternateBase, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_NonTextMedium, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Button, fromColor(color));
    hr = uiSettings()->UIElementColor(UIElementType_NonTextMediumHigh, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Midlight, fromColor(color));
    hr = uiSettings()->UIElementColor(UIElementType_NonTextHigh, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Light, fromColor(color));
    hr = uiSettings()->UIElementColor(UIElementType_NonTextMediumLow, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Mid, fromColor(color));
    hr = uiSettings()->UIElementColor(UIElementType_NonTextLow, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Dark, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_TextHigh, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::ButtonText, fromColor(color));
    p.setColor(QPalette::Text, fromColor(color));
    p.setColor(QPalette::WindowText, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_TextMedium, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::ToolTipText, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_AccentColor, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Highlight, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_PageBackground, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Window, fromColor(color));
    p.setColor(QPalette::Base, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_TextContrastWithHigh, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::BrightText, fromColor(color));
#else
    hr = uiSettings()->UIElementColor(UIElementType_ActiveCaption, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::ToolTipBase, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_Background, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::AlternateBase, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_ButtonFace, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Button, fromColor(color));
    p.setColor(QPalette::Midlight, fromColor(color).lighter(110));
    p.setColor(QPalette::Light, fromColor(color).lighter(150));
    p.setColor(QPalette::Mid, fromColor(color).dark(130));
    p.setColor(QPalette::Dark, fromColor(color).dark(150));

    hr = uiSettings()->UIElementColor(UIElementType_ButtonText, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::ButtonText, fromColor(color));
    p.setColor(QPalette::Text, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_CaptionText, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::ToolTipText, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_Highlight, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Highlight, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_HighlightText, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::HighlightedText, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_Window, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::Window, fromColor(color));
    p.setColor(QPalette::Base, fromColor(color));

    hr = uiSettings()->UIElementColor(UIElementType_Hotlight, &color);
    Q_ASSERT_SUCCEEDED(hr);
    p.setColor(QPalette::BrightText, fromColor(color));
#endif
}
#endif // _MSC_VER < 1900

QWinRTTheme::QWinRTTheme()
    : d_ptr(new QWinRTThemePrivate)
{
    Q_D(QWinRTTheme);
    qCDebug(lcQpaTheme) << __FUNCTION__;

    nativeColorSettings(d->palette);
}

bool QWinRTTheme::usePlatformNativeDialog(DialogType type) const
{
    qCDebug(lcQpaTheme) << __FUNCTION__ << type;
    static bool useNativeDialogs = qEnvironmentVariableIsSet("QT_USE_WINRT_NATIVE_DIALOGS")
            ? qEnvironmentVariableIntValue("QT_USE_WINRT_NATIVE_DIALOGS") : true;

    if (type == FileDialog || type == MessageDialog)
        return useNativeDialogs;
    return false;
}

QPlatformDialogHelper *QWinRTTheme::createPlatformDialogHelper(DialogType type) const
{
    qCDebug(lcQpaTheme) << __FUNCTION__ << type;
    switch (type) {
    case FileDialog:
        return new QWinRTFileDialogHelper;
    case MessageDialog:
        return new QWinRTMessageDialogHelper(this);
    default:
        break;
    }
    return QPlatformTheme::createPlatformDialogHelper(type);
}

QVariant QWinRTTheme::styleHint(QPlatformIntegration::StyleHint hint)
{
    qCDebug(lcQpaTheme) << __FUNCTION__ << hint;
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
        return false;
    case QPlatformIntegration::PasswordMaskDelay:
        return defaultThemeHint(PasswordMaskDelay);
    case QPlatformIntegration::FontSmoothingGamma:
        return qreal(1.7);
    case QPlatformIntegration::StartDragVelocity:
        return defaultThemeHint(StartDragVelocity);
    case QPlatformIntegration::UseRtlExtensions:
        return false;
    case QPlatformIntegration::PasswordMaskCharacter:
        return defaultThemeHint(PasswordMaskCharacter);
    case QPlatformIntegration::SetFocusOnTouchRelease:
        return true;
    case QPlatformIntegration::ShowIsMaximized:
        return true;
    case QPlatformIntegration::MousePressAndHoldInterval:
        return defaultThemeHint(MousePressAndHoldInterval);
    default:
        break;
    }
    return QVariant();
}

QVariant QWinRTTheme::themeHint(ThemeHint hint) const
{
    qCDebug(lcQpaTheme) << __FUNCTION__ << hint;
    switch (hint) {
    case StyleNames:
        return QStringList() << QStringLiteral("fusion") << QStringLiteral("windows");
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

const QPalette *QWinRTTheme::palette(Palette type) const
{
    Q_D(const QWinRTTheme);
    qCDebug(lcQpaTheme) << __FUNCTION__ << type;
    if (type == SystemPalette)
        return &d->palette;
    return QPlatformTheme::palette(type);
}

QT_END_NAMESPACE
