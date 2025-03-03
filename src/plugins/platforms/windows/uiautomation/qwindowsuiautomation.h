// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAUTOMATION_H
#define QWINDOWSUIAUTOMATION_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <uiautomation.h>

#ifndef Q_CC_MSVC
// MinGW's headers were missing these, but have since added an incomplete set
// of the constants and structures from the .idl file. Fortunately, their IDL
// processor generates #define's for the constants, so we can check if they are
// present.

#ifndef UIA_IsReadOnlyAttributeId
#define UIA_IsReadOnlyAttributeId                40015
#endif
#ifndef UIA_StrikethroughStyleAttributeId
#define UIA_StrikethroughStyleAttributeId        40026
#endif
#ifndef UIA_StyleIdAttributeId
#define UIA_StyleIdAttributeId                   40034
#endif
#ifndef UIA_CaretPositionAttributeId
#define UIA_CaretPositionAttributeId             40038
#endif

#ifndef StyleId_Heading1
#define StyleId_Heading1    70001
#endif
#ifndef StyleId_Heading2
#define StyleId_Heading2    70002
#endif
#ifndef StyleId_Heading3
#define StyleId_Heading3    70003
#endif
#ifndef StyleId_Heading4
#define StyleId_Heading4    70004
#endif
#ifndef StyleId_Heading5
#define StyleId_Heading5    70005
#endif
#ifndef StyleId_Heading6
#define StyleId_Heading6    70006
#endif
#ifndef StyleId_Heading7
#define StyleId_Heading7    70007
#endif
#ifndef StyleId_Heading8
#define StyleId_Heading8    70008
#endif
#ifndef StyleId_Heading9
#define StyleId_Heading9    70009
#endif

#if !defined(UIA_SelectionPattern2Id)
#define UIA_SelectionPattern2Id                  10034

// Unfortunately, there's no way to detect the definition of enums, so we are
// guessing their presence from a constant that is also missing.

enum CaretPosition {
    CaretPosition_Unknown           = 0,
    CaretPosition_EndOfLine         = 1,
    CaretPosition_BeginningOfLine   = 2
};

enum TextDecorationLineStyle {
    TextDecorationLineStyle_None = 0,
    TextDecorationLineStyle_Single = 1,
    TextDecorationLineStyle_WordsOnly = 2,
    TextDecorationLineStyle_Double = 3,
    TextDecorationLineStyle_Dot = 4,
    TextDecorationLineStyle_Dash = 5,
    TextDecorationLineStyle_DashDot = 6,
    TextDecorationLineStyle_DashDotDot = 7,
    TextDecorationLineStyle_Wavy = 8,
    TextDecorationLineStyle_ThickSingle = 9,
    TextDecorationLineStyle_DoubleWavy = 11,
    TextDecorationLineStyle_ThickWavy = 12,
    TextDecorationLineStyle_LongDash = 13,
    TextDecorationLineStyle_ThickDash = 14,
    TextDecorationLineStyle_ThickDashDot = 15,
    TextDecorationLineStyle_ThickDashDotDot = 16,
    TextDecorationLineStyle_ThickDot = 17,
    TextDecorationLineStyle_ThickLongDash = 18,
    TextDecorationLineStyle_Other = -1
};
#endif // UIA_SelectionPattern2Id

BOOL WINAPI UiaClientsAreListening();

#ifndef __ISelectionProvider2_INTERFACE_DEFINED__
#define __ISelectionProvider2_INTERFACE_DEFINED__
DEFINE_GUID(IID_ISelectionProvider2, 0x14f68475, 0xee1c, 0x44f6, 0xa8, 0x69, 0xd2, 0x39, 0x38, 0x1f, 0x0f, 0xe7);
MIDL_INTERFACE("14f68475-ee1c-44f6-a869-d239381f0fe7")
ISelectionProvider2 : public ISelectionProvider
{
public:
    virtual HRESULT STDMETHODCALLTYPE get_FirstSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **retVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_LastSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **retVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CurrentSelectedItem(__RPC__deref_out_opt IRawElementProviderSimple **retVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ItemCount(__RPC__out int *retVal) = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(ISelectionProvider2,  0x14f68475, 0xee1c, 0x44f6, 0xa8, 0x69, 0xd2, 0x39, 0x38, 0x1f, 0x0f, 0xe7)
#endif
#endif // __ISelectionProvider2_INTERFACE_DEFINED__

#endif // !Q_CC_MSVC

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAUTOMATION_H
