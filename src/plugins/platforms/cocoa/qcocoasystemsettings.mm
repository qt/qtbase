/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

QT_BEGIN_NAMESPACE

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

    qc = qt_mac_toQColor([NSColor controlTextColor]);
    palette->setColor(QPalette::Active, QPalette::Text, qc);
    palette->setColor(QPalette::Active, QPalette::WindowText, qc);
    palette->setColor(QPalette::Active, QPalette::HighlightedText, qc);
    palette->setColor(QPalette::Inactive, QPalette::Text, qc);
    palette->setColor(QPalette::Inactive, QPalette::WindowText, qc);
    palette->setColor(QPalette::Inactive, QPalette::HighlightedText, qc);

    qc = qt_mac_toQColor([NSColor disabledControlTextColor]);
    palette->setColor(QPalette::Disabled, QPalette::Text, qc);
    palette->setColor(QPalette::Disabled, QPalette::WindowText, qc);
    palette->setColor(QPalette::Disabled, QPalette::HighlightedText, qc);

    palette->setBrush(QPalette::ToolTipBase, qt_mac_toQBrush([NSColor controlColor]));

    return palette;
}

struct QMacPaletteMap {
    inline QMacPaletteMap(QPlatformTheme::Palette p, NSColor *a, NSColor *i) :
        paletteRole(p), active(a), inactive(i) { }

    QPlatformTheme::Palette paletteRole;
    NSColor *active, *inactive;
};

#define MAC_PALETTE_ENTRY(pal, active, inactive) \
    QMacPaletteMap(pal, [NSColor active], [NSColor inactive])
static QMacPaletteMap mac_widget_colors[] = {
    MAC_PALETTE_ENTRY(QPlatformTheme::ToolButtonPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::ButtonPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::HeaderPalette, headerTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::ComboBoxPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::ItemViewPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::MessageBoxLabelPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::TabBarPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::LabelPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::GroupBoxPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::MenuPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::MenuBarPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::TextEditPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::TextLineEditPalette, textColor, disabledControlTextColor)
};
#undef MAC_PALETTE_ENTRY

static const int mac_widget_colors_count = sizeof(mac_widget_colors) / sizeof(mac_widget_colors[0]);

QHash<QPlatformTheme::Palette, QPalette*> qt_mac_createRolePalettes()
{
    QHash<QPlatformTheme::Palette, QPalette*> palettes;
    QColor qc;
    for (int i = 0; i < mac_widget_colors_count; i++) {
        QPalette &pal = *qt_mac_createSystemPalette();
        if (mac_widget_colors[i].active != 0) {
            qc = qt_mac_toQColor(mac_widget_colors[i].active);
            pal.setColor(QPalette::Active, QPalette::Text, qc);
            pal.setColor(QPalette::Inactive, QPalette::Text, qc);
            pal.setColor(QPalette::Active, QPalette::WindowText, qc);
            pal.setColor(QPalette::Inactive, QPalette::WindowText, qc);
            pal.setColor(QPalette::Active, QPalette::HighlightedText, qc);
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
            qc = qt_mac_toQColor(mac_widget_colors[i].inactive);
            pal.setColor(QPalette::Disabled, QPalette::Text, qc);
            pal.setColor(QPalette::Disabled, QPalette::WindowText, qc);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
        }
        if (mac_widget_colors[i].paletteRole == QPlatformTheme::MenuPalette
                || mac_widget_colors[i].paletteRole == QPlatformTheme::MenuBarPalette) {
            pal.setBrush(QPalette::Highlight, qt_mac_toQColor([NSColor selectedMenuItemColor]));
            qc = qt_mac_toQColor([NSColor labelColor]);
            pal.setBrush(QPalette::ButtonText, qc);
            pal.setBrush(QPalette::Text, qc);
            qc = qt_mac_toQColor([NSColor selectedMenuItemTextColor]);
            pal.setBrush(QPalette::HighlightedText, qc);
            qc = qt_mac_toQColor([NSColor disabledControlTextColor]);
            pal.setBrush(QPalette::Disabled, QPalette::Text, qc);
        } else if ((mac_widget_colors[i].paletteRole == QPlatformTheme::ButtonPalette)
                || (mac_widget_colors[i].paletteRole == QPlatformTheme::HeaderPalette)
                || (mac_widget_colors[i].paletteRole == QPlatformTheme::TabBarPalette)) {
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
