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

#include <eiklabel.h>
#include <avkon.rsg>
#include <aknviewappui.h>
#include <aknconsts.h>

#include "pm_mapper.hrh"
#include <pm_mapper.rsg>
#include "pm_mapperView.h"
#include "pm_mapperApp.h"

#include <aknlists.h>
#include <avkon.hrh>
#include <AknUtils.h>

// -----------------------------------------------------------------------------
// C++ constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CPixelMetricsMapperViewContainer::CPixelMetricsMapperViewContainer(): iCount( 1 )
    {
    }


// -----------------------------------------------------------------------------
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperViewContainer::ConstructL( const TRect& aRect )
    {
    CreateWindowL();
    SetCanDrawOutsideRect();

    iTexts = new( ELeave ) CDesCArrayFlat( 10 );
    iTexts->AppendL( _L( "\tStarted." ) );

    iListbox = new( ELeave ) CAknSingleStyleListBox;
    iListbox->SetContainerWindowL( *this );
    iListbox->ConstructL( this, EAknListBoxViewerFlags  );

    iListbox->Model()->SetItemTextArray( iTexts );
    iListbox->SetRect( TRect( aRect.Size() ) );

    iListbox->CreateScrollBarFrameL( ETrue );
    iListbox->ScrollBarFrame()->SetScrollBarVisibilityL(
        CEikScrollBarFrame::EOn,
        CEikScrollBarFrame::EOn );

    SetRect( aRect );
    iListbox->ActivateL();
    ActivateL();
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperViewContainer::ShowL( const TDesC& aString, TBool& aLast, const TBool& aFileOutput )
    {
    MDesCArray* itemList = iListbox->Model()->ItemTextArray();
    CDesCArray* itemArray = ( CDesCArray* ) itemList;

    itemArray->AppendL( aString );

    iListbox->HandleItemAdditionL();
    iListbox->SetCurrentItemIndex( iCount );
    iCount++;
    if ( aLast )
        {
        if (aFileOutput)
            {
            RFile file;
            RFs& fs = CEikonEnv::Static()->FsSession();
            TFileName fileName =_L("Layout_");

            TRect screenRect;
            AknLayoutUtils::LayoutMetricsRect(
                AknLayoutUtils::EApplicationWindow,
                screenRect );

            // Add screen dimensions
            TInt height = screenRect.Height();
            TInt width = screenRect.Width();
            fileName.AppendNum(height);
            fileName.Append('_');
            fileName.AppendNum(width);

            fileName.Append(_L(".txt"));

            TInt err=file.Open(fs,fileName,EFileStreamText|EFileWrite|EFileShareAny);
            if (err==KErrNotFound) // file does not exist - create it
                err=file.Create(fs,fileName,EFileStreamText|EFileWrite|EFileShareAny);
            else
                file.SetSize(0); //sweep the file
            TFileText textFile;
            textFile.Set(file);
            err = textFile.Seek(ESeekStart);
            if (err) User::InfoPrint(_L("File corrupted"));

            // Finally loop through all the entries:
            TInt idx = 0;
            for(;idx!=itemList->MdcaCount();idx++)
                {
                err = textFile.Write(itemList->MdcaPoint(idx));
                if (err) User::InfoPrint(_L("File corrupted"));
                }
            file.Close();
            }
        DrawNow();
        }
    }

void CPixelMetricsMapperViewContainer::ClearL()
    {
    MDesCArray* itemList = iListbox->Model()->ItemTextArray();
    CDesCArray* itemArray = ( CDesCArray* ) itemList;

    itemArray->Reset();

    iListbox->HandleItemAdditionL();
    iCount = 0;
    DrawNow();
    }


// -----------------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------------
//
CPixelMetricsMapperViewContainer::~CPixelMetricsMapperViewContainer()
    {
    delete iListbox;
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperViewContainer::SizeChanged()
    {
    CCoeControl::SizeChanged();
    if ( iListbox )
        {
        iListbox->SetRect( Rect() );
        }
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
TInt CPixelMetricsMapperViewContainer::CountComponentControls() const
    {
    return 1;
    }


// -----------------------------------------------------------------------------
// CTestAppViewContainer::ComponentControl
//
//
// -----------------------------------------------------------------------------
//
CCoeControl* CPixelMetricsMapperViewContainer::ComponentControl(
        TInt /*aIndex*/ ) const
    {
    return iListbox;
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperViewContainer::Draw( const TRect& /*aRect*/ ) const
    {
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperViewContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,
    TCoeEvent /*aEventType*/ )
    {
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
TKeyResponse CPixelMetricsMapperViewContainer::OfferKeyEventL(
    const TKeyEvent& aKeyEvent,
    TEventCode aType )
    {
    if (aKeyEvent.iCode == EKeyUpArrow ||
        aKeyEvent.iCode == EKeyDownArrow )
        {
        return iListbox->OfferKeyEventL( aKeyEvent, aType );
        }
    return EKeyWasNotConsumed;
    }

void CPixelMetricsMapperViewContainer::HandleResourceChange(TInt aType)
    {
    CCoeControl::HandleResourceChange( aType );
    if ( aType == KEikDynamicLayoutVariantSwitch )
         {
         TRect mainPaneRect;
         AknLayoutUtils::LayoutMetricsRect(
             AknLayoutUtils::EMainPane,
             mainPaneRect );
         SetRect( mainPaneRect );

         }
    if (iListbox)
        iListbox->HandleResourceChange(aType);
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperView::ShowL( const TDesC& aString, TBool& aLast, const TBool& aFileOutput )
    {
    iView->ShowL( aString, aLast, aFileOutput );
    }

void CPixelMetricsMapperView::ClearL()
    {
    iView->ClearL();
    }


// -----------------------------------------------------------------------------
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperView::ConstructL()
    {
    BaseConstructL( R_PMMAPPER_VIEW );
    }


// -----------------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------------
//
CPixelMetricsMapperView::~CPixelMetricsMapperView()
    {
    if ( iView )
        {
        AppUi()->RemoveFromViewStack( *this, iView );
        }
    delete iView;
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
TUid CPixelMetricsMapperView::Id() const
    {
    return TUid::Uid( EPMMapperViewId );
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperView::HandleCommandL( TInt aCommand )
    {
    AppUi()->HandleCommandL( aCommand );
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperView::HandleStatusPaneSizeChange()
    {
    if ( iView )
        {
        TRect cr = ClientRect();
        iView->SetRect( cr );
        }
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperView::DoActivateL(
    const TVwsViewId& /*aPrevViewId*/,
    TUid              /*aCustomMessageId*/,
    const TDesC8&     /*aCustomMessage*/ )
    {
    iView = new( ELeave ) CPixelMetricsMapperViewContainer;

    TRect cr = ClientRect();
    iView->ConstructL( cr );
    AppUi()->AddToViewStackL( *this, iView );
    }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
//
void CPixelMetricsMapperView::DoDeactivate()
    {
    if (iView)
        {
        AppUi()->RemoveFromViewStack( *this, iView );
        }
    delete iView;
    iView = NULL;
    }


// End of File
