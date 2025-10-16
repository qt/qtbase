// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmaccessibility.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"
#include "qwasmintegration.h"
#include <QtCore/private/qwasmsuspendresumecontrol_p.h>
#include <QtGui/qwindow.h>

#include <sstream>

#if QT_CONFIG(accessibility)

#include <QtGui/private/qaccessiblebridgeutils_p.h>

Q_LOGGING_CATEGORY(lcQpaAccessibility, "qt.qpa.accessibility")

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
    Q_ASSERT(suspendResume);
    m_eventHandlerIndex = suspendResume->registerEventHandler([this](const emscripten::val event){
        this->handleEventFromHtmlElement(event);
    });
}

QWasmAccessibility::~QWasmAccessibility()
{
    // Remove accessibility element event handler
    QWasmSuspendResumeControl *suspendResume = QWasmSuspendResumeControl::get();
    Q_ASSERT(suspendResume);
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

    populateAccessibilityTree(QAccessible::queryAccessibleInterface(window));
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
    auto describedByContainer = document.call<emscripten::val>("createElement", std::string("div"));

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
    if (!iface)
        return emscripten::val::undefined();
    return getElementContainer(getWindow(iface));
}

QWindow *QWasmAccessibility::getWindow(QAccessibleInterface *iface)
{
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


void QWasmAccessibility::addEventListener(emscripten::val element, const char *eventType)
{
    element.call<void>("addEventListener", emscripten::val(eventType),
                       QWasmSuspendResumeControl::get()->jsEventHandlerAt(m_eventHandlerIndex),
                       true);
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
            addEventListener(element, "click");
        } break;
        case QAccessible::CheckBox: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "checkbox");
            setAttribute(element, "checked", iface->state().checked);
            addEventListener(element, "change");

        } break;

        case QAccessible::RadioButton: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "radio");
            setAttribute(element, "checked", iface->state().checked);
            setProperty(element, "name", "buttonGroup");
            addEventListener(element, "change");
        } break;

        case QAccessible::SpinBox: {
            const std::string valueString =
                    iface->valueInterface()->currentValue().toString().toStdString();
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "number");
            setAttribute(element, "value", valueString);
            addEventListener(element, "change");
        } break;

        case QAccessible::Slider: {
            const std::string valueString =
                    iface->valueInterface()->currentValue().toString().toStdString();
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "range");
            setAttribute(element, "value", valueString);
            addEventListener(element, "change");
        } break;

        case QAccessible::PageTabList:{
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "role", "tablist");

            for (int i = 0; i < iface->childCount();  ++i) {
                if (iface->child(i)->role() == QAccessible::PageTab){
                    emscripten::val elementTab = emscripten::val::undefined();
                    elementTab = ensureHtmlElement(iface->child(i));
                    setHtmlElementGeometry(iface->child(i));
                }
            }
        } break;

        case QAccessible::PageTab:{
            const QString text = iface->text(QAccessible::Name);
            element =   document.call<emscripten::val>("createElement", std::string("button"));
            setAttribute(element, "role", "tab");
            setAttribute(element, "title", text.toStdString());
            addEventListener(element, "click");
        } break;

        case QAccessible::ScrollBar: {
            const std::string valueString =
                    iface->valueInterface()->currentValue().toString().toStdString();
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "role", "scrollbar");
            setAttribute(element, "aria-valuenow", valueString);
            addEventListener(element, "change");
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
            addEventListener(element, "click");
        }break;
        case QAccessible::MenuItem:
        case QAccessible::ButtonMenu: {
            const QString text = iface->text(QAccessible::Name);
            element = document.call<emscripten::val>("createElement", std::string("button"));
            setAttribute(element, "role", "menuitem");
            setAttribute(element, "title", text.toStdString());
            addEventListener(element, "click");
        }break;
        case QAccessible::MenuBar:
        case QAccessible::PopupMenu: {
            const QString text = iface->text(QAccessible::Name);
            element = document.call<emscripten::val>("createElement", std::string("div"));
            setAttribute(element, "role", "menubar");
            setAttribute(element, "title", text.toStdString());
            for (int i = 0; i < iface->childCount(); ++i) {
                emscripten::val childElement = emscripten::val::undefined();
                childElement= ensureHtmlElement(iface->child(i));
                setAttribute(childElement, "aria-owns", text.toStdString());
                setHtmlElementTextName(iface->child(i));
                setHtmlElementGeometry(iface->child(i));
            }
        }break;
        case QAccessible::EditableText: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            setAttribute(element, "type", "text");
            addEventListener(element, "input");
        } break;
        default:
            qCDebug(lcQpaAccessibility) << "TODO: createHtmlElement() handle" << iface->role();
            element = document.call<emscripten::val>("createElement", std::string("div"));
        }

        addEventListener(element, "focus");
        return element;

    }();

    // Add the html element to the container if we have one. If not there
    // is a second chance when handling the ObjectShow event.
    if (!container.isUndefined())
        container.call<void>("appendChild", element);

    return element;
}

void QWasmAccessibility::destroyHtmlElement(QAccessibleInterface *iface)
{
    Q_UNUSED(iface);
    qCDebug(lcQpaAccessibility) << "TODO destroyHtmlElement";
}

emscripten::val QWasmAccessibility::ensureHtmlElement(QAccessibleInterface *iface)
{
    auto it = m_elements.find(iface);
    if (it != m_elements.end())
        return it.value();

    emscripten::val element = createHtmlElement(iface);
    m_elements.insert(iface, element);

    return element;
}

void QWasmAccessibility::setHtmlElementVisibility(QAccessibleInterface *iface, bool visible)
{
    emscripten::val element = ensureHtmlElement(iface);
    emscripten::val container = getElementContainer(iface);

    if (container.isUndefined()) {
        qCDebug(lcQpaAccessibility) << "TODO: setHtmlElementVisibility: unable to find html container for element" << iface;
        return;
    }

    container.call<void>("appendChild", element);

    visible = visible && !iface->state().invisible && !iface->state().disabled;
    setProperty(element, "ariaHidden", !visible); // ariaHidden mean completely hidden; maybe some sort of soft-hidden should be used.
}

void QWasmAccessibility::setHtmlElementGeometry(QAccessibleInterface *iface)
{
    emscripten::val element = ensureHtmlElement(iface);

    // QAccessibleInterface gives us the geometry in global (screen) coordinates. Translate that
    // to window geometry in order to position elements relative to window origin.
    QWindow *window = getWindow(iface);
    if (!window)
        qCWarning(lcQpaAccessibility) << "Unable to find window for" << iface << "setting null geometry";
    QRect screenGeometry = iface->rect();
    QPoint windowPos = window ? window->mapFromGlobal(screenGeometry.topLeft()) : QPoint();
    QRect windowGeometry(windowPos, screenGeometry.size());

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

void QWasmAccessibility::setHtmlElementTextName(QAccessibleInterface *iface)
{
    emscripten::val element = ensureHtmlElement(iface);
    const QString name = iface->text(QAccessible::Name);

    if (iface->role() == QAccessible::StaticText)
        setProperty(element, "innerHTML", name.toStdString());
    else
        setAttribute(element, "aria-label", name.toStdString());
}

void QWasmAccessibility::setHtmlElementTextNameLE(QAccessibleInterface *iface) {
    emscripten::val element = ensureHtmlElement(iface);
    QString text = iface->text(QAccessible::Name);
    setAttribute(element, "name", text.toStdString());
    QString value = iface->text(QAccessible::Value);
    setProperty(element, "innerHTML", value.toStdString());
}

void QWasmAccessibility::setHtmlElementFocus(QAccessibleInterface *iface)
{
    auto element = ensureHtmlElement(iface);
    element.call<void>("focus");
}

void QWasmAccessibility::handleStaticTextUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleStaticTextUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleLineEditUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::Focus:
    case QAccessible::TextRemoved:
    case QAccessible::TextInserted:
    case QAccessible::TextCaretMoved: {
        setHtmlElementTextNameLE(event->accessibleInterface());
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleLineEditUpdate for event" << event->type();
        break;
    }
}

void QWasmAccessibility::handleEventFromHtmlElement(const emscripten::val event)
{

    QAccessibleInterface *iface = m_elements.key(event["target"]);

    if (iface == nullptr) {
        return;
    } else {
        QString eventType = QString::fromStdString(event["type"].as<std::string>());
        const auto& actionNames = QAccessibleBridgeUtils::effectiveActionNames(iface);

        if (eventType == "focus") {
            if (actionNames.contains(QAccessibleActionInterface::setFocusAction()))
                iface->actionInterface()->doAction(QAccessibleActionInterface::setFocusAction());
        } else if (actionNames.contains(QAccessibleActionInterface::pressAction())) {
            iface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());

        } else if (actionNames.contains(QAccessibleActionInterface::toggleAction())) {

            iface->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());

        } else if (actionNames.contains(QAccessibleActionInterface::increaseAction()) ||
                   actionNames.contains(QAccessibleActionInterface::decreaseAction())) {

            QString val = QString::fromStdString(event["target"]["value"].as<std::string>());

            iface->valueInterface()->setCurrentValue(val.toInt());

        } else if (eventType == "input") {

            // as EditableTextInterface is not implemented in qml accessibility
            // so we need to check the role for text to update in the textbox during accessibility

            if (iface->editableTextInterface() || iface->role() == QAccessible::EditableText) {
                std::string insertText = event["target"]["value"].as<std::string>();
                iface->setText(QAccessible::Value, QString::fromStdString(insertText));
            }
        }
    }
}

void QWasmAccessibility::handleButtonUpdate(QAccessibleEvent *event)
{
    qCDebug(lcQpaAccessibility) << "TODO: implement handleButtonUpdate for event" << event->type();
}

void QWasmAccessibility::handleCheckBoxUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::StateChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        emscripten::val element = ensureHtmlElement(accessible);

        setAttribute(element, "checked", accessible->state().checked);
    } break;
    default:
        qCDebug(lcQpaAccessibility) << "TODO: implement handleCheckBoxUpdate for event" << event->type();
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
      emscripten::val element = ensureHtmlElement(iface);
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
      emscripten::val element = ensureHtmlElement(iface);
      setAttribute(element, "title", text.toStdString());
    } break;
    case QAccessible::PopupMenuStart: {
        ensureHtmlElement(iface);
        if (iface->childCount() > 0)
            m_elements[iface->child(0)].call<void>("focus");
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
      setHtmlElementTextName(event->accessibleInterface());
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
    QWindow *window1 = getWindow(iface);
    QWindow *window0 = (iface->parent()) ? getWindow(iface->parent()) : nullptr;

    if (window1 && window0 == window1) {
        // Create html element for the interface, sync up properties.
        ensureHtmlElement(iface);
        setHtmlElementVisibility(iface, true);
        setHtmlElementGeometry(iface);
        setHtmlElementTextName(iface);
        handleIdentifierUpdate(iface);
        handleDescriptionChanged(iface);
    }
    for (int i = 0; i < iface->childCount(); ++i)
        populateAccessibilityTree(iface->child(i));
}

void QWasmAccessibility::handleRadioButtonUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::StateChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        emscripten::val element = ensureHtmlElement(accessible);
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
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        emscripten::val element = ensureHtmlElement(accessible);
        std::string valueString = accessible->valueInterface()->currentValue().toString().toStdString();
        setAttribute(element, "value", valueString);
    } break;
    default:
        qDebug() << "TODO: implement handleSpinBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleSliderUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::Focus:
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        emscripten::val element = ensureHtmlElement(accessible);
        std::string valueString = accessible->valueInterface()->currentValue().toString().toStdString();
        setAttribute(element, "value", valueString);
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
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::ValueChanged: {
        QAccessibleInterface *accessible = event->accessibleInterface();
        emscripten::val element = ensureHtmlElement(accessible);
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
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::Focus: {
        setHtmlElementTextName(event->accessibleInterface());
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
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    case QAccessible::Focus: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    default:
        qDebug() << "TODO: implement handlePageTabUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleIdentifierUpdate(QAccessibleInterface *iface)
{
    emscripten::val element = ensureHtmlElement(iface);
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
                auto child = ensureHtmlElement(iface->child(i));
                setAttribute(child, "aria-owns", id.toStdString());
            }
        }
    }
}

void QWasmAccessibility::handleDescriptionChanged(QAccessibleInterface *iface)
{
    const auto desc = iface->text(QAccessible::Description).toStdString();
    auto element = ensureHtmlElement(iface);
    auto container = getDescribedByContainer(iface);
    if (!container.isUndefined()) {
        auto document = getDocument(container);
        std::ostringstream oss;
        oss << "dbid_" << (void *)iface;
        auto id = oss.str();

        if (desc.empty()) {
            auto describedBy = document.call<emscripten::val>("getElementById", id);
            if (!describedBy.isUndefined() && !describedBy.isNull()) {
                container.call<void>("removeChild", describedBy);
            }
            setAttribute(element, "aria-describedby", "");
        } else {
            auto describedBy = document.call<emscripten::val>("createElement", std::string("p"));
            setAttribute(describedBy, "id", id);
            setAttribute(describedBy, "aria-hidden", true);
            setAttribute(element, "aria-describedby", id);
            container.call<void>("appendChild", describedBy);
            setProperty(describedBy, "innerHTML", desc);
        }
    }
}

void QWasmAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!m_accessibilityEnabled)
        return;

    QAccessibleInterface *iface = event->accessibleInterface();
    if (!iface) {
        qWarning() << "notifyAccessibilityUpdate with null a11y interface" ;
        return;
    }

    // Handle some common event types. See
    // https://doc.qt.io/qt-5/qaccessible.html#Event-enum
    switch (event->type()) {
    case QAccessible::DescriptionChanged:
        handleDescriptionChanged(iface);
        return;

    case QAccessible::Focus:
        setHtmlElementFocus(iface);
        break;

    case QAccessible::IdentifierChanged:
        handleIdentifierUpdate(iface);
        return;
    case QAccessible::ObjectShow:
        setHtmlElementVisibility(iface, true);

        // Sync up properties on show;
        setHtmlElementGeometry(iface);
        setHtmlElementTextName(iface);
        return;

    case QAccessible::ObjectHide:
        setHtmlElementVisibility(iface, false);
        return;

    case QAccessible::LocationChanged:
        setHtmlElementGeometry(iface);
        return;

    // TODO: maybe handle more types here
    default:
    break;
    };

    // Switch on interface role, see
    // https://doc.qt.io/qt-5/qaccessibleinterface.html#role
    switch (iface->role()) {
    case QAccessible::StaticText:
        handleStaticTextUpdate(event);
    break;
    case QAccessible::Button:
        handleStaticTextUpdate(event);
    break;
    case QAccessible::CheckBox:
        handleCheckBoxUpdate(event);
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
