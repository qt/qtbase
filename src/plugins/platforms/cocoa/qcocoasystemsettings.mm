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

#include "qcocoasystemsettings.h"

#include "qcocoahelpers.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/qfont.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <Carbon/Carbon.h>

QT_BEGIN_NAMESPACE

QBrush qt_mac_brushForTheme(ThemeBrush brush)
{
    QMacAutoReleasePool pool;

    QCFType<CGColorRef> cgClr = 0;
    HIThemeBrushCreateCGColor(brush, &cgClr);
    return qt_mac_toQBrush(cgClr);
}

QColor qt_mac_colorForThemeTextColor(ThemeTextColor themeColor)
{
    // No GetThemeTextColor in 64-bit mode, use hardcoded values:
    switch (themeColor) {
    case kThemeTextColorAlertActive:
    case kThemeTextColorTabFrontActive:
    case kThemeTextColorBevelButtonActive:
    case kThemeTextColorListView:
    case kThemeTextColorPlacardActive:
    case kThemeTextColorPopupButtonActive:
    case kThemeTextColorPopupLabelActive:
    case kThemeTextColorPushButtonActive:
        return Qt::black;
    case kThemeTextColorAlertInactive:
    case kThemeTextColorDialogInactive:
    case kThemeTextColorPlacardInactive:
    case kThemeTextColorPopupButtonInactive:
    case kThemeTextColorPopupLabelInactive:
    case kThemeTextColorPushButtonInactive:
    case kThemeTextColorTabFrontInactive:
    case kThemeTextColorBevelButtonInactive:
    case kThemeTextColorMenuItemDisabled:
        return QColor(127, 127, 127, 255);
    case kThemeTextColorMenuItemSelected:
        return Qt::white;
    default:
        return QColor(0, 0, 0, 255); // ### TODO: Sample color like Qt 4.
    }
}

QPalette * qt_mac_createSystemPalette()
{
    QColor qc;

    // Standard palette initialization (copied from Qt 4 styles)
    QBrush backgroundBrush = qt_mac_toQBrush([NSColor windowBackgroundColor]);
    QColor background = backgroundBrush.color();
    QColor light(background.lighter(110));
    QColor dark(background.darker(160));
    QColor mid(background.darker(140));
    QPalette *palette = new QPalette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);

    palette->setBrush(QPalette::Window, backgroundBrush);

    palette->setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette->setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Base, backgroundBrush);
    palette->setColor(QPalette::Disabled, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Active, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Inactive, QPalette::Dark, QColor(191, 191, 191));

    // System palette initialization:
    palette->setBrush(QPalette::Active, QPalette::Highlight,
        qt_mac_toQBrush([NSColor selectedControlColor]));
    QBrush br = qt_mac_toQBrush([NSColor secondarySelectedControlColor]);
    palette->setBrush(QPalette::Inactive, QPalette::Highlight, br);
    palette->setBrush(QPalette::Disabled, QPalette::Highlight, br);

    palette->setBrush(QPalette::Shadow, background.darker(170));

    qc = qt_mac_colorForThemeTextColor(kThemeTextColorDialogActive);
    palette->setColor(QPalette::Active, QPalette::Text, qc);
    palette->setColor(QPalette::Active, QPalette::WindowText, qc);
    palette->setColor(QPalette::Active, QPalette::HighlightedText, qc);
    palette->setColor(QPalette::Inactive, QPalette::Text, qc);
    palette->setColor(QPalette::Inactive, QPalette::WindowText, qc);
    palette->setColor(QPalette::Inactive, QPalette::HighlightedText, qc);

    qc = qt_mac_colorForThemeTextColor(kThemeTextColorDialogInactive);
    palette->setColor(QPalette::Disabled, QPalette::Text, qc);
    palette->setColor(QPalette::Disabled, QPalette::WindowText, qc);
    palette->setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
    palette->setBrush(QPalette::ToolTipBase, QColor(255, 255, 199));
    return palette;
}

struct QMacPaletteMap {
    inline QMacPaletteMap(QPlatformTheme::Palette p, ThemeBrush a, ThemeBrush i) :
        paletteRole(p), active(a), inactive(i) { }

    QPlatformTheme::Palette paletteRole;
    ThemeBrush active, inactive;
};

static QMacPaletteMap mac_widget_colors[] = {
    QMacPaletteMap(QPlatformTheme::ToolButtonPalette, kThemeTextColorBevelButtonActive, kThemeTextColorBevelButtonInactive),
    QMacPaletteMap(QPlatformTheme::ButtonPalette, kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
    QMacPaletteMap(QPlatformTheme::HeaderPalette, kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
    QMacPaletteMap(QPlatformTheme::ComboBoxPalette, kThemeTextColorPopupButtonActive, kThemeTextColorPopupButtonInactive),
    QMacPaletteMap(QPlatformTheme::ItemViewPalette, kThemeTextColorListView, kThemeTextColorDialogInactive),
    QMacPaletteMap(QPlatformTheme::MessageBoxLabelPalette, kThemeTextColorAlertActive, kThemeTextColorAlertInactive),
    QMacPaletteMap(QPlatformTheme::TabBarPalette, kThemeTextColorTabFrontActive, kThemeTextColorTabFrontInactive),
    QMacPaletteMap(QPlatformTheme::LabelPalette, kThemeTextColorPlacardActive, kThemeTextColorPlacardInactive),
    QMacPaletteMap(QPlatformTheme::GroupBoxPalette, kThemeTextColorPlacardActive, kThemeTextColorPlacardInactive),
    QMacPaletteMap(QPlatformTheme::MenuPalette, kThemeTextColorMenuItemActive, kThemeTextColorMenuItemDisabled),
    QMacPaletteMap(QPlatformTheme::MenuBarPalette, kThemeTextColorMenuItemActive, kThemeTextColorMenuItemDisabled),
    //### TODO: The zeros below gives white-on-black text.
    QMacPaletteMap(QPlatformTheme::TextEditPalette, 0, 0),
    QMacPaletteMap(QPlatformTheme::TextLineEditPalette, 0, 0),
    QMacPaletteMap(QPlatformTheme::NPalettes, 0, 0) };

QHash<QPlatformTheme::Palette, QPalette*> qt_mac_createRolePalettes()
{
    QHash<QPlatformTheme::Palette, QPalette*> palettes;
    QColor qc;
    for (int i = 0; mac_widget_colors[i].paletteRole != QPlatformTheme::NPalettes; i++) {
        QPalette &pal = *qt_mac_createSystemPalette();
        if (mac_widget_colors[i].active != 0) {
            qc = qt_mac_colorForThemeTextColor(mac_widget_colors[i].active);
            pal.setColor(QPalette::Active, QPalette::Text, qc);
            pal.setColor(QPalette::Inactive, QPalette::Text, qc);
            pal.setColor(QPalette::Active, QPalette::WindowText, qc);
            pal.setColor(QPalette::Inactive, QPalette::WindowText, qc);
            pal.setColor(QPalette::Active, QPalette::HighlightedText, qc);
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
            qc = qt_mac_colorForThemeTextColor(mac_widget_colors[i].inactive);
            pal.setColor(QPalette::Disabled, QPalette::Text, qc);
            pal.setColor(QPalette::Disabled, QPalette::WindowText, qc);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
        }
        if (mac_widget_colors[i].paletteRole == QPlatformTheme::MenuPalette
                || mac_widget_colors[i].paletteRole == QPlatformTheme::MenuBarPalette) {
            pal.setBrush(QPalette::Background, qt_mac_brushForTheme(kThemeBrushMenuBackground));
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemActive);
            pal.setBrush(QPalette::ButtonText, qc);
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemSelected);
            pal.setBrush(QPalette::HighlightedText, qc);
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemDisabled);
            pal.setBrush(QPalette::Disabled, QPalette::Text, qc);
        } else if ((mac_widget_colors[i].paletteRole == QPlatformTheme::ButtonPalette)
                || (mac_widget_colors[i].paletteRole == QPlatformTheme::HeaderPalette)) {
            pal.setColor(QPalette::Disabled, QPalette::ButtonText,
                         pal.color(QPalette::Disabled, QPalette::Text));
            pal.setColor(QPalette::Inactive, QPalette::ButtonText,
                         pal.color(QPalette::Inactive, QPalette::Text));
            pal.setColor(QPalette::Active, QPalette::ButtonText,
                         pal.color(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::ItemViewPalette) {
            pal.setBrush(QPalette::Active, QPalette::Highlight,
                         qt_mac_toQBrush([NSColor alternateSelectedControlColor]));
            pal.setBrush(QPalette::Active, QPalette::HighlightedText,
                         qt_mac_toQBrush([NSColor alternateSelectedControlTextColor]));
            pal.setBrush(QPalette::Inactive, QPalette::Text,
                          pal.brush(QPalette::Active, QPalette::Text));
            pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                          pal.brush(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::TextEditPalette) {
            pal.setBrush(QPalette::Inactive, QPalette::Text,
                          pal.brush(QPalette::Active, QPalette::Text));
            pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                          pal.brush(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::TextLineEditPalette) {
            pal.setBrush(QPalette::Disabled, QPalette::Base,
                         pal.brush(QPalette::Active, QPalette::Base));
        }
        palettes.insert(mac_widget_colors[i].paletteRole, &pal);
    }
    return palettes;
}

QT_END_NAMESPACE
