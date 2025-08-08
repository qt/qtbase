// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

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
#include "qwindowsuiawindowprovider.h"
#include "qwindowsuiaexpandcollapseprovider.h"
#include "qwindowscontext.h"
#include "qwindowsuiautils.h"
#include "qwindowsuiaprovidercache.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qaccessiblebridgeutils_p.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <QtCore/private/qcomvariant_p.h>

#if !defined(Q_CC_BOR) && !defined (Q_CC_GNU)
#include <comdef.h>
#endif

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;

// Returns a cached instance of the provider for a specific accessible interface.
ComPtr<QWindowsUiaMainProvider> QWindowsUiaMainProvider::providerForAccessible(QAccessibleInterface *accessible)
{
    if (!accessible)
        return nullptr;

    QAccessible::Id id = QAccessible::uniqueId(accessible);
    QWindowsUiaProviderCache *providerCache = QWindowsUiaProviderCache::instance();
    ComPtr<QWindowsUiaMainProvider> provider = providerCache->providerForId(id);

    if (!provider) {
        provider = makeComObject<QWindowsUiaMainProvider>(accessible);
        providerCache->insert(id, provider.Get()); // Cache holds weak references
    }
    return provider;
}

QWindowsUiaMainProvider::QWindowsUiaMainProvider(QAccessibleInterface *a)
    : QWindowsUiaBaseProvider(QAccessible::uniqueId(a))
{
}

QWindowsUiaMainProvider::~QWindowsUiaMainProvider()
{
}

void QWindowsUiaMainProvider::notifyFocusChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        // If this is a complex element, raise event for the focused child instead.
        if (accessible->childCount()) {
            if (QAccessibleInterface *child = accessible->focusChild())
                accessible = child;
        }
        if (auto provider = providerForAccessible(accessible))
            UiaRaiseAutomationEvent(provider.Get(), UIA_AutomationFocusChangedEventId);
    }
}

void QWindowsUiaMainProvider::notifyStateChange(QAccessibleStateChangeEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (event->changedStates().checked || event->changedStates().checkStateMixed) {
           // Notifies states changes in checkboxes.
           if (accessible->role() == QAccessible::CheckBox) {
                if (auto provider = providerForAccessible(accessible)) {
                    long toggleState = ToggleState_Off;
                    if (accessible->state().checked)
                        toggleState = accessible->state().checkStateMixed ? ToggleState_Indeterminate : ToggleState_On;

                    QComVariant oldVal;
                    QComVariant newVal{toggleState};
                    UiaRaiseAutomationPropertyChangedEvent(
                            provider.Get(), UIA_ToggleToggleStatePropertyId, oldVal.get(), newVal.get());
                }
            }
        }
        if (event->changedStates().active) {
            if (accessible->role() == QAccessible::Window) {
                // Notifies window opened/closed.
                if (auto provider = providerForAccessible(accessible)) {
                    if (accessible->state().active) {
                        UiaRaiseAutomationEvent(provider.Get(), UIA_Window_WindowOpenedEventId);
                        if (QAccessibleInterface *focused = accessible->focusChild()) {
                            if (auto focusedProvider = providerForAccessible(focused)) {
                                UiaRaiseAutomationEvent(focusedProvider.Get(),
                                                        UIA_AutomationFocusChangedEventId);
                            }
                        }
                    } else {
                        UiaRaiseAutomationEvent(provider.Get(), UIA_Window_WindowClosedEventId);
                    }
                }
            }
        }
    }
}

void QWindowsUiaMainProvider::notifyValueChange(QAccessibleValueChangeEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (accessible->role() == QAccessible::ComboBox && accessible->childCount() > 0) {
            QAccessibleInterface *listacc = accessible->child(0);
            if (listacc && listacc->role() == QAccessible::List) {
                int count = listacc->childCount();
                for (int i = 0; i < count; ++i) {
                    QAccessibleInterface *item = listacc->child(i);
                    if (item && item->isValid() && item->text(QAccessible::Name) == event->value()) {
                        if (!item->state().selected) {
                            if (QAccessibleActionInterface *actionInterface = item->actionInterface())
                                actionInterface->doAction(QAccessibleActionInterface::toggleAction());
                        }
                        break;
                    }
                }
            }
        }
        if (event->value().typeId() == QMetaType::QString) {
            if (auto provider = providerForAccessible(accessible)) {
                // Notifies changes in string values.
                const QComVariant oldVal;
                const QComVariant newVal{ event->value().toString() };
                UiaRaiseAutomationPropertyChangedEvent(provider.Get(), UIA_ValueValuePropertyId,
                                                       oldVal.get(), newVal.get());
            }
        } else if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
            if (auto provider = providerForAccessible(accessible)) {
                // Notifies changes in values of controls supporting the value interface.
                const QComVariant oldVal;
                const QComVariant newVal{ valueInterface->currentValue().toDouble() };
                UiaRaiseAutomationPropertyChangedEvent(
                        provider.Get(), UIA_RangeValueValuePropertyId, oldVal.get(), newVal.get());
            }
        }
    }
}

void QWindowsUiaMainProvider::notifyNameChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        // Restrict notification to combo boxes and the currently focused element, which
        // need it for accessibility, in order to avoid slowdowns with unnecessary notifications.
        if (accessible->role() == QAccessible::ComboBox || accessible->state().focused) {
            if (auto provider = providerForAccessible(accessible)) {
                QComVariant oldVal;
                QComVariant newVal{ accessible->text(QAccessible::Name) };
                UiaRaiseAutomationPropertyChangedEvent(provider.Get(), UIA_NamePropertyId,
                                                       oldVal.get(), newVal.get());
            }
        }
    }
}

void QWindowsUiaMainProvider::notifySelectionChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (auto provider = providerForAccessible(accessible))
            UiaRaiseAutomationEvent(provider.Get(), UIA_SelectionItem_ElementSelectedEventId);
    }
}

// Notifies changes in text content and selection state of text controls.
void QWindowsUiaMainProvider::notifyTextChange(QAccessibleEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (accessible->textInterface()) {
            if (auto provider = providerForAccessible(accessible)) {
                if (event->type() == QAccessible::TextSelectionChanged) {
                    UiaRaiseAutomationEvent(provider.Get(), UIA_Text_TextSelectionChangedEventId);
                } else if (event->type() == QAccessible::TextCaretMoved) {
                    if (!accessible->state().readOnly) {
                        UiaRaiseAutomationEvent(provider.Get(),
                                                UIA_Text_TextSelectionChangedEventId);
                    }
                } else {
                    UiaRaiseAutomationEvent(provider.Get(), UIA_Text_TextChangedEventId);
                }
            }
        }
    }
}

void QWindowsUiaMainProvider::raiseNotification(QAccessibleAnnouncementEvent *event)
{
    if (QAccessibleInterface *accessible = event->accessibleInterface()) {
        if (auto provider = providerForAccessible(accessible)) {
            QBStr message{ event->message() };
            QAccessible::AnnouncementPoliteness prio = event->politeness();
            NotificationProcessing processing = (prio == QAccessible::AnnouncementPoliteness::Assertive)
                    ? NotificationProcessing_ImportantAll
                    : NotificationProcessing_All;
            QBStr activityId{ QString::fromLatin1("") };
#if !defined(Q_CC_MSVC) || !defined(QT_WIN_SERVER_2016_COMPAT)
            UiaRaiseNotificationEvent(provider.Get(), NotificationKind_Other, processing, message.bstr(),
                                      activityId.bstr());
#else
            HMODULE uiautomationcore = GetModuleHandleW(L"UIAutomationCore.dll");
            if (uiautomationcore != NULL) {
                typedef HRESULT (WINAPI *EVENTFUNC)(IRawElementProviderSimple *, NotificationKind,
                                                    NotificationProcessing, BSTR, BSTR);

                EVENTFUNC uiaRaiseNotificationEvent =
                    (EVENTFUNC)GetProcAddress(uiautomationcore, "UiaRaiseNotificationEvent");
                if (uiaRaiseNotificationEvent != NULL) {
                    uiaRaiseNotificationEvent(provider.Get(), NotificationKind_Other, processing,
                                              message.bstr(), activityId.bstr());
                }
            }
#endif
        }
    }
}

HRESULT STDMETHODCALLTYPE QWindowsUiaMainProvider::QueryInterface(REFIID iid, LPVOID *iface)
{
    HRESULT result = QComObject::QueryInterface(iid, iface);

    if (SUCCEEDED(result) && iid == __uuidof(IRawElementProviderFragmentRoot)) {
        QAccessibleInterface *accessible = accessibleInterface();
        if (accessible && hwndForAccessible(accessible)) {
            result = S_OK;
        } else {
            Release();
            result = E_NOINTERFACE;
            *iface = nullptr;
        }
    }

    return result;
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
    case UIA_WindowPatternId:
        if (accessible->parent() && (accessible->parent()->role() == QAccessible::Application)) {
            *pRetVal = makeComObject<QWindowsUiaWindowProvider>(id()).Detach();
        }
        break;
    case UIA_TextPatternId:
    case UIA_TextPattern2Id:
        // All text controls.
        if (accessible->textInterface()) {
            *pRetVal = makeComObject<QWindowsUiaTextProvider>(id()).Detach();
        }
        break;
    case UIA_ValuePatternId:
        // All non-static controls support the Value pattern.
        if (accessible->role() != QAccessible::StaticText)
            *pRetVal = makeComObject<QWindowsUiaValueProvider>(id()).Detach();
        break;
    case UIA_RangeValuePatternId:
        // Controls providing a numeric value within a range (e.g., sliders, scroll bars, dials).
        if (accessible->valueInterface()) {
            *pRetVal = makeComObject<QWindowsUiaRangeValueProvider>(id()).Detach();
        }
        break;
    case UIA_TogglePatternId:
        // Checkboxes and other checkable controls.
        if (accessible->state().checkable)
            *pRetVal = makeComObject<QWindowsUiaToggleProvider>(id()).Detach();
        break;
    case UIA_SelectionPatternId:
    case UIA_SelectionPattern2Id:
        // Selections via QAccessibleSelectionInterface or lists of items.
        if (accessible->selectionInterface()
                || accessible->role() == QAccessible::List
                || accessible->role() == QAccessible::PageTabList) {
            *pRetVal = makeComObject<QWindowsUiaSelectionProvider>(id()).Detach();
        }
        break;
    case UIA_SelectionItemPatternId:
        // Parent supports selection interface or items within a list and radio buttons.
        if ((accessible->parent() && accessible->parent()->selectionInterface())
                || (accessible->role() == QAccessible::RadioButton)
                || (accessible->role() == QAccessible::ListItem)
                || (accessible->role() == QAccessible::PageTab)) {
            *pRetVal = makeComObject<QWindowsUiaSelectionItemProvider>(id()).Detach();
        }
        break;
    case UIA_TablePatternId:
        // Table/tree.
        if (accessible->tableInterface()
                && ((accessible->role() == QAccessible::Table) || (accessible->role() == QAccessible::Tree))) {
            *pRetVal = makeComObject<QWindowsUiaTableProvider>(id()).Detach();
        }
        break;
    case UIA_TableItemPatternId:
        // Item within a table/tree.
        if (accessible->tableCellInterface()
                && ((accessible->role() == QAccessible::Cell) || (accessible->role() == QAccessible::TreeItem))) {
            *pRetVal = makeComObject<QWindowsUiaTableItemProvider>(id()).Detach();
        }
        break;
    case UIA_GridPatternId:
        // Table/tree.
        if (accessible->tableInterface()
                && ((accessible->role() == QAccessible::Table) || (accessible->role() == QAccessible::Tree))) {
            *pRetVal = makeComObject<QWindowsUiaGridProvider>(id()).Detach();
        }
        break;
    case UIA_GridItemPatternId:
        // Item within a table/tree.
        if (accessible->tableCellInterface()
                && ((accessible->role() == QAccessible::Cell) || (accessible->role() == QAccessible::TreeItem))) {
            *pRetVal = makeComObject<QWindowsUiaGridItemProvider>(id()).Detach();
        }
        break;
    case UIA_InvokePatternId:
        // Things that have an invokable action (e.g., simple buttons).
        if (accessible->actionInterface()) {
            *pRetVal = makeComObject<QWindowsUiaInvokeProvider>(id()).Detach();
        }
        break;
    case UIA_ExpandCollapsePatternId:
        // Menu items with submenus.
        if ((accessible->role() == QAccessible::MenuItem
                && accessible->childCount() > 0
                && accessible->child(0)->role() == QAccessible::PopupMenu)
            || accessible->role() == QAccessible::ComboBox
            || (accessible->role() == QAccessible::TreeItem && accessible->state().expandable)) {
            *pRetVal = makeComObject<QWindowsUiaExpandCollapseProvider>(id()).Detach();
        }
        break;
    default:
        break;
    }

    return S_OK;
}

void QWindowsUiaMainProvider::setLabelledBy(QAccessibleInterface *accessible, VARIANT *pRetVal)
{
    Q_ASSERT(accessible);

    typedef std::pair<QAccessibleInterface*, QAccessible::Relation> RelationPair;
    const QList<RelationPair> relationInterfaces = accessible->relations(QAccessible::Label);
    if (relationInterfaces.empty())
        return;

    // UIA_LabeledByPropertyId only supports one relation
    ComPtr<IRawElementProviderSimple> provider = providerForAccessible(relationInterfaces.first().first);
    if (!provider)
        return;

    pRetVal->vt = VT_UNKNOWN;
    pRetVal->punkVal = provider.Detach();
}

void QWindowsUiaMainProvider::fillVariantArrayForRelation(QAccessibleInterface* accessible,
                                                          QAccessible::Relation relation, VARIANT *pRetVal)
{
    Q_ASSERT(accessible);

    typedef std::pair<QAccessibleInterface*, QAccessible::Relation> RelationPair;
    const QList<RelationPair> relationInterfaces = accessible->relations(relation);
    if (relationInterfaces.empty())
        return;

    SAFEARRAY *elements = SafeArrayCreateVector(VT_UNKNOWN, 0, relationInterfaces.size());
    for (LONG i = 0; i < relationInterfaces.size(); ++i) {
        if (ComPtr<IRawElementProviderSimple> provider =
                    providerForAccessible(relationInterfaces.at(i).first)) {
            SafeArrayPutElement(elements, &i, provider.Get());
        }
    }

    pRetVal->vt = VT_UNKNOWN | VT_ARRAY;
    pRetVal->parray = elements;
}

void QWindowsUiaMainProvider::setAriaProperties(QAccessibleInterface *accessible, VARIANT *pRetVal)
{
    Q_ASSERT(accessible);

    QAccessibleAttributesInterface *attributesIface = accessible->attributesInterface();
    if (!attributesIface)
        return;

    QString ariaString;
    const QList<QAccessible::Attribute> attrKeys = attributesIface->attributeKeys();
    for (qsizetype i = 0; i < attrKeys.size(); ++i) {
        if (i != 0)
            ariaString += QStringLiteral(";");
        const QAccessible::Attribute key = attrKeys.at(i);
        const QVariant value = attributesIface->attributeValue(key);
        // see "Core Accessibility API Mappings" spec: https://www.w3.org/TR/core-aam-1.2/
        switch (key) {
        case QAccessible::Attribute::Custom:
        {
            // forward custom attributes as-is
            Q_ASSERT((value.canConvert<QHash<QString, QString>>()));
            const QHash<QString, QString> attrMap = value.value<QHash<QString, QString>>();
            for (auto [name, val] : attrMap.asKeyValueRange()) {
                if (name != *attrMap.keyBegin())
                    ariaString += QStringLiteral(";");
                ariaString += name + QStringLiteral("=") + val;
            }
            break;
        }
        case QAccessible::Attribute::Level:
            Q_ASSERT(value.canConvert<int>());
            ariaString += QStringLiteral("level=") + QString::number(value.toInt());
            break;
        default:
            break;
        }
    }

    *pRetVal = QComVariant{ ariaString }.release();
}

void QWindowsUiaMainProvider::setStyle(QAccessibleInterface *accessible, VARIANT *pRetVal)
{
    Q_ASSERT(accessible);

    QAccessibleAttributesInterface *attributesIface = accessible->attributesInterface();
    if (!attributesIface)
        return;

    // currently, only heading styles are implemented here
    if (accessible->role() != QAccessible::Role::Heading)
        return;

    const QVariant levelVariant = attributesIface->attributeValue(QAccessible::Attribute::Level);
    if (!levelVariant.isValid())
        return;

    Q_ASSERT(levelVariant.canConvert<int>());
    // UIA only has styles for heading levels 1-9
    const int level = levelVariant.toInt();
    if (level < 1 || level > 9)
        return;

    const long styleId = styleIdForHeadingLevel(level);
    *pRetVal = QComVariant{ styleId }.release();
}

int QWindowsUiaMainProvider::styleIdForHeadingLevel(int headingLevel)
{
    // only heading levels 1-9 have a corresponding UIA style ID
    Q_ASSERT(headingLevel > 0 && headingLevel <= 9);

    static constexpr int styles[] = {
        StyleId_Heading1,
        StyleId_Heading2,
        StyleId_Heading3,
        StyleId_Heading4,
        StyleId_Heading5,
        StyleId_Heading6,
        StyleId_Heading7,
        StyleId_Heading8,
        StyleId_Heading9,
    };

    return styles[headingLevel - 1];
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

    bool topLevelWindow = accessible->parent() && (accessible->parent()->role() == QAccessible::Application);

    switch (idProp) {
    case UIA_ProcessIdPropertyId:
        // PID
        *pRetVal = QComVariant{ static_cast<long>(GetCurrentProcessId()) }.release();
        break;
    case UIA_AccessKeyPropertyId:
        // Accelerator key.
        *pRetVal = QComVariant{ accessible->text(QAccessible::Accelerator) }.release();
        break;
    case UIA_AriaRolePropertyId:
        if (accessible->role() == QAccessible::Heading)
            *pRetVal = QComVariant{ QStringLiteral("heading") }.release();
        break;
    case UIA_AriaPropertiesPropertyId:
        setAriaProperties(accessible, pRetVal);
        break;
    case UIA_AutomationIdPropertyId:
        // Automation ID, which can be used by tools to select a specific control in the UI.
        *pRetVal = QComVariant{ QAccessibleBridgeUtils::accessibleId(accessible) }.release();
        break;
    case UIA_ClassNamePropertyId:
        // Class name.
        if (QObject *o = accessible->object()) {
            QString className = QLatin1StringView(o->metaObject()->className());
            *pRetVal = QComVariant{ className }.release();
        }
        break;
    case UIA_CulturePropertyId:
    {
        QLocale locale;
        if (QAccessibleAttributesInterface *attributesIface = accessible->attributesInterface()) {
            const QVariant localeVariant = attributesIface->attributeValue(QAccessible::Attribute::Locale);
            if (localeVariant.isValid()) {
                Q_ASSERT(localeVariant.canConvert<QLocale>());
                locale = localeVariant.toLocale();
            }
        }
        LCID lcid = LocaleNameToLCID(qUtf16Printable(locale.bcp47Name()), 0);
        *pRetVal = QComVariant{ long(lcid) }.release();
        break;
    }
    case UIA_DescribedByPropertyId:
        fillVariantArrayForRelation(accessible, QAccessible::DescriptionFor, pRetVal);
        break;
    case UIA_FlowsFromPropertyId:
        fillVariantArrayForRelation(accessible, QAccessible::FlowsTo, pRetVal);
        break;
    case UIA_FlowsToPropertyId:
        fillVariantArrayForRelation(accessible, QAccessible::FlowsFrom, pRetVal);
        break;
    case UIA_LabeledByPropertyId:
        setLabelledBy(accessible, pRetVal);
        break;
    case UIA_FrameworkIdPropertyId:
        *pRetVal = QComVariant{ QStringLiteral("Qt") }.release();
        break;
    case UIA_ControlTypePropertyId:
        if (topLevelWindow) {
            // Reports a top-level widget as a window, instead of "custom".
            *pRetVal = QComVariant{ UIA_WindowControlTypeId }.release();
        } else {
            // Control type converted from role.
            auto controlType = roleToControlTypeId(accessible->role());

            // The native OSK should be disabled if the Qt OSK is in use,
            // or if disabled via application attribute.
            static bool imModuleEmpty = QPlatformInputContextFactory::requested().isEmpty();
            bool nativeVKDisabled = QCoreApplication::testAttribute(Qt::AA_DisableNativeVirtualKeyboard);

            // If we want to disable the native OSK auto-showing
            // we have to report text fields as non-editable.
            if (controlType == UIA_EditControlTypeId && (!imModuleEmpty || nativeVKDisabled))
                controlType = UIA_TextControlTypeId;

            *pRetVal = QComVariant{ controlType }.release();
        }
        break;
    case UIA_HelpTextPropertyId:
        *pRetVal = QComVariant{ accessible->text(QAccessible::Help) }.release();
        break;
    case UIA_HasKeyboardFocusPropertyId:
        if (topLevelWindow) {
            // Windows set the active state to true when they are focused
            *pRetVal = QComVariant{ accessible->state().active ? true : false }.release();
        } else {
            *pRetVal = QComVariant{ accessible->state().focused ? true : false }.release();
        }
        break;
    case UIA_IsKeyboardFocusablePropertyId:
        if (topLevelWindow) {
            // Windows should always be focusable
            *pRetVal = QComVariant{ true }.release();
        } else {
            *pRetVal = QComVariant{ accessible->state().focusable ? true : false }.release();
        }
        break;
    case UIA_IsOffscreenPropertyId:
        *pRetVal = QComVariant{ accessible->state().offscreen ? true : false }.release();
        break;
    case UIA_IsContentElementPropertyId:
        *pRetVal = QComVariant{ true }.release();
        break;
    case UIA_IsControlElementPropertyId:
        *pRetVal = QComVariant{ true }.release();
        break;
    case UIA_IsEnabledPropertyId:
        *pRetVal = QComVariant{ !accessible->state().disabled }.release();
        break;
    case UIA_IsPasswordPropertyId:
        *pRetVal = QComVariant{ accessible->role() == QAccessible::EditableText
                                && accessible->state().passwordEdit }
                           .release();
        break;
    case UIA_IsPeripheralPropertyId:
        // True for peripheral UIs.
        if (QWindow *window = windowForAccessible(accessible)) {
            const Qt::WindowType wt = window->type();
            *pRetVal = QComVariant{ wt == Qt::Popup || wt == Qt::ToolTip || wt == Qt::SplashScreen }
                               .release();
        }
        break;
    case UIA_IsDialogPropertyId:
        *pRetVal = QComVariant{ accessible->role() == QAccessible::Dialog
                                || accessible->role() == QAccessible::AlertMessage }
                           .release();
        break;
    case UIA_FullDescriptionPropertyId:
        *pRetVal = QComVariant{ accessible->text(QAccessible::Description) }.release();
        break;
    case UIA_LocalizedControlTypePropertyId:
        // see Core Accessibility API Mappings spec:
        // https://www.w3.org/TR/core-aam-1.2/#role-map-blockquote
        if (accessible->role() == QAccessible::BlockQuote)
            *pRetVal = QComVariant{ tr("blockquote") }.release();
        break;
    case UIA_NamePropertyId: {
        QString name = accessible->text(QAccessible::Name);
        if (name.isEmpty() && topLevelWindow)
           name = QCoreApplication::applicationName();
        *pRetVal = QComVariant{ name }.release();
        break;
    }
    case UIA_StyleIdAttributeId:
        setStyle(accessible, pRetVal);
        break;
    default:
        break;
    }
    return S_OK;
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
            return UiaHostProviderFromHwnd(hwnd, pRetVal);
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
        *pRetVal = providerForAccessible(targetacc).Detach();
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
            if (QAccessibleInterface *rootacc = window->accessibleRoot())
                *pRetVal = providerForAccessible(rootacc).Detach();
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
        *pRetVal = providerForAccessible(targetacc).Detach();
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
            *pRetVal = providerForAccessible(focusacc).Detach();
        }
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
