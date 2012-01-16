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

#ifndef PMMAPPERVIEW_H
#define PMMAPPERVIEW_H


//  INCLUDES
#include <aknview.h>
#include <EIKLBO.H>

// CONSTANTS
// FORWARD DECLARATIONS
class CAknSingleStyleListBox;
class CAknSettingStyleListBox;

// CLASS DECLARATION

/**
*  CPixelMetricsMapperViewContainer
*
*/
class CPixelMetricsMapperViewContainer
:   public CCoeControl,
    public MCoeControlObserver
    {
    public:  // Constructors and destructor

        /**
        * C++ constructor.
        */
    	CPixelMetricsMapperViewContainer();

        /**
        * Symbian 2nd phase constructor.
        *
        * @param aRect Rectangle.
        */
        void ConstructL( const TRect& aRect );

        /**
        * Destructor.
        */
        ~CPixelMetricsMapperViewContainer();


    public: // New functions

        /**
        * Show the given string.
        *
        * @param aString The string to be shown.
        */
        void ShowL( const TDesC& aString, TBool& aLast, const TBool& aFileOutput = EFalse );

        void ClearL();


    public:  // Functions from base classes

        /**
        * From CCoeControl.
        */
        TKeyResponse OfferKeyEventL(
            const TKeyEvent& aKeyEvent,
            TEventCode aType );


        void HandleResourceChange(TInt aType);


    private:  // Functions from base classes

        /**
        * From CCoeControl.
        */
        void SizeChanged();

        /**
        * From CCoeControl.
        */
        TInt CountComponentControls() const;

        /**
        * From CCoeControl.
        */
        CCoeControl* ComponentControl( TInt aIndex ) const;

        /**
        * From CCoeControl.
        */
        void Draw( const TRect& aRect ) const;


    private:  // Functions from base classes

        /**
        * From MCoeControlObserver.
        */
        void HandleControlEventL(
            CCoeControl* aControl,
            TCoeEvent aEventType );


    private:    // Data

        // Texts.
        CDesCArray*                 iTexts;

        // Count.
        TInt                        iCount;

        // Listbox.
        CAknSingleStyleListBox*     iListbox;

    };



/**
*  CPixelMetricsMapperView
*
*
*  @since 1.0
*/
class CPixelMetricsMapperView : public CAknView
    {
    public:  // Constructors and destructor

        /**
        * Symbian 2nd phase constructor.
        */
        void ConstructL();

        /**
        * Destructor.
        */
        ~CPixelMetricsMapperView();


    public: // Functions from base classes

        /**
        * From CAknView.
        */
        TUid Id() const;

        /**
        * From CAknView.
        */
        void HandleCommandL( TInt aCommand );

        /**
        * From CAknView.
        */
        void HandleStatusPaneSizeChange();

        /**
        * From CAknView.
        */
        void ShowL( const TDesC& aString, TBool& aLast, const TBool& aFileOutput =EFalse );
        void ClearL();


    private: // from CAknView

        /**
        * From CAknView.
        */
        void DoActivateL(
            const TVwsViewId& aPrevViewId,
            TUid aCustomMessageId,
            const TDesC8& aCustomMessage );

        /**
        * From CAknView.
        */
        void DoDeactivate();


    private:    // Data

        // The view container.
        CPixelMetricsMapperViewContainer*    iView;

    };

#endif // PMMAPPERVIEW_H

// End of File
