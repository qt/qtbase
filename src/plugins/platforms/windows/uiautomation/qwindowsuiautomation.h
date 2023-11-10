// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAUTOMATION_H
#define QWINDOWSUIAUTOMATION_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <uiautomation.h>

#if defined(__MINGW32__) || defined(__MINGW64__)

#define UIA_SelectionPattern2Id                  10034
#define UIA_IsReadOnlyAttributeId                40015
#define UIA_StrikethroughStyleAttributeId        40026
#define UIA_StyleIdAttributeId                   40034
#define UIA_CaretPositionAttributeId             40038

#define StyleId_Heading1    70001
#define StyleId_Heading2    70002
#define StyleId_Heading3    70003
#define StyleId_Heading4    70004
#define StyleId_Heading5    70005
#define StyleId_Heading6    70006
#define StyleId_Heading7    70007
#define StyleId_Heading8    70008
#define StyleId_Heading9    70009

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

#endif // defined(__MINGW32__) || defined(__MINGW64__)

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAUTOMATION_H
