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

#include "pixel_metrics.h"

#include <AknLayout2ScalableDef.h>
#include <AknLayoutScalable_Avkon.cdl.h>
#include <AknLayoutScalable_Apps.cdl.h>
#include <AknUtils.h>

// Version number for dynamic calculations. These are to be exported to static data,
// so that we can keep dynamic and static values inline.
// Please adjust version data if correcting dynamic PM calculations.
const TInt KPMMajorVersion = 1;
const TInt KPMMinorVersion = 19;

TPixelMetricsVersion PixelMetrics::Version()
    {
    TPixelMetricsVersion version;
    version.majorVersion = KPMMajorVersion;
    version.minorVersion = KPMMinorVersion;
    return version;
    }

TInt PixelMetrics::PixelMetricValue(QStyle::PixelMetric metric)
    {
    TInt value = -909;
    // Main pane
    TRect mainPaneRect;
    AknLayoutUtils::LayoutMetricsRect(
        AknLayoutUtils::EMainPane,
        mainPaneRect );
    // Screen
    TRect screenRect;
    AknLayoutUtils::LayoutMetricsRect(
        AknLayoutUtils::EApplicationWindow,
        screenRect );
    // Navi pane
    TRect naviPaneRect;
    AknLayoutUtils::LayoutMetricsRect(
        AknLayoutUtils::ENaviPane,
        naviPaneRect );

    TAknLayoutRect appWindow;
    appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(0) );

    TInt variety = 0;
    TBool landscape = EFalse;
    if ( screenRect.iBr.iX > screenRect.iBr.iY )
        {
        // in landscape another variety is used
        landscape = ETrue;
        }
    switch (metric)
        {
        case QStyle::PM_DockWidgetHandleExtent:
            // what's this??? Not in S60
            break;
        case QStyle::PM_CheckListControllerSize:
        case QStyle::PM_CheckListButtonSize:
            {
            // hierarchical menu - checkbox / radiobutton
            // Area (width/height) of the checkbox/radio button in a Q3CheckListItem.
            TAknLayoutRect listScrollPane;
            listScrollPane.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_gen_pane(0));
            TAknLayoutRect listGenPane;
            listGenPane.LayoutRect( listScrollPane.Rect(), AknLayoutScalable_Avkon::list_gen_pane(0));
            TAknLayoutRect listHierarchyPane;
            listHierarchyPane.LayoutRect( listGenPane.Rect(), AknLayoutScalable_Avkon::list_single_graphic_hl_pane(0));

            TAknLayoutRect listHierarchyControllerPane;
            listHierarchyPane.LayoutRect( listHierarchyPane.Rect(), AknLayoutScalable_Avkon::list_single_graphic_hl_pane_g3(0));
            TAknLayoutRect listHierarchyPropertyPane;
            listHierarchyPropertyPane.LayoutRect( listHierarchyPane.Rect(), AknLayoutScalable_Avkon::list_single_graphic_hl_pane_g2(0));

            if (metric==QStyle::PM_CheckListControllerSize)value = Max( listHierarchyPane.Rect().Width(), listHierarchyPane.Rect().Width());
            else value = Max( listHierarchyPropertyPane.Rect().Width(), listHierarchyPropertyPane.Rect().Width());
            }
            break;
        case QStyle::PM_DialogButtonsSeparator:   //Distance between buttons in a dialog buttons widget.
        case QStyle::PM_DialogButtonsButtonWidth: // Minimum width of a button in a dialog buttons widget.
        case QStyle::PM_DialogButtonsButtonHeight:// Minimum height of a button in a dialog buttons widget.
            {
            TAknLayoutRect appWindow;
            appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(0) );
            variety = 0;
            if ( landscape )
                {
                variety = 2;
                }
            TAknLayoutRect areaBottomRect;
            areaBottomRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_bottom_pane(variety) );

            TAknLayoutRect controlPaneRect;
            controlPaneRect.LayoutRect( areaBottomRect.Rect(), AknLayoutScalable_Avkon::control_pane() );
            TAknLayoutText controlPaneLSKText;
            TAknLayoutText controlPaneRSKText;
            TAknLayoutText controlPaneMSKText;
            variety = 0;
            if (AknLayoutUtils::MSKEnabled())
                {
                variety = 3;
                controlPaneMSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t3(variety)); //MSK text area
                }
            controlPaneLSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t1(variety)); //LSK text area
            controlPaneRSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t2(variety)); //RSK text area

            /*
             *
            ==================================================================================
            |  A  |     LSK_rect     |  B  |     MSK_rect     |  C  |     RSK_rect     |  D  |
            ==================================================================================
            where A is left padding (between control pane and LSK rect)
                  B is mid-left padding (between LSK and MSK rects)
                  C is mid-right padding (between MSK and RSK rects)
                  D is right padding (between RSK and control pane)

                  ==> Since all these can be separate, lets take Max of {A..D} for PM value
            */

            TInt itemSpacingA = 0;
            TInt itemSpacingB = 0;
            TInt itemSpacingC = 0;
            TInt itemSpacingMax = 0;
            if ( !AknLayoutUtils::MSKEnabled() )
                {
                itemSpacingA = controlPaneRect.Rect().iBr.iX - controlPaneRSKText.TextRect().iBr.iX;
                itemSpacingB = controlPaneLSKText.TextRect().iTl.iX - controlPaneRect.Rect().iTl.iX;
                if (!landscape)
                    {
                    // use mid-gap only in portrait
                    itemSpacingC = controlPaneRSKText.TextRect().iTl.iX - controlPaneLSKText.TextRect().iBr.iX;
                    }
                itemSpacingMax = Max(itemSpacingA, Max( itemSpacingB, itemSpacingC));
                // no itemspacing4 if no MSK
                }
            else
                {
                TInt itemSpacingD = 0;
                itemSpacingA = controlPaneRect.Rect().iBr.iX - controlPaneRSKText.TextRect().iBr.iX;
                itemSpacingB = controlPaneLSKText.TextRect().iTl.iX - controlPaneRect.Rect().iTl.iX;
                if ( !(AknLayoutUtils::PenEnabled() || landscape) ) // no MSK in touch, nor in landscape
                    {
                    itemSpacingC = controlPaneRSKText.TextRect().iTl.iX - controlPaneMSKText.TextRect().iBr.iX;
                    itemSpacingD = controlPaneMSKText.TextRect().iTl.iX - controlPaneLSKText.TextRect().iBr.iX;
                    }
                itemSpacingMax = Max(itemSpacingA, Max( itemSpacingB, Max( itemSpacingC, itemSpacingD )));
                }
            if (metric==QStyle::PM_DialogButtonsSeparator) value = itemSpacingMax;
            else if (metric==QStyle::PM_DialogButtonsButtonWidth)
                {
                value = Max( controlPaneLSKText.TextRect().Width(), controlPaneRSKText.TextRect().Width());
                if (AknLayoutUtils::MSKEnabled())
                    {
                    value = Max(value, controlPaneMSKText.TextRect().Width());
                    }
                }
            else if (metric==QStyle::PM_DialogButtonsButtonHeight)
                {
                value = Max( controlPaneLSKText.TextRect().Height(), controlPaneRSKText.TextRect().Height());
                if (AknLayoutUtils::MSKEnabled())
                    {
                    value = Max(value, controlPaneMSKText.TextRect().Height());
                    }
                }
            }
            break;
        case QStyle::PM_DockWidgetTitleMargin: // not in S60, lets use the same margin as in button
        case QStyle::PM_DockWidgetTitleBarButtonMargin: // not in S60, lets use the same margin as in button
        case QStyle::PM_ButtonMargin:
            {
            TRect myRect(TSize( 80, 20)); // this arbitrary size - user can set it - button border does not seem to have any scalability in it
            TAknLayoutRect buttonRect;
            TAknLayoutRect buttonBordersRect;
            TAknLayoutText buttonText;

            buttonRect.LayoutRect( myRect, AknLayoutScalable_Avkon::eswt_ctrl_button_pane());
            buttonBordersRect.LayoutRect( buttonRect.Rect(), AknLayoutScalable_Avkon::common_borders_pane_copy2(0)); //with text
            buttonText.LayoutText( buttonRect.Rect(), AknLayoutScalable_Avkon::control_button_pane_t1() );

            // Its better to use left-right margins, since font deployment can create funny top / bottom margins
            TInt leftMargin = buttonText.TextRect().iTl.iX - buttonBordersRect.Rect().iTl.iX;
            TInt rightMargin = buttonBordersRect.Rect().iBr.iX - buttonText.TextRect().iBr.iX;
            value = (TInt) ((leftMargin+rightMargin)/2);
            }
            break;
        case QStyle::PM_ButtonDefaultIndicator:
            {
            // no default button indicators in S60
            value = 0;
            }
            break;
        case QStyle::PM_MdiSubWindowFrameWidth:
        case QStyle::PM_ComboBoxFrameWidth:
        case QStyle::PM_SpinBoxFrameWidth:
            value = 0;
            break;
        case QStyle::PM_ToolBarFrameWidth:
        case QStyle::PM_DefaultFrameWidth:
            {
            TAknLayoutRect highLightPaneRect;
            TAknLayoutRect centerPaneRect;
            TRect rectParent( mainPaneRect );
            highLightPaneRect.LayoutRect( rectParent, AknLayoutScalable_Avkon::toolbar_button_pane(0).LayoutLine());
            centerPaneRect.LayoutRect(rectParent, AknLayoutScalable_Avkon::toolbar_button_pane_g1().LayoutLine());

            value = highLightPaneRect.Rect().iBr.iX - centerPaneRect.Rect().iBr.iX;
            }
            break;
        case QStyle::PM_RadioButtonLabelSpacing:
            {
            /*
             *
            ===================================================================================
            | A  | iconLayoutRect |B|                itemText                             | C |
            ===================================================================================
            mirrored:
            ===================================================================================
            | C |                      itemText                      |B| iconLayoutRect |  A  |
            ===================================================================================
            where A is left padding
                  B is gap between icon and text
                  C is right padding
            */

            TRect rectParent( mainPaneRect );
            TAknLayoutRect layoutRect;
            layoutRect.LayoutRect( rectParent,AknLayoutScalable_Avkon::list_choice_list_pane(1).LayoutLine() ); // w/ scrollbar
            TAknLayoutText itemText;
            itemText.LayoutText( layoutRect.Rect(), AknLayoutScalable_Avkon::list_single_choice_list_pane_t1(1) );
            TAknLayoutRect iconLayoutRect;
            iconLayoutRect.LayoutRect( layoutRect.Rect(), AknLayoutScalable_Avkon::list_single_choice_list_pane_g1().LayoutLine() );

            if ( !AknLayoutUtils::LayoutMirrored() )
                {
                value = itemText.TextRect().iTl.iX - iconLayoutRect.Rect().iBr.iX;
                }
            else
                {
                value = iconLayoutRect.Rect().iTl.iX - itemText.TextRect().iBr.iX;
                }
            }

            break;
        case QStyle::PM_CheckBoxLabelSpacing:
            {
            /*
             *
            ===================================================================================
            | A  | iconLayoutRect |B|                itemText                             | C |
            ===================================================================================
            mirrored:
            ===================================================================================
            | C |                      itemText                      |B| iconLayoutRect |  A  |
            ===================================================================================
            where A is left padding
                  B is gap between icon and text
                  C is right padding
            */

            TRect rectParent( mainPaneRect );
            TAknLayoutRect layoutRect;
            layoutRect.LayoutRect( rectParent, AknLayoutScalable_Avkon::listscroll_gen_pane(0).LayoutLine() );

            TAknLayoutRect layoutRect2;
            layoutRect2.LayoutRect( layoutRect.Rect(), AknLayoutScalable_Avkon::list_gen_pane(0).LayoutLine() );
            TAknLayoutRect layoutRect3;
            layoutRect3.LayoutRect( layoutRect2.Rect(), AknLayoutScalable_Avkon::list_single_graphic_pane(0).LayoutLine() );

            TAknLayoutText itemText;
            itemText.LayoutText( layoutRect3.Rect(), AknLayoutScalable_Avkon::list_single_graphic_pane_t1(0) );
            TAknLayoutRect iconLayoutRect;
            iconLayoutRect.LayoutRect( layoutRect3.Rect(), AknLayoutScalable_Avkon::list_single_graphic_pane_g1(0).LayoutLine() );

            if ( !AknLayoutUtils::LayoutMirrored() )
                {
                value = itemText.TextRect().iTl.iX - iconLayoutRect.Rect().iBr.iX;
                }
            else
                {
                value = iconLayoutRect.Rect().iTl.iX - itemText.TextRect().iBr.iX;
                }
            }
            break;
        case QStyle::PM_ToolTipLabelFrameWidth:
            {
            /*
             *
            |===================================================================================|
            |   info popup note                        B                                        |
            |   ==============================================================================  |
            | A |                                   hintText                                 | D|
            |   ==============================================================================  |
            |                                          C                                        |
            |===================================================================================|
            where A is left padding
                  B is top padding
                  C is bottom padding
                  D is right padding
                  we'll provide the average of top and bottom padding as PM_ToolTipLabelFrameWidth
            */

            // Set pop-up to contain only one line of text
            TInt index = 0;
            if ( landscape )
                {
                // in landscape another variety is used
                index += 5;
                }
            // Get parameter and table limits for popup preview text window
            TAknLayoutScalableParameterLimits limits =
                AknLayoutScalable_Avkon::popup_preview_text_window_ParamLimits();

            TAknLayoutScalableTableLimits tableLimits =
                AknLayoutScalable_Avkon::popup_preview_text_window_t_Limits();

            TInt windowVariety = Min( Max( index, limits.FirstVariety() ), limits.LastVariety() );

            TAknLayoutScalableParameterLimits tParamLimits =
                    AknLayoutScalable_Avkon:: popup_preview_text_window_t_ParamLimits(
                             tableLimits.FirstIndex() );

            TInt lineVariety = Min( Max( index, tParamLimits.FirstVariety() ), tParamLimits.LastVariety() );

            TAknWindowLineLayout lineLayout = AknLayoutScalable_Avkon::popup_preview_text_window(windowVariety).LayoutLine();

            // rect for the whole info popup
            TAknLayoutRect layoutRect;
            layoutRect.LayoutRect(screenRect, lineLayout);
            TRect rectPopupWindow = layoutRect.Rect();

            TAknTextComponentLayout popupTextLayout =
                AknLayoutScalable_Avkon::popup_preview_text_window_t(
                    tableLimits.FirstIndex(), lineVariety );

            // rect for the whole the text inside the popup
            TAknLayoutText layoutText;
            layoutText.LayoutText( rectPopupWindow, popupTextLayout );

            // Each margin has different value in S60 - let's take average of top & bottom
            TInt topMargin = layoutText.TextRect().iTl.iY - layoutRect.Rect().iTl.iY;
            TInt bottomMargin = layoutRect.Rect().iBr.iY - layoutText.TextRect().iBr.iY;
            TInt averageMargin = (TInt)(topMargin+bottomMargin)/2;
            value = averageMargin;
            }
            break;
        case QStyle::PM_ListViewIconSize:
            {
            // todo: there are lots and lots of views with different sized icons - which one to use?
            // todo: this is probably not a good default icon size, as this fetches A column icon size
            // todo: preferably use settings item with graphic instead
            TAknLayoutRect iconRect;
            iconRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::list_double_graphic_pane_g1_cp2(0).LayoutLine());
            //icon areas are usually squares - lets take bigger of two dimensions
            value = Max( iconRect.Rect().Width(), iconRect.Rect().Height() );
            }
            break;

        case QStyle::PM_LargeIconSize: // lets use AS icon as a base for large icon
        case QStyle::PM_IconViewIconSize:
            {
            // Lets assume that we'd take these from grid (3x4) layout
            TAknLayoutRect appPaneRect;
            TAknLayoutRect gridAppRect;
            TAknLayoutRect cellAppRect;
            TInt varietyGrid = 2; //Let's use the 3x4 grid as a base.
            TInt varietyCell = 1; //Let's use the 3x4 grid as a base.
            if ( landscape )
                {
                varietyGrid = 3;
                varietyCell = 2;
                }
            appPaneRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_app_pane(1).LayoutLine()); //3x4 grid
            gridAppRect.LayoutRect( appPaneRect.Rect(), AknLayoutScalable_Avkon::grid_app_pane(varietyGrid).LayoutLine());
            cellAppRect.LayoutRect( gridAppRect.Rect(), AknLayoutScalable_Avkon::cell_app_pane(varietyCell, 0, 0).LayoutLine());
            TAknLayoutRect cellGraphRect;
            TAknWindowComponentLayout appIcon = AknLayoutScalable_Avkon::cell_app_pane_g1(0); // no mark, no highlight
            cellGraphRect.LayoutRect( gridAppRect.Rect(), appIcon);
            //icon areas are usually squares - if not, lets take larger
            value = Max( cellGraphRect.Rect().Width(), cellGraphRect.Rect().Height());
            }
            break;
        case QStyle::PM_TabBarIconSize:
            {
            TAknLayoutRect naviNaviRect;
            naviNaviRect.LayoutRect( naviPaneRect, AknLayoutScalable_Avkon::navi_navi_tabs_pane().LayoutLine()); // two tabs
            TAknLayoutRect tabRect;
            tabRect.LayoutRect( naviNaviRect.Rect(), AknLayoutScalable_Avkon::navi_tabs_3_pane().LayoutLine()); //active tab on left
            TAknLayoutRect activeTabRect;
            activeTabRect.LayoutRect( tabRect.Rect(), AknLayoutScalable_Avkon::tabs_3_active_pane(0).LayoutLine()); //active tab
            TAknLayoutRect activeTabGraphicRect;

            activeTabGraphicRect.LayoutRect( activeTabRect.Rect(), AknLayoutScalable_Avkon::tabs_3_active_pane_g1().LayoutLine()); //active tab graphic
            value = Min(activeTabGraphicRect.Rect().Width(), activeTabGraphicRect.Rect().Height());
            }
            break;
        case QStyle::PM_MessageBoxIconSize:
            {
            TAknLayoutRect noteRect;
            noteRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_note_image_window(0).LayoutLine()); //note with image
            TAknLayoutRect noteImageRect;
            noteImageRect.LayoutRect( noteRect.Rect(), AknLayoutScalable_Avkon::popup_note_image_window_g2(2).LayoutLine()); //note with image
            value = noteImageRect.Rect().Width();
            }
            break;
        case QStyle::PM_TextCursorWidth:
            {
            TAknLayoutRect miscGraphicsRect;
            miscGraphicsRect.LayoutRect( screenRect, AknLayoutScalable_Avkon::misc_graphics());
            miscGraphicsRect.LayoutRect( miscGraphicsRect.Rect(), AknLayoutScalable_Avkon::misc_graphics());
            TAknLayoutRect textsGraphicsRect;
            textsGraphicsRect.LayoutRect( miscGraphicsRect.Rect(), AknLayoutScalable_Avkon::texts_graphics());
            TAknLayoutRect cursorGraphicsRect;
            cursorGraphicsRect.LayoutRect( textsGraphicsRect.Rect(), AknLayoutScalable_Avkon::cursor_graphics_pane());
            TAknLayoutRect cursorPrimaryRect;
            cursorPrimaryRect.LayoutRect( cursorGraphicsRect.Rect(), AknLayoutScalable_Avkon::cursor_primary_pane());
            TAknLayoutRect cursorRect;
            cursorRect.LayoutRect( cursorPrimaryRect.Rect(), AknLayoutScalable_Avkon::cursor_digital_pane_g1());
            value = cursorRect.Rect().Width();
            }
            break;
        case QStyle::PM_SliderLength:
            {
            TAknLayoutRect settingRect;
            settingRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_set_pane() );
            TAknLayoutRect settingContentRect;
            settingContentRect.LayoutRect( settingRect.Rect(), AknLayoutScalable_Avkon::set_content_pane() );
            TAknLayoutRect sliderRect;
            sliderRect.LayoutRect( settingContentRect.Rect(), AknLayoutScalable_Avkon::setting_slider_graphic_pane() );
            TAknLayoutRect sliderSettingRect;
            sliderSettingRect.LayoutRect( sliderRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_cp() );
            TAknLayoutRect sliderGraph2Rect;
            sliderGraph2Rect.LayoutRect( sliderSettingRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_g6() );
            value = sliderGraph2Rect.Rect().Width();
            }
            break;
        case QStyle::PM_SliderThickness:
            {
            TAknLayoutRect settingRect;
            settingRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_set_pane() );
            TAknLayoutRect settingContentRect;
            settingContentRect.LayoutRect( settingRect.Rect(), AknLayoutScalable_Avkon::set_content_pane() );
            TAknLayoutRect sliderRect;
            sliderRect.LayoutRect( settingContentRect.Rect(), AknLayoutScalable_Avkon::setting_slider_graphic_pane() );
            TAknLayoutRect sliderSettingRect;
            sliderSettingRect.LayoutRect( sliderRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_cp() );
            TAknLayoutRect sliderGraph2Rect;
            sliderGraph2Rect.LayoutRect( sliderSettingRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_g6() );
            //todo: make a proper calculation for tick marks
            value = (TInt)(sliderGraph2Rect.Rect().Height()*1.5); // add assumed tickmark height
            }
            break;
        case QStyle::PM_SliderTickmarkOffset:
            {
            TAknLayoutRect settingRect;
            settingRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_set_pane() );
            TAknLayoutRect settingContentRect;
            settingContentRect.LayoutRect( settingRect.Rect(), AknLayoutScalable_Avkon::set_content_pane() );
            TAknLayoutRect sliderRect;
            sliderRect.LayoutRect( settingContentRect.Rect(), AknLayoutScalable_Avkon::setting_slider_graphic_pane() );
            TAknLayoutRect sliderSettingRect;
            sliderSettingRect.LayoutRect( sliderRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_cp() );
            TAknLayoutRect sliderGraph2Rect;
            sliderGraph2Rect.LayoutRect( sliderSettingRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_g6() );
            //todo: make a proper calculation for tick marks
            value = (TInt)(sliderGraph2Rect.Rect().Height()*0.5); // no tickmarks in S60, lets assume they are half the size of slider indicator
            }
            break;
        case QStyle::PM_SliderControlThickness:
            {
            TAknLayoutRect settingRect;
            settingRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_set_pane() );
            TAknLayoutRect settingContentRect;
            settingContentRect.LayoutRect( settingRect.Rect(), AknLayoutScalable_Avkon::set_content_pane() );
            TAknLayoutRect sliderRect;
            sliderRect.LayoutRect( settingContentRect.Rect(), AknLayoutScalable_Avkon::setting_slider_graphic_pane() );
            TAknLayoutRect sliderSettingRect;
            sliderSettingRect.LayoutRect( sliderRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_cp() );
            TAknLayoutRect sliderGraph2Rect;
            sliderGraph2Rect.LayoutRect( sliderSettingRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_g6() );
            value = sliderGraph2Rect.Rect().Height();
            }
            break;
        case QStyle::PM_SliderSpaceAvailable:
            {
            TAknLayoutRect settingRect;
            settingRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_set_pane() );
            TAknLayoutRect settingContentRect;
            settingContentRect.LayoutRect( settingRect.Rect(), AknLayoutScalable_Avkon::set_content_pane() );
            TAknLayoutRect sliderRect;
            sliderRect.LayoutRect( settingContentRect.Rect(), AknLayoutScalable_Avkon::setting_slider_graphic_pane() );
            TAknLayoutRect sliderSettingRect;
            sliderSettingRect.LayoutRect( sliderRect.Rect(), AknLayoutScalable_Avkon::slider_set_pane_cp() );
            value = sliderSettingRect.Rect().Width();
            }
            break;
        case QStyle::PM_MenuBarItemSpacing:
            {
            TAknLayoutRect appWindow;
            appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(0) );

            variety = 0;
            if ( landscape )
                {
                variety = 2;
                }
            TAknLayoutRect areaBottomRect;
            areaBottomRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_bottom_pane(variety) );

            TAknLayoutRect controlPaneRect;
            controlPaneRect.LayoutRect( areaBottomRect.Rect(), AknLayoutScalable_Avkon::control_pane() );
            TAknLayoutText controlPaneLSKText;
            TAknLayoutText controlPaneRSKText;
            TAknLayoutText controlPaneMSKText;
            variety = 0;
            if (AknLayoutUtils::MSKEnabled())
                {
                variety = 3;
                controlPaneMSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t3(variety)); //MSK text area
                }
            controlPaneLSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t1(variety)); //LSK text area
            controlPaneRSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t2(variety)); //RSK text area

            /*
             *
            ==================================================================================
            |  A  |     LSK_rect     |  B  |     MSK_rect     |  C  |     RSK_rect     |  D  |
            ==================================================================================
            where A is left padding (between control pane and LSK rect)
                  B is mid-left padding (between LSK and MSK rects)
                  C is mid-right padding (between MSK and RSK rects)
                  D is right padding (between RSK and control pane)

                  ==> Since all these can be separate, lets take Max of {A..D} for PM value
            */

            TInt itemSpacing1 = 0;
            TInt itemSpacing2 = 0;
            TInt itemSpacing3 = 0;
            TInt itemSpacing4 = 0;
            TInt itemSpacingMax = 0;
            if ( !AknLayoutUtils::MSKEnabled() )
                {
                itemSpacing1 = controlPaneRect.Rect().iBr.iX - controlPaneRSKText.TextRect().iBr.iX;
                itemSpacing2 = controlPaneLSKText.TextRect().iTl.iX - controlPaneRect.Rect().iTl.iX;
                if ( !landscape )
                    {
                    // use mid gap only in portrait
                    itemSpacing3 = controlPaneRSKText.TextRect().iTl.iX - controlPaneLSKText.TextRect().iBr.iX;
                    }
                itemSpacingMax = Max(itemSpacing1, Max( itemSpacing2, itemSpacing3));
                // no itemspacing4 if no MSK
                }
            else
                {
                itemSpacing1 = controlPaneRect.Rect().iBr.iX - controlPaneRSKText.TextRect().iBr.iX;
                itemSpacing2 = controlPaneLSKText.TextRect().iTl.iX - controlPaneRect.Rect().iTl.iX;
                if ( !(AknLayoutUtils::PenEnabled() || landscape) ) // no MSK in touch, nor in landscape
                    {
                    itemSpacing3 = controlPaneRSKText.TextRect().iTl.iX - controlPaneMSKText.TextRect().iBr.iX;
                    itemSpacing4 = controlPaneMSKText.TextRect().iTl.iX - controlPaneLSKText.TextRect().iBr.iX;
                    }
                itemSpacingMax = Max(itemSpacing1, Max( itemSpacing2, Max( itemSpacing3, itemSpacing4 )));
                }
            value = itemSpacingMax;
            }
            break;
        case QStyle::PM_MenuBarHMargin:
            {
            TAknLayoutRect appWindow;
            appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(0) );

            variety = 0;
            if ( landscape )
                {
                variety = 6;
                }
            TAknLayoutRect areaBottomRect;
            areaBottomRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_bottom_pane(variety) );

            // variety 7 if thin status pane, 1 if no status pane, 3 if small status pane and with main pane, 4 otherwise (idle has bunch of own varieties)
            TAknLayoutRect controlPaneRect;
            controlPaneRect.LayoutRect( areaBottomRect.Rect(), AknLayoutScalable_Avkon::control_pane() );
            value = areaBottomRect.Rect().Height() - controlPaneRect.Rect().Height();
            }
            break;
        case QStyle::PM_MenuBarVMargin:
            {
            TAknLayoutRect appWindow;
            appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(0) );

            variety = 0;
            if ( landscape )
                {
                variety = 6;
                }
            TAknLayoutText controlPaneLSKText;
            TAknLayoutRect areaBottomRect;
            areaBottomRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_bottom_pane(variety) );
            // variety 7 if thin status pane, 1 if no status pane, 3 if small status pane and with main pane, 4 otherwise (idle has bunch of own varieties)
            TAknLayoutRect controlPaneRect;
            controlPaneRect.LayoutRect( areaBottomRect.Rect(), AknLayoutScalable_Avkon::control_pane() );

            variety = 0;
            if (AknLayoutUtils::MSKEnabled())
                {
                variety = 3;
                }
            controlPaneLSKText.LayoutText( controlPaneRect.Rect(), AknLayoutScalable_Avkon::control_pane_t1(variety)); //LSK text area

            value = controlPaneRect.Rect().Height() - controlPaneLSKText.TextRect().Height();
            }
            break;
        case QStyle::PM_ToolBarItemSpacing:
            {
            TAknLayoutRect popupToolBarWindow;
            variety = 4;
            popupToolBarWindow.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_toolbar_window(variety) );
            TAknLayoutRect gridToolBarRect;
            gridToolBarRect.LayoutRect( popupToolBarWindow.Rect(), AknLayoutScalable_Avkon::grid_toobar_pane() );
            TAknLayoutRect cellToolBarRect1;
            TAknLayoutRect cellToolBarRect2;
            cellToolBarRect1.LayoutRect( gridToolBarRect.Rect(), AknLayoutScalable_Avkon::cell_toolbar_pane(0).LayoutLine() ); //first item
            cellToolBarRect2.LayoutRect( gridToolBarRect.Rect(), AknLayoutScalable_Avkon::cell_toolbar_pane(1).LayoutLine() ); //second item
            value = cellToolBarRect1.Rect().iBr.iX - cellToolBarRect2.Rect().iTl.iX;
            }
            break;
        case QStyle::PM_ToolBarItemMargin:
            {
            variety = 4;
            TAknLayoutRect popupToolBarWindow;
            popupToolBarWindow.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_toolbar_window(variety) );
            TAknLayoutRect gridToolBarRect;
            gridToolBarRect.LayoutRect( popupToolBarWindow.Rect(), AknLayoutScalable_Avkon::grid_toobar_pane() );
            TAknLayoutRect cellToolBarRect1;
            cellToolBarRect1.LayoutRect( gridToolBarRect.Rect(), AknLayoutScalable_Avkon::cell_toolbar_pane(0).LayoutLine() ); //first item
            value = gridToolBarRect.Rect().iTl.iX - cellToolBarRect1.Rect().iTl.iX;
            }
            break;
        case QStyle::PM_LayoutLeftMargin: // there really isn't a default layoutting on s60, but lets use AppShell icon deployment as base
        case QStyle::PM_LayoutRightMargin:
        case QStyle::PM_LayoutTopMargin:
        case QStyle::PM_LayoutBottomMargin:
        case QStyle::PM_LayoutHorizontalSpacing:
        case QStyle::PM_LayoutVerticalSpacing:
            {
            //since spacing and margins should be globally same, lets use same easy component as base - such as find popup
            TAknLayoutRect popup_find_windowRect;
            TAknLayoutRect bg_popup_window_pane_cp12Rect;
            TAknLayoutRect find_popup_paneRect;
            popup_find_windowRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_find_window(0).LayoutLine());
            bg_popup_window_pane_cp12Rect.LayoutRect( popup_find_windowRect.Rect(), AknLayoutScalable_Avkon::bg_popup_window_pane_cp12().LayoutLine());
            find_popup_paneRect.LayoutRect( bg_popup_window_pane_cp12Rect.Rect(), AknLayoutScalable_Avkon::find_popup_pane().LayoutLine());

            const TBool mirrored = AknLayoutUtils::LayoutMirrored();
            if ((metric==QStyle::PM_LayoutVerticalSpacing && !mirrored) || metric==QStyle::PM_LayoutLeftMargin)
                {
                if (mirrored)
                    {
                    value = find_popup_paneRect.Rect().iTl.iX - bg_popup_window_pane_cp12Rect.Rect().iTl.iX;
                    }
                else
                    {
                    value = find_popup_paneRect.Rect().iTl.iX - bg_popup_window_pane_cp12Rect.Rect().iTl.iX;
                    }
                }
            else if (metric==QStyle::PM_LayoutRightMargin || (metric==QStyle::PM_LayoutVerticalSpacing && mirrored))
                {
                if (mirrored)
                    {
                    value = bg_popup_window_pane_cp12Rect.Rect().iBr.iX - find_popup_paneRect.Rect().iBr.iX;
                    }
                else
                    {
                    value = bg_popup_window_pane_cp12Rect.Rect().iBr.iX - find_popup_paneRect.Rect().iBr.iX;
                    }
                }
            else if (metric==QStyle::PM_LayoutTopMargin || metric==QStyle::PM_LayoutHorizontalSpacing)
                {
                value = find_popup_paneRect.Rect().iTl.iY - bg_popup_window_pane_cp12Rect.Rect().iTl.iY;
                }
            else if (metric==QStyle::PM_LayoutBottomMargin)
                {
                value = bg_popup_window_pane_cp12Rect.Rect().iBr.iY - find_popup_paneRect.Rect().iBr.iY;
                }
            }
            break;
        case QStyle::PM_MaximumDragDistance:
            {
            value = -1; //disable - not in S60
            }
            break;
        case QStyle::PM_SplitterWidth:
        case QStyle::PM_ScrollBarExtent:
            {
            TAknLayoutRect miscGraphicsRect;
            miscGraphicsRect.LayoutRect( screenRect, AknLayoutScalable_Avkon::misc_graphics());
            miscGraphicsRect.LayoutRect( miscGraphicsRect.Rect(), AknLayoutScalable_Avkon::misc_graphics());
            TAknLayoutRect textsGraphicsRect;
            textsGraphicsRect.LayoutRect( miscGraphicsRect.Rect(), AknLayoutScalable_Avkon::texts_graphics());
            TAknLayoutRect editorScrollRect;
            editorScrollRect.LayoutRect( textsGraphicsRect.Rect(), AknLayoutScalable_Avkon::editor_scroll_pane());
            TAknLayoutRect scrollPaneRect;
            scrollPaneRect.LayoutRect( editorScrollRect.Rect(), AknLayoutScalable_Avkon::scroll_pane_cp13());
            value = scrollPaneRect.Rect().Width(); // width of editor's scroll bar
            }
            break;
        case QStyle::PM_ScrollBarSliderMin:
            {
            TAknLayoutRect listScrollPane;
            listScrollPane.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::listscroll_gen_pane(0));
            TAknLayoutRect scrollPane;
            scrollPane.LayoutRect( listScrollPane.Rect(), AknLayoutScalable_Avkon::scroll_pane());
            TAknLayoutRect scrollHandlePane;
            scrollHandlePane.LayoutRect( scrollPane.Rect(), AknLayoutScalable_Avkon::scroll_handle_pane());
            TAknLayoutRect aidMinSizePane;
            aidMinSizePane.LayoutRect( scrollHandlePane.Rect(), AknLayoutScalable_Avkon::aid_size_min_handle()); // this gives min width size for horizontal scroll bar - same can be used for vertical height minimum
            value = aidMinSizePane.Rect().Height();
            }
            break;
        case QStyle::PM_MenuBarPanelWidth:
            {
            TAknLayoutRect appWindow;
            appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(0) );

            variety = 0;
            if ( landscape )
                {
                variety = 2;
                }
            TAknLayoutRect areaBottomRect;
            areaBottomRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_bottom_pane(variety) );

            // todo: prt: variety 7 if thin status pane, 1 if no status pane, 3 if small status pane and with main pane, 4 otherwise (idle has bunch of own varieties)
            // todo: lsc: variety 6 if thin status pane
            // todo: should stacon be considered?
            TAknLayoutRect controlPaneRect;
            controlPaneRect.LayoutRect( areaBottomRect.Rect(), AknLayoutScalable_Avkon::control_pane() );
            value = areaBottomRect.Rect().Height() - controlPaneRect.Rect().Height(); //usually zero
            }
            break;
        case QStyle::PM_ProgressBarChunkWidth:
            {
            // This is either deduced or skinned (for Java) in S60
            // Layout data does not know it. It would require parameters from the
            // actual progress dialog to be able to calc this (max. value and increment)
            // So we need to set up some values - lets take one tenth of progress dialog area:
            TAknLayoutRect appWindow;
            appWindow.LayoutRect( screenRect, AknLayoutScalable_Avkon::application_window(variety) );
            if (landscape)
                {
                variety = 6;
                }
            TAknLayoutRect popupWaitWindowRect;
            popupWaitWindowRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_note_wait_window(variety) );
            TAknLayoutRect waitbarPaneRect;
            waitbarPaneRect.LayoutRect( popupWaitWindowRect.Rect(), AknLayoutScalable_Avkon::wait_bar_pane(0) );
            TAknLayoutRect waitAnimRect;
            waitAnimRect.LayoutRect( waitbarPaneRect.Rect(), AknLayoutScalable_Avkon::wait_anim_pane() );
            value = (TInt) (waitAnimRect.Rect().Width() / 10);
            }
            break;
        case QStyle::PM_TabBarTabOverlap:
        case QStyle::PM_TabBarTabHSpace:
        case QStyle::PM_TabBarTabVSpace:
        case QStyle::PM_TabBarBaseHeight:
        case QStyle::PM_TabBarBaseOverlap:
        case QStyle::PM_TabBarScrollButtonWidth:
        case QStyle::PM_TabBarTabShiftHorizontal:
        case QStyle::PM_TabBarTabShiftVertical:
            value = PixelMetricTabValue(metric, appWindow.Rect(), landscape);
            break;
        case QStyle::PM_MenuPanelWidth:
        case QStyle::PM_MenuHMargin:
        case QStyle::PM_MenuVMargin:
            value = PixelMetricMenuValue(metric, mainPaneRect);
            break;
        case QStyle::PM_ButtonIconSize:
            {
            //lets use voice recorder icons as a base
            //Unfortunately S60 graphics don't separate button bevel graphics from the actual icon.
            //Se we have no means to query the margin from bevel border to "central icon" border.
            //So, we need to make a estimate...

            TAknLayoutRect vRMainRect;
            vRMainRect.LayoutRect( mainPaneRect, AknLayoutScalable_Apps::main_vorec_pane() );

            TAknLayoutRect vRButtonGridRect;
            vRButtonGridRect.LayoutRect( vRMainRect.Rect(), AknLayoutScalable_Apps::grid_vorec_pane() );

            TAknLayoutRect vRButtonCellRect;
            vRButtonCellRect.LayoutRect( vRButtonGridRect.Rect(), AknLayoutScalable_Apps::cell_vorec_pane(0) );

            TAknLayoutRect vRButtonCellGraphicsRect;
            vRButtonCellGraphicsRect.LayoutRect( vRButtonCellRect.Rect(), AknLayoutScalable_Apps::cell_vorec_pane_g1() );

            // 0.32 is the estimate how much the icon occupies of the button bevel area
            value = vRButtonCellGraphicsRect.Rect().Width() * 0.32;
            }
            break;
        case QStyle::PM_SmallIconSize:
            {
            // lets use AI2 icon as a base
            TAknLayoutRect idlePaneRect;
            idlePaneRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::main_idle_act2_pane() );
            TAknLayoutRect idleDataRect;
            idleDataRect.LayoutRect( idlePaneRect.Rect(), AknLayoutScalable_Avkon::popup_ai2_data_window(1) );
            TAknLayoutRect ai2GridRect;
            ai2GridRect.LayoutRect( idleDataRect.Rect(), AknLayoutScalable_Avkon::grid_ai2_button_pane() );
            TAknLayoutRect ai2MpRect;
            ai2MpRect.LayoutRect( ai2GridRect.Rect(), AknLayoutScalable_Avkon::ai2_mp_button_pane() );
            TAknLayoutRect ai2CellPaneRect;
            ai2CellPaneRect.LayoutRect( ai2MpRect.Rect(), AknLayoutScalable_Avkon::cell_ai2_button_pane(1).LayoutLine() );
            TAknLayoutRect ai2CellButtonRect;
            ai2CellButtonRect.LayoutRect( ai2CellPaneRect.Rect(), AknLayoutScalable_Avkon::cell_ai2_button_pane_g1());
            value = Min( ai2CellButtonRect.Rect().Width(), ai2CellButtonRect.Rect().Height());
            }
            break;
        case QStyle::PM_FocusFrameHMargin:
        case QStyle::PM_FocusFrameVMargin:
            {
            TAknLayoutRect listScrollPane;
            listScrollPane.LayoutRect(mainPaneRect, AknLayoutScalable_Avkon::listscroll_gen_pane(0));
            TAknLayoutRect listGenPane;
            listGenPane.LayoutRect(listScrollPane.Rect(), AknLayoutScalable_Avkon::list_gen_pane(0));
            TAknLayoutRect listSinglePane;
            listSinglePane.LayoutRect(listGenPane.Rect(), AknLayoutScalable_Avkon::list_single_pane(0));
            TAknLayoutText listSinglePaneText;
            listSinglePaneText.LayoutText(listSinglePane.Rect(), AknLayoutScalable_Avkon::list_single_pane_t1(0));
            TAknLayoutRect highlightRect;
            highlightRect.LayoutRect(listSinglePane.Rect(), AknLayoutScalable_Avkon::list_highlight_pane_cp1().LayoutLine());

            // The difference of center piece from border tell the frame width.
            if ( value == QStyle::PM_FocusFrameHMargin)
                {
                 //use topleft for horizontal as S60 uses different values for right and left borders
                 value = listSinglePaneText.TextRect().iTl.iX - highlightRect.Rect().iTl.iX;
                }
            else
                {
                 value = highlightRect.Rect().iBr.iY - listSinglePaneText.TextRect().iBr.iY;
                }
            }
            break;
        case QStyle::PM_ToolBarIconSize:
            {
            TAknLayoutRect popupToolBarWindow;
            variety = 4;
            popupToolBarWindow.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_toolbar_window(variety) );
            TAknLayoutRect gridToolBarRect;
            gridToolBarRect.LayoutRect( popupToolBarWindow.Rect(), AknLayoutScalable_Avkon::grid_toobar_pane() );
            TAknLayoutRect cellToolBarRect1;
            TAknLayoutRect cellToolBarRect2;
            cellToolBarRect1.LayoutRect( gridToolBarRect.Rect(), AknLayoutScalable_Avkon::cell_toolbar_pane(0).LayoutLine() ); //first item
            value = Min( cellToolBarRect1.Rect().Height(), cellToolBarRect1.Rect().Width() );
            }
            break;

        case QStyle::PM_TitleBarHeight: // use titlepane height
            {
            TAknLayoutRect statusPaneRect;
            TAknLayoutRect titlePane;
            TAknLayoutRect areaTopRect;
            if (landscape)
                {
                if ( AknLayoutUtils::PenEnabled() )
                    {
                    // Top area - 0 is for classic landscape (used in touch landscape as well)
                    areaTopRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_top_pane(2) );
                    // Status pane - 0 softkeys on right
                    statusPaneRect.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::stacon_top_pane() );
                    }
                else
                    {
                    // Top area - 2 is for classic landscape.
                    areaTopRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_bottom_pane(2) );
                    // Stacon top pane (default ok)
                    statusPaneRect.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::stacon_bottom_pane() );
                    }
                titlePane.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::title_pane_stacon(0) ); //softkeys on right
                }
            else
                {
                // Top area - 0 is for classic portrait
                areaTopRect.LayoutRect( appWindow.Rect(), AknLayoutScalable_Avkon::area_top_pane(0) );
                // Status pane - 0 is for classic portrait
                statusPaneRect.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::status_pane(0) );
                titlePane.LayoutRect( statusPaneRect.Rect(), AknLayoutScalable_Avkon::title_pane(0) );
                }
            value = titlePane.Rect().Height();
            }
            break;
        case QStyle::PM_IndicatorWidth:
        case QStyle::PM_IndicatorHeight:
            {
            TRect rectParent( mainPaneRect );

            TAknLayoutRect layoutRect;
            layoutRect.LayoutRect( rectParent,AknLayoutScalable_Avkon::set_content_pane().LayoutLine() );
            TAknLayoutRect layoutRect2;
            layoutRect2.LayoutRect( layoutRect.Rect(),AknLayoutScalable_Avkon::list_set_graphic_pane(0).LayoutLine() );

            TAknLayoutRect iconLayoutRect;
            iconLayoutRect.LayoutRect( layoutRect2.Rect(), AknLayoutScalable_Avkon::list_set_graphic_pane_g1(0).LayoutLine() );
            if (metric==QStyle::PM_IndicatorWidth)
                {
                value = iconLayoutRect.Rect().Width();
                }
            else
                {
                value = iconLayoutRect.Rect().Height();
                }
            }
            break;
        case QStyle::PM_ExclusiveIndicatorHeight:
        case QStyle::PM_ExclusiveIndicatorWidth:
            {
            TRect rectParent( mainPaneRect );
            TAknLayoutRect layoutRect;
            layoutRect.LayoutRect( rectParent,AknLayoutScalable_Avkon::list_choice_list_pane(1).LayoutLine() ); // w/ scrollbar
            TAknLayoutText itemText;
            itemText.LayoutText( layoutRect.Rect(), AknLayoutScalable_Avkon::list_single_choice_list_pane_t1(1) );
            TAknLayoutRect iconLayoutRect;
            iconLayoutRect.LayoutRect( layoutRect.Rect(), AknLayoutScalable_Avkon::list_single_choice_list_pane_g1().LayoutLine() );

            if (metric==QStyle::PM_ExclusiveIndicatorHeight)
                {
                value = iconLayoutRect.Rect().Height();
                }
            else
                {
                value = iconLayoutRect.Rect().Width();
                }
            }
            break;

        // These are obsolete.
        case QStyle::PM_DefaultTopLevelMargin:
        case QStyle::PM_DefaultChildMargin:
        case QStyle::PM_DefaultLayoutSpacing:
            break;

        case QStyle::PM_Custom_FrameCornerWidth:
            {
            TAknLayoutRect inputFocusRect;
            inputFocusRect.LayoutRect(mainPaneRect, AknLayoutScalable_Avkon::input_focus_pane(0));
            TAknLayoutRect inputFocusInnerRect;
            inputFocusInnerRect.LayoutRect(mainPaneRect, AknLayoutScalable_Avkon::input_focus_pane_g1());

            value = inputFocusRect.Rect().iBr.iX - inputFocusInnerRect.Rect().iBr.iX;
            value+= 2; //visually better value for generic cases
            }
            break;
        case QStyle::PM_Custom_FrameCornerHeight:
            {
            TAknLayoutRect inputFocusRect;
            inputFocusRect.LayoutRect(mainPaneRect, AknLayoutScalable_Avkon::input_focus_pane(0));
            TAknLayoutRect inputFocusInnerRect;
            inputFocusInnerRect.LayoutRect(mainPaneRect, AknLayoutScalable_Avkon::input_focus_pane_g1());
            value = inputFocusRect.Rect().iBr.iY - inputFocusInnerRect.Rect().iBr.iY;
            value+= 2; //visually better value for generic cases
            }
            break;
        case QStyle::PM_Custom_BoldLineWidth:
            value = 3;
            break;
        case QStyle::PM_Custom_ThinLineWidth:
            value = 1;
            break;
        case QStyle::PM_Custom_MessageBoxHeight:
            {
            TAknLayoutRect popupRect;
            popupRect.LayoutRect(mainPaneRect, AknLayoutScalable_Avkon::popup_window_general(0));
            value = popupRect.Rect().Height();
            }
            break;
        case QStyle::PM_ButtonShiftHorizontal:
        case QStyle::PM_ButtonShiftVertical:
            value = 0;
            break;

        case QStyle::PM_ToolBarExtensionExtent:
            value = PixelMetricTabValue(QStyle::PM_TabBarScrollButtonWidth, appWindow.Rect(), landscape);
            break;

        case QStyle::PM_MenuScrollerHeight:
            {
            TRect rectParent( mainPaneRect );
            TAknLayoutRect listWidthScrollBarsRect;
            listWidthScrollBarsRect.LayoutRect( rectParent, AknLayoutScalable_Avkon::listscroll_gen_pane(0).LayoutLine() );

            TAknLayoutRect listWidgetRect;
            listWidgetRect.LayoutRect( listWidthScrollBarsRect.Rect(), AknLayoutScalable_Avkon::list_gen_pane(0).LayoutLine() );
            TAknLayoutRect singleLineListWidgetRect;
            singleLineListWidgetRect.LayoutRect( listWidgetRect.Rect(), AknLayoutScalable_Avkon::list_single_pane(0).LayoutLine() );

            TAknLayoutRect listHighlightRect;
            listHighlightRect.LayoutRect( singleLineListWidgetRect.Rect(), AknLayoutScalable_Avkon::list_highlight_pane_cp1(0).LayoutLine() );

            value = listHighlightRect.Rect().Height();
            }
            break;

// todo: re-check if these really are not available in s60
        case QStyle::PM_MenuDesktopFrameWidth:    // not needed in S60 - dislocates Menu both horizontally and vertically
        case QStyle::PM_HeaderMarkSize:           // The size of the sort indicator in a header. Not in S60
        case QStyle::PM_SpinBoxSliderHeight:       // The height of the optional spin box slider. Not in S60
        case QStyle::PM_HeaderMargin: // not in S60
        case QStyle::PM_MenuTearoffHeight: // not in S60
        case QStyle::PM_DockWidgetFrameWidth: // not in S60
        case QStyle::PM_DockWidgetSeparatorExtent: // not in S60
        case QStyle::PM_MdiSubWindowMinimizedWidth: //no such thing in S60
        case QStyle::PM_HeaderGripMargin: // not in S60
        case QStyle::PM_ToolBarSeparatorExtent: // not in S60
        case QStyle::PM_ToolBarHandleExtent: // not in s60
        case QStyle::PM_MenuButtonIndicator: // none???
        case QStyle::PM_TabBar_ScrollButtonOverlap: // not used in S60 - tab arrows are on left and right side of tab group - not together
        case QStyle::PM_SizeGripSize: // use default
        case QStyle::PM_TabCloseIndicatorWidth:
        case QStyle::PM_TabCloseIndicatorHeight:
        case QStyle::PM_ScrollView_ScrollBarSpacing:
        case QStyle::PM_SubMenuOverlap:
        default:
            break;
        }
    return value;
    }

TInt PixelMetrics::PixelMetricTabValue(QStyle::PixelMetric tabMetric, TRect appWindow, TBool landscape)
    {
    TInt tabValue = 0;
    // common ones
    TAknLayoutRect mainAreaRect;
    TAknLayoutRect rightIndicationRect;
    TAknLayoutRect leftIndicationRect;
    TAknLayoutRect activeTabRect;
    TAknLayoutText activeTabTextRect;
    TAknLayoutRect passiveTabRect;
    TAknLayoutText passiveTabTextRect;
    TAknLayoutRect tabsPaneRect;
    if ( landscape )
        {
        TAknLayoutRect statusPaneRect;
        TAknLayoutRect areaTopRect;
        if ( AknLayoutUtils::PenEnabled() )
            {
            // Top area - 0 is for classic landscape (used in touch landscape as well)
            areaTopRect.LayoutRect( appWindow, AknLayoutScalable_Avkon::area_top_pane(2) );
            // Status pane - 0 softkeys on right
            statusPaneRect.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::stacon_top_pane() );
            }
        else
            {
            // Top area - 2 is for classic landscape.
            areaTopRect.LayoutRect( appWindow, AknLayoutScalable_Avkon::area_bottom_pane(2) );
            // Stacon top pane (default ok)
            statusPaneRect.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::stacon_bottom_pane() );
            }
        // main pane for landscape
        mainAreaRect.LayoutRect( appWindow, AknLayoutScalable_Avkon::main_pane(4) );

        // navi pane
        TAknLayoutRect naviPaneRect;
        naviPaneRect.LayoutRect( statusPaneRect.Rect(), AknLayoutScalable_Avkon::navi_pane_stacon(0) ); // softkeys on right
        // navi-navi pane
        tabsPaneRect.LayoutRect( naviPaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_pane_stacon(0) ); // softkeys on right
        // Passive tab item - lets use layout where active is on left side of passive
        passiveTabRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::tabs_3_passive_pane(0) );
        // Active tab item
        activeTabRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::tabs_3_active_pane(0) );
        // Left indication
        leftIndicationRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_pane_g1(0) );
        // Right indication
        rightIndicationRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_pane_g2(0) );
        // active tab text rect
        activeTabTextRect.LayoutText( activeTabRect.Rect(), AknLayoutScalable_Avkon::tabs_3_active_pane_t1(1) );
        // passive tab text rect
        passiveTabTextRect.LayoutText( passiveTabRect.Rect(), AknLayoutScalable_Avkon::tabs_3_passive_pane_t1(1) );
        }
    else
        {
        // main pane for portait
        mainAreaRect.LayoutRect( appWindow, AknLayoutScalable_Avkon::main_pane(3) );
        // Top area - 0 is for classic portrait
        TAknLayoutRect areaTopRect;
        areaTopRect.LayoutRect( appWindow, AknLayoutScalable_Avkon::area_top_pane(0) );
        // Status pane - 0 is for classic portrait
        TAknLayoutRect statusPaneRect;
        statusPaneRect.LayoutRect( areaTopRect.Rect(), AknLayoutScalable_Avkon::status_pane(0) );

        // Navi pane
        TAknLayoutRect naviPaneRect;
        naviPaneRect.LayoutRect( statusPaneRect.Rect(), AknLayoutScalable_Avkon::navi_pane(0) );
        // Navi-navi pane for tabs (0)
        TAknLayoutRect navi2PaneRect;
        navi2PaneRect.LayoutRect( naviPaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_pane() );
        // Short tab pane
        tabsPaneRect.LayoutRect( navi2PaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_tabs_pane() );
        // Tab pane for 2 items
        TAknLayoutRect tab2PaneRect;
        tab2PaneRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::navi_tabs_3_pane() );
        // Passive tab item - lets use layout where active is on left side of passive
        passiveTabRect.LayoutRect( tab2PaneRect.Rect(), AknLayoutScalable_Avkon::tabs_3_passive_pane(0) );
        // Active tab item
        activeTabRect.LayoutRect( tab2PaneRect.Rect(), AknLayoutScalable_Avkon::tabs_3_active_pane(0) );
        // Left indication
        leftIndicationRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_pane_g1(0) );
        // Right indication
        rightIndicationRect.LayoutRect( tabsPaneRect.Rect(), AknLayoutScalable_Avkon::navi_navi_pane_g2(0) );
        // active tab text rect
        activeTabTextRect.LayoutText( activeTabRect.Rect(), AknLayoutScalable_Avkon::tabs_3_active_pane_t1(0) );
        // passive tab text rect
        passiveTabTextRect.LayoutText( passiveTabRect.Rect(), AknLayoutScalable_Avkon::tabs_3_passive_pane_t1(0) );
        }

    // active tab on left, passive on rightside
    TInt tabOverlap = activeTabRect.Rect().iBr.iX - passiveTabRect.Rect().iTl.iX;
    TInt tabHSpace = (TInt) ((activeTabTextRect.TextRect().iTl.iX - activeTabRect.Rect().iTl.iX + activeTabRect.Rect().iBr.iX - activeTabTextRect.TextRect().iBr.iX)/2);
    TInt tabVSpace = (TInt) ((activeTabTextRect.TextRect().iTl.iY - activeTabRect.Rect().iTl.iY + activeTabRect.Rect().iBr.iY - activeTabTextRect.TextRect().iBr.iY)/2);
    TInt tabBaseHeight = 0;
    if ( landscape && !AknLayoutUtils::PenEnabled())
        {
        // In landscape tab is below mainpane
        tabBaseHeight = mainAreaRect.Rect().iBr.iY - tabsPaneRect.Rect().iTl.iY;
        }
    else
        {
        // In portrait (and in landscape touch) tab is above mainpane
        tabBaseHeight = tabsPaneRect.Rect().iBr.iY - mainAreaRect.Rect().iTl.iY;
        }
    TInt tabBaseOverlap = 0;
    if ( landscape && !AknLayoutUtils::PenEnabled())
        {
        // In landscape tab is below mainpane
        tabBaseOverlap = Max( 0, mainAreaRect.Rect().iBr.iY - tabsPaneRect.Rect().iTl.iY);
        }
    else
        {
        // In portrait tab is above mainpane
        tabBaseOverlap = Max( 0, mainAreaRect.Rect().iTl.iY - tabsPaneRect.Rect().iBr.iY);
        }
    TInt tabButtonWidth = Max(leftIndicationRect.Rect().Width(), rightIndicationRect.Rect().Width());
    TInt tabVShift = Max( Abs(activeTabTextRect.TextRect().iBr.iY - passiveTabTextRect.TextRect().iBr.iY), Abs(activeTabTextRect.TextRect().iTl.iY - passiveTabTextRect.TextRect().iTl.iY) );
    TInt tabHShift = Max( Abs(activeTabTextRect.TextRect().iBr.iX - passiveTabTextRect.TextRect().iBr.iX), Abs(activeTabTextRect.TextRect().iTl.iX - passiveTabTextRect.TextRect().iTl.iX) );
    tabHShift -= (passiveTabRect.Rect().Width() - tabOverlap); // remove tab change and add overlapping area

    switch( tabMetric )
        {
        case QStyle::PM_TabBarTabOverlap:
            tabValue = tabOverlap;
            break;
        case QStyle::PM_TabBarTabHSpace:
            tabValue = tabHSpace;
            break;
        case QStyle::PM_TabBarTabVSpace:
            tabValue = tabVSpace;
            break;
        case QStyle::PM_TabBarBaseHeight:
            tabValue = tabBaseHeight;
            break;
        case QStyle::PM_TabBarBaseOverlap:
            tabValue = tabBaseOverlap;
            break;
        case QStyle::PM_TabBarScrollButtonWidth:
            // Since in Qt the scroll indicator is shown within a button, we need to add button margins to this value
            {
            tabValue = tabButtonWidth + 2*PixelMetricValue(QStyle::PM_ButtonMargin);
            }
            break;
        case QStyle::PM_TabBarTabShiftHorizontal:
            tabValue = tabHShift;
            break;
        case QStyle::PM_TabBarTabShiftVertical:
            tabValue = tabVShift;
            break;
        default:
            break;
        }
    return tabValue;
    }

TInt PixelMetrics::PixelMetricMenuValue(QStyle::PixelMetric tabMetric, TRect mainPaneRect )
    {
    TInt menuValue = 0;
    TAknLayoutRect popupMenuRect;
    popupMenuRect.LayoutRect( mainPaneRect, AknLayoutScalable_Avkon::popup_menu_window(0) );
    TAknLayoutRect listScrollPaneRect;
    listScrollPaneRect.LayoutRect( popupMenuRect.Rect(), AknLayoutScalable_Avkon::listscroll_menu_pane(0) );
    TAknLayoutRect listMenuPaneRect;
    listMenuPaneRect.LayoutRect( listScrollPaneRect.Rect(), AknLayoutScalable_Avkon::list_menu_pane(0) );
    TAknLayoutRect listMenuRow1Rect;
    listMenuRow1Rect.LayoutRect( listScrollPaneRect.Rect(), AknLayoutScalable_Avkon::list_single_pane_cp2(0));

    switch (tabMetric)
        {
        case QStyle::PM_MenuPanelWidth:
            menuValue = listMenuPaneRect.Rect().iTl.iX - listScrollPaneRect.Rect().iTl.iX;
            if ( AknLayoutUtils::LayoutMirrored() )
                {
                menuValue = listScrollPaneRect.Rect().iBr.iX - listMenuPaneRect.Rect().iBr.iX;
                }
            break;
        case QStyle::PM_MenuHMargin:
            menuValue = listMenuRow1Rect.Rect().iTl.iX - popupMenuRect.Rect().iTl.iX;
            if ( AknLayoutUtils::LayoutMirrored() )
                {
                menuValue = popupMenuRect.Rect().iBr.iX - listMenuRow1Rect.Rect().iBr.iX;
                }
            break;
        case QStyle::PM_MenuVMargin:
            menuValue = listMenuRow1Rect.Rect().iTl.iY - popupMenuRect.Rect().iTl.iY;
            break;
        default:
            break;
        }
    return menuValue;
    }
