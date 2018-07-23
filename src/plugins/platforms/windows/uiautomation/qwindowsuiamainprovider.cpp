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

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsuiamainprovider.h"
#include "qwindowsuiavalueprovider.h"
#include "qwindowsuiarangevalueprovider.h"
#include "qwindowsuiatextprovider.h"
#include "qwindowsuiatoggleprovider.h"
#include "qwindowsuiainvokeprovider.h"
#include "qwindowsuiaselectionprovider.h"
#include "qwindowsuiaselectionitemprovider.h"
#include "qwindowsuiatableprovider.h"
#include "qwindowsuiatableitemprovider.h"
#include "qwindowsuiagridprovider.h"
#include "qwindowsuiagriditemprovider.h"
#include "qwindowscombase.h"
#include "qwindowscontext.h"
#include "qwindowsuiautils.h"
#include "qwindowsuiaprovidercache.h"

#include <QtCore/QDebug>
#include <QtGui/QAccessible>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

#if !defined(Q_CC_BOR) && !defined (Q_CC_GNU)
#include <comdef.h>
#endif

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


// Returns a cached instance of the provider for a specific acessible interface.
QWindowsUiaMainProvider *QWindowsUiaMainProvider::providerForAccessible(QAccessibleInterface *accessible)
{
    if (!accessible)
        return nullptr;

    QAccessible::Id id = QAccessible::uniqueId(accessible);
    QWindowsUiaProviderCache *providerCache = QWindowsUiaProviderCache::instance();
    QWindowsUiaMainProvider *provider = qobject_cast<QWindowsUiaMainProvider *>(providerCache->providerForId(id));

    if (provider) {
        provider->AddRef();
    } else {
        provider = new QWindowsUiaMainProvider(accessible);
        providerCache->insert(id, provider);
    }
    return provider;
}

QWindowsUiaMainProvider::QWindowsUiaMainProvider(QAccessibleInterface *a, int initialRefCount)
    : QWindowsUiaBaseProvider(QAccessible::uniqueId(a)),
      m_ref(initialRefCount)
{
}

QWindowsUiaMainProvider::~QWindowsUiaMainProvider()
{
}

void QWindowsUiaMainProvider::notifyFocusChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (QWindowsUiaMainProvider *provider = providerForAccessible(accessible)) {
            QWindowsUiaWrapper::instance()->raiseAutomationEvent(provider, UIA_AutomationFocusChangedEventId);
        }
    }
}

void QWindowsUiaMainProvider::notifyStateChange(QAccessibleStateChangeEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (event->changedStates().checked || event->changedStates().checkStateMixed) {
           // Notifies states changes in checkboxes.
           if (accessible->role() == QAccessible::CheckBox) {
                if (QWindowsUiaMainProvider *provider = providerForAccessible(accessible)) {
                    VARIANT oldVal, newVal;
                    clearVariant(&oldVal);
                    int toggleState = ToggleState_Off;
                    if (accessible->state().checked)
                        toggleState = accessible->state().checkStateMixed ? ToggleState_Indeterminate : ToggleState_On;
                    setVariantI4(toggleState, &newVal);
                    QWindowsUiaWrapper::instance()->raiseAutomationPropertyChangedEvent(provider, UIA_ToggleToggleStatePropertyId, oldVal, newVal);
                }
            }
        }
        if (event->changedStates().active) {
            if (accessible->role() == QAccessible::Window) {
                // Notifies window opened/closed.
                if (QWindowsUiaMainProvider *provider = providerForAccessible(accessible)) {
                    if (accessible->state().active) {
                        QWindowsUiaWrapper::instance()->raiseAutomationEvent(provider, UIA_Window_WindowOpenedEventId);
                    } else {
                        QWindowsUiaWrapper::instance()->raiseAutomationEvent(provider, UIA_Window_WindowClosedEventId);
                    }
                }
            }
        }
    }
}

void QWindowsUiaMainProvider::notifyValueChange(QAccessibleValueChangeEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
       if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
           // Notifies changes in values of controls supporting the value interface.
            if (QWindowsUiaMainProvider *provider = providerForAccessible(accessible)) {
                VARIANT oldVal, newVal;
                clearVariant(&oldVal);
                setVariantDouble(valueInterface->currentValue().toDouble(), &newVal);
                QWindowsUiaWrapper::instance()->raiseAutomationPropertyChangedEvent(provider, UIA_RangeValueValuePropertyId, oldVal, newVal);
            }
        }
    }
}

// Notifies changes in text content and selection state of text controls.
void QWindowsUiaMainProvider::notifyTextChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (accessible->textInterface()) {
            if (QWindowsUiaMainProvider *provider = providerForAccessible(accessible)) {
                if (event->type() == QAccessible::TextSelectionChanged) {
                    QWindowsUiaWrapper::instance()->raiseAutomationEvent(provider, UIA_Text_TextSelectionChangedEventId);
                } else if (event->type() == QAccessible::TextCaretMoved) {
                    if (!accessible->state().readOnly) {
                        QWindowsUiaWrapper::instance()->raiseAutomationEvent(provider, UIA_Text_TextSelectionChangedEventId);
                    }
                } else {
                    QWindowsUiaWrapper::instance()->raiseAutomationEvent(provider, UIA_Text_TextChangedEventId);
                }
            }
        }
    }
}

HRESULT STDMETHODCALLTYPE QWindowsUiaMainProvider::QueryInterface(REFIID iid, LPVOID *iface)
{
    if (!iface)
        return E_INVALIDARG;
    *iface = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();

    const bool result = qWindowsComQueryUnknownInterfaceMulti<IRawElementProviderSimple>(this, iid, iface)
        || qWindowsComQueryInterface<IRawElementProviderSimple>(this, iid, iface)
        || qWindowsComQueryInterface<IRawElementProviderFragment>(this, iid, iface)
        || (accessible && hwndForAccessible(accessible) && qWindowsComQueryInterface<IRawElementProviderFragmentRoot>(this, iid, iface));
    return result ? S_OK : E_NOINTERFACE;
}

ULONG QWindowsUiaMainProvider::AddRef()
{
    return ++m_ref;
}

ULONG STDMETHODCALLTYPE QWindowsUiaMainProvider::Release()
{
    if (!--m_ref) {
        delete this;
        return 0;
    }
    return m_ref;
}

HRESULT QWindowsUiaMainProvider::get_ProviderOptions(ProviderOptions *pRetVal)
{
    if (!pRetVal)
        return E_INVALIDARG;
    // We are STA, (OleInitialize()).
    *pRetVal = static_cast<ProviderOptions>(ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading);
    return S_OK;
}

// Return providers for specific control patterns
HRESULT QWindowsUiaMainProvider::GetPatternProvider(PATTERNID idPattern, IUnknown **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << idPattern;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    switch (idPattern) {
    case UIA_TextPatternId:
    case UIA_TextPattern2Id:
        // All text controls.
        if (accessible->textInterface()) {
            *pRetVal = new QWindowsUiaTextProvider(id());
        }
        break;
    case UIA_ValuePatternId:
        // All accessible controls return text(QAccessible::Value) (which may be empty).
        *pRetVal = new QWindowsUiaValueProvider(id());
        break;
    case UIA_RangeValuePatternId:
        // Controls providing a numeric value within a range (e.g., sliders, scroll bars, dials).
        if (accessible->valueInterface()) {
            *pRetVal = new QWindowsUiaRangeValueProvider(id());
        }
        break;
    case UIA_TogglePatternId:
        // Checkbox controls.
        if (accessible->role() == QAccessible::CheckBox) {
            *pRetVal = new QWindowsUiaToggleProvider(id());
        }
        break;
    case UIA_SelectionPatternId:
        // Lists of items.
        if (accessible->role() == QAccessible::List) {
            *pRetVal = new QWindowsUiaSelectionProvider(id());
        }
        break;
    case UIA_SelectionItemPatternId:
        // Items within a list and radio buttons.
        if ((accessible->role() == QAccessible::RadioButton)
                || (accessible->role() == QAccessible::ListItem)) {
            *pRetVal = new QWindowsUiaSelectionItemProvider(id());
        }
        break;
    case UIA_TablePatternId:
        // Table/tree.
        if (accessible->tableInterface()
                && ((accessible->role() == QAccessible::Table) || (accessible->role() == QAccessible::Tree))) {
            *pRetVal = new QWindowsUiaTableProvider(id());
        }
        break;
    case UIA_TableItemPatternId:
        // Item within a table/tree.
        if (accessible->tableCellInterface()
                && ((accessible->role() == QAccessible::Cell) || (accessible->role() == QAccessible::TreeItem))) {
            *pRetVal = new QWindowsUiaTableItemProvider(id());
        }
        break;
    case UIA_GridPatternId:
        // Table/tree.
        if (accessible->tableInterface()
                && ((accessible->role() == QAccessible::Table) || (accessible->role() == QAccessible::Tree))) {
            *pRetVal = new QWindowsUiaGridProvider(id());
        }
        break;
    case UIA_GridItemPatternId:
        // Item within a table/tree.
        if (accessible->tableCellInterface()
                && ((accessible->role() == QAccessible::Cell) || (accessible->role() == QAccessible::TreeItem))) {
            *pRetVal = new QWindowsUiaGridItemProvider(id());
        }
        break;
    case UIA_InvokePatternId:
        // Things that have an invokable action (e.g., simple buttons).
        if (accessible->actionInterface()) {
            *pRetVal = new QWindowsUiaInvokeProvider(id());
        }
        break;
    default:
        break;
    }

    return S_OK;
}

HRESULT QWindowsUiaMainProvider::GetPropertyValue(PROPERTYID idProp, VARIANT *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << idProp;

    if (!pRetVal)
        return E_INVALIDARG;
    clearVariant(pRetVal);

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    bool clientTopLevel = (accessible->role() == QAccessible::Client)
            && accessible->parent() && (accessible->parent()->role() == QAccessible::Application);

    switch (idProp) {
    case UIA_ProcessIdPropertyId:
        // PID
        setVariantI4(int(GetCurrentProcessId()), pRetVal);
        break;
    case UIA_AccessKeyPropertyId:
        // Accelerator key.
        setVariantString(accessible->text(QAccessible::Accelerator), pRetVal);
        break;
    case UIA_AutomationIdPropertyId:
        // Automation ID, which can be used by tools to select a specific control in the UI.
        setVariantString(automationIdForAccessible(accessible), pRetVal);
        break;
    case UIA_ClassNamePropertyId:
        // Class name.
        if (QObject *o = accessible->object()) {
            QString className = QLatin1String(o->metaObject()->className());
            setVariantString(className, pRetVal);
        }
        break;
    case UIA_FrameworkIdPropertyId:
        setVariantString(QStringLiteral("Qt"), pRetVal);
        break;
    case UIA_ControlTypePropertyId:
        if (clientTopLevel) {
            // Reports a top-level widget as a window, instead of "custom".
            setVariantI4(UIA_WindowControlTypeId, pRetVal);
        } else {
            // Control type converted from role.
            setVariantI4(roleToControlTypeId(accessible->role()), pRetVal);
        }
        break;
    case UIA_HelpTextPropertyId:
        setVariantString(accessible->text(QAccessible::Help), pRetVal);
        break;
    case UIA_HasKeyboardFocusPropertyId:
        setVariantBool(accessible->state().focused, pRetVal);
        break;
    case UIA_IsKeyboardFocusablePropertyId:
        setVariantBool(accessible->state().focusable, pRetVal);
        break;
    case UIA_IsOffscreenPropertyId:
        setVariantBool(accessible->state().offscreen, pRetVal);
        break;
    case UIA_IsContentElementPropertyId:
        setVariantBool(true, pRetVal);
        break;
    case UIA_IsControlElementPropertyId:
        setVariantBool(true, pRetVal);
        break;
    case UIA_IsEnabledPropertyId:
        setVariantBool(!accessible->state().disabled, pRetVal);
        break;
    case UIA_IsPasswordPropertyId:
        setVariantBool(accessible->role() == QAccessible::EditableText
                       && accessible->state().passwordEdit, pRetVal);
        break;
    case UIA_IsPeripheralPropertyId:
        // True for peripheral UIs.
        if (QWindow *window = windowForAccessible(accessible)) {
            const Qt::WindowType wt = window->type();
            setVariantBool(wt == Qt::Popup || wt == Qt::ToolTip || wt == Qt::SplashScreen, pRetVal);
        }
        break;
    case UIA_FullDescriptionPropertyId:
        setVariantString(accessible->text(QAccessible::Description), pRetVal);
        break;
    case UIA_NamePropertyId: {
        QString name = accessible->text(QAccessible::Name);
        if (name.isEmpty() && clientTopLevel) {
           name = QCoreApplication::applicationName();
        }
        setVariantString(name, pRetVal);
        break;
    }
    default:
        break;
    }
    return S_OK;
}

// Generates an ID based on the name of the controls and their parents.
QString QWindowsUiaMainProvider::automationIdForAccessible(const QAccessibleInterface *accessible)
{
    QString result;
    if (accessible) {
        QObject *obj = accessible->object();
        while (obj) {
            QString name = obj->objectName();
            if (name.isEmpty())
                return QString();
            if (!result.isEmpty())
                result.prepend(QLatin1Char('.'));
            result.prepend(name);
            obj = obj->parent();
        }
    }
    return result;
}

HRESULT QWindowsUiaMainProvider::get_HostRawElementProvider(IRawElementProviderSimple **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    // Returns a host provider only for controls associated with a native window handle. Others should return NULL.
    if (QAccessibleInterface *accessible = accessibleInterface()) {
        if (HWND hwnd = hwndForAccessible(accessible)) {
            return QWindowsUiaWrapper::instance()->hostProviderFromHwnd(hwnd, pRetVal);
        }
    }
    return S_OK;
}

// Navigates within the tree of accessible controls.
HRESULT QWindowsUiaMainProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << direction << " this: " << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleInterface *targetacc = nullptr;

    if (direction == NavigateDirection_Parent) {
        if (QAccessibleInterface *parent = accessible->parent()) {
            // The Application's children are considered top level objects.
            if (parent->isValid() && parent->role() != QAccessible::Application) {
                targetacc = parent;
            }
        }
    } else {
        QAccessibleInterface *parent = nullptr;
        int index = 0;
        int incr = 1;
        switch (direction) {
        case NavigateDirection_FirstChild:
            parent = accessible;
            index = 0;
            incr = 1;
            break;
        case NavigateDirection_LastChild:
            parent = accessible;
            index = accessible->childCount() - 1;
            incr = -1;
            break;
        case NavigateDirection_NextSibling:
            if ((parent = accessible->parent()))
                index = parent->indexOfChild(accessible) + 1;
            incr = 1;
            break;
        case NavigateDirection_PreviousSibling:
            if ((parent = accessible->parent()))
                index = parent->indexOfChild(accessible) - 1;
            incr = -1;
            break;
        default:
            Q_UNREACHABLE();
            break;
        }

        if (parent && parent->isValid()) {
            for (int count = parent->childCount(); index >= 0 && index < count; index += incr) {
                if (QAccessibleInterface *child = parent->child(index)) {
                    if (child->isValid() && !child->state().invisible) {
                        targetacc = child;
                        break;
                    }
                }
            }
        }
    }

    if (targetacc)
        *pRetVal = providerForAccessible(targetacc);
    return S_OK;
}

// Returns a unique id assigned to the UI element, used as key by the UI Automation framework.
HRESULT QWindowsUiaMainProvider::GetRuntimeId(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // The UiaAppendRuntimeId constant is used to make then ID unique
    // among multiple instances running on the system.
    int rtId[] = { UiaAppendRuntimeId, int(id()) };

    if ((*pRetVal = SafeArrayCreateVector(VT_I4, 0, 2))) {
        for (LONG i = 0; i < 2; ++i)
            SafeArrayPutElement(*pRetVal, &i, &rtId[i]);
    }
    return S_OK;
}

// Returns the bounding rectangle for the accessible control.
HRESULT QWindowsUiaMainProvider::get_BoundingRectangle(UiaRect *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QWindow *window = windowForAccessible(accessible);
    if (!window)
        return UIA_E_ELEMENTNOTAVAILABLE;

    rectToNativeUiaRect(accessible->rect(), window, pRetVal);
    return S_OK;
}

HRESULT QWindowsUiaMainProvider::GetEmbeddedFragmentRoots(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;
    // No embedded roots.
    return S_OK;
}

// Sets focus to the control.
HRESULT QWindowsUiaMainProvider::SetFocus()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    actionInterface->doAction(QAccessibleActionInterface::setFocusAction());
    return S_OK;
}

HRESULT QWindowsUiaMainProvider::get_FragmentRoot(IRawElementProviderFragmentRoot **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    // Our UI Automation implementation considers the window as the root for
    // non-native controls/fragments.
    if (QAccessibleInterface *accessible = accessibleInterface()) {
        if (QWindow *window = windowForAccessible(accessible)) {
            if (QAccessibleInterface *rootacc = window->accessibleRoot()) {
                *pRetVal = providerForAccessible(rootacc);
            }
        }
    }
    return S_OK;
}

// Returns a provider for the UI element present at the specified screen coordinates.
HRESULT QWindowsUiaMainProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << x << y;

    if (!pRetVal) {
        return E_INVALIDARG;
    }
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QWindow *window = windowForAccessible(accessible);
    if (!window)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // Scales coordinates from High DPI screens.
    UiaPoint uiaPoint = {x, y};
    QPoint point;
    nativeUiaPointToPoint(uiaPoint, window, &point);

    QAccessibleInterface *targetacc = accessible->childAt(point.x(), point.y());

    if (targetacc) {
        QAccessibleInterface *acc = targetacc;
        // Controls can be embedded within grouping elements. By default returns the innermost control.
        while (acc) {
            targetacc = acc;
            // For accessibility tools it may be better to return the text element instead of its subcomponents.
            if (targetacc->textInterface()) break;
            acc = acc->childAt(point.x(), point.y());
        }
        *pRetVal = providerForAccessible(targetacc);
    }
    return S_OK;
}

// Returns the fragment with focus.
HRESULT QWindowsUiaMainProvider::GetFocus(IRawElementProviderFragment **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__ << this;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    if (QAccessibleInterface *accessible = accessibleInterface()) {
        if (QAccessibleInterface *focusacc = accessible->focusChild()) {
            *pRetVal = providerForAccessible(focusacc);
        }
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
