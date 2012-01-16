/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the utility applications of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PIXELMETRICS_H
#define PIXELMETRICS_H

#include <e32base.h>
#define S60_Rnd_Env

#ifdef S60_Rnd_Env
#pragma message ("Building in supported environment")

const TInt KUnknownBase = -5000;

NONSHARABLE_CLASS( QStyle )
    {
    public:
        enum PixelMetric {
            PM_ButtonMargin,
            PM_ButtonDefaultIndicator,
            PM_MenuButtonIndicator,
            PM_ButtonShiftHorizontal,
            PM_ButtonShiftVertical,

            PM_DefaultFrameWidth,
            PM_SpinBoxFrameWidth,
            PM_ComboBoxFrameWidth,

            PM_MaximumDragDistance,

            PM_ScrollBarExtent,
            PM_ScrollBarSliderMin,

            PM_SliderThickness,             // total slider thickness
            PM_SliderControlThickness,      // thickness of the business part
            PM_SliderLength,                // total length of slider
            PM_SliderTickmarkOffset,        //
            PM_SliderSpaceAvailable,        // available space for slider to move

            PM_DockWidgetSeparatorExtent,
            PM_DockWidgetHandleExtent,
            PM_DockWidgetFrameWidth,

            PM_TabBarTabOverlap,
            PM_TabBarTabHSpace,
            PM_TabBarTabVSpace,
            PM_TabBarBaseHeight,
            PM_TabBarBaseOverlap,

            PM_ProgressBarChunkWidth,

            PM_SplitterWidth,
            PM_TitleBarHeight,

            PM_MenuScrollerHeight,
            PM_MenuHMargin,
            PM_MenuVMargin,
            PM_MenuPanelWidth,
            PM_MenuTearoffHeight,
            PM_MenuDesktopFrameWidth,

            PM_MenuBarPanelWidth,
            PM_MenuBarItemSpacing,
            PM_MenuBarVMargin,
            PM_MenuBarHMargin,

            PM_IndicatorWidth,
            PM_IndicatorHeight,
            PM_ExclusiveIndicatorWidth,
            PM_ExclusiveIndicatorHeight,
            PM_CheckListButtonSize,
            PM_CheckListControllerSize,

            PM_DialogButtonsSeparator,
            PM_DialogButtonsButtonWidth,
            PM_DialogButtonsButtonHeight,

            PM_MdiSubWindowFrameWidth,
            PM_MDIFrameWidth = PM_MdiSubWindowFrameWidth,            //obsolete
            PM_MdiSubWindowMinimizedWidth,
            PM_MDIMinimizedWidth = PM_MdiSubWindowMinimizedWidth,    //obsolete

            PM_HeaderMargin,
            PM_HeaderMarkSize,
            PM_HeaderGripMargin,
            PM_TabBarTabShiftHorizontal,
            PM_TabBarTabShiftVertical,
            PM_TabBarScrollButtonWidth,

            PM_ToolBarFrameWidth,
            PM_ToolBarHandleExtent,
            PM_ToolBarItemSpacing,
            PM_ToolBarItemMargin,
            PM_ToolBarSeparatorExtent,
            PM_ToolBarExtensionExtent,

            PM_SpinBoxSliderHeight,

            PM_DefaultTopLevelMargin,
            PM_DefaultChildMargin,
            PM_DefaultLayoutSpacing,

            PM_ToolBarIconSize,
            PM_ListViewIconSize,
            PM_IconViewIconSize,
            PM_SmallIconSize,
            PM_LargeIconSize,

            PM_FocusFrameVMargin,
            PM_FocusFrameHMargin,

            PM_ToolTipLabelFrameWidth,
            PM_CheckBoxLabelSpacing,
            PM_TabBarIconSize,
            PM_SizeGripSize,
            PM_DockWidgetTitleMargin,
            PM_MessageBoxIconSize,
            PM_ButtonIconSize,

            PM_DockWidgetTitleBarButtonMargin,

            PM_RadioButtonLabelSpacing,
            PM_LayoutLeftMargin,
            PM_LayoutTopMargin,
            PM_LayoutRightMargin,
            PM_LayoutBottomMargin,
            PM_LayoutHorizontalSpacing,
            PM_LayoutVerticalSpacing,
            PM_TabBar_ScrollButtonOverlap,

            PM_TextCursorWidth,

            PM_TabCloseIndicatorWidth,
            PM_TabCloseIndicatorHeight,

            PM_ScrollView_ScrollBarSpacing,
            PM_SubMenuOverlap,

            // do not add any values below/greater than this
            PM_CustomBase = 0xf0000000,

            // The following are custom values needed to draw the S60Style according scalable UIs.
            // Width of 9-part frame-corner
            PM_Custom_FrameCornerWidth,
            // Height of 9-part frame corner
            PM_Custom_FrameCornerHeight,
            // Bold line width
            PM_Custom_BoldLineWidth,
            // Thin line width
            PM_Custom_ThinLineWidth,
            // Height of a popup info messagebox
            PM_Custom_MessageBoxHeight
        };

    };
#else
#pragma message ("Building in non-supported environment, this probably fails")
#endif


// Pixel metrics version information.
class TPixelMetricsVersion
    {
    public:
        TInt majorVersion;
        TInt minorVersion;
    };

NONSHARABLE_CLASS(PixelMetrics)
{
    public:
        static TPixelMetricsVersion Version();
        static TInt PixelMetricValue(QStyle::PixelMetric);

    private:
        static TInt PixelMetricMenuValue( QStyle::PixelMetric menuValue, TRect mainPaneRect );
        static TInt PixelMetricTabValue( QStyle::PixelMetric tabValue, TRect appWindow, TBool landscape );
};

#endif // PIXELMETRICS_H
