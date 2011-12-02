/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY


#include "qwindowsaccessibility.h"
#include "qwindowscontext.h"

#include <private/qsystemlibrary_p.h>

#include <QtCore/qmap.h>
#include <QtCore/qsettings.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qpair.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qplatformnativeinterface_qpa.h>
#include <QtGui/qwindow.h>
#include <QtGui/qaccessible2.h>
#include <OleAcc.h>

//#include <uiautomationcoreapi.h>
#ifndef UiaRootObjectId
#define UiaRootObjectId        -25
#endif

#include <winuser.h>
#if !defined(WINABLEAPI)
#  if defined(Q_WS_WINCE)
#    include <bldver.h>
#  endif
#  include <winable.h>
#endif

#include <oleacc.h>
#if !defined(Q_CC_BOR) && !defined (Q_CC_GNU)
#include <comdef.h>
#endif

#ifdef Q_WS_WINCE
#include "qguifunctions_wince.h"
#endif

QT_BEGIN_NAMESPACE

//#define DEBUG_SHOW_ATCLIENT_COMMANDS
#ifdef DEBUG_SHOW_ATCLIENT_COMMANDS
QT_BEGIN_INCLUDE_NAMESPACE
#include <qdebug.h>
QT_END_INCLUDE_NAMESPACE

static const char *roleString(QAccessible::Role role)
{
    static const char *roles[] = {
       "NoRole"         /* = 0x00000000 */,
       "TitleBar"       /* = 0x00000001 */,
       "MenuBar"        /* = 0x00000002 */,
       "ScrollBar"      /* = 0x00000003 */,
       "Grip"           /* = 0x00000004 */,
       "Sound"          /* = 0x00000005 */,
       "Cursor"         /* = 0x00000006 */,
       "Caret"          /* = 0x00000007 */,
       "AlertMessage"   /* = 0x00000008 */,
       "Window"         /* = 0x00000009 */,
       "Client"         /* = 0x0000000A */,
       "PopupMenu"      /* = 0x0000000B */,
       "MenuItem"       /* = 0x0000000C */,
       "ToolTip"        /* = 0x0000000D */,
       "Application"    /* = 0x0000000E */,
       "Document"       /* = 0x0000000F */,
       "Pane"           /* = 0x00000010 */,
       "Chart"          /* = 0x00000011 */,
       "Dialog"         /* = 0x00000012 */,
       "Border"         /* = 0x00000013 */,
       "Grouping"       /* = 0x00000014 */,
       "Separator"      /* = 0x00000015 */,
       "ToolBar"        /* = 0x00000016 */,
       "StatusBar"      /* = 0x00000017 */,
       "Table"          /* = 0x00000018 */,
       "ColumnHeader"   /* = 0x00000019 */,
       "RowHeader"      /* = 0x0000001A */,
       "Column"         /* = 0x0000001B */,
       "Row"            /* = 0x0000001C */,
       "Cell"           /* = 0x0000001D */,
       "Link"           /* = 0x0000001E */,
       "HelpBalloon"    /* = 0x0000001F */,
       "Assistant"      /* = 0x00000020 */,
       "List"           /* = 0x00000021 */,
       "ListItem"       /* = 0x00000022 */,
       "Tree"           /* = 0x00000023 */,
       "TreeItem"       /* = 0x00000024 */,
       "PageTab"        /* = 0x00000025 */,
       "PropertyPage"   /* = 0x00000026 */,
       "Indicator"      /* = 0x00000027 */,
       "Graphic"        /* = 0x00000028 */,
       "StaticText"     /* = 0x00000029 */,
       "EditableText"   /* = 0x0000002A */,  // Editable, selectable, etc.
       "PushButton"     /* = 0x0000002B */,
       "CheckBox"       /* = 0x0000002C */,
       "RadioButton"    /* = 0x0000002D */,
       "ComboBox"       /* = 0x0000002E */,
       "DropList"       /* = 0x0000002F */,    // commented out
       "ProgressBar"    /* = 0x00000030 */,
       "Dial"           /* = 0x00000031 */,
       "HotkeyField"    /* = 0x00000032 */,
       "Slider"         /* = 0x00000033 */,
       "SpinBox"        /* = 0x00000034 */,
       "Canvas"         /* = 0x00000035 */,
       "Animation"      /* = 0x00000036 */,
       "Equation"       /* = 0x00000037 */,
       "ButtonDropDown" /* = 0x00000038 */,
       "ButtonMenu"     /* = 0x00000039 */,
       "ButtonDropGrid" /* = 0x0000003A */,
       "Whitespace"     /* = 0x0000003B */,
       "PageTabList"    /* = 0x0000003C */,
       "Clock"          /* = 0x0000003D */,
       "Splitter"       /* = 0x0000003E */,
       "LayeredPane"    /* = 0x0000003F */,
       "UserRole"       /* = 0x0000ffff*/
   };

   if (role >=0x40)
        role = QAccessible::UserRole;
   return roles[int(role)];
}

static const char *eventString(QAccessible::Event ev)
{
    static const char *events[] = {
        "null",                                 // 0
        "SoundPlayed"          /*= 0x0001*/,
        "Alert"                /*= 0x0002*/,
        "ForegroundChanged"    /*= 0x0003*/,
        "MenuStart"            /*= 0x0004*/,
        "MenuEnd"              /*= 0x0005*/,
        "PopupMenuStart"       /*= 0x0006*/,
        "PopupMenuEnd"         /*= 0x0007*/,
        "ContextHelpStart"     /*= 0x000C*/,    // 8
        "ContextHelpEnd"       /*= 0x000D*/,
        "DragDropStart"        /*= 0x000E*/,
        "DragDropEnd"          /*= 0x000F*/,
        "DialogStart"          /*= 0x0010*/,
        "DialogEnd"            /*= 0x0011*/,
        "ScrollingStart"       /*= 0x0012*/,
        "ScrollingEnd"         /*= 0x0013*/,
        "MenuCommand"          /*= 0x0018*/,    // 16

        // Values from IAccessible2
        "ActionChanged"        /*= 0x0101*/,    // 17
        "ActiveDescendantChanged",
        "AttributeChanged",
        "DocumentContentChanged",
        "DocumentLoadComplete",
        "DocumentLoadStopped",
        "DocumentReload",
        "HyperlinkEndIndexChanged",
        "HyperlinkNumberOfAnchorsChanged",
        "HyperlinkSelectedLinkChanged",
        "HypertextLinkActivated",
        "HypertextLinkSelected",
        "HyperlinkStartIndexChanged",
        "HypertextChanged",
        "HypertextNLinksChanged",
        "ObjectAttributeChanged",
        "PageChanged",
        "SectionChanged",
        "TableCaptionChanged",
        "TableColumnDescriptionChanged",
        "TableColumnHeaderChanged",
        "TableModelChanged",
        "TableRowDescriptionChanged",
        "TableRowHeaderChanged",
        "TableSummaryChanged",
        "TextAttributeChanged",
        "TextCaretMoved",
        // TextChanged, deprecated, use TextUpdated
        //TextColumnChanged = TextCaretMoved + 2,
        "TextInserted",
        "TextRemoved",
        "TextUpdated",
        "TextSelectionChanged",
        "VisibleDataChanged",  /*= 0x0101+32*/
        "ObjectCreated"        /*= 0x8000*/,    // 49
        "ObjectDestroyed"      /*= 0x8001*/,
        "ObjectShow"           /*= 0x8002*/,
        "ObjectHide"           /*= 0x8003*/,
        "ObjectReorder"        /*= 0x8004*/,
        "Focus"                /*= 0x8005*/,
        "Selection"            /*= 0x8006*/,
        "SelectionAdd"         /*= 0x8007*/,
        "SelectionRemove"      /*= 0x8008*/,
        "SelectionWithin"      /*= 0x8009*/,
        "StateChanged"         /*= 0x800A*/,
        "LocationChanged"      /*= 0x800B*/,
        "NameChanged"          /*= 0x800C*/,
        "DescriptionChanged"   /*= 0x800D*/,
        "ValueChanged"         /*= 0x800E*/,
        "ParentChanged"        /*= 0x800F*/,
        "HelpChanged"          /*= 0x80A0*/,
        "DefaultActionChanged" /*= 0x80B0*/,
        "AcceleratorChanged"   /*= 0x80C0*/
    };
    int e = int(ev);
    if (e <= 0x80c0) {
        const int last = sizeof(events)/sizeof(char*) - 1;

        if (e <= 0x07)
            return events[e];
        else if (e <= 0x13)
            return events[e - 0x0c + 8];
        else if (e == 0x18)
            return events[16];
        else if (e <= 0x0101 + 32)
            return events[e - 0x101 + 17];
        else if (e <= 0x800f)
            return events[e - 0x8000 + 49];
        else if (e == 0x80a0)
            return events[last - 2];
        else if (e == 0x80b0)
            return events[last - 1];
        else if (e == 0x80c0)
            return events[last];
    }
    return "unknown";
};

void showDebug(const char* funcName, const QAccessibleInterface *iface)
{
    qDebug() << "Role:" << roleString(iface->role(0))
             << "Name:" << iface->text(QAccessible::Name, 0)
             << "State:" << QString::number(int(iface->state(0)), 16)
             << QLatin1String(funcName);
}
#else
# define showDebug(f, iface)
#endif

typedef QSharedPointer<QAccessibleInterface> QAIPointer;

// This stuff is used for widgets/items with no window handle:
typedef QMap<int, QPair<QObject*,int> > NotifyMap;
Q_GLOBAL_STATIC(NotifyMap, qAccessibleRecentSentEvents)
static int eventNum = 0;

class QWindowsEnumerate : public IEnumVARIANT
{
public:
    QWindowsEnumerate(const QVector<int> &a)
        : ref(0), current(0),array(a)
    {
    }

    virtual ~QWindowsEnumerate() {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **ppEnum);
    HRESULT STDMETHODCALLTYPE Next(unsigned long  celt, VARIANT FAR*  rgVar, unsigned long FAR*  pCeltFetched);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Skip(unsigned long celt);

private:
    ULONG ref;
    ULONG current;
    QVector<int> array;
};

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::QueryInterface(REFIID id, LPVOID *iface)
{
    *iface = 0;
    if (id == IID_IUnknown)
        *iface = (IUnknown*)this;
    else if (id == IID_IEnumVARIANT)
        *iface = (IEnumVARIANT*)this;

    if (*iface) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE QWindowsEnumerate::AddRef()
{
    return ++ref;
}

ULONG STDMETHODCALLTYPE QWindowsEnumerate::Release()
{
    if (!--ref) {
        delete this;
        return 0;
    }
    return ref;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Clone(IEnumVARIANT **ppEnum)
{
    QWindowsEnumerate *penum = 0;
    *ppEnum = 0;

    penum = new QWindowsEnumerate(array);
    if (!penum)
        return E_OUTOFMEMORY;
    penum->current = current;
    penum->array = array;
    penum->AddRef();
    *ppEnum = penum;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Next(unsigned long  celt, VARIANT FAR*  rgVar, unsigned long FAR*  pCeltFetched)
{
    if (pCeltFetched)
        *pCeltFetched = 0;

    ULONG l;
    for (l = 0; l < celt; l++) {
        VariantInit(&rgVar[l]);
        if ((current+1) > (ULONG)array.size()) {
            *pCeltFetched = l;
            return S_FALSE;
        }

        rgVar[l].vt = VT_I4;
        rgVar[l].lVal = array[(int)current];
        ++current;
    }
    *pCeltFetched = l;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Reset()
{
    current = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Skip(unsigned long celt)
{
    current += celt;
    if (current > (ULONG)array.size()) {
        current = array.size();
        return S_FALSE;
    }
    return S_OK;
}

/*
*/
class QWindowsAccessible : public IAccessible, IOleWindow
{
public:
    QWindowsAccessible(QAccessibleInterface *a)
        : ref(0), accessible(a)
    {
    }

    virtual ~QWindowsAccessible()
    {
        delete accessible;
    }

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IDispatch */
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int *);
    HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int, unsigned long, ITypeInfo **);
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(const _GUID &, wchar_t **, unsigned int, unsigned long, long *);
    HRESULT STDMETHODCALLTYPE Invoke(long, const _GUID &, unsigned long, unsigned short, tagDISPPARAMS *, tagVARIANT *, tagEXCEPINFO *, unsigned int *);

    /* IAccessible */
    HRESULT STDMETHODCALLTYPE accHitTest(long xLeft, long yTop, VARIANT *pvarID);
    HRESULT STDMETHODCALLTYPE accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varID);
    HRESULT STDMETHODCALLTYPE accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEnd);
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT varChildID, IDispatch** ppdispChild);
    HRESULT STDMETHODCALLTYPE get_accChildCount(long* pcountChildren);
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** ppdispParent);

    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT varID);
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT varID, BSTR* pszDefaultAction);
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT varID, BSTR* pszDescription);
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT varID, BSTR *pszHelp);
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT varID, BSTR *pszKeyboardShortcut);
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT varID, BSTR* pszName);
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT varChild, BSTR szName);
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT varID, VARIANT *pvarRole);
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT varID, VARIANT *pvarState);
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT varID, BSTR* pszValue);
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT varChild, BSTR szValue);

    HRESULT STDMETHODCALLTYPE accSelect(long flagsSelect, VARIANT varID);
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT *pvarID);
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT *pvarChildren);

    /* IOleWindow */
    HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

private:
    ULONG ref;
    QAccessibleInterface *accessible;

    QAIPointer childPointer(VARIANT varID)
    {
        return QAIPointer(accessible->child(varID.lVal - 1));
    }
};

static inline BSTR QStringToBSTR(const QString &str)
{
    BSTR bstrVal;

    int wlen = str.length()+1;
    bstrVal = SysAllocStringByteLen(0, wlen*2);
    memcpy(bstrVal, str.unicode(), sizeof(QChar)*(wlen));
    bstrVal[wlen] = 0;

    return bstrVal;
}

/*
*/

/*
  IUnknown
*/
HRESULT STDMETHODCALLTYPE QWindowsAccessible::QueryInterface(REFIID id, LPVOID *iface)
{
    *iface = 0;
    if (id == IID_IUnknown)
        *iface = (IUnknown*)(IDispatch*)this;
    else if (id == IID_IDispatch)
        *iface = (IDispatch*)this;
    else if (id == IID_IAccessible)
        *iface = (IAccessible*)this;
    else if (id == IID_IOleWindow)
        *iface = (IOleWindow*)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE QWindowsAccessible::AddRef()
{
    return ++ref;
}

ULONG STDMETHODCALLTYPE QWindowsAccessible::Release()
{
    if (!--ref) {
        delete this;
        return 0;
    }
    return ref;
}

/*
  IDispatch
*/

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetTypeInfoCount(unsigned int * pctinfo)
{
    // We don't use a type library
    *pctinfo = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetTypeInfo(unsigned int, unsigned long, ITypeInfo **pptinfo)
{
    // We don't use a type library
    *pptinfo = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetIDsOfNames(const _GUID &, wchar_t **rgszNames, unsigned int, unsigned long, long *rgdispid)
{
#if !defined(Q_CC_BOR) && !defined(Q_CC_GNU)
    // PROPERTIES:  Hierarchical
    if (_bstr_t(rgszNames[0]) == _bstr_t(L"accParent"))
        rgdispid[0] = DISPID_ACC_PARENT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accChildCount"))
        rgdispid[0] = DISPID_ACC_CHILDCOUNT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accChild"))
        rgdispid[0] = DISPID_ACC_CHILD;

    // PROPERTIES:  Descriptional
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accName("))
        rgdispid[0] = DISPID_ACC_NAME;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accValue"))
        rgdispid[0] = DISPID_ACC_VALUE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accDescription"))
        rgdispid[0] = DISPID_ACC_DESCRIPTION;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accRole"))
        rgdispid[0] = DISPID_ACC_ROLE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accState"))
        rgdispid[0] = DISPID_ACC_STATE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accHelp"))
        rgdispid[0] = DISPID_ACC_HELP;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accHelpTopic"))
        rgdispid[0] = DISPID_ACC_HELPTOPIC;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accKeyboardShortcut"))
        rgdispid[0] = DISPID_ACC_KEYBOARDSHORTCUT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accFocus"))
        rgdispid[0] = DISPID_ACC_FOCUS;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accSelection"))
        rgdispid[0] = DISPID_ACC_SELECTION;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accDefaultAction"))
        rgdispid[0] = DISPID_ACC_DEFAULTACTION;

    // METHODS
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accSelect"))
        rgdispid[0] = DISPID_ACC_SELECT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accLocation"))
        rgdispid[0] = DISPID_ACC_LOCATION;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accNavigate"))
        rgdispid[0] = DISPID_ACC_NAVIGATE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accHitTest"))
        rgdispid[0] = DISPID_ACC_HITTEST;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accDoDefaultAction"))
        rgdispid[0] = DISPID_ACC_DODEFAULTACTION;
    else
        return DISP_E_UNKNOWNINTERFACE;

    return S_OK;
#else
    Q_UNUSED(rgszNames);
    Q_UNUSED(rgdispid);

    return DISP_E_MEMBERNOTFOUND;
#endif
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::Invoke(long dispIdMember, const _GUID &, unsigned long, unsigned short wFlags, tagDISPPARAMS *pDispParams, tagVARIANT *pVarResult, tagEXCEPINFO *, unsigned int *)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    switch (dispIdMember)
    {
        case DISPID_ACC_PARENT:
            if (wFlags == DISPATCH_PROPERTYGET) {
                if (!pVarResult)
                    return E_INVALIDARG;
                hr = get_accParent(&pVarResult->pdispVal);
            } else {
                hr = DISP_E_MEMBERNOTFOUND;
            }
            break;

        case DISPID_ACC_CHILDCOUNT:
            if (wFlags == DISPATCH_PROPERTYGET) {
                if (!pVarResult)
                    return E_INVALIDARG;
                hr = get_accChildCount(&pVarResult->lVal);
            } else {
                hr = DISP_E_MEMBERNOTFOUND;
            }
            break;

        case DISPID_ACC_CHILD:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accChild(pDispParams->rgvarg[0], &pVarResult->pdispVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_NAME:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accName(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else if (wFlags == DISPATCH_PROPERTYPUT)
                hr = put_accName(pDispParams->rgvarg[0], pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_VALUE:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accValue(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else if (wFlags == DISPATCH_PROPERTYPUT)
                hr = put_accValue(pDispParams->rgvarg[0], pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_DESCRIPTION:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accDescription(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_ROLE:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accRole(pDispParams->rgvarg[0], pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_STATE:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accState(pDispParams->rgvarg[0], pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_HELP:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accHelp(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_HELPTOPIC:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accHelpTopic(&pDispParams->rgvarg[2].bstrVal, pDispParams->rgvarg[1], &pDispParams->rgvarg[0].lVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_KEYBOARDSHORTCUT:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accKeyboardShortcut(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_FOCUS:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accFocus(pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_SELECTION:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accSelection(pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_DEFAULTACTION:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accDefaultAction(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_SELECT:
            if (wFlags == DISPATCH_METHOD)
                hr = accSelect(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0]);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_LOCATION:
            if (wFlags == DISPATCH_METHOD)
                hr = accLocation(&pDispParams->rgvarg[4].lVal, &pDispParams->rgvarg[3].lVal, &pDispParams->rgvarg[2].lVal, &pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0]);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_NAVIGATE:
            if (wFlags == DISPATCH_METHOD)
                hr = accNavigate(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0], pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_HITTEST:
            if (wFlags == DISPATCH_METHOD)
                hr = accHitTest(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].lVal, pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_DODEFAULTACTION:
            if (wFlags == DISPATCH_METHOD)
                hr = accDoDefaultAction(pDispParams->rgvarg[0]);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }

    if (!SUCCEEDED(hr)) {
        return hr;
    }
    return hr;
}

/*
  IAccessible
*/
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accHitTest(long xLeft, long yTop, VARIANT *pvarID)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    int control = accessible->childAt(xLeft, yTop);
    if (control == -1) {
        (*pvarID).vt = VT_EMPTY;
        return S_FALSE;
    }
    QAccessibleInterface *acc = 0;
    if (control)
        accessible->navigate(QAccessible::Child, control, &acc);
    if (!acc) {
        (*pvarID).vt = VT_I4;
        (*pvarID).lVal = control;
        return S_OK;
    }

    QWindowsAccessible* wacc = new QWindowsAccessible(acc);
    IDispatch *iface = 0;
    wacc->QueryInterface(IID_IDispatch, (void**)&iface);
    if (iface) {
        (*pvarID).vt = VT_DISPATCH;
        (*pvarID).pdispVal = iface;
        return S_OK;
    } else {
        delete wacc;
    }

    (*pvarID).vt = VT_EMPTY;
    return S_FALSE;
}

/*
 It is recommended to read
    "Implementing a Microsoft Active Accessibility (MSAA) Server.
    Practical Tips for Developers and How Mozilla Does It"
    (https://developer.mozilla.org/En/Accessibility/Implementing_an_MSAA_Server)

 to get an overview of what's important to implement and what parts of MSAA
 can be ignored. All stuff prefixed with "moz" are information from that page.
*/
// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varID)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QRect rect;
    if (varID.lVal) {
        QAIPointer child = QAIPointer(accessible->child(varID.lVal - 1));
        if (child->isValid())
            rect = child->rect();
    } else {
        rect = accessible->rect();
    }

    *pxLeft = rect.x();
    *pyTop = rect.y();
    *pcxWidth = rect.width();
    *pcyHeight = rect.height();

    return S_OK;
}

// moz: [important, but no need to implement up/down/left/right]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEnd)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *acc = 0;
    int control = -1;
    switch (navDir) {
    case NAVDIR_FIRSTCHILD:
        control = accessible->navigate(QAccessible::Child, 1, &acc);
        break;
    case NAVDIR_LASTCHILD:
        control = accessible->navigate(QAccessible::Child, accessible->childCount(), &acc);
        break;
    case NAVDIR_NEXT:
    case NAVDIR_PREVIOUS:
        if (!varStart.lVal){
            QAccessibleInterface *parent = accessible->parent();
            if (parent) {
                int index = parent->indexOfChild(accessible);
                index += (navDir == NAVDIR_NEXT) ? 1 : -1;
                if (index > 0 && index <= parent->childCount())
                    control = parent->navigate(QAccessible::Child, index, &acc);
                delete parent;
            }
        } else {
            int index = varStart.lVal;
            index += (navDir == NAVDIR_NEXT) ? 1 : -1;
            if (index > 0 && index <= accessible->childCount())
                control = accessible->navigate(QAccessible::Child, index, &acc);
        }
        break;
    case NAVDIR_UP:
        control = accessible->navigate(QAccessible::Up, varStart.lVal, &acc);
        break;
    case NAVDIR_DOWN:
        control = accessible->navigate(QAccessible::Down, varStart.lVal, &acc);
        break;
    case NAVDIR_LEFT:
        control = accessible->navigate(QAccessible::Left, varStart.lVal, &acc);
        break;
    case NAVDIR_RIGHT:
        control = accessible->navigate(QAccessible::Right, varStart.lVal, &acc);
        break;
    default:
        break;
    }
    if (control == -1) {
        (*pvarEnd).vt = VT_EMPTY;
        return S_FALSE;
    }
    if (!acc) {
        (*pvarEnd).vt = VT_I4;
        (*pvarEnd).lVal = control;
        return S_OK;
    }

    QWindowsAccessible* wacc = new QWindowsAccessible(acc);

    IDispatch *iface = 0;
    wacc->QueryInterface(IID_IDispatch, (void**)&iface);
    if (iface) {
        (*pvarEnd).vt = VT_DISPATCH;
        (*pvarEnd).pdispVal = iface;
        return S_OK;
    } else {
        delete wacc;
    }

    (*pvarEnd).vt = VT_EMPTY;
    return S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accChild(VARIANT varChildID, IDispatch** ppdispChild)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    if (varChildID.vt == VT_EMPTY)
        return E_INVALIDARG;


    int childIndex = varChildID.lVal;
    QAccessibleInterface *acc = 0;

    if (childIndex < 0) {
        const int entry = childIndex;
        QPair<QObject*, int> ref = qAccessibleRecentSentEvents()->value(entry);
        if (ref.first) {
            acc = QAccessible::queryAccessibleInterface(ref.first);
            if (acc && ref.second) {
                if (ref.second) {
                    QAccessibleInterface *res;
                    int index = acc->navigate(QAccessible::Child, ref.second, &res);
                    delete acc;
                    if (index == -1)
                        return E_INVALIDARG;
                    acc = res;
                }
            }
        }
    } else {
        QAccessible::RelationFlag rel = childIndex ? QAccessible::Child : QAccessible::Self;
        accessible->navigate(rel, childIndex, &acc);
    }

    if (acc) {
        QWindowsAccessible* wacc = new QWindowsAccessible(acc);
        wacc->QueryInterface(IID_IDispatch, (void**)ppdispChild);
        return S_OK;
    }

    *ppdispChild = 0;
    return S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accChildCount(long* pcountChildren)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    *pcountChildren = accessible->childCount();
    return S_OK;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accParent(IDispatch** ppdispParent)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *acc = accessible->parent();
    if (acc) {
        QWindowsAccessible* wacc = new QWindowsAccessible(acc);
        wacc->QueryInterface(IID_IDispatch, (void**)ppdispParent);

        if (*ppdispParent)
            return S_OK;
    }

    *ppdispParent = 0;
    return S_FALSE;
}

/*
  Properties and methods
*/
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accDoDefaultAction(VARIANT varID)
{
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    if (QAccessibleActionInterface *actionIface = accessible->actionInterface()) {
        const QString def = actionIface->actionNames().value(0);
        if (!def.isEmpty()) {
            actionIface->doAction(def);
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accDefaultAction(VARIANT varID, BSTR* pszDefaultAction)
{
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    *pszDefaultAction = 0;
    if (QAccessibleActionInterface *actionIface = accessible->actionInterface()) {
        const QString def = actionIface->actionNames().value(0);
        if (!def.isEmpty())
            *pszDefaultAction = QStringToBSTR(def);
    }
    return *pszDefaultAction ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accDescription(VARIANT varID, BSTR* pszDescription)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;


    QString descr;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        descr = child->text(QAccessible::Description);
    } else {
        descr = accessible->text(QAccessible::Description);
    }
    if (descr.size()) {
        *pszDescription = QStringToBSTR(descr);
        return S_OK;
    }

    *pszDescription = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accHelp(VARIANT varID, BSTR *pszHelp)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QString help;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        help = child->text(QAccessible::Help);
    } else {
        help = accessible->text(QAccessible::Help);
    }
    if (help.size()) {
        *pszHelp = QStringToBSTR(help);
        return S_OK;
    }

    *pszHelp = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accHelpTopic(BSTR *, VARIANT, long *)
{
    return DISP_E_MEMBERNOTFOUND;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accKeyboardShortcut(VARIANT varID, BSTR *pszKeyboardShortcut)
{
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    *pszKeyboardShortcut = 0;
    if (QAccessibleActionInterface *actionIface = accessible->actionInterface()) {
        const QString def = actionIface->actionNames().value(0);
        if (!def.isEmpty()) {
            const QString keyBoardShortCut = actionIface->keyBindingsForAction(def).value(0);
            if (!keyBoardShortCut.isEmpty())
                *pszKeyboardShortcut = QStringToBSTR(keyBoardShortCut);
        }
    }
    return *pszKeyboardShortcut ? S_OK : S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accName(VARIANT varID, BSTR* pszName)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QString name;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        name = child->text(QAccessible::Name);
    } else {
        name = accessible->text(QAccessible::Name);
    }
    if (name.size()) {
        *pszName = QStringToBSTR(name);
        return S_OK;
    }

    *pszName = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::put_accName(VARIANT, BSTR)
{
    showDebug(__FUNCTION__, accessible);
    return DISP_E_MEMBERNOTFOUND;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accRole(VARIANT varID, VARIANT *pvarRole)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessible::Role role;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        role = child->role();
    } else {
        role = accessible->role();
    }

    if (role != QAccessible::NoRole) {
        if (role == QAccessible::LayeredPane)
            role = QAccessible::Pane;
        (*pvarRole).vt = VT_I4;
        (*pvarRole).lVal = role;
    } else {
        (*pvarRole).vt = VT_EMPTY;
    }
    return S_OK;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accState(VARIANT varID, VARIANT *pvarState)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessible::State state;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child.data())
            return E_FAIL;
        state = child->state();
    } else {
        state = accessible->state();
    }

    (*pvarState).vt = VT_I4;
    (*pvarState).lVal = state;
    return S_OK;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accValue(VARIANT varID, BSTR* pszValue)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid() || varID.lVal)
        return E_FAIL;

    QString value;
    if (accessible->valueInterface()) {
        value = QString::number(accessible->valueInterface()->currentValue().toDouble());
    } else {
        value = accessible->text(QAccessible::Value);
    }
    if (!value.isNull()) {
        *pszValue = QStringToBSTR(value);
        return S_OK;
    }

    *pszValue = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::put_accValue(VARIANT, BSTR)
{
    showDebug(__FUNCTION__, accessible);
    return DISP_E_MEMBERNOTFOUND;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accSelect(long flagsSelect, VARIANT varID)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    bool res = false;

/*
  ### Check for accessibleTableInterface() or accessibleTextInterface()

  ### and if there are no ia2 interfaces we should do nothing??
    if (flagsSelect & SELFLAG_TAKEFOCUS)
        res = accessible->doAction(SetFocus, varID.lVal, QVariantList());
    if (flagsSelect & SELFLAG_TAKESELECTION) {
        accessible->doAction(ClearSelection, 0, QVariantList());
        res = accessible->doAction(AddToSelection, varID.lVal, QVariantList());
    }
    if (flagsSelect & SELFLAG_EXTENDSELECTION)
        res = accessible->doAction(ExtendSelection, varID.lVal, QVariantList());
    if (flagsSelect & SELFLAG_ADDSELECTION)
        res = accessible->doAction(AddToSelection, varID.lVal, QVariantList());
    if (flagsSelect & SELFLAG_REMOVESELECTION)
        res = accessible->doAction(RemoveSelection, varID.lVal, QVariantList());
*/
    return res ? S_OK : S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accFocus(VARIANT *pvarID)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *acc = 0;
    int control = accessible->navigate(QAccessible::FocusChild, 1, &acc);
    if (control == -1) {
        (*pvarID).vt = VT_EMPTY;
        return S_FALSE;
    }
    if (!acc || control == 0) {
        (*pvarID).vt = VT_I4;
        (*pvarID).lVal = control ? control : CHILDID_SELF;
        return S_OK;
    }

    QWindowsAccessible* wacc = new QWindowsAccessible(acc);
    IDispatch *iface = 0;
    wacc->QueryInterface(IID_IDispatch, (void**)&iface);
    if (iface) {
        (*pvarID).vt = VT_DISPATCH;
        (*pvarID).pdispVal = iface;
        return S_OK;
    } else {
        delete wacc;
    }

    (*pvarID).vt = VT_EMPTY;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accSelection(VARIANT *pvarChildren)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    int cc = accessible->childCount();
    QVector<int> sel(cc);
    int selIndex = 0;
    for (int i = 0; i < cc; ++i) {
        bool isSelected = false;
        QAccessibleInterface *child = accessible->child(i);
        if (child) {
            isSelected = child->state() & QAccessible::Selected;
            delete child;
        }
        if (isSelected)
            sel[selIndex++] = i+1;
    }
    sel.resize(selIndex);
    if (sel.isEmpty()) {
        (*pvarChildren).vt = VT_EMPTY;
        return S_FALSE;
    }
    if (sel.size() == 1) {
        (*pvarChildren).vt = VT_I4;
        (*pvarChildren).lVal = sel[0];
        return S_OK;
    }
    IEnumVARIANT *iface = new QWindowsEnumerate(sel);
    IUnknown *uiface;
    iface->QueryInterface(IID_IUnknown, (void**)&uiface);
    (*pvarChildren).vt = VT_UNKNOWN;
    (*pvarChildren).punkVal = uiface;

    return S_OK;
}

static QWindow *window_helper(const QAccessibleInterface *iface)
{
    QWindow *window = iface->window();
    if (!window) {
        QAccessibleInterface *acc = iface->parent();
        while (acc && !window) {
            window = acc->window();
            QAccessibleInterface *par = acc->parent();
            delete acc;
            acc = par;
        }
    }
    return window;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetWindow(HWND *phwnd)
{
    *phwnd = 0;
    if (!accessible->isValid())
        return E_UNEXPECTED;

    QWindow *window = window_helper(accessible);
    if (!window)
        return E_FAIL;

    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    Q_ASSERT(platform);
    *phwnd = (HWND)platform->nativeResourceForWindow("handle", window);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::ContextSensitiveHelp(BOOL)
{
    return S_OK;
}


QWindowsAccessibility::QWindowsAccessibility()
{
}


void QWindowsAccessibility::notifyAccessibilityUpdate(QObject *o, int who, QAccessible::Event reason)
{
    QString soundName;
    switch (reason) {
    case QAccessible::PopupMenuStart:
        soundName = QLatin1String("MenuPopup");
        break;

    case QAccessible::MenuCommand:
        soundName = QLatin1String("MenuCommand");
        break;

    case QAccessible::Alert:
        {
        /*      ### FIXME
#ifndef QT_NO_MESSAGEBOX
            QMessageBox *mb = qobject_cast<QMessageBox*>(o);
            if (mb) {
                switch (mb->icon()) {
                case QMessageBox::Warning:
                    soundName = QLatin1String("SystemExclamation");
                    break;
                case QMessageBox::Critical:
                    soundName = QLatin1String("SystemHand");
                    break;
                case QMessageBox::Information:
                    soundName = QLatin1String("SystemAsterisk");
                    break;
                default:
                    break;
                }
            } else
#endif // QT_NO_MESSAGEBOX
*/
            {
                soundName = QLatin1String("SystemAsterisk");
            }

        }
        break;
    default:
        break;
    }

    if (!soundName.isEmpty()) {
#ifndef QT_NO_SETTINGS
        QSettings settings(QLatin1String("HKEY_CURRENT_USER\\AppEvents\\Schemes\\Apps\\.Default\\") + soundName,
                           QSettings::NativeFormat);
        QString file = settings.value(QLatin1String(".Current/.")).toString();
#else
        QString file;
#endif
        if (!file.isEmpty()) {
            PlaySound(reinterpret_cast<const wchar_t *>(soundName.utf16()), 0, SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
        }
    }

    typedef void (WINAPI *PtrNotifyWinEvent)(DWORD, HWND, LONG, LONG);

#if defined(Q_WS_WINCE) // ### TODO: check for NotifyWinEvent in CE 6.0
    // There is no user32.lib nor NotifyWinEvent for CE
    return;
#else
    static PtrNotifyWinEvent ptrNotifyWinEvent = 0;
    static bool resolvedNWE = false;
    if (!resolvedNWE) {
        resolvedNWE = true;
        ptrNotifyWinEvent = (PtrNotifyWinEvent)QSystemLibrary::resolve(QLatin1String("user32"), "NotifyWinEvent");
    }
    if (!ptrNotifyWinEvent)
        return;

    // An event has to be associated with a window,
    // so find the first parent that is a widget and that has a WId
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
    QWindow *window = window_helper(iface);

    if (!window) {
        window = QGuiApplication::activeWindow();
        if (!window)
            return;
    }

    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    HWND hWnd = (HWND)platform->nativeResourceForWindow("handle", window);

    if (reason != QAccessible::MenuCommand) { // MenuCommand is faked
        // See comment "SENDING EVENTS TO OBJECTS WITH NO WINDOW HANDLE"
        eventNum %= 50;              //[0..49]
        int eventId = - eventNum - 1;

        qAccessibleRecentSentEvents()->insert(eventId, qMakePair(o, who));
        ptrNotifyWinEvent(reason, hWnd, OBJID_CLIENT, eventId );

        ++eventNum;
    }
#endif // Q_WS_WINCE

}

/*
void QWindowsAccessibility::setRootObject(QObject *o)
{

}

void QWindowsAccessibility::initialize()
{

}

void QWindowsAccessibility::cleanup()
{

}

*/

bool QWindowsAccessibility::handleAccessibleObjectFromWindowRequest(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
        /* For UI Automation
      */
    } else if ((DWORD)lParam == OBJID_CLIENT) {
#if 1
        // Ignoring all requests while starting up
        // ### Maybe QPA takes care of this???
        if (QApplication::startingUp() || QApplication::closingDown())
            return false;
#endif

        typedef LRESULT (WINAPI *PtrLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);
        static PtrLresultFromObject ptrLresultFromObject = 0;
        static bool oleaccChecked = false;

        if (!oleaccChecked) {
            oleaccChecked = true;
#if !defined(Q_OS_WINCE)
            ptrLresultFromObject = (PtrLresultFromObject)QSystemLibrary::resolve(QLatin1String("oleacc"), "LresultFromObject");
#endif
        }

        if (ptrLresultFromObject) {
            QWindow *window = QWindowsContext::instance()->findWindow(hwnd);
            if (window) {
                QAccessibleInterface *acc = window->accessibleRoot();
                if (acc) {
                    QWindowsAccessible *winacc = new QWindowsAccessible(acc);
                    IAccessible *iface;
                    HRESULT hr = winacc->QueryInterface(IID_IAccessible, (void**)&iface);
                    if (SUCCEEDED(hr)) {
                        *lResult = ptrLresultFromObject(IID_IAccessible, wParam, iface);  // ref == 2
                        if (*lResult) {
                            iface->Release(); // the client will release the object again, and then it will destroy itself
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
