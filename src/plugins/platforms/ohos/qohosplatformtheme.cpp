// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qohosplatformtheme_p.h"
#include "qohosplatformtheme.h"
#include "qohosplatformscreen.h"
#include"qohosplatformdialoghelper.h"
#include "qohosplatformintegration.h"
#include "qohossystemtrayicon.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/private/qabstractfileiconengine_p.h>
#include <initializer_list>
#include <qohosenums.h>
#include <qohosjsmain.h>
#include <qohosjsutils.h>
#include <qohosutils.h>
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformintegration.h>
#include <string>
#include <tuple>

QT_BEGIN_NAMESPACE

namespace {

QColor makeInactiveOrDisabledFromColor(const QColor &color)
{
    auto disabledColor = color;
    disabledColor.setAlphaF(qBound(0.0, color.alphaF() * 0.4, 1.0));

    return disabledColor;
}

struct PalettesColors
{
    QColor activeWindow;
    QColor inactiveWindow;
    QColor activeButtonText;
    QColor inactiveButtonText;
    QColor activeHighlight;
    QColor inactiveHighlight;
    QColor clickEffect;
    QColor hover;
    QColor foregroundContrary;
    QColor highlighted;
    QColor inactiveHighlighted;
    QColor textPrimary;
    QColor inactiveTextPrimary;
    QColor disabledTextPrimary;
    QColor inactiveText;
    QColor componentNormal;
    QColor componentActivated;
    QColor indicatorArrow;
    QColor scrollBarSlider;
    QColor switchBgOff;
    QColor switchOutlineOff;
    QColor window;
};

PalettesColors makePalettesColorsLight()
{
    const auto activeWindow = QColor("#FFFFFFFF");
    const auto activeButtonText = QColor("#0A59F7");
    const auto activeHighlight = QColor("#FF007DFF");
    const auto highlighted = QColor("#FF0A59F7");
    const auto textPrimary = QColor("#FF182431");

    return {
        .activeWindow = activeWindow,
        .inactiveWindow = makeInactiveOrDisabledFromColor(activeWindow),
        .activeButtonText = activeButtonText,
        .inactiveButtonText = makeInactiveOrDisabledFromColor(activeButtonText),
        .activeHighlight = activeHighlight,
        .inactiveHighlight = makeInactiveOrDisabledFromColor(activeHighlight),
        .clickEffect = QColor("#1A000000"),
        .hover = QColor("#0D000000"),
        .foregroundContrary = QColor("#FFFFFF"),
        .highlighted = highlighted,
        .inactiveHighlighted = makeInactiveOrDisabledFromColor(highlighted),
        .textPrimary = textPrimary,
        .inactiveTextPrimary = textPrimary,
        .disabledTextPrimary = makeInactiveOrDisabledFromColor(textPrimary),
        .inactiveText = makeInactiveOrDisabledFromColor(textPrimary),
        .componentNormal = QColor("#19182431"),
        .componentActivated = QColor("#007DFF"),
        .indicatorArrow = QColor("#E5182431"),
        .scrollBarSlider = QColor("#182431"),
        .switchBgOff = QColor("#33FFFFFF"),
        .switchOutlineOff = QColor("#66182431"),
        .window = QColor("#FFF1F3F5"),
    };
};

PalettesColors makePalettesColorsDark()
{
    const auto activeWindow = QColor("#18181A");
    const auto activeButtonText = QColor("#3F97E9");
    const auto activeHighlight = QColor("#006CDE");
    const auto highlighted = QColor("#3F97E9");
    const auto textPrimary = QColor("#DBFFFFFF");

    return {
        .activeWindow = activeWindow,
        .inactiveWindow = makeInactiveOrDisabledFromColor(activeWindow),
        .activeButtonText = activeButtonText,
        .inactiveButtonText = makeInactiveOrDisabledFromColor(activeButtonText),
        .activeHighlight = activeHighlight,
        .inactiveHighlight = makeInactiveOrDisabledFromColor(activeHighlight),
        .clickEffect = QColor("#26FFFFFF"),
        .hover = QColor("#19FFFFFF"),
        .foregroundContrary = QColor("#E5E5E5"),
        .highlighted = highlighted,
        .inactiveHighlighted = makeInactiveOrDisabledFromColor(highlighted),
        .textPrimary = textPrimary,
        .inactiveTextPrimary = textPrimary,
        .disabledTextPrimary = makeInactiveOrDisabledFromColor(textPrimary),
        .inactiveText = makeInactiveOrDisabledFromColor(textPrimary),
        .componentNormal = QColor("#19FFFFFF"),
        .componentActivated = QColor("#3F97E9"),
        .indicatorArrow = QColor("#DBFFFFFF"),
        .scrollBarSlider = QColor("#FFFFFF"),
        .switchBgOff = QColor("#33000000"),
        .switchOutlineOff = QColor("#66FFFFFF"),
        .window = QColor("#FF000000"),
    };
}

struct ButtonPaletteColors
{
    QColor activeButton;
    QColor inactiveButton;
    QColor focusedOutline;
    QColor enabledDefaultButtonText;
};

ButtonPaletteColors makeButtonPaletteColorsLight()
{
    const auto activeButton = QColor("#0C182431");

    return {
        .activeButton = activeButton,
        .inactiveButton = makeInactiveOrDisabledFromColor(activeButton),
        .focusedOutline = QColor("#007DFF"),
        .enabledDefaultButtonText = QColor("#FFFFFFFF"),
    };
};

ButtonPaletteColors makeButtonPaletteColorsDark()
{
    const auto activeButton = QColor("#19FFFFFF");

    return {
        .activeButton = activeButton,
        .inactiveButton = makeInactiveOrDisabledFromColor(activeButton),
        .focusedOutline = QColor("#3F97E9"),
        .enabledDefaultButtonText = QColor("#E5E5E5"),
    };
}

struct ToolButtonPaletteColors
{
    QColor windowButtonBackground;
};

ToolButtonPaletteColors makeToolButtonPaletteColorsLight()
{
    return {
        .windowButtonBackground = QColor("#FFD0CFD5"),
    };
};

ToolButtonPaletteColors makeToolButtonPaletteColorsDark()
{
    return {
        .windowButtonBackground = QColor("#FFD0CFD5"),
    };
};

struct SystemPaletteColors
{
    QColor activeWindowFrame;
    QColor inactiveWindowFrame;
    QColor textHint;
    QColor textTertiary;
};

SystemPaletteColors makeSystemButtonPaletteColorsLight()
{
    return {
        .activeWindowFrame = QColor("#FFE4E4E4"),
        .inactiveWindowFrame = QColor("#FFF2F3F5"),
        .textHint = QColor("#99182431"),
        .textTertiary = QColor("#66182431"),
    };
};

SystemPaletteColors makeSystemButtonPaletteColorsDark()
{
    return {
        .activeWindowFrame = QColor("#FF000000"),
        .inactiveWindowFrame = QColor("#FF18181A"),
        .textHint = QColor("#99FFFFFF"),
        .textTertiary = QColor("#66FFFFFF"),
    };
}

struct MenuPaletteColors
{
    QColor listSeparator;
    QColor inactiveText;
};

MenuPaletteColors makeMenuPaletteColorsLight(const PalettesColors &palettes)
{
    return {
        .listSeparator = QColor("#1A000000"),
        .inactiveText = makeInactiveOrDisabledFromColor(palettes.textPrimary),
    };
};

MenuPaletteColors makeMenuPaletteColorsDark(const PalettesColors &palettes)
{
    return {
        .listSeparator = QColor("#33FFFFFF"),
        .inactiveText = makeInactiveOrDisabledFromColor(palettes.textPrimary),
    };
}

struct TabBarPaletteColors
{
    QColor activeWindowText;
    QColor inactiveWindowText;
    QColor disabledWindowText;
};

TabBarPaletteColors makeTabBarPaletteColorsLight()
{
    const auto inactiveWindowText = QColor("#99182431");

    return {
        .activeWindowText = QColor("#FF007DFF"),
        .inactiveWindowText = inactiveWindowText,
        .disabledWindowText = makeInactiveOrDisabledFromColor(inactiveWindowText),
    };
};

TabBarPaletteColors makeTabBarPaletteColorsDark()
{
    const auto inactiveWindowText = QColor("#99FFFFFF");

    return {
        .activeWindowText = QColor("#FF3F97E9"),
        .inactiveWindowText = inactiveWindowText,
        .disabledWindowText = makeInactiveOrDisabledFromColor(inactiveWindowText),
    };
}

struct TextLineEditPaletteColors
{
    QColor textEditBackground;
    QColor inactiveTextEditBackground;
};

TextLineEditPaletteColors makeTextLineEditPaletteColorsLight()
{
    const auto textEditBackground = QColor("#0C182431");
    return {
        .textEditBackground = textEditBackground,
        .inactiveTextEditBackground = makeInactiveOrDisabledFromColor(textEditBackground),
    };
};

TextLineEditPaletteColors makeTextLineEditPaletteColorsDark()
{
    const auto textEditBackground = QColor("#19FFFFFF");
    return {
        .textEditBackground = textEditBackground,
        .inactiveTextEditBackground = makeInactiveOrDisabledFromColor(textEditBackground),
    };
}

struct GroupboxPaletteColors
{
    QColor background;
    QColor groupBoxFrame;
    QColor shadowXS;
};

GroupboxPaletteColors makeGroupboxPaletteColorsLight()
{
    return {
        .background = QColor("#00000000"),
        .groupBoxFrame = QColor("#66000000"),
        .shadowXS = QColor("#19000000"),
    };
};

GroupboxPaletteColors makeGroupboxPaletteColorsDark()
{
    return {
        .background = QColor("#00000000"),
        .groupBoxFrame = QColor("#66FFFFFF"),
        .shadowXS = QColor("#19FFFFFF"),
    };
}

struct HeaderPaletteColors
{
    QColor activeButton;
};

HeaderPaletteColors makeHeaderPaletteColorsLight()
{
    return {
        .activeButton = QColor("#FFFFFFFF"),
    };
};

HeaderPaletteColors makeHeaderPaletteColorsDark()
{
    return {
        .activeButton = QColor("#FF18181A"),
    };
}

struct TooltipPaletteColors
{
    QColor toolTipBorder;
    QColor toolTipBase;
};

TooltipPaletteColors makeTooltipPaletteColorsLight()
{
    return {
        .toolTipBorder = QColor("#1A000000"),
        .toolTipBase = QColor("#FFFFFFFF"),
    };
};

TooltipPaletteColors makeTooltipPaletteColorsDark()
{
    return {
        .toolTipBorder = QColor("#1AFFFFFF"),
        .toolTipBase = QColor("#404040"),
    };
}

struct AllPalletesColors
{
    PalettesColors palettes;
    ButtonPaletteColors button;
    ToolButtonPaletteColors toolButton;
    SystemPaletteColors system;
    MenuPaletteColors menu;
    TabBarPaletteColors tabBar;
    TextLineEditPaletteColors textLineEdit;
    GroupboxPaletteColors groupBox;
    HeaderPaletteColors header;
    TooltipPaletteColors toolTip;
};

AllPalletesColors makeAllPalettesColorsLight()
{
    const auto palettes = makePalettesColorsLight();

    return {
        .palettes = palettes,
        .button = makeButtonPaletteColorsLight(),
        .toolButton = makeToolButtonPaletteColorsLight(),
        .system = makeSystemButtonPaletteColorsLight(),
        .menu = makeMenuPaletteColorsLight(palettes),
        .tabBar = makeTabBarPaletteColorsLight(),
        .textLineEdit = makeTextLineEditPaletteColorsLight(),
        .groupBox = makeGroupboxPaletteColorsLight(),
        .header = makeHeaderPaletteColorsLight(),
        .toolTip = makeTooltipPaletteColorsLight(),
    };
}

AllPalletesColors makeAllPalettesColorsDark()
{
    const auto palettes = makePalettesColorsDark();

    return {
        .palettes = palettes,
        .button = makeButtonPaletteColorsDark(),
        .toolButton = makeToolButtonPaletteColorsDark(),
        .system = makeSystemButtonPaletteColorsDark(),
        .menu = makeMenuPaletteColorsDark(palettes),
        .tabBar = makeTabBarPaletteColorsDark(),
        .textLineEdit = makeTextLineEditPaletteColorsDark(),
        .groupBox = makeGroupboxPaletteColorsDark(),
        .header = makeHeaderPaletteColorsDark(),
        .toolTip = makeTooltipPaletteColorsDark(),
    };
}

AllPalletesColors makeAllPalettesColors(Qt::ColorScheme scheme)
{
    switch (scheme) {
    case Qt::ColorScheme::Light: return makeAllPalettesColorsLight();
    case Qt::ColorScheme::Dark: return makeAllPalettesColorsDark();
    default:
        break;
    }

    qOhosReportFatalErrorAndAbort(
        "%s: got unsupported ColorScheme: %d", Q_FUNC_INFO, static_cast<int>(scheme));
}

constexpr int mouseDoubleClickDistance = 60;
constexpr int mouseDoubleClickInterval = 300;

QPalette makePalette(
    const QPalette &basePalette, std::initializer_list<std::tuple<QPalette::ColorGroup, QPalette::ColorRole, const QColor &>> brushEntries)
{
    auto palette = basePalette;

    for (const auto &brushEntry : brushEntries) {
        palette.setBrush(
            std::get<QPalette::ColorGroup>(brushEntry),
            std::get<QPalette::ColorRole>(brushEntry),
            std::get<const QColor &>(brushEntry));
    }

    return palette;
}

QPalette makePalette(std::initializer_list<std::tuple<QPalette::ColorGroup, QPalette::ColorRole, const QColor &>> brushEntries)
{
    return makePalette(QPalette(), brushEntries);
}

QPalette makeButtonPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::AlternateBase, palettesColors.palettes.componentActivated},

            {QPalette::Active, QPalette::Button, palettesColors.button.activeButton},
            {QPalette::Disabled, QPalette::Button, palettesColors.button.inactiveButton},
            {QPalette::Inactive, QPalette::Button, palettesColors.button.inactiveButton},

            {QPalette::Active, QPalette::ButtonText, palettesColors.palettes.activeButtonText},
            {QPalette::Disabled, QPalette::ButtonText, palettesColors.palettes.inactiveButtonText},
            {QPalette::Inactive, QPalette::ButtonText, palettesColors.palettes.inactiveButtonText},

            {QPalette::All, QPalette::BrightText, palettesColors.button.enabledDefaultButtonText},

            {QPalette::Active, QPalette::Light, palettesColors.palettes.hover},
            {QPalette::Active, QPalette::Dark, palettesColors.palettes.clickEffect},

            {QPalette::Active, QPalette::Highlight, palettesColors.button.focusedOutline},
            {QPalette::Active, QPalette::WindowText, palettesColors.palettes.indicatorArrow}
        });
}

QPalette makeToolButtonPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        makeButtonPalette(palettesColors),
        {
            {QPalette::Active, QPalette::AlternateBase, palettesColors.toolButton.windowButtonBackground},
        });
}

QPalette makeSystemPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::Window, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::Window, palettesColors.palettes.inactiveWindow},
            {QPalette::Inactive, QPalette::Window, palettesColors.palettes.inactiveWindow},

            {QPalette::Active, QPalette::WindowText, palettesColors.palettes.textPrimary},
            {QPalette::Disabled, QPalette::WindowText, palettesColors.palettes.disabledTextPrimary},
            {QPalette::Inactive, QPalette::WindowText, palettesColors.palettes.inactiveTextPrimary},

            {QPalette::Active, QPalette::Text, palettesColors.palettes.textPrimary},
            {QPalette::Disabled, QPalette::Text, palettesColors.palettes.disabledTextPrimary},
            {QPalette::Inactive, QPalette::Text, palettesColors.palettes.inactiveTextPrimary},

            {QPalette::Active, QPalette::Base, palettesColors.system.activeWindowFrame},
            {QPalette::Disabled, QPalette::Base, palettesColors.system.inactiveWindowFrame},
            {QPalette::Inactive, QPalette::Base, palettesColors.system.inactiveWindowFrame},

            {QPalette::Active, QPalette::AlternateBase, palettesColors.palettes.textPrimary},

            {QPalette::Active, QPalette::Highlight, palettesColors.palettes.highlighted},

            {QPalette::All, QPalette::PlaceholderText, palettesColors.system.textHint},
            {QPalette::Disabled, QPalette::PlaceholderText, palettesColors.system.textTertiary},

            {QPalette::Active, QPalette::Midlight, palettesColors.palettes.scrollBarSlider},

            {QPalette::Active, QPalette::Mid, palettesColors.palettes.componentNormal}
        });
}

QPalette makeCheckBoxOrRadioButtonPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Inactive, QPalette::Base, palettesColors.palettes.switchBgOff},
            {QPalette::Inactive, QPalette::Button, palettesColors.palettes.switchOutlineOff},
            {QPalette::Active, QPalette::Button, palettesColors.palettes.highlighted},
            {QPalette::Active, QPalette::WindowText, palettesColors.palettes.highlighted},
            {QPalette::Inactive, QPalette::WindowText, palettesColors.palettes.inactiveText},
            {QPalette::Disabled, QPalette::WindowText, palettesColors.palettes.inactiveHighlighted},
            {QPalette::Active, QPalette::Light, palettesColors.palettes.hover},
            {QPalette::Active, QPalette::Dark, palettesColors.palettes.clickEffect},
        });
}

QPalette makeComboBoxPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::AlternateBase, palettesColors.palettes.componentActivated},
            {QPalette::All, QPalette::Base, palettesColors.palettes.window},
            {QPalette::All, QPalette::Window, palettesColors.palettes.window},
            {QPalette::Active, QPalette::Text, palettesColors.palettes.highlighted},
            {QPalette::Disabled, QPalette::Text, palettesColors.palettes.inactiveText},
            {QPalette::Inactive, QPalette::Text, palettesColors.palettes.inactiveHighlighted},
            {QPalette::Active, QPalette::Light, palettesColors.palettes.hover},
            {QPalette::Active, QPalette::Dark, palettesColors.palettes.clickEffect},
            {QPalette::Active, QPalette::WindowText, palettesColors.palettes.indicatorArrow}
        });
}

QPalette makeMenuPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::Window, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::Base, palettesColors.palettes.switchBgOff},
            {QPalette::Inactive, QPalette::Button, palettesColors.palettes.switchOutlineOff},
            {QPalette::Active, QPalette::Button, palettesColors.palettes.highlighted},

            {QPalette::Active, QPalette::Text, palettesColors.palettes.textPrimary},
            {QPalette::Disabled, QPalette::Text, palettesColors.menu.inactiveText},
            {QPalette::Inactive, QPalette::Text, palettesColors.menu.inactiveText},

            {QPalette::Active, QPalette::Light, palettesColors.palettes.hover},
            {QPalette::Active, QPalette::Dark, palettesColors.palettes.clickEffect},

            {QPalette::Active, QPalette::Mid, palettesColors.menu.listSeparator}
        });
}

QPalette makeMenuBarPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::ButtonText, palettesColors.palettes.textPrimary},
            {QPalette::Disabled, QPalette::ButtonText, palettesColors.menu.inactiveText},
            {QPalette::Inactive, QPalette::ButtonText, palettesColors.menu.inactiveText},
        });
}

QPalette makeTabBarPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::WindowText, palettesColors.tabBar.activeWindowText},
            {QPalette::Inactive, QPalette::WindowText, palettesColors.tabBar.inactiveWindowText},
            {QPalette::Disabled, QPalette::WindowText, palettesColors.tabBar.disabledWindowText},

            {QPalette::Active, QPalette::Window, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::Window, palettesColors.palettes.inactiveWindow},
            {QPalette::Disabled, QPalette::Window, palettesColors.palettes.inactiveWindow},

            {QPalette::Active, QPalette::Highlight, palettesColors.palettes.activeHighlight},
            {QPalette::Inactive, QPalette::Highlight, palettesColors.palettes.inactiveHighlight},
            {QPalette::Disabled, QPalette::Highlight, palettesColors.palettes.inactiveHighlight},
        });
}

QPalette makeTextLineEditPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        makeSystemPalette(palettesColors),
        {
            {QPalette::Active, QPalette::AlternateBase, palettesColors.palettes.componentActivated},
            {QPalette::All, QPalette::Base, palettesColors.textLineEdit.textEditBackground},
            {QPalette::Disabled, QPalette::Base, palettesColors.textLineEdit.inactiveTextEditBackground},
            {QPalette::All, QPalette::Text, palettesColors.palettes.textPrimary},
            {QPalette::Disabled, QPalette::Text, palettesColors.palettes.disabledTextPrimary},
        });
}

QPalette makeTextEditPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        makeTextLineEditPalette(palettesColors),
        {
            {QPalette::All, QPalette::Base, palettesColors.palettes.activeWindow},
        });
}

QPalette makeGroupBoxPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::Button, palettesColors.palettes.highlighted},
            {QPalette::Inactive, QPalette::Button, palettesColors.palettes.switchOutlineOff},
            {QPalette::Active, QPalette::Base, palettesColors.system.activeWindowFrame},
            {QPalette::Inactive, QPalette::Base, palettesColors.palettes.switchBgOff},
            {QPalette::Active, QPalette::AlternateBase, palettesColors.groupBox.groupBoxFrame},
            {QPalette::Active, QPalette::Window, palettesColors.groupBox.background},
            {QPalette::Inactive, QPalette::Window, palettesColors.groupBox.background},
            {QPalette::Active, QPalette::Window, palettesColors.palettes.window},
            {QPalette::Disabled, QPalette::Window, palettesColors.palettes.window},
            {QPalette::Active, QPalette::Text, palettesColors.palettes.highlighted},
            {QPalette::Inactive, QPalette::Text, palettesColors.palettes.highlighted},
            {QPalette::Disabled, QPalette::Text, palettesColors.palettes.inactiveHighlighted},
            {QPalette::Current, QPalette::Text, palettesColors.palettes.highlighted},
            {QPalette::Disabled, QPalette::WindowText, palettesColors.palettes.inactiveHighlighted},
            {QPalette::Active, QPalette::ButtonText, palettesColors.palettes.textPrimary},
            {QPalette::Disabled, QPalette::ButtonText, palettesColors.palettes.inactiveHighlighted},
            {QPalette::Active, QPalette::Dark, palettesColors.palettes.clickEffect},
            {QPalette::Active, QPalette::Light, palettesColors.palettes.hover},
            {QPalette::Active, QPalette::Shadow, palettesColors.groupBox.shadowXS},
            {QPalette::Active, QPalette::Midlight, palettesColors.palettes.scrollBarSlider},
            {QPalette::Active, QPalette::Mid, palettesColors.palettes.componentNormal}
        });
}

QPalette makeHeaderPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::Button, palettesColors.header.activeButton},
            {QPalette::Inactive, QPalette::Button, palettesColors.header.activeButton},
            {QPalette::Disabled, QPalette::Button, palettesColors.palettes.inactiveWindow},

            {QPalette::Current, QPalette::ButtonText, palettesColors.palettes.activeButtonText},
            {QPalette::Active, QPalette::ButtonText, palettesColors.palettes.activeButtonText},
            {QPalette::Inactive, QPalette::ButtonText, palettesColors.palettes.activeButtonText},
            {QPalette::Disabled, QPalette::ButtonText, palettesColors.palettes.inactiveButtonText},
        });
}

QPalette makeItemViewPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::Base, palettesColors.palettes.window},
            {QPalette::Inactive, QPalette::Base, palettesColors.palettes.window},
            {QPalette::Disabled, QPalette::Base, palettesColors.palettes.window},

            {QPalette::Active, QPalette::AlternateBase, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::AlternateBase, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::AlternateBase, palettesColors.palettes.inactiveWindow},
            {QPalette::Current, QPalette::AlternateBase, palettesColors.palettes.activeWindow},

            {QPalette::Active, QPalette::Highlight, palettesColors.palettes.activeHighlight},
            {QPalette::Inactive, QPalette::Highlight, palettesColors.palettes.activeHighlight},
            {QPalette::Disabled, QPalette::Highlight, palettesColors.palettes.inactiveHighlight},

            {QPalette::Active, QPalette::Window, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::Window, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::Window, palettesColors.palettes.inactiveWindow},

            {QPalette::Active, QPalette::Text, palettesColors.palettes.activeHighlight},
            {QPalette::Inactive, QPalette::Text, palettesColors.palettes.activeHighlight},
            {QPalette::Disabled, QPalette::Text, palettesColors.palettes.inactiveHighlight},

            {QPalette::Active, QPalette::HighlightedText, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::HighlightedText, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::HighlightedText, palettesColors.palettes.inactiveWindow},

            {QPalette::Active, QPalette::Dark, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::Dark, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::Dark, palettesColors.palettes.inactiveWindow},
            {QPalette::Current, QPalette::Dark, palettesColors.palettes.activeWindow},
            {QPalette::All, QPalette::Dark, palettesColors.palettes.activeWindow},

            {QPalette::Active, QPalette::Light, palettesColors.palettes.hover},
            {QPalette::Inactive, QPalette::Light, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::Light, palettesColors.palettes.inactiveWindow},

            {QPalette::Active, QPalette::Mid, palettesColors.palettes.activeWindow},
            {QPalette::Inactive, QPalette::Mid, palettesColors.palettes.activeWindow},
            {QPalette::Disabled, QPalette::Mid, palettesColors.palettes.inactiveWindow},
            {QPalette::Current, QPalette::Mid, palettesColors.palettes.activeWindow},
            {QPalette::All, QPalette::Mid, palettesColors.palettes.activeWindow},
        });
}

QPalette makeLabelPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::Active, QPalette::WindowText, palettesColors.palettes.highlighted},
            {QPalette::Inactive, QPalette::WindowText, palettesColors.palettes.inactiveText},
            {QPalette::Disabled, QPalette::WindowText, palettesColors.palettes.inactiveHighlighted},

            {QPalette::Active, QPalette::Text, palettesColors.palettes.highlighted},
            {QPalette::Disabled, QPalette::Text, palettesColors.palettes.inactiveText},
            {QPalette::Inactive, QPalette::Text, palettesColors.palettes.inactiveHighlighted},
        });
}

QPalette makeToolTipPalette(const AllPalletesColors &palettesColors)
{
    return makePalette(
        {
            {QPalette::All, QPalette::Base, palettesColors.toolTip.toolTipBorder},
            {QPalette::All, QPalette::ToolTipBase, palettesColors.toolTip.toolTipBase},
            {QPalette::All, QPalette::ToolTipText, palettesColors.palettes.highlighted},
        });
}

QFont makeTitleFont()
{
    auto font = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont();
    font.setPointSize(12);
    font.setBold(false);

    return font;
}

QFont makePushButtonFont()
{
    auto font = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont();
    font.setPointSize(16);

    return font;
}

QFont makeListViewFont()
{
    auto font = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont();
    font.setPointSize(16);

    return font;
}

QFont makeMenuFont()
{
    auto font = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont();
    font.setPointSize(16);

    return font;
}

QFont makeTipLabelFont()
{
    auto font = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont();
    font.setPointSize(14);

    return font;
}

QHash<QPlatformTheme::Palette, QPalette> makePalettesMap(Qt::ColorScheme scheme)
{
    const AllPalletesColors palettesColors = makeAllPalettesColors(scheme);
    return {
        {QPlatformTheme::ButtonPalette, makeButtonPalette(palettesColors)},
        {QPlatformTheme::CheckBoxPalette, makeCheckBoxOrRadioButtonPalette(palettesColors)},
        {QPlatformTheme::ComboBoxPalette, makeComboBoxPalette(palettesColors)},
        {QPlatformTheme::GroupBoxPalette, makeGroupBoxPalette(palettesColors)},
        {QPlatformTheme::HeaderPalette, makeHeaderPalette(palettesColors)},
        {QPlatformTheme::ItemViewPalette, makeItemViewPalette(palettesColors)},
        {QPlatformTheme::LabelPalette, makeLabelPalette(palettesColors)},
        {QPlatformTheme::MenuPalette, makeMenuPalette(palettesColors)},
        {QPlatformTheme::MenuBarPalette, makeMenuBarPalette(palettesColors)},
        {QPlatformTheme::RadioButtonPalette, makeCheckBoxOrRadioButtonPalette(palettesColors)},
        {QPlatformTheme::SystemPalette, makeSystemPalette(palettesColors)},
        {QPlatformTheme::TabBarPalette, makeTabBarPalette(palettesColors)},
        {QPlatformTheme::TextEditPalette, makeTextEditPalette(palettesColors)},
        {QPlatformTheme::TextLineEditPalette, makeTextLineEditPalette(palettesColors)},
        {QPlatformTheme::ToolButtonPalette, makeToolButtonPalette(palettesColors)},
        {QPlatformTheme::ToolTipPalette, makeToolTipPalette(palettesColors)},
    };
}

QOhosOptional<bool> mapOhosThemeFromColorScheme(
    Qt::ColorScheme scheme)
{
    switch (scheme) {
    case Qt::ColorScheme::Unknown:
        return makeEmptyQOhosOptional();
    case Qt::ColorScheme::Light:
        return makeQOhosOptional(false);
    case Qt::ColorScheme::Dark:
        return makeQOhosOptional(true);
    };
    return makeEmptyQOhosOptional();
}

Qt::ColorScheme mapOhosThemeToColorScheme(
    QOhosOptional<bool> darkModeFlag)
{
    if (darkModeFlag.has_value()) {
        if (!darkModeFlag.value_or(false))
            return Qt::ColorScheme::Light;
        else
            return Qt::ColorScheme::Dark;
    } else {
        return Qt::ColorScheme::Unknown;
    }
}

using OhosConfigurationColorMode = QtOhos::enums::ohos::app::ability::ConfigurationConstant::ColorMode;

OhosConfigurationColorMode mapOhosConfigurationColorModeFromJs(QtOhos::JsState &jsState, QNapi::Number colorModeJsEnum)
{
    constexpr auto fallbackColorMode = OhosConfigurationColorMode::COLOR_MODE_NOT_SET;
    auto optColorMode = jsState.tryMapOhosEnumFromJs<OhosConfigurationColorMode>(colorModeJsEnum);
    return optColorMode.value_or(fallbackColorMode);
};

void setOhosConfigColorMode(OhosConfigurationColorMode colorMode)
{
    if (QtOhos::isOhosNoUiChildMode()) {
        qCWarning(QtForOhos, "%s: cannot set a color mode in 'no UI child mode'", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &jsState) {
            auto qAbility = jsState.defaultQAbilityPeer()->qAbility();
            const auto jsColorMode = jsState.mapOhosEnumToJs(colorMode);
            qAbility.call("context.getApplicationContext().setColorMode", {jsColorMode});
        },
        Q_FUNC_INFO);
}

QOhosSupplier<OhosConfigurationColorMode> makeOhosConfigColorModeDataSource(
    QOhosConsumer<OhosConfigurationColorMode> valueChangedHandler)
{
    return QtOhos::makeOhosConfigValueDataSource<OhosConfigurationColorMode>(
        [](QtOhos::JsState &jsState) {
            return mapOhosConfigurationColorModeFromJs(
                jsState, jsState.defaultQAbilityPeer()->qAbility().eval<QNapi::Number>("context.config.colorMode"));
        },
        [](QtOhos::JsState &jsState, const QNapi::Object &config) {
            return mapOhosConfigurationColorModeFromJs(jsState, config.get<QNapi::Number>("colorMode"));
        },
        std::move(valueChangedHandler));
}

std::optional<bool> mapOhosConfigurationColorModeToDarkModeFlag(OhosConfigurationColorMode colorMode)
{
    switch (colorMode) {
    case OhosConfigurationColorMode::COLOR_MODE_NOT_SET:
        return {};
    case OhosConfigurationColorMode::COLOR_MODE_LIGHT:
        return false;
    case OhosConfigurationColorMode::COLOR_MODE_DARK:
        return true;
    }

    return {};
}

QOhosOptional<QPixmap> tryGetFilePixmapByResourceObject(QtOhos::JsState &jsState, QNapi::Object resource)
{
    std::string undocumentedParamsPropertyName("params");
    auto optParams = QNapi::getOptionalPropOrEmpty<QNapi::Value>(resource, undocumentedParamsPropertyName);
    if (optParams.IsEmpty()) {
        qOhosPrintfWarning(
            "%s: expected '%s' property not found in resource object, ignore getting pixmap",
            Q_FUNC_INFO, undocumentedParamsPropertyName.c_str());
        return makeEmptyQOhosOptional();
    }
    if (!optParams.IsArray()) {
        qOhosPrintfWarning(
            "%s: '%s' property is not an array, ignore getting pixmap",
            Q_FUNC_INFO, undocumentedParamsPropertyName.c_str());
        return makeEmptyQOhosOptional();
    }

    auto params = QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(
        QNapi::checkedCast<QNapi::Array>(optParams));
    if (params.empty()) {
        qOhosPrintfWarning(
            "%s: '%s' property is empty, ignore getting pixmap",
            Q_FUNC_INFO, undocumentedParamsPropertyName.c_str());
        return makeEmptyQOhosOptional();
    }

    const auto resourceIdentifier = params.front();
    const auto lastSeparatorPosition = resourceIdentifier.rfind('.');
    if (lastSeparatorPosition == std::string::npos) {
        qOhosPrintfWarning(
            "%s: '%s' property has unknown resource name, ignore getting pixmap",
            Q_FUNC_INFO, undocumentedParamsPropertyName.c_str());
        return makeEmptyQOhosOptional();
    }

    const auto simpleName = resourceIdentifier.substr(lastSeparatorPosition + 1);
    auto resourceContentBuffer = jsState.eval<QNapi::TypedArrayOf<std::uint8_t>>(
        "@ohos.resourceManager.getSysResourceManager().getMediaByNameSync(*)", {simpleName});

    QPixmap pixmap;
    bool pixmapLoadResult = pixmap.loadFromData(
        static_cast<const uchar *>(resourceContentBuffer.Data()), resourceContentBuffer.ByteLength());

    if (pixmapLoadResult) {
        return makeQOhosOptional(pixmap);
    } else {
        qOhosPrintfWarning("%s: cannot load pixmap", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
    }
}

QOhosOptional<QPixmap> tryGetFilePixmapByFilenameExtension(
    QtOhos::JsState &jsState, const std::string &filenameExtension)
{
    std::string ohosDotFilenameExtension = "." + filenameExtension;
    auto uniformDataType = jsState.eval<QNapi::String>(
        "@ohos.data.uniformTypeDescriptor.getUniformDataTypeByFilenameExtension(*)",
        {ohosDotFilenameExtension});

    auto resourceValue = jsState.eval(
        "@kit.FileManagerServiceKit.fileManagerService.getFileIconSync(*)", {uniformDataType});
    if (resourceValue.IsObject()) {
        return tryGetFilePixmapByResourceObject(jsState, QNapi::checkedCast<QNapi::Object>(resourceValue));
    } else {
        qOhosPrintfWarning("%s: got unhandled resource result type, ignore it", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
    }
}

class QOhosFileIconEngine : public QAbstractFileIconEngine
{
public:
    explicit QOhosFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts);

protected:
    QPixmap filePixmap(const QSize &size, QIcon::Mode mode, QIcon::State) override;
};

QOhosFileIconEngine::QOhosFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts)
    : QAbstractFileIconEngine(info, opts)
{
}

QPixmap QOhosFileIconEngine::filePixmap(const QSize &size, QIcon::Mode, QIcon::State)
{
    auto fileNameSuffix = fileInfo().suffix();
    auto optPixmap = QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            return tryGetFilePixmapByFilenameExtension(jsState, fileNameSuffix.toStdString());
        },
        Q_FUNC_INFO);

    return optPixmap.has_value()
        ? optPixmap.value().scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        : QPixmap();
}

}

QOhosPlatformTheme::QOhosPlatformTheme()
    : m_themesPalettes(
        {
            {Qt::ColorScheme::Light, makePalettesMap(Qt::ColorScheme::Light)},
            {Qt::ColorScheme::Dark, makePalettesMap(Qt::ColorScheme::Dark)},
        })
    , m_fonts(
        {
            {DockWidgetTitleFont, makeTitleFont()},
            {GroupBoxTitleFont, makeTitleFont()},
            {ListViewFont, makeListViewFont()},
            {MenuBarFont, makeMenuFont()},
            {MenuFont, makeMenuFont()},
            {MenuItemFont, makeMenuFont()},
            {PushButtonFont, makePushButtonFont()},
            {TipLabelFont, makeTipLabelFont()},
        })
    , m_wheelScrollLines(defaultWheelScrollLines)
{
    auto colorModeDataSource = makeOhosConfigColorModeDataSource(
        [](auto) {
            QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
        });
    m_ohosConfigDarkModeFlagSupplier = [colorModeDataSource = std::move(colorModeDataSource)]() {
        return mapOhosConfigurationColorModeToDarkModeFlag(colorModeDataSource());
    };
}

void QOhosPlatformTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    QOhosOptional<bool> isDarkMode = mapOhosThemeFromColorScheme(scheme);
    setOhosConfigColorMode(
        isDarkMode.has_value()
            ? isDarkMode.value()
                ? OhosConfigurationColorMode::COLOR_MODE_DARK
                : OhosConfigurationColorMode::COLOR_MODE_LIGHT
            : OhosConfigurationColorMode::COLOR_MODE_NOT_SET);
}

Qt::ColorScheme QOhosPlatformTheme::colorScheme() const
{
    return mapOhosThemeToColorScheme(m_ohosConfigDarkModeFlagSupplier());
}

QPlatformDialogHelper *QOhosPlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformTheme::createPlatformDialogHelper");
    return QOhosDialogs::createHelper(type);
}

bool QOhosPlatformTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformTheme::usePlatformNativeDialog");
    return (type == QOhosPlatformTheme::FileDialog);
}

QVariant QOhosPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case StyleNames:
        return QStringList(QString::fromUtf8(ohosThemeName));
    case MouseDoubleClickDistance:
    case TouchDoubleTapDistance:
    {
        auto *primaryScreen = qGuiApp->primaryScreen();
        auto *platformScreen = primaryScreen != nullptr
            ? static_cast<QOhosPlatformScreen *>(primaryScreen->handle())
            : nullptr;
        return platformScreen != nullptr
            ? QHighDpi::fromNativePixels(
                QHighDpi::toNative(
                    mouseDoubleClickDistance, platformScreen->pixelScalingCoefficient()),
                platformScreen)
            : mouseDoubleClickDistance;
    }
    case MouseDoubleClickInterval:
        return mouseDoubleClickInterval;
    case WheelScrollLines:
        return m_wheelScrollLines;
    case MenuBarFocusOnAltPressRelease:
        return true;
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

const QPalette *QOhosPlatformTheme::palette(Palette type) const
{
    const auto &palettes = m_themesPalettes[colorScheme()];
    const auto it = palettes.find(type);
    if (it != palettes.end())
        return &it.value();
    else
        return QPlatformTheme::palette(type);
}

const QFont *QOhosPlatformTheme::font(Font type) const
{
    const auto fontIter = m_fonts.find(type);
    return fontIter != m_fonts.end()
        ? &fontIter.value()
        : QPlatformTheme::font(type);
}

void QOhosPlatformTheme::setWheelScrollLines(int wheelScrollLines)
{
    m_wheelScrollLines = wheelScrollLines;
}

QPlatformSystemTrayIcon *QOhosPlatformTheme::createPlatformSystemTrayIcon() const
{
    return makeQOhosSystemTrayIcon().release();
}

QIcon QOhosPlatformTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions) const
{
    return !fileInfo.suffix().isEmpty()
        ? QIcon(new QOhosFileIconEngine(fileInfo, iconOptions))
        : QPlatformTheme::fileIcon(fileInfo, iconOptions);
}

QT_END_NAMESPACE
