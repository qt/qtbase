// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwasmaccessibility.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"
#include "qwasmintegration.h"
#include <QtCore/private/qwasmsuspendresumecontrol_p.h>
#include <QtGui/qwindow.h>

#include <sstream>

void QWasmAccessibilityEnable()
{
    QWasmAccessibility::enable();
}

#if QT_CONFIG(accessibility)

#include <QtGui/private/qaccessiblebridgeutils_p.h>

Q_LOGGING_CATEGORY(lcQpaAccessibility, "qt.qpa.accessibility")

namespace {
EM_JS(emscripten::EM_VAL, getActiveElement_js, (emscripten::EM_VAL undefHandle), {
    var activeEl = document.activeElement;
    while (true) {
        if (!activeEl) {
            return undefHandle;
        } else if (activeEl.shadowRoot) {
            activeEl = activeEl.shadowRoot.activeElement;
        } else {
            return Emval.toHandle(activeEl);
        }
    }
})
}

// Qt WebAssembly a11y backend
//
// This backend implements accessibility support by creating "shadowing" html
// elements for each Qt UI element. We access the DOM by using Emscripten's
// val.h API.
//
// Currently, html elements are created in response to notifyAccessibilityUpdate
// events. In addition or alternatively, we could also walk the accessibility tree
// from setRootObject().

QWasmAccessibility::QWasmAccessibility()
{
    s_instance = this;

    if (qEnvironmentVariableIntValue("QT_WASM_ENABLE_ACCESSIBILITY") == 1)
        enableAccessibility();

    // Register accessibility element event handler
    QWasmSuspendResumeControl *suspendResume = QWasmSuspendResumeControl::get();

    m_eventHandlerIndex = suspendResume->registerEventHandler([this](const emscripten::val event){
        this->handleEventFromHtmlElement(event);
    });
}

QWasmAccessibility::~QWasmAccessibility()
{
    // Remove accessibility element event handler
    QWasmSuspendResumeControl *suspendResume = QWasmSuspendResumeControl::get();
    suspendResume->removeEventHandler(m_eventHandlerIndex);

    s_instance = nullptr;
}

QWasmAccessibility *QWasmAccessibility::s_instance = nullptr;

QWasmAccessibility* QWasmAccessibility::get()
{
    return s_instance;
}

void QWasmAccessibility::addAccessibilityEnableButton(QWindow *window)
{
    get()->addAccessibilityEnableButtonImpl(window);
}

void QWasmAccessibility::onShowWindow(QWindow *window)
{
    get()->onShowWindowImpl(window);
}

void QWasmAccessibility::onRemoveWindow(QWindow *window)
{
    get()->onRemoveWindowImpl(window);
}

bool QWasmAccessibility::isEnabled()
{
    return get()->m_accessibilityEnabled;
}
void QWasmAccessibility::enable()
{
    if (!isEnabled())
        get()->enableAccessibility();
}

void QWasmAccessibility::addAccessibilityEnableButtonImpl(QWindow *window)
{
    if (m_accessibilityEnabled)
        return;

    emscripten::val container = getElementContainer(window);
    emscripten::val document = getDocument(container);
    emscripten::val button = document.call<emscripten::val>("createElement", std::string("button"));
    setProperty(button, "innerText", "Enable Screen Reader");
    button["classList"].call<void>("add", emscripten::val("hidden-visually-read-by-screen-reader"));
    container.call<void>("appendChild", button);

    auto enableContext = std::make_tuple(button, std::make_unique<qstdweb::EventCallback>
        (button, std::string("click"), [this](emscripten::val) { enableAccessibility(); }));
    m_enableButtons.insert(std::make_pair(window, std::move(enableContext)));
}

void QWasmAccessibility::onShowWindowImpl(QWindow *window)
{
    if (!m_accessibilityEnabled)
        return;
    populateAccessibilityTree(window->accessibleRoot());
}

void QWasmAccessibility::onRemoveWindowImpl(QWindow *window)
{
    {
        const auto it = m_enableButtons.find(window);
        if (it != m_enableButtons.end()) {
            // Remove button
            auto [element, callback] = it->second;
            Q_UNUSED(callback);
            element["parentElement"].call<void>("removeChild", element);
            m_enableButtons.erase(it);
        }
    }
    {
        auto a11yContainer = getA11yContainer(window);
        auto describedByContainer =
                     getDescribedByContainer(window);
        auto elementContainer = getElementContainer(window);
        auto document = getDocument(a11yContainer);

        // Remove all items by replacing the container
        if (!describedByContainer.isUndefined()) {
            a11yContainer.call<void>("removeChild", describedByContainer);
            describedByContainer = document.call<emscripten::val>("createElement", std::string("div"));

            a11yContainer.call<void>("appendChild", elementContainer);
            a11yContainer.call<void>("appendChild", describedByContainer);
        }
    }
}

void QWasmAccessibility::enableAccessibility()
{
    // Enable accessibility. Remove all "enable" buttons and populate the
    // accessibility tree for each window.

    Q_ASSERT(!m_accessibilityEnabled);
    m_accessibilityEnabled = true;
    setActive(true);
    for (const auto& [key, value] : m_enableButtons) {
        const auto &[element, callback] = value;
        Q_UNUSED(callback);
        if (auto wasmWindow = QWasmWindow::fromWindow(key))
            wasmWindow->onAccessibilityEnable();
        onShowWindowImpl(key);
        element["parentElement"].call<void>("removeChild", element);
    }
    m_enableButtons.clear();
}

bool QWasmAccessibility::isWindowNode(QAccessibleInterface *iface)
{
    return (iface && !getWindow(iface->parent()) && getWindow(iface));
}

emscripten::val QWasmAccessibility::getA11yContainer(QWindow *window)
{
    const auto wasmWindow = QWasmWindow::fromWindow(window);
    if (!wasmWindow)
        return emscripten::val::undefined();

    auto a11yContainer = wasmWindow->a11yContainer();
    if (a11yContainer["childElementCount"].as<unsigned>() == 2)
        return a11yContainer;

    Q_ASSERT(a11yContainer["childElementCount"].as<unsigned>() == 0);

    const auto document = getDocument(a11yContainer);
    if (document.isUndefined())
        return emscripten::val::undefined();

    auto elementContainer = document.call<emscripten::val>("createElement", std::string("div"));
    elementContainer["classList"].call<void>("add", emscripten::val("qt-window-a11y-elements-container"));
    auto describedByContainer = document.call<emscripten::val>("createElement", std::string("div"));
    describedByContainer["classList"].call<void>("add", emscripten::val("qt-window-a11y-describedby-container"));

    a11yContainer.call<void>("appendChild", elementContainer);
    a11yContainer.call<void>("appendChild", describedByContainer);

    return a11yContainer;
}

emscripten::val QWasmAccessibility::getA11yContainer(QAccessibleInterface *iface)
{
    return getA11yContainer(getWindow(iface));
}

emscripten::val QWasmAccessibility::getDescribedByContainer(QWindow *window)
{
    auto a11yContainer = getA11yContainer(window);
    if (a11yContainer.isUndefined())
        return emscripten ::val::undefined();

    Q_ASSERT(a11yContainer["childElementCount"].as<unsigned>() == 2);
    Q_ASSERT(!a11yContainer["children"][1].isUndefined());

    return a11yContainer["children"][1];
}

emscripten::val QWasmAccessibility::getDescribedByContainer(QAccessibleInterface *iface)
{
    return getDescribedByContainer(getWindow(iface));
}

emscripten::val QWasmAccessibility::getElementContainer(QWindow *window)
{
    auto a11yContainer = getA11yContainer(window);
    if (a11yContainer.isUndefined())
        return emscripten ::val::undefined();

    Q_ASSERT(a11yContainer["childElementCount"].as<unsigned>() == 2);
    Q_ASSERT(!a11yContainer["children"][0].isUndefined());
    return a11yContainer["children"][0];
}

emscripten::val QWasmAccessibility::getElementContainer(QAccessibleInterface *iface)
{
    // Here we skip QWindow nodes, as they are already present. Such nodes
    // has a parent window of null.
    //
    // The next node should return the a11y container.
    // Further nodes should return the element of the parent.
    if (!getWindow(iface))
        return emscripten::val::undefined();

    if (isWindowNode(iface))
        return emscripten::val::undefined();

    if (isWindowNode(iface->parent()))
        return getElementContainer(getWindow(iface->parent()));

    // Regular node
    return getHtmlElement(iface->parent());
}

QWindow *QWasmAccessibility::getWindow(QAccessibleInterface *iface)
{
    if (!iface)
        return nullptr;

    QWindow *window = iface->window();
    // this is needed to add tabs as the window is not available
    if (!window && iface->parent())
        window = iface->parent()->window();
    return window;
}

emscripten::val QWasmAccessibility::getDocument(const emscripten::val &container)
{
    if (container.isUndefined())
        return emscripten::val::global("document");
    return container["ownerDocument"];
}

emscripten::val QWasmAccessibility::getDocument(QAccessibleInterface *iface)
{
    return getDocument(getA11yContainer(iface));
}

void QWasmAccessibility::setAttribute(emscripten::val element, const std::string &attr,
                                      const std::string &val)
{
    if (val != "")
        element.call<void>("setAttribute", attr, val);
    else
        element.call<void>("removeAttribute", attr);
}

void QWasmAccessibility::setAttribute(emscripten::val element, const std::string &attr,
                                      const char *val)
{
    setAttribute(element, attr, std::string(val));
}

void QWasmAccessibility::setAttribute(emscripten::val element, const std::string &attr, bool val)
{
    if (val)
        element.call<void>("setAttribute", attr, val);
    else
        element.call<void>("removeAttribute", attr);
}

void QWasmAccessibility::setProperty(emscripten::val element, const std::string &property,
                                     const std::string &val)
{
    element.set(property, val);
}

void QWasmAccessibility::setProperty(emscripten::val element, const std::string &property, const char *val)
{
    setProperty(element, property, std::string(val));
}

void QWasmAccessibility::setProperty(emscripten::val element, const std::string &property, bool val)
{
    element.set(property, val);
}

void QWasmAccessibility::setNamedAttribute(QAccessibleInterface *iface, const std::string &attribute, QAccessible::Text text)
{
    const emscripten::val element = getHtmlElement(iface);
    setAttribute(element, attribute, iface->text(text).toStdString());
}
void QWasmAccessibility::setNamedProperty(QAccessibleInterface *iface, const std::string &property, QAccessible::Text text)
{
    const emscripten::val element = getHtmlElement(iface);
    setProperty(element, property, iface->text(text).toStdString());
}

void QWasmAccessibility::addEventListener(QAccessibleInterface *iface, emscripten::val element, const char *eventType)
{
    element.set("data-qta11yinterface", reinterpret_cast<size_t>(iface));
    element.call<void>("addEventListener", emscripten::val(eventType),
                       QWasmSuspendResumeControl::get()->jsEventHandlerAt(m_eventHandlerIndex),
                       true);
}

void QWasmAccessibility::sendEvent(QAccessibleInterface *iface, QAccessible::Event eventType)
{
    if (iface->object()) {
        QAccessibleEvent event(iface->object(), eventType);
        handleUpdateByInterfaceRole(&event);
    } else {
        QAccessibleEvent event(iface, eventType);
        handleUpdateByInterfaceRole(&event);
    }
}

emscripten::val QWasmAccessibility::createHtmlElement(QAccessibleInterface *iface)
{
    // Get the html container element for the interface; this depends on which
    // QScreen it is on. If the interface is not on a screen yet we get an undefined
    // container, and the code below handles that case as well.
    emscripten::val container = getElementContainer(iface);

    // Get the correct html document for the container, or fall back
    // to the global document. TODO: Does using the correct document actually matter?
    emscripten::val document = getDocument(container);

    // Translate the Qt a11y elemen role into html element type + ARIA role.
    // Here we can either create <div> elements with a spesific ARIA role,
    // or create e.g. <button> elements which should have built-in accessibility.
    emscripten::val element = [this, iface, document] {

        emscripten::val element = emscripten::val::undefined();

        switch (iface->role()) {

        case QAccessible::Button: {
            element = document.call<emscripten::val>("createElement", std::string("button"));
            addEventListener(iface, element, "click");
        } break;

        case QAccessible::Grouping:
        case QAccessible::CheckBox: {
            // Three cases:
            // 1) role=CheckBox, childCount() == 0 -> Checkbox
            // 2) role=CheckBox, childCount() >  0 -> GroupBox w/checkbox
            // 3) role=Grouping                    -> GroupBox w/label

            emscripten::val checkbox = emscripten::val::undefined();
            if (iface->role() == QAccessible::CheckBox) {
                checkbox = document.call<emscripten::val>("createElement", std::string("input"));
                setAttribute(checkbox, "type", "checkbox");
                setAttribute(checkbox, "checked", iface->state().checked);
                setProperty(checkbox, "indeterminate", iface->state().checkStateMixed);
                addEventListener(iface, checkbox, "change");
            }

            if (iface->childCount() > 0 || iface->role() == QAccessible::Grouping) {
                auto label = document.call<emscripten::val>("createElement", std::string("span"));

                const std::string id = QString::asprintf("lbid%p", iface).toStdString();
                setAttribute(label, "id", id);

                element = document.call<emscripten::val>("createElement", std::string("div"));
                element.call<void>("appendChild", label);

                setAttribute(element, "role", "group");
                setAttribute(element, "aria-labelledby", id);

                if (!checkbox.isUndefined()) {
                    element.call<void>("appendChild", checkbox);
                    addEventListener(iface, checkbox, "focus");
                }
            } else {
                element = checkbox;
            }
        } break;

        case QAccessible::Switch: {
            element = document.call<emscripten::val>("createElement", std::string("button"));
            setAttribute(element, "type", "button");
            setAttribute(element, "role", "switch");
            if (iface->state().checked)
                setAttribute(element, "aria-checked", "true");
            else
                setAttribute(element, "aria-checked", "false");
            addEventListener(iface, element, "change");
        } break;

        case QAccessible::RadioButton: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "radio");
            setAttribute(element, "checked", iface->state().checked);
            setProperty(element, "name", "buttonGroup");
            addEventListener(iface, element, "change");
        } break;

        case QAccessible::Dial:
        case QAccessible::SpinBox:
        case QAccessible::Slider: {
            const auto minValue = iface->valueInterface()->minimumValue().toString().toStdString();
            const auto maxValue = iface->valueInterface()->maximumValue().toString().toStdString();
            const auto stepValue =
                    iface->valueInterface()->minimumStepSize().toString().toStdString();
            const auto value = iface->valueInterface()->currentValue().toString().toStdString();
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element,"type", "number");
            setAttribute(element, "min", minValue);
            setAttribute(element, "max", maxValue);
            setAttribute(element, "step", stepValue);
            setProperty(element, "value", value);
            addEventListener(iface, element, "change");
        } break;

        case QAccessible::PageTabList:{
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "role", "tablist");
            setHtmlElementOrientation(element, iface);

            m_elements[iface] = element;

            for (int i = 0; i < iface->childCount(); ++i)
                createHtmlElement(iface->child(i));

        } break;

        case QAccessible::PageTab:{
            const QString text = iface->text(QAccessible::Name);
            element =   document.call<emscripten::val>("createElement", std::string("button"));
            setAttribute(element, "role", "tab");
            setAttribute(element, "title", text.toStdString());
            addEventListener(iface, element, "click");
        } break;

        case QAccessible::ScrollBar: {
            // Events for scrollbars are handled by Qt, but setFocusPolicy must
            // be called for this to happen.
            const auto minValue = iface->valueInterface()->minimumValue().toString().toStdString();
            const auto maxValue = iface->valueInterface()->maximumValue().toString().toStdString();
            const auto value = iface->valueInterface()->currentValue().toString().toStdString();
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "tabindex", "0");
            setAttribute(element, "role", "scrollbar");
            setAttribute(element, "aria-valuemin", minValue);
            setAttribute(element, "aria-valuemax", maxValue);
            setAttribute(element, "aria-valuenow", value);
            setHtmlElementOrientation(element, iface);
            addEventListener(iface, element, "change");
        } break;

        case QAccessible::StaticText: {
            element = document.call<emscripten::val>("createElement", std::string("div"));
        } break;
        case QAccessible::Dialog: {
            element = document.call<emscripten::val>("createElement", std::string("dialog"));
        }break;
        case QAccessible::ToolBar:{
            const QString text = iface->text(QAccessible::Name);
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "role", "toolbar");
            setAttribute(element, "title", text.toStdString());
            setHtmlElementOrientation(element, iface);
            addEventListener(iface, element, "click");
        }break;
        case QAccessible::MenuItem:
        case QAccessible::ButtonDropDown:
        case QAccessible::ButtonMenu: {
            const QString text = iface->text(QAccessible::Name);
            element = document.call<emscripten::val>("createElement", std::string("button"));
            setAttribute(element, "role", "menuitem");
            setAttribute(element, "title", text.toStdString());
            addEventListener(iface, element, "click");
        }break;
        case QAccessible::MenuBar:
        case QAccessible::PopupMenu: {
            const QString text = iface->text(QAccessible::Name);
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "role", "menubar");
            setAttribute(element, "title", text.toStdString());
            setHtmlElementOrientation(element, iface);
            m_elements[iface] = element;

            for (int i = 0; i < iface->childCount(); ++i) {
                emscripten::val childElement = createHtmlElement(iface->child(i));
                setAttribute(childElement, "aria-owns", text.toStdString());
            }
        }break;
        case QAccessible::EditableText: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "text");
            setAttribute(element, "contenteditable", "true");
            setAttribute(element, "readonly", iface->state().readOnly);
            setAttribute(element, "autocorrect", "off");
            setProperty(element, "inputMode", "text");
        } break;
        case QAccessible::List: {
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "tabindex", "0");
            setAttribute(element, "role", "list");
        } break;
        case QAccessible::ListItem: {
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "tabindex", "0");
            setAttribute(element, "role", "listitem");
        } break;
        case QAccessible::ProgressBar: {
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "tabindex", "0");
            setAttribute(element, "role", "progressbar");
            setAttribute(element, "aria-live", "polite");
            if (auto valueInterface = iface->valueInterface()) {
                const auto name = iface->text(QAccessible::Name).toStdString();
                const auto min = valueInterface->minimumValue().toInt();
                const auto max = valueInterface->maximumValue().toInt();
                const auto cur = valueInterface->currentValue().toInt();

                // from qprogressbar.cpp
                const auto totalSteps = qint64(max) - min;
                const auto progress = (totalSteps == 0) ? 0 : static_cast<int>((qint64(cur) - min) * 100.0 / totalSteps);

                if (cur <= min)
                    setAttribute(element, "aria-valuenow", "0");
                else if (cur >= max)
                    setAttribute(element, "aria-valuenow", "100");
                else
                    setAttribute(element, "aria-valuenow", std::to_string(progress));
                setAttribute(element, "aria-valuemin", "0");
                setAttribute(element, "aria-valuemax", "100");
                setAttribute(element, "aria-label", name);
            }
        } break;
        default:
            qCDebug(lcQpaAccessibility) << "TODO: createHtmlElement() handle" << iface->role();
            element = document.call<emscripten::val>("createElement", std::string("div"));
        }

        addEventListener(iface, element, "focus");
        return element;

    }();

    m_elements[iface] = element;

    setHtmlElementGeometry(iface);
    setHtmlElementDisabled(iface);
    setHtmlElementVisibility(iface, !iface->state().invisible);
    handleIdentifierUpdate(iface);
    handleDescriptionChanged(iface);
    sendEvent(iface, QAccessible::NameChanged);

    linkToParent(iface);
    // Link in child elements
    for (int i = 0; i < iface->childCount(); ++i) {
        if (!getHtmlElement(iface->child(i)).isUndefined())
            linkToParent(iface->child(i));
    }

    return element;
}

void QWasmAccessibility::destroyHtmlElement(QAccessibleInterface *iface)
{
    Q_UNUSED(iface);
    qCDebug(lcQpaAccessibility) << "TODO destroyHtmlElement";
}

emscripten::val QWasmAccessibility::getHtmlElement(QAccessibleInterface *iface)
{
    auto it = m_elements.find(iface);
    if (it != m_elements.end())
        return it.value();

    return emscripten::val::undefined();
}

void QWasmAccessibility::repairLinks(QAccessibleInterface *iface)
{
    // relink any children that are linked to the wrong parent,
    // This can be caused by a missing ParentChanged event.
    bool moved = false;
    for (int i = 0; i < iface->childCount(); ++i) {
        const auto elementI = getHtmlElement(iface->child(i));
        const auto containerI = getElementContainer(iface->child(i));

        if (!elementI.isUndefined() &&
            !containerI.isUndefined() &&
            !elementI["parentElement"].isUndefined() &&
            !elementI["parentElement"].isNull() &&
            elementI["parentElement"] != containerI) {
                moved = true;
                break;
        }
    }
    if (moved) {
        for (int i = 0; i < iface->childCount(); ++i) {
            const auto elementI = getHtmlElement(iface->child(i));
            const auto containerI = getElementContainer(iface->child(i));
            if (!elementI.isUndefined() && !containerI.isUndefined())
                containerI.call<void>("appendChild", elementI);
        }
    }
}

void QWasmAccessibility::linkToParent(QAccessibleInterface *iface)
{
    emscripten::val element = getHtmlElement(iface);
    emscripten::val container = getElementContainer(iface);

    if (container.isUndefined() || element.isUndefined())
        return;

    // Make sure that we don't change the focused element
    const auto activeElementBefore = emscripten::val::take_ownership(
        getActiveElement_js(emscripten::val::undefined().as_handle()));


    repairLinks(iface->parent());

    emscripten::val next = emscripten::val::undefined();
    const int thisIndex = iface->parent()->indexOfChild(iface);
    if (thisIndex >= 0) {
        Q_ASSERT(thisIndex < iface->parent()->childCount());
        for (int i = thisIndex + 1; i < iface->parent()->childCount(); ++i) {
            const auto elementI = getHtmlElement(iface->parent()->child(i));
            if (!elementI.isUndefined() &&
                elementI["parentElement"] == container) {
                next = elementI;
                break;
            }
        }
        if (next.isUndefined())
            container.call<void>("appendChild", element);
        else
            container.call<void>("insertBefore", element, next);
    }
    const auto activeElementAfter = emscripten::val::take_ownership(
        getActiveElement_js(emscripten::val::undefined().as_handle()));
    if (activeElementBefore != activeElementAfter) {
        if (!activeElementBefore.isUndefined() && !activeElementBefore.isNull())
            activeElementBefore.call<void>("focus");
    }
}

void QWasmAccessibility::setHtmlElementVisibility(QAccessibleInterface *iface, bool visible)
{
    emscripten::val element = getHtmlElement(iface);
    setAttribute(element, "inert", !visible);
}

void QWasmAccessibility::setHtmlElementGeometry(QAccessibleInterface *iface)
{
    const emscripten::val element = getHtmlElement(iface);

    QRect windowGeometry = iface->rect();
    if (iface->parent()) {
        // Both iface and iface->parent returns geometry in screen coordinates
        // We only want the relative coordinates, so the coordinate system does
        // not matter as long as it is the same.
        const QRect parentRect = iface->parent()->rect();
        const QRect thisRect = iface->rect();
        const QRect result(thisRect.topLeft() - parentRect.topLeft(), thisRect.size());
        windowGeometry = result;
    } else {
        // Elements without a parent are not a part of the a11y tree, and don't
        // have meaningful geometry.
        Q_ASSERT(!getWindow(iface));
    }
    setHtmlElementGeometry(element, windowGeometry);
}

void QWasmAccessibility::setHtmlElementGeometry(emscripten::val element, QRect geometry)
{
    // Position the element using "position: absolute" in order to place
    // it under the corresponding Qt element in the screen.
    emscripten::val style = element["style"];
    style.set("position", std::string("absolute"));
    style.set("z-index", std::string("-1")); // FIXME: "0" should be sufficient to order beheind the
                                             // screen element, but isn't
    style.set("left", std::to_string(geometry.x()) + "px");
    style.set("top", std::to_string(geometry.y()) + "px");
    style.set("width", std::to_string(geometry.width()) + "px");
    style.set("height", std::to_string(geometry.height()) + "px");
}

void QWasmAccessibility::setHtmlElementFocus(QAccessibleInterface *iface)
{
    const auto element = getHtmlElement(iface);
    element.call<void>("focus");
}

void QWasmAccessibility::setHtmlElementDisabled(QAccessibleInterface *iface)
{
    auto element = getHtmlElement(iface);
    setAttribute(element, "aria-disabled", iface->state().disabled);
}

void QWasmAccessibility::setHtmlElementOrientation(emscripten::val element, QAccessibleInterface *iface)
{
    Q_ASSERT(iface);
    if (QAccessibleAttributesInterface *attributesIface = iface->attributesInterface()) {
        const QVariant orientationVariant =
                attributesIface->attributeValue(QAccessible::Attribute::Orientation);
        if (orientationVariant.isValid()) {
            Q_ASSERT(orientationVariant.canConvert<Qt::Orientation>());
            const Qt::Orientation orientation = orientationVariant.value<Qt::Orientation>();
            const std::string value = orientation == Qt::Horizontal ? "horizontal" : "vertical";
            setAttribute(element, "aria-orientation", value);
        }
    }
}

void QWasmAccessibility::handleListItemUpdate(QAccessibleEvent *event)
{
    auto iface = event->accessibleInterface();

    switch (event->type()) {
    case QAccessible::NameChanged: {
        // ListItem is a div
        setNamedProperty(iface, "innerText", QAccessible::Name);
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleListItemUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleDialUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::ObjectCreated:
    case QAccessible::StateChanged: {
    } break;
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        std::string valueString = accessible->valueInterface()->currentValue().toString().toStdString();
        setProperty(element, "value", valueString);
    } break;
    default:
        qDebug() << "TODO: implement handleSpinBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleStaticTextUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        // StaticText is a div
        setNamedProperty(event->accessibleInterface(), "innerText", QAccessible::Name);
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleStaticTextUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleLineEditUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::StateChanged: {
        auto iface = event->accessibleInterface();
        auto element = getHtmlElement(iface);
        setAttribute(element, "readonly", iface->state().readOnly);
        if (iface->state().passwordEdit)
            setProperty(element, "type", "password");
        else
            setProperty(element, "type", "text");
    } break;
    case QAccessible::ValueChanged:
    case QAccessible::NameChanged: {
        setNamedProperty(event->accessibleInterface(), "value", QAccessible::Value);
    } break;
    case QAccessible::ObjectShow:
    case QAccessible::Focus: {
        auto iface = event->accessibleInterface();
        auto element = getHtmlElement(iface);
        if (!element.isUndefined()) {
            setAttribute(element, "readonly", iface->state().readOnly);
            if (iface->state().passwordEdit)
                setProperty(element, "type", "password");
            else
                setProperty(element, "type", "text");
        }
        setNamedProperty(event->accessibleInterface(), "value", QAccessible::Value);
    } break;
    case QAccessible::TextRemoved:
    case QAccessible::TextInserted: {
        setNamedProperty(event->accessibleInterface(), "value", QAccessible::Value);
    } break;
    case QAccessible::TextCaretMoved: {
        // We lack ValueChanged event, sync on caret move instead
        setNamedProperty(event->accessibleInterface(), "value", QAccessible::Value);
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleLineEditUpdate for event" << event->type();
        break;
    }
}

void QWasmAccessibility::handleEventFromHtmlElement(const emscripten::val event)
{
    if (event["target"].isNull() || event["target"].isUndefined())
        return;

    if (event["target"]["data-qta11yinterface"].isNull() || event["target"]["data-qta11yinterface"].isUndefined())
        return;

    auto iface = reinterpret_cast<QAccessibleInterface *>(event["target"]["data-qta11yinterface"].as<size_t>());
    if (m_elements.find(iface) == m_elements.end())
        return;

    const auto eventType = QString::fromStdString(event["type"].as<std::string>());
    const auto& actionNames = QAccessibleBridgeUtils::effectiveActionNames(iface);
    bool handled = false;

    if (eventType == "blur") {
        ;
    } else if (eventType == "keydown") {
        ;
    } else if (eventType == "keyup") {
        ;
    } else if (eventType == "focus") {
        if (actionNames.contains(QAccessibleActionInterface::setFocusAction()))
            iface->actionInterface()->doAction(QAccessibleActionInterface::setFocusAction());
    } else if (eventType == "click") {
        handled = true;
        if (actionNames.contains(QAccessibleActionInterface::pressAction()))
            iface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        else if (actionNames.contains(QAccessibleActionInterface::showMenuAction()))
            iface->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    } else if (eventType == "change") {
        handled = true;
        // Checkboxes can have both toggle and press, in this case it is
        // important to invoke press to handle tristate correctly
        if (actionNames.contains(QAccessibleActionInterface::pressAction()))
            iface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
        else if (actionNames.contains(QAccessibleActionInterface::toggleAction()))
            iface->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    } else {
        qWarning() << " Unknown event" << eventType;
    }

    if (handled) {
        event.call<void>("preventDefault");
        event.call<void>("stopPropagation");
    }
}

void QWasmAccessibility::handleButtonUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleCheckBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleCheckBoxUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::StateChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        setAttribute(element, "checked", accessible->state().checked);
        setProperty(element, "indeterminate", accessible->state().checkStateMixed);
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleCheckBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleGroupBoxUpdate(QAccessibleEvent *event)
{
    QAccessibleInterface *iface = event->accessibleInterface();

    emscripten::val parent = getHtmlElement(iface);
    emscripten::val label = parent["children"][0];
    emscripten::val checkbox = emscripten::val::undefined();
    if (iface->role() == QAccessible::CheckBox)
        checkbox = parent["children"][1];

    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setProperty(label, "innerText", iface->text(QAccessible::Name).toStdString());
        if (!checkbox.isUndefined())
            setAttribute(checkbox, "aria-label", iface->text(QAccessible::Name).toStdString());
    } break;
    case QAccessible::StateChanged: {
        if (!checkbox.isUndefined()) {
            setAttribute(checkbox, "checked", iface->state().checked);
            setProperty(checkbox, "indeterminate", iface->state().checkStateMixed);
        }
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleCheckBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleSwitchUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        /* A switch is like a button in this regard */
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::StateChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        if (accessible->state().checked)
            setAttribute(element, "aria-checked", "true");
        else
            setAttribute(element, "aria-checked", "false");
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleSwitchUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleToolUpdate(QAccessibleEvent *event)
{
    QAccessibleInterface *iface = event->accessibleInterface();
    QString text = iface->text(QAccessible::Name);
    QString desc = iface->text(QAccessible::Description);
    switch (event->type()) {
    case QAccessible::NameChanged:
    case QAccessible::StateChanged:{
      const emscripten::val element = getHtmlElement(iface);
      setAttribute(element, "title", text.toStdString());
    } break;
    default:
      qCDebug(lcQpaAccessibility) << "TODO: implement handleToolUpdate for event" << event->type();
      break;
    }
}
void QWasmAccessibility::handleMenuUpdate(QAccessibleEvent *event)
{
    QAccessibleInterface *iface = event->accessibleInterface();
    QString text = iface->text(QAccessible::Name);
    QString desc = iface->text(QAccessible::Description);
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged:
    case QAccessible::MenuStart  ://"TODO: To implement later
    case QAccessible::StateChanged:{
      const emscripten::val element = getHtmlElement(iface);
      setAttribute(element, "title", text.toStdString());
    } break;
    case QAccessible::PopupMenuStart: {
        if (iface->childCount() > 0) {
            const auto childElement = getHtmlElement(iface->child(0));
            childElement.call<void>("focus");
        }
    } break;
    default:
      qCDebug(lcQpaAccessibility) << "TODO: implement handleMenuUpdate for event" << event->type();
      break;
    }
}
void QWasmAccessibility::handleDialogUpdate(QAccessibleEvent *event) {

    switch (event->type()) {
    case QAccessible::NameChanged:
    case QAccessible::Focus:
    case QAccessible::DialogStart:
    case QAccessible::StateChanged: {
      setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    default:
      qCDebug(lcQpaAccessibility) << "TODO: implement handleLineEditUpdate for event" << event->type();
      break;
    }
}

void QWasmAccessibility::populateAccessibilityTree(QAccessibleInterface *iface)
{
    if (!iface)
        return;

    // We ignore toplevel windows which is categorized
    // by getWindow(iface->parent()) != getWindow(iface)
    const QWindow *window1 = getWindow(iface);
    const QWindow *window0 = (iface->parent()) ? getWindow(iface->parent()) : nullptr;

    if (window1 && window0 == window1) {
        // Create html element for the interface, sync up properties.
        bool exists = !getHtmlElement(iface).isUndefined();
        if (!exists)
            exists = !createHtmlElement(iface).isUndefined();

        if (exists) {
            linkToParent(iface);
            setHtmlElementVisibility(iface, !iface->state().invisible);
            setHtmlElementGeometry(iface);
            setHtmlElementDisabled(iface);
            handleIdentifierUpdate(iface);
            handleDescriptionChanged(iface);
            sendEvent(iface, QAccessible::NameChanged);
        }
    }
    for (int i = 0; i < iface->childCount(); ++i)
        populateAccessibilityTree(iface->child(i));
}

void QWasmAccessibility::handleRadioButtonUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::StateChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        setAttribute(element, "checked", accessible->state().checked);
    } break;
    default:
        qDebug() << "TODO: implement handleRadioButtonUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleSpinBoxUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::ObjectCreated:
    case QAccessible::StateChanged: {
    } break;
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        std::string valueString = accessible->valueInterface()->currentValue().toString().toStdString();
        setProperty(element, "value", valueString);
    } break;
    default:
        qDebug() << "TODO: implement handleSpinBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleSliderUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::ObjectCreated:
    case QAccessible::StateChanged: {
    } break;
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        std::string valueString = accessible->valueInterface()->currentValue().toString().toStdString();
        setProperty(element, "value", valueString);
    } break;
    default:
        qDebug() << "TODO: implement handleSliderUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleScrollBarUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        const emscripten::val element = getHtmlElement(accessible);
        std::string valueString = accessible->valueInterface()->currentValue().toString().toStdString();
        setAttribute(element, "aria-valuenow", valueString);
    } break;
    default:
        qDebug() << "TODO: implement handleSliderUpdate for event" << event->type();
    break;
    }

}

void QWasmAccessibility::handlePageTabUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::Focus: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    default:
        qDebug() << "TODO: implement handlePageTabUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handlePageTabListUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    case QAccessible::Focus: {
        setNamedAttribute(event->accessibleInterface(), "aria-label", QAccessible::Name);
    } break;
    default:
        qDebug() << "TODO: implement handlePageTabUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleIdentifierUpdate(QAccessibleInterface *iface)
{
    const emscripten::val element = getHtmlElement(iface);
    QString id = iface->text(QAccessible::Identifier).replace(" ", "_");
    if (id.isEmpty() && iface->role() == QAccessible::PageTabList) {
        std::ostringstream oss;
        oss << "tabList_0x" << (void *)iface;
        id = QString::fromUtf8(oss.str());
    }

    setAttribute(element, "id", id.toStdString());
    if (!id.isEmpty()) {
        if (iface->role() == QAccessible::PageTabList) {
            for (int i = 0; i < iface->childCount(); ++i) {
                const auto child = getHtmlElement(iface->child(i));
                setAttribute(child, "aria-owns", id.toStdString());
            }
        }
    }
}

void QWasmAccessibility::handleDescriptionChanged(QAccessibleInterface *iface)
{
    const auto desc = iface->text(QAccessible::Description).toStdString();
    auto element = getHtmlElement(iface);
    auto container = getDescribedByContainer(iface);
    if (!container.isUndefined()) {
        std::ostringstream oss;
        oss << "dbid_" << (void *)iface;
        auto id = oss.str();

        auto describedBy = container.call<emscripten::val>("querySelector", "#" + std::string(id));
        if (desc.empty()) {
            if (!describedBy.isUndefined() && !describedBy.isNull()) {
                container.call<void>("removeChild", describedBy);
            }
            setAttribute(element, "aria-describedby", "");
        } else {
            if (describedBy.isUndefined() || describedBy.isNull()) {
                auto document = getDocument(container);
                describedBy = document.call<emscripten::val>("createElement", std::string("p"));

                container.call<void>("appendChild", describedBy);
            }
            setAttribute(describedBy, "id", id);
            setAttribute(describedBy, "inert", true);
            setAttribute(element, "aria-describedby", id);
            setProperty(describedBy, "innerText", desc);
        }
    }
}

void QWasmAccessibility::handleAnnouncement(QAccessibleInterface *iface,
                                            QAccessibleAnnouncementEvent *announcementEvent)
{
    emscripten::val element = getHtmlElement(iface);
    if (element["ariaNotify"].isUndefined())
        return;

    const std::string message = announcementEvent->message().toStdString();
    const std::string prio(announcementEvent->politeness()
                                           == QAccessible::AnnouncementPoliteness::Assertive
                                   ? "high"
                                   : "normal");
    emscripten::val options = emscripten::val::object();
    options.set("priority", prio);
    element.call<void>("ariaNotify", message, options);
}

void QWasmAccessibility::handleProgressBarUpdate(QAccessibleEvent *event)
{
    auto iface = event->accessibleInterface();
    auto valueInterface = iface->valueInterface();
    auto element = getHtmlElement(iface);

    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        const auto name = iface->text(QAccessible::Name).toStdString();
        setAttribute(element, "aria-label", name);
    } // fallthrough
    case QAccessible::ValueChanged: {
        if (valueInterface) {
            const auto min = valueInterface->minimumValue().toInt();
            const auto max = valueInterface->maximumValue().toInt();
            const auto cur = valueInterface->currentValue().toInt();

            // from qprogressbar.cpp
            const auto totalSteps = qint64(max) - min;
            const auto progress = (totalSteps == 0) ? 0 : static_cast<int>((qint64(cur) - min) * 100.0 / totalSteps);

            if (cur <= min)
                setAttribute(element, "aria-valuenow", "0");
            else if (cur >= max)
                setAttribute(element, "aria-valuenow", "100");
            else
                setAttribute(element, "aria-valuenow", std::to_string(progress));
        }
    } break;
    default: {
    } break;
    } /* end-switch */
}

void QWasmAccessibility::createObject(QAccessibleInterface *iface)
{
    if (getHtmlElement(iface).isUndefined())
        createHtmlElement(iface);
}

void QWasmAccessibility::removeObject(QAccessibleInterface *iface)
{
    // Do not dereference the object pointer. it might be invalid.
    // Do not dereference the iface either, it refers to the object.
    // Note: we may remove children, making them have parentElement undefined
    // so we need to check for parentElement here. We do assume that removeObject
    // is called on all objects, just not in any predefined order.
    const auto it = m_elements.find(iface);
    if (it != m_elements.end()) {
        auto element = it.value();
        auto container = getDescribedByContainer(iface);
        if (!container.isUndefined()) {
            std::ostringstream oss;
            oss << "dbid_" << (void *)iface;
            auto id = oss.str();
            auto describedBy = container.call<emscripten::val>("querySelector", "#" + std::string(id));
            if (!describedBy.isUndefined() && !describedBy.isNull() &&
                !describedBy["parentElement"].isUndefined() && !describedBy["parentElement"].isNull())
                describedBy["parentElement"].call<void>("removeChild", describedBy);
        }
        if (!element["parentElement"].isUndefined() && !element["parentElement"].isNull())
            element["parentElement"].call<void>("removeChild", element);
        m_elements.erase(it);
    }
}

void QWasmAccessibility::unlinkParentForChildren(QAccessibleInterface *iface)
{
    auto element = getHtmlElement(iface);
    if (!element.isUndefined()) {
        auto oldContainer = element["parentElement"];
        auto newContainer = getElementContainer(iface);
        if (!oldContainer.isUndefined() &&
            !oldContainer.isNull() &&
            oldContainer != newContainer) {
            oldContainer.call<void>("removeChild", element);
        }
    }
    for (int i = 0; i < iface->childCount(); ++i)
        unlinkParentForChildren(iface->child(i));
}

void QWasmAccessibility::relinkParentForChildren(QAccessibleInterface *iface)
{
    auto element = getHtmlElement(iface);
    if (!element.isUndefined()) {
        if (element["parentElement"].isUndefined() ||
            element["parentElement"].isNull()) {
            linkToParent(iface);
        }
    }
    for (int i = 0; i < iface->childCount(); ++i)
        relinkParentForChildren(iface->child(i));
}

void QWasmAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (handleUpdateByEventType(event))
        handleUpdateByInterfaceRole(event);
}

bool QWasmAccessibility::handleUpdateByEventType(QAccessibleEvent *event)
{
    if (!m_accessibilityEnabled)
        return false;

    QAccessibleInterface *iface = event->accessibleInterface();
    if (!iface) {
        qWarning() << "handleUpdateByEventType with null a11y interface" << event->type() << event->object();
        return false;
    }

    // Handle event types that creates/removes objects.
    switch (event->type()) {
    case QAccessible::ObjectCreated:
        // Do nothing, there are too many changes to the interface
        // before ObjectShow is called
        return false;

    case QAccessible::ObjectDestroyed:
        // The object might  be under destruction, and the interface is not valid
        // but we can look at the pointer,
        removeObject(iface);
        return false;

    case QAccessible::ObjectShow: // We do not get ObjectCreated from widgets, we get ObjectShow
        createObject(iface);
        break;

    case QAccessible::ParentChanged:
        unlinkParentForChildren(iface);
        relinkParentForChildren(iface);
        break;

    default:
        break;
    };

    if (getHtmlElement(iface).isUndefined())
        return false;

    // Handle some common event types. See
    // https://doc.qt.io/qt-6/qaccessible.html#Event-enum
    switch (event->type()) {
    case QAccessible::Announcement:
        handleAnnouncement(iface, static_cast<QAccessibleAnnouncementEvent *>(event));
        return false;

    case QAccessible::StateChanged: {
        QAccessibleStateChangeEvent *stateChangeEvent = (QAccessibleStateChangeEvent *)event;
        if (stateChangeEvent->changedStates().disabled)
            setHtmlElementDisabled(iface);
    } break;

    case QAccessible::DescriptionChanged:
        handleDescriptionChanged(iface);
        return false;

    case QAccessible::Focus:
        // We do not get all callbacks for the geometry
        // hence we update here as well.
        setHtmlElementGeometry(iface);
        setHtmlElementFocus(iface);
        break;

    case QAccessible::IdentifierChanged:
        handleIdentifierUpdate(iface);
        return false;

    case QAccessible::ObjectShow:
        linkToParent(iface);
        setHtmlElementVisibility(iface, true);

        // Sync up properties on show;
        setHtmlElementGeometry(iface);
        sendEvent(iface, QAccessible::NameChanged);
        break;

    case QAccessible::ObjectHide:
        // Do not call linkToParent, only set visibility to false
        setHtmlElementVisibility(iface, false);
        return false;

    case QAccessible::LocationChanged:
        setHtmlElementGeometry(iface);
        return false;

    // TODO: maybe handle more types here
    default:
        break;
    };

    return true;
}

void QWasmAccessibility::handleUpdateByInterfaceRole(QAccessibleEvent *event)
{
    if (!m_accessibilityEnabled)
        return;

    QAccessibleInterface *iface = event->accessibleInterface();
    if (!iface) {
        qWarning() << "handleUpdateByInterfaceRole with null a11y interface" << event->type() << event->object();
        return;
    }

    // Switch on interface role, see
    // https://doc.qt.io/qt-5/qaccessibleinterface.html#role
    switch (iface->role()) {
    case QAccessible::List:
    break;
    case QAccessible::ListItem:
        handleListItemUpdate(event);
    break;
    case QAccessible::StaticText:
        handleStaticTextUpdate(event);
    break;
    case QAccessible::Button:
        handleButtonUpdate(event);
    break;
    case QAccessible::CheckBox:
        if (iface->childCount() > 0)
            handleGroupBoxUpdate(event);
        else
            handleCheckBoxUpdate(event);
    break;
    case QAccessible::Switch:
        handleSwitchUpdate(event);
    break;
    case QAccessible::EditableText:
        handleLineEditUpdate(event);
    break;
    case QAccessible::Dialog:
        handleDialogUpdate(event);
    break;
    case QAccessible::MenuItem:
    case QAccessible::MenuBar:
    case QAccessible::PopupMenu:
        handleMenuUpdate(event);
    break;
    case QAccessible::ToolBar:
    case QAccessible::ButtonMenu:
       handleToolUpdate(event);
    case QAccessible::RadioButton:
        handleRadioButtonUpdate(event);
    break;
    case QAccessible::Dial:
        handleDialUpdate(event);
    break;
    case QAccessible::SpinBox:
        handleSpinBoxUpdate(event);
    break;
    case QAccessible::Slider:
        handleSliderUpdate(event);
    break;
    case QAccessible::PageTab:
        handlePageTabUpdate(event);
    break;
    case QAccessible::PageTabList:
        handlePageTabListUpdate(event);
    break;
    case QAccessible::ScrollBar:
        handleScrollBarUpdate(event);
    break;
    case QAccessible::Grouping:
        handleGroupBoxUpdate(event);
    break;
    case QAccessible::ProgressBar:
        handleProgressBarUpdate(event);
    break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement notifyAccessibilityUpdate for role" << iface->role();
    };
}

void QWasmAccessibility::setRootObject(QObject *root)
{
    m_rootObject = root;
}

void QWasmAccessibility::initialize()
{

}

void QWasmAccessibility::cleanup()
{

}

#endif // QT_CONFIG(accessibility)
