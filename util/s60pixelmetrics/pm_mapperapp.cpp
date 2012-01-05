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

// INCLUDE FILES

#include <avkon.rsg>
#include <avkon.hrh>
#include "pm_mapper.hrh"
#include "pm_mapperapp.h"
#include "pm_mapperview.h"
#include <pm_mapper.rsg>

#include <BldVariant.hrh>

#include <w32std.h>
#include <apgwgnam.h>
#include <eikstart.h>
#include <eikenv.h>
#include <f32file.h>

#include <avkon.hrh>
#include <aknenv.h>

#include <aknnotedialog.h>
#include <stringloader.h>
#include <coneresloader.h>
#include <aknglobalnote.h>

#include <CentralRepository.h>

#include <Aknsutils.h>
#include <AknUtils.h>
#include "pixel_metrics.h"

#include <avkon.mbg>

#include <AknLayoutConfig.h>
#include <aknsgcc.h>

typedef TBuf<2048> TMySmallBuffer;
typedef TBuf<8192> TMyBigBuffer;

_LIT(KLayoutSourceFileAndPath, "\\private\\2002121f\\pm_layout.cpp");
_LIT(KPixelMetricsDataFiles, "\\private\\2002121f\\*.txt");
_LIT(KOpenBrace, "{");
_LIT(KComma, ",");
_LIT(KColon, ":");
_LIT(KTab, "\t");
_LIT(KEndBraceWithCommaAndCRLF, "},\n");
_LIT(KCRLF, "\n");

// Number of header lines in layout data.
const TInt KHeaderValues = 4;

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// C++ constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CPixelMetricsMapperAppUi::CPixelMetricsMapperAppUi() : iFileOutputOn(EFalse)
    {
    }

// -----------------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------------
//
CPixelMetricsMapperAppUi::~CPixelMetricsMapperAppUi()
    {
    }

// -----------------------------------------------------------------------------
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperAppUi::ConstructL()
    {
    BaseConstructL();

    CEikonEnv& eikEnv = *CEikonEnv::Static();

    eikEnv.WsSession().ComputeMode(
        RWsSession::EPriorityControlDisabled );
    RThread().SetProcessPriority( EPriorityHigh );

    CPixelMetricsMapperView* view = new( ELeave ) CPixelMetricsMapperView;
    CleanupStack::PushL( view );
    view->ConstructL();
    CleanupStack::Pop();    // view
    AddViewL(view);    // transfer ownership to CAknViewAppUi
    iView = view;
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
TKeyResponse CPixelMetricsMapperAppUi::HandleKeyEventL(
        const TKeyEvent& /*aKeyEvent*/,
        TEventCode /*aType*/ )
    {
    return EKeyWasNotConsumed;
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperAppUi::HandleCommandL( TInt aCommand )
    {
    switch ( aCommand )
        {
        case EAknSoftkeyExit:
        case EEikCmdExit:
            Exit();
            break;
        case ECmdSwitchOutput:
            {
            HBufC* buffer = HBufC::NewLC( 100 );
            TPtr bufferPtr = buffer->Des();
            TBool last = ETrue;
            bufferPtr.Append(_L("Output switched to "));
            iFileOutputOn = !iFileOutputOn;
            if (iFileOutputOn)
                bufferPtr.Append(_L("file."));
            else
                bufferPtr.Append(_L("screen."));
            ShowL( *buffer, last );
            CleanupStack::PopAndDestroy( buffer );
            }
            break;
        case ECmdStatus:
            {
            ClearL();

            // layout
            HBufC* buffer = HBufC::NewLC( 100 );
            TPtr bufferPtr = buffer->Des();
            TBool last = ETrue;

            // Orientation
            bufferPtr.Append(_L("Orientation: "));
            bufferPtr.AppendNum((TInt)iAvkonAppUi->Orientation());
            ShowL( *buffer, last );
            bufferPtr.Zero();

            // Output
            bufferPtr.Append(_L("Output: "));
            if (iFileOutputOn) bufferPtr.Append(_L("File"));
            else bufferPtr.Append(_L("Screen"));
            ShowL( *buffer, last );
            bufferPtr.Zero();

            CAknLayoutConfig::TScreenMode localAppScreenMode = CAknSgcClient::ScreenMode();
            TInt hashValue = localAppScreenMode.ScreenStyleHash();
            TPixelsTwipsAndRotation pixels = CAknSgcClient::PixelsAndRotation();
            TSize pixelSize = pixels.iPixelSize;

            bufferPtr.Append(_L("LayoutName: "));

            if ( (pixelSize.iWidth == 320 || pixelSize.iWidth == 240 )&&
                 (pixelSize.iHeight == 320 || pixelSize.iHeight == 240 ))
                 {
                if (hashValue==0x996F7AA7)
                    bufferPtr.Append(_L("QVGA2"));
                else
                    bufferPtr.Append(_L("QVGA1"));
                }
            else if ((pixelSize.iWidth == 640 || pixelSize.iWidth == 360 )&&
                    (pixelSize.iHeight == 360 || pixelSize.iHeight == 640 ))
                {
                bufferPtr.Append(_L("nHD"));
                }
            else if ((pixelSize.iWidth == 640 || pixelSize.iWidth == 480 )&&
                    (pixelSize.iHeight == 480 || pixelSize.iHeight == 640 ))
                {
                bufferPtr.Append(_L("VGA"));
                }
            else if ((pixelSize.iWidth == 352 || pixelSize.iWidth == 800 )&&
                    (pixelSize.iHeight == 800 || pixelSize.iHeight == 352 ))
                {
                bufferPtr.Append(_L("E90"));
                }
            else if ((pixelSize.iWidth == 320 || pixelSize.iWidth == 480 ||
                      pixelSize.iWidth == 240 || pixelSize.iWidth == 640 )&&
                    (pixelSize.iHeight == 320 || pixelSize.iHeight == 480 ||
                     pixelSize.iHeight == 240 || pixelSize.iHeight == 640))
                {
                bufferPtr.Append(_L("HVGA"));
                }
            else if ((pixelSize.iWidth == 480 || pixelSize.iWidth == 854 ||
                      pixelSize.iWidth == 848 || pixelSize.iWidth == 800 )&&
                    (pixelSize.iHeight == 800 || pixelSize.iHeight == 480 ||
                     pixelSize.iHeight == 848 || pixelSize.iHeight == 854))
                {
                bufferPtr.Append(_L("WVGA"));
                }
            else
                {
                bufferPtr.Append(_L("Unknown"));
                }

            ShowL( *buffer, last );
            bufferPtr.Zero();
            CleanupStack::PopAndDestroy( buffer );
            }
            break;
        case ECmdSwitchOrientation:
            {
            ClearL();
            HBufC* buffer = HBufC::NewLC( 100 );
            TPtr bufferPtr = buffer->Des();
            TBool last = ETrue;

            #ifndef __SERIES60_31__
            if (!iAvkonAppUi->OrientationCanBeChanged())
                {
                bufferPtr.Append(_L("Orientation cannot be changed."));
                ShowL( *buffer, last );
                bufferPtr.Zero();
                CleanupStack::PopAndDestroy( buffer );
                break;
                }
            #endif //__SERIES60_31__

            if ( iAvkonAppUi->Orientation() == CAknAppUiBase::EAppUiOrientationPortrait)
                {
                iAvkonAppUi->SetOrientationL(CAknAppUiBase::EAppUiOrientationLandscape);
                }
            else if (iAvkonAppUi->Orientation() == CAknAppUiBase::EAppUiOrientationLandscape)
                {
                iAvkonAppUi->SetOrientationL(CAknAppUiBase::EAppUiOrientationPortrait);
                }
            else
                {
                // unspecified
                iAvkonAppUi->SetOrientationL(CAknAppUiBase::EAppUiOrientationLandscape);
                }
            bufferPtr.Append(_L("Orientation changed."));
            ShowL( *buffer, last );
            bufferPtr.Zero();
            CleanupStack::PopAndDestroy( buffer );
            break;
            }
        case ECmdStartCalculations:
            {
            ClearL();
            // Get known values
            TInt index = 0;
            TBool last = EFalse;
            if (iFileOutputOn)
                {
                TRect screenRect;
                AknLayoutUtils::LayoutMetricsRect(
                    AknLayoutUtils::EApplicationWindow,
                    screenRect );

                // Add screen dimensions
                TInt height = screenRect.Height();
                TInt width = screenRect.Width();
                TBuf16<32> tgt;
                // HEIGHT
                tgt.Append(_L("height: \t"));
                tgt.AppendNum(height, EDecimal); // put max height into text file
                ShowL( tgt, last );
                tgt.Zero();
                // WIDTH
                tgt.Append(_L("width: \t"));
                tgt.AppendNum(width, EDecimal); // put max width into text file
                ShowL( tgt, last );
                tgt.Zero();
                // VERSION
                TPixelMetricsVersion version = PixelMetrics::Version();
                tgt.Append(_L("major_version: \t"));
                tgt.AppendNum(version.majorVersion, EDecimal); // put major version into text file
                ShowL( tgt, last );
                tgt.Zero();
                tgt.Append(_L("minor_version: \t"));
                tgt.AppendNum(version.minorVersion, EDecimal); // put minor version into text file
                ShowL( tgt, last );
                tgt.Zero();
                }

            TInt myValue = KErrNotFound;
            for (;;)
                {
                if (index==QStyle::PM_Custom_MessageBoxHeight)
                    {
                    last = ETrue;
                    }
                myValue = PixelMetrics::PixelMetricValue(static_cast<QStyle::PixelMetric>(index));
                ShowSingleValueL( index, myValue, last );

                if (last) break;
                // if last before custom values, "jump" to custom base
                if (index==QStyle::PM_SubMenuOverlap) index = QStyle::PM_CustomBase;
                index++;
                }
            }
            break;
        case ECmdCreateHeaderFile:
            CreateHeaderFileL();
            break;
        default:
            break;
        }
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperAppUi::ShowL( const TDesC& aText, TBool& aLast, const TBool& aFileOutput )
    {
    _LIT( KTestPrefix, "\t" );

    HBufC* buffer = HBufC::NewLC( aText.Length() + KTestPrefix().Length() );
    TPtr ptr = buffer->Des();
    ptr.Append( KTestPrefix );
    ptr.Append( aText );
    iView->ShowL( *buffer, aLast, aFileOutput );
    CleanupStack::PopAndDestroy( buffer );
    }

void CPixelMetricsMapperAppUi::ShowSingleValueL(TInt& aPixelMetric, TInt& aValue, TBool& aLast )
    {
    HBufC* buffer = HBufC::NewLC( 100 );
    TPtr bufferPtr = buffer->Des();

    switch (aPixelMetric)
        {
        case QStyle::PM_DockWidgetTitleMargin:
            bufferPtr.Append(_L("DockTitleMargin: "));
            break;
        case QStyle::PM_DockWidgetTitleBarButtonMargin:
            bufferPtr.Append(_L("DockTitleBtnMargin: "));
            break;
        case QStyle::PM_ButtonMargin:
            bufferPtr.Append(_L("ButtonMargin: "));
            break;
        case QStyle::PM_ButtonDefaultIndicator:
            bufferPtr.Append(_L("ButtonDefaultIndicator: "));
            break;
        case QStyle::PM_MdiSubWindowFrameWidth:
            bufferPtr.Append(_L("MdiSubWndFrameW: "));
            break;
        case QStyle::PM_ComboBoxFrameWidth:
            bufferPtr.Append(_L("ComboBoxFrameWidth: "));
            break;
        case QStyle::PM_SpinBoxFrameWidth:
            bufferPtr.Append(_L("SpinBoxFrameWidth: "));
            break;
        case QStyle::PM_DefaultFrameWidth:
            bufferPtr.Append(_L("DefaultFrameWidth: "));
            break;
        case QStyle::PM_RadioButtonLabelSpacing:
            bufferPtr.Append(_L("RadioButtonLabelSpc: "));
            break;
        case QStyle::PM_CheckBoxLabelSpacing:
            bufferPtr.Append(_L("CheckBoxLabelSpacing: "));
            break;
        case QStyle::PM_ToolTipLabelFrameWidth:
            bufferPtr.Append(_L("ToolTipLabelFrameW: "));
            break;
        case QStyle::PM_ListViewIconSize:
            bufferPtr.Append(_L("ListViewIconSize: "));
            break;
        case QStyle::PM_LargeIconSize:
            bufferPtr.Append(_L("LargeIconSize: "));
            break;
        case QStyle::PM_IconViewIconSize:
            bufferPtr.Append(_L("IconViewIconSize: "));
            break;
        case QStyle::PM_TabBarIconSize:
            bufferPtr.Append(_L("TabBarIconSize: "));
            break;
        case QStyle::PM_MessageBoxIconSize:
            bufferPtr.Append(_L("MessageBoxIconSize: "));
            break;
        case QStyle::PM_ButtonIconSize:
            bufferPtr.Append(_L("ButtonIconSize: "));
            break;
        case QStyle::PM_TextCursorWidth:
            bufferPtr.Append(_L("TextCursorWidth: "));
            break;
        case QStyle::PM_SliderLength:
            bufferPtr.Append(_L("SliderLength: "));
            break;
        case QStyle::PM_SliderThickness:
            bufferPtr.Append(_L("SliderThickness: "));
            break;
        case QStyle::PM_SliderTickmarkOffset:
            bufferPtr.Append(_L("SliderTickmarkOffset: "));
            break;
        case QStyle::PM_SliderControlThickness:
            bufferPtr.Append(_L("SliderCntrlThickness: "));
            break;
        case QStyle::PM_SliderSpaceAvailable:
            bufferPtr.Append(_L("SliderSpaceAvailable: "));
            break;
        case QStyle::PM_MenuBarItemSpacing:
            bufferPtr.Append(_L("MenuBarItemSpacing: "));
            break;
        case QStyle::PM_MenuBarHMargin:
            bufferPtr.Append(_L("MenuBarHMargin: "));
            break;
        case QStyle::PM_MenuBarVMargin:
            bufferPtr.Append(_L("MenuBarVMargin: "));
            break;
        case QStyle::PM_ToolBarItemSpacing:
            bufferPtr.Append(_L("ToolBarItemSpacing: "));
            break;
        case QStyle::PM_ToolBarFrameWidth:
            bufferPtr.Append(_L("ToolBarFrameWidth: "));
            break;
        case QStyle::PM_ToolBarItemMargin:
            bufferPtr.Append(_L("ToolBarItemMargin: "));
            break;
        case QStyle::PM_LayoutLeftMargin:
            bufferPtr.Append(_L("LayoutLeftMargin: "));
            break;
        case QStyle::PM_LayoutRightMargin:
            bufferPtr.Append(_L("LayoutRightMargin: "));
            break;
        case QStyle::PM_LayoutTopMargin:
            bufferPtr.Append(_L("LayoutTopMargin: "));
            break;
        case QStyle::PM_LayoutBottomMargin:
            bufferPtr.Append(_L("LayoutBottomMargin: "));
            break;
        case QStyle::PM_LayoutHorizontalSpacing:
            bufferPtr.Append(_L("LayoutHSpacing: "));
            break;
        case QStyle::PM_LayoutVerticalSpacing:
            bufferPtr.Append(_L("LayoutVSpacing: "));
            break;
        case QStyle::PM_MaximumDragDistance:
            bufferPtr.Append(_L("MaxDragDistance: "));
            break;
        case QStyle::PM_ScrollBarExtent:
            bufferPtr.Append(_L("ScrollBarExtent: "));
            break;
        case QStyle::PM_ScrollBarSliderMin:
            bufferPtr.Append(_L("ScrollBarSliderMin: "));
            break;
        case QStyle::PM_MenuBarPanelWidth:
            bufferPtr.Append(_L("MenuBarPanelWidth: "));
            break;
        case QStyle::PM_ProgressBarChunkWidth:
            bufferPtr.Append(_L("ProgBarChunkWidth: "));
            break;
        case QStyle::PM_TabBarTabOverlap:
            bufferPtr.Append(_L("TabBarTabOverlap: "));
            break;
        case QStyle::PM_TabBarTabHSpace:
            bufferPtr.Append(_L("TabBarTabHSpace: "));
            break;
        case QStyle::PM_TabBarTabVSpace:
            bufferPtr.Append(_L("TabBarTabVSpace: "));
            break;
        case QStyle::PM_TabBarBaseHeight:
            bufferPtr.Append(_L("TabBarBaseHeight: "));
            break;
        case QStyle::PM_TabBarBaseOverlap:
            bufferPtr.Append(_L("TabBarBaseOverlap: "));
            break;
        case QStyle::PM_TabBarScrollButtonWidth:
            bufferPtr.Append(_L("TabBarScrollBtnWidth: "));
            break;
        case QStyle::PM_TabBarTabShiftHorizontal:
            bufferPtr.Append(_L("TabBarTabShiftH: "));
            break;
        case QStyle::PM_TabBarTabShiftVertical:
            bufferPtr.Append(_L("TabBarTabShiftV: "));
            break;
        case QStyle::PM_MenuPanelWidth:
            bufferPtr.Append(_L("MenuPanelWidth: "));
            break;
        case QStyle::PM_MenuHMargin:
            bufferPtr.Append(_L("MenuHMargin: "));
            break;
        case QStyle::PM_MenuVMargin:
            bufferPtr.Append(_L("MenuVMargin: "));
            break;
        case QStyle::PM_MenuDesktopFrameWidth:
            bufferPtr.Append(_L("MenuFrameWidth: "));
            break;
        case QStyle::PM_SmallIconSize:
            bufferPtr.Append(_L("SmallIconSize: "));
            break;
        case QStyle::PM_FocusFrameHMargin:
            bufferPtr.Append(_L("FocusFrameHMargin: "));
            break;
        case QStyle::PM_FocusFrameVMargin:
            bufferPtr.Append(_L("FocusFrameVMargin: "));
            break;
        case QStyle::PM_ToolBarIconSize:
            bufferPtr.Append(_L("ToolBarIconSize: "));
            break;
        case QStyle::PM_TitleBarHeight: // use titlepane height
            bufferPtr.Append(_L("TitleBarHeight: "));
            break;
        case QStyle::PM_IndicatorWidth:
            bufferPtr.Append(_L("IndicatorWidth: "));
            break;
        case QStyle::PM_IndicatorHeight:
            bufferPtr.Append(_L("IndicatorHeight: "));
            break;
        case QStyle::PM_ExclusiveIndicatorHeight:
            bufferPtr.Append(_L("ExclusiveIndHeight: "));
            break;
        case QStyle::PM_ExclusiveIndicatorWidth:
            bufferPtr.Append(_L("ExclusiveIndWidth: "));
            break;
        case QStyle::PM_HeaderMargin: // not in S60
            bufferPtr.Append(_L("HeaderMargin: "));
            break;
        case QStyle::PM_MenuScrollerHeight: // not in S60
            bufferPtr.Append(_L("MenuScrollerHeight: "));
            break;
        case QStyle::PM_MenuTearoffHeight: // not in S60
            bufferPtr.Append(_L("MenuTearoffHeight: "));
            break;
        case QStyle::PM_DockWidgetFrameWidth: // not in S60
            bufferPtr.Append(_L("DockFrameWidth: "));
            break;
        case QStyle::PM_DockWidgetSeparatorExtent: // not in S60
            bufferPtr.Append(_L("DockSepExtent: "));
            break;
        case QStyle::PM_MdiSubWindowMinimizedWidth: //no such thing in S60
            bufferPtr.Append(_L("MdiSubWndMinWidth: "));
            break;
        case QStyle::PM_HeaderGripMargin: // not in S60
            bufferPtr.Append(_L("HeaderGripMargin: "));
            break;
        case QStyle::PM_SplitterWidth: // not in S60
            bufferPtr.Append(_L("SplitterWidth: "));
            break;
        case QStyle::PM_ToolBarExtensionExtent: // not in S60
            bufferPtr.Append(_L("ToolBarExtExtent: "));
            break;
        case QStyle::PM_ToolBarSeparatorExtent: // not in S60
            bufferPtr.Append(_L("ToolBarSepExtent: "));
            break;
        case QStyle::PM_ToolBarHandleExtent: // not in s60
            bufferPtr.Append(_L("ToolBarHandleExtent: "));
            break;
        case QStyle::PM_MenuButtonIndicator: // none???
            bufferPtr.Append(_L("MenuButtonIndicator: "));
            break;
        case QStyle::PM_ButtonShiftHorizontal: //none in 3.x
            bufferPtr.Append(_L("ButtonShiftHorizontal: "));
            break;
        case QStyle::PM_ButtonShiftVertical: // none in 3.x
            bufferPtr.Append(_L("ButtonShiftVertical: "));
            break;
        case QStyle::PM_TabBar_ScrollButtonOverlap: // not used in S60 - tab arrows are on left and right side of tab group - not together
            bufferPtr.Append(_L("TabScrollBtnOverlap: "));
            break;
        case QStyle::PM_SizeGripSize: // use default
            bufferPtr.Append(_L("SizeGripSize: "));
            break;
        case QStyle::PM_DockWidgetHandleExtent:
            bufferPtr.Append(_L("DockWdgtHandleExt: "));
            break;
        case QStyle::PM_CheckListButtonSize:
            bufferPtr.Append(_L("CheckListButtonSize: "));
            break;
        case QStyle::PM_CheckListControllerSize:
            bufferPtr.Append(_L("CheckListCntlerSize: "));
            break;
        case QStyle::PM_DialogButtonsSeparator:
            bufferPtr.Append(_L("DialogBtnSeparator: "));
            break;
        case QStyle::PM_DialogButtonsButtonWidth:
            bufferPtr.Append(_L("DialogBtnWidth: "));
            break;
        case QStyle::PM_DialogButtonsButtonHeight:
            bufferPtr.Append(_L("DialogBtnHeight: "));
            break;
        case QStyle::PM_HeaderMarkSize:
            bufferPtr.Append(_L("HeaderMarkSize: "));
            break;
        case QStyle::PM_SpinBoxSliderHeight:
            bufferPtr.Append(_L("SpinBoxSliderHeight: "));
            break;
        case QStyle::PM_DefaultTopLevelMargin:
            bufferPtr.Append(_L("DefaultTopLvlMrg: "));
            break;
        case QStyle::PM_DefaultChildMargin:
            bufferPtr.Append(_L("DefaultChildMrg: "));
            break;
        case QStyle::PM_DefaultLayoutSpacing:
            bufferPtr.Append(_L("DefaultlayoutSpc: "));
            break;
        case QStyle::PM_TabCloseIndicatorWidth:
            bufferPtr.Append(_L("TabCloseIndWidth: "));
            break;
        case QStyle::PM_TabCloseIndicatorHeight:
            bufferPtr.Append(_L("TabCloseIndHeight: "));
            break;
        case QStyle::PM_ScrollView_ScrollBarSpacing:
            bufferPtr.Append(_L("ScrollViewBarSpc: "));
            break;
        case QStyle::PM_SubMenuOverlap:
            bufferPtr.Append(_L("SubMenuOverlap: "));
            break;
        case QStyle::PM_Custom_FrameCornerHeight:
            bufferPtr.Append(_L("C_FrCornerHeight: "));
            break;
        case QStyle::PM_Custom_FrameCornerWidth:
            bufferPtr.Append(_L("C_FrCornerWidth: "));
            break;
        case QStyle::PM_Custom_ThinLineWidth:
            bufferPtr.Append(_L("C_ThinLineWidth: "));
            break;
        case QStyle::PM_Custom_BoldLineWidth:
            bufferPtr.Append(_L("C_BoldLineWidth: "));
            break;
        case QStyle::PM_Custom_MessageBoxHeight:
            bufferPtr.Append(_L("C_MsgBoxHeight: "));
            break;
        default:
            bufferPtr.Append(_L("Default: "));
            break;
        }

    if (iFileOutputOn)
        {
        bufferPtr.Append('\t');
        }
    bufferPtr.AppendNum(aValue);
    bufferPtr.Append(_L(" "));
    ShowL( *buffer, aLast, iFileOutputOn );
    CleanupStack::PopAndDestroy( buffer );
    }

void CPixelMetricsMapperAppUi::ClearL()
    {
    iView->ClearL();
    }

void CPixelMetricsMapperAppUi::CreateHeaderFileL() const
    {
    // Open/create resulting file.
    RFile file;
    HBufC* layoutFile = HBufC::NewLC( KMaxFileName );
    *layoutFile = KLayoutSourceFileAndPath;
    TFileName fileName = *layoutFile;
    CleanupStack::PopAndDestroy(layoutFile);
    RFs& fs = CEikonEnv::Static()->FsSession();
    TInt error = file.Open(fs,fileName, EFileWrite|EFileShareAny|EFileStreamText );
    if (error==KErrNotFound)
        {
       file.Create(fs,fileName, EFileWrite|EFileShareAny|EFileStreamText);
        }
    CleanupClosePushL( file );
    file.SetSize( 0 );

    // Make all writes as from textfile.
    TFileText textFile;
    textFile.Set( file );
    textFile.Seek( ESeekStart );

    // Take all layout files from private folder.
    CDir* dirList;
    User::LeaveIfError(fs.GetDir(
        KPixelMetricsDataFiles,
        KEntryAttMaskSupported,
        ESortByName,
        dirList));

    TMySmallBuffer bufferLayoutHdr;
    TMyBigBuffer bufferPMData;
    TInt fileCount = dirList->Count();
    for (TInt i=0;i<fileCount;i++)
        {
        // open sourcefile
        RFile sourceFile;
        TFileName layoutFile = (*dirList)[i].iName;
        User::LeaveIfError( sourceFile.Open(
            fs,layoutFile, EFileRead|EFileShareAny|EFileStreamText ));
        CleanupClosePushL( sourceFile );

        // Make all reads as from textfile.
        TFileText textSourceFile;
        textSourceFile.Set( sourceFile );
        TFileName layoutName = CreateLayoutNameL( textSourceFile );

        // rewind - just in case.
        textSourceFile.Seek( ESeekStart );
        TFileName oneline;
        bufferLayoutHdr.Append(KOpenBrace);
        bufferPMData.Append(KOpenBrace);
        TInt loop = 0;
        FOREVER
            {
            if( textSourceFile.Read(oneline) != KErrNone )
                {
                break;
                }
            // Add commas for all but first line
            if (loop != 0)
                {
                if ( loop <= KHeaderValues-1)
                    {
                    bufferLayoutHdr.Append(KComma);
                    }
                else
                    {
                    if (loop != KHeaderValues)
                        {
                        bufferPMData.Append(KComma);
                        }
                    }
                if (loop==KHeaderValues)
                    {
                    bufferLayoutHdr.Append(_L(",QLatin1String(\""));
                    bufferLayoutHdr.Append(layoutName);
                    bufferLayoutHdr.Append(_L("\")"));
                    }
                }
            // Remove pixel metrics name and ":"
            oneline = oneline.Mid(oneline.Find(KColon)+1);
            // Remove tab
            oneline = oneline.Mid(oneline.Find(KTab)+1);
            // remove crap from the end of line
            TLex lex(oneline);
            TInt nextValue = -666;
            User::LeaveIfError( lex.Val(nextValue) );
            if ( loop <= KHeaderValues-1)
                {
                bufferLayoutHdr.AppendNum(nextValue);
                }
            else
                {
                if (nextValue == -909)
                    bufferPMData.Append(_L("ECommonStyleValue"));
                else
                    bufferPMData.AppendNum(nextValue);
                }
            oneline.Zero();
            loop++;
            }
        file.Flush();
        bufferLayoutHdr.Append(KEndBraceWithCommaAndCRLF);
        bufferPMData.Append(KEndBraceWithCommaAndCRLF);
        CleanupStack::PopAndDestroy(); //sourceFile
        }

    if (fileCount > 0)
        {
        bufferLayoutHdr = bufferLayoutHdr.Left(bufferLayoutHdr.Length()-2);
        bufferPMData = bufferPMData.Left(bufferPMData.Length()-2);
        textFile.Write(bufferLayoutHdr);
        textFile.Write(KCRLF);
        textFile.Write(bufferPMData);
        }
    delete dirList;

    CleanupStack::PopAndDestroy(); //file
    }

TFileName CPixelMetricsMapperAppUi::CreateLayoutNameL(TFileText& aFileHandle) const
{
    aFileHandle.Seek(ESeekStart);
    // Layout data is deployed like this:
    // first line - height
    // second line - width
    TFileName lines;
    TFileName layoutName;

    TInt height = -666;
    TInt width = -666;
    // Collect name information.
    for (TInt i=0; i<6; i++)
        {
        User::LeaveIfError(aFileHandle.Read(lines));
        // Remove pixel metrics name and ":"
        lines = lines.Mid(lines.Find(KColon)+1);
        // Remove tab
        lines = lines.Mid(lines.Find(KTab)+1);
        TLex myLexer(lines);
        TInt error = KErrNone;
        if (i==0) //height is first
            {
            error = myLexer.Val(height);
            }
        if (i==1) //width is second
            {
            error = myLexer.Val(width);
            }
        User::LeaveIfError(error);
        }

    // Interpret results and write name to buffer.
    if ( (width == 240 && height == 320) ||
         (width == 320 && height == 240))
        {
        layoutName.Append(_L("QVGA "));
        }
    else if ( (width == 360 && height == 640) ||
         (width == 640 && height == 360))
        {
        layoutName.Append(_L("NHD "));
        }
    else if ( (width == 480 && height == 640) ||
         (width == 640 && height == 480))
        {
        layoutName.Append(_L("VGA "));
        }
    else if ( (width == 800 && height == 352) ||
         (width == 352 && height == 800))
        {
        layoutName.Append(_L("E90 "));
        }
    else if ( (width == 800 && height == 480) ||
         (width == 480 && height == 800) ||
         (width == 848 && height == 480) ||
         (width == 480 && height == 848) ||
         (width == 854 && height == 480) ||
         (width == 480 && height == 854))
        {
        layoutName.Append(_L("WVGA "));
        }
    else if ( (width == 480 && height == 320) ||
         (width == 320 && height == 480) ||
         (width == 640 && height == 240) ||
         (width == 240 && height == 640))
        {
        layoutName.Append(_L("HVGA "));
        }
    else
        {
        layoutName.Append(_L("Unknown "));
        layoutName.AppendNum(height);
        layoutName.Append(_L("x"));
        layoutName.AppendNum(width);
        }
    if (width > height)
        {
        layoutName.Append(_L("Landscape"));
        }
    else
        {
        layoutName.Append(_L("Portrait"));
        }
    return layoutName;
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
CEikAppUi* CPixelMetricsMapperDocument::CreateAppUiL()
    {
    return( new ( ELeave ) CPixelMetricsMapperAppUi );
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperDocument::ConstructL()
    {
    }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
TUid CPixelMetricsMapperApplication::AppDllUid() const
    {
    return KUidPMMapperApplication;
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
CApaDocument* CPixelMetricsMapperApplication::CreateDocumentL()
    {
    CPixelMetricsMapperDocument* document =
        new( ELeave ) CPixelMetricsMapperDocument( *this );
    CleanupStack::PushL( document );
    document->ConstructL();
    CleanupStack::Pop();
    return( document );
    }

// ========================== OTHER EXPORTED FUNCTIONS =========================
// ---------------------------------------------------------
// NewApplication implements
//
// Creates an instance of application.
//
// Returns: an instance of CVtUiApp
// ---------------------------------------------------------
//
LOCAL_C CApaApplication* NewApplication()
    {
    return new CPixelMetricsMapperApplication;
    }

// ---------------------------------------------------------
// E32Main implements
//
// It is called when executable is started.
//
// Returns: error code.
// ---------------------------------------------------------
//
GLDEF_C TInt E32Main()
    {
    return EikStart::RunApplication( NewApplication );
    }

// End of File
