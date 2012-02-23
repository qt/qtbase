/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoasystemsettings.h"

#include <Carbon/Carbon.h>
#include <QtCore/private/qcore_mac_p.h>

QColor qt_mac_colorFromCGColor(CGColorRef cgcolor)
{
    QColor pc;
    CGColorSpaceModel model = CGColorSpaceGetModel(CGColorGetColorSpace(cgcolor));
    const CGFloat *components = CGColorGetComponents(cgcolor);
    if (model == kCGColorSpaceModelRGB) {
        pc.setRgbF(components[0], components[1], components[2], components[3]);
    } else if (model == kCGColorSpaceModelCMYK) {
        pc.setCmykF(components[0], components[1], components[2], components[3]);
    } else if (model == kCGColorSpaceModelMonochrome) {
        pc.setRgbF(components[0], components[0], components[0], components[1]);
    } else {
        // Colorspace we can't deal with.
        qWarning("Qt: qcolorFromCGColor: cannot convert from colorspace model: %d", model);
        Q_ASSERT(false);
    }
    return pc;
}

QColor qt_mac_colorForTheme(ThemeBrush brush)
{
    QCFType<CGColorRef> cgClr = 0;
    HIThemeBrushCreateCGColor(brush, &cgClr);
    return qt_mac_colorFromCGColor(cgClr);
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
        return QColor(69, 69, 69, 255);
    case kThemeTextColorPopupButtonInactive:
    case kThemeTextColorPopupLabelInactive:
    case kThemeTextColorPushButtonInactive:
    case kThemeTextColorTabFrontInactive:
    case kThemeTextColorBevelButtonInactive:
        return QColor(127, 127, 127, 255);
    default:
        return QColor(0, 0, 0, 255); // ### TODO: Sample color like Qt 4.
    }
}

QPalette * qt_mac_createSystemPalette()
{
    QColor qc;

    // Standard palette initialization (copied from Qt 4 styles)
    QColor background = QColor(0xd4, 0xd0, 0xc8); // win 2000 grey
    QColor light(background.lighter());
    QColor dark(background.darker());
    QColor mid(Qt::gray);
    QPalette *palette = new QPalette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);

    palette->setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette->setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Base, background);
    palette->setColor(QPalette::Disabled, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Active, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Inactive, QPalette::Dark, QColor(191, 191, 191));

    // System palette initialization:
    palette->setBrush( QPalette::Active, QPalette::Highlight, qt_mac_colorForTheme(kThemeBrushPrimaryHighlightColor) );
    palette->setBrush( QPalette::Inactive, QPalette::Highlight, qt_mac_colorForTheme(kThemeBrushSecondaryHighlightColor) );

    palette->setBrush( QPalette::Disabled, QPalette::Highlight, qt_mac_colorForTheme(kThemeBrushSecondaryHighlightColor) );
    palette->setBrush( QPalette::Active, QPalette::Shadow, qt_mac_colorForTheme(kThemeBrushButtonActiveDarkShadow) );

    palette->setBrush( QPalette::Inactive, QPalette::Shadow, qt_mac_colorForTheme(kThemeBrushButtonInactiveDarkShadow) );
    palette->setBrush( QPalette::Disabled, QPalette::Shadow, qt_mac_colorForTheme(kThemeBrushButtonInactiveDarkShadow) );

    qc = qt_mac_colorForThemeTextColor(kThemeTextColorDialogActive);
    palette->setColor(QPalette::Active, QPalette::Text, qc);
    palette->setColor(QPalette::Active, QPalette::WindowText, qc);
    palette->setColor(QPalette::Active, QPalette::HighlightedText, qc);

    qc = qt_mac_colorForThemeTextColor(kThemeTextColorDialogInactive);
    palette->setColor(QPalette::Inactive, QPalette::Text, qc);
    palette->setColor(QPalette::Inactive, QPalette::WindowText, qc);
    palette->setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
    palette->setColor(QPalette::Disabled, QPalette::Text, qc);
    palette->setColor(QPalette::Disabled, QPalette::WindowText, qc);
    palette->setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
    palette->setBrush(QPalette::ToolTipBase, QColor(255, 255, 199));
    return palette;
}

