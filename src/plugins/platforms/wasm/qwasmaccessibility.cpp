// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmaccessibility.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"
#include "qwasmintegration.h"

#include <QtGui/qwindow.h>

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
}

QWasmAccessibility::~QWasmAccessibility()
{
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

void QWasmAccessibility::removeAccessibilityEnableButton(QWindow *window)
{
    get()->removeAccessibilityEnableButtonImpl(window);
}

void QWasmAccessibility::addAccessibilityEnableButtonImpl(QWindow *window)
{
    if (m_accessibilityEnabled)
        return;

    emscripten::val container = getContainer(window);
    emscripten::val document = getDocument(container);
    emscripten::val button = document.call<emscripten::val>("createElement", std::string("button"));
    button.set("innerText", std::string("Enable Screen Reader"));
    button["classList"].call<void>("add", emscripten::val("hidden-visually-read-by-screen-reader"));
    container.call<void>("appendChild", button);

    auto enableContext = std::make_tuple(button, std::make_unique<qstdweb::EventCallback>
        (button, std::string("click"), [this](emscripten::val) { enableAccessibility(); }));
    m_enableButtons.insert(std::make_pair(window, std::move(enableContext)));
}

void QWasmAccessibility::removeAccessibilityEnableButtonImpl(QWindow *window)
{
    auto it = m_enableButtons.find(window);
    if (it == m_enableButtons.end())
        return;

    // Remove button
    auto [element, callback] = it->second;
    Q_UNUSED(callback);
    element["parentElement"].call<void>("removeChild", element);
    m_enableButtons.erase(it);
}

void QWasmAccessibility::enableAccessibility()
{
    // Enable accessibility globally for the applicaton. Remove all "enable"
    // buttons and populate the accessibility tree, starting from the root object.

    Q_ASSERT(!m_accessibilityEnabled);
    m_accessibilityEnabled = true;
    for (const auto& [key, value] : m_enableButtons) {
        const auto &[element, callback] = value;
        Q_UNUSED(key);
        Q_UNUSED(callback);
        element["parentElement"].call<void>("removeChild", element);
    }
    m_enableButtons.clear();
    populateAccessibilityTree(QAccessible::queryAccessibleInterface(m_rootObject));
}

emscripten::val QWasmAccessibility::getContainer(QWindow *window)
{
    return window ? static_cast<QWasmWindow *>(window->handle())->a11yContainer()
                  : emscripten::val::undefined();
}

emscripten::val QWasmAccessibility::getContainer(QAccessibleInterface *iface)
{
    if (!iface)
        return emscripten::val::undefined();
    return getContainer(getWindow(iface));
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
    return getDocument(getContainer(iface));
}

emscripten::val QWasmAccessibility::createHtmlElement(QAccessibleInterface *iface)
{
    // Get the html container element for the interface; this depends on which
    // QScreen it is on. If the interface is not on a screen yet we get an undefined
    // container, and the code below handles that case as well.
    emscripten::val container = getContainer(iface);

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
            element.call<void>("addEventListener", emscripten::val("click"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;
        case QAccessible::CheckBox: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            element.call<void>("setAttribute", std::string("type"), std::string("checkbox"));
            if (iface->state().checked) {
                element.call<void>("setAttribute", std::string("checked"), std::string("true"));
            }
            element.call<void>("addEventListener", emscripten::val("change"),
                               emscripten::val::module_property("qtEventReceived"), true);

        } break;

        case QAccessible::RadioButton: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            element.call<void>("setAttribute", std::string("type"), std::string("radio"));
            if (iface->state().checked) {
                element.call<void>("setAttribute", std::string("checked"), std::string("true"));
            }
            element.set(std::string("name"), std::string("buttonGroup"));
            element.call<void>("addEventListener", emscripten::val("change"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;

        case QAccessible::SpinBox: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            element.call<void>("setAttribute", std::string("type"), std::string("number"));
            std::string valueString = iface->valueInterface()->currentValue().toString().toStdString();
            element.call<void>("setAttribute", std::string("value"), valueString);
            element.call<void>("addEventListener", emscripten::val("change"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;

        case QAccessible::Slider: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            element.call<void>("setAttribute", std::string("type"), std::string("range"));
            std::string valueString = iface->valueInterface()->currentValue().toString().toStdString();
            element.call<void>("setAttribute", std::string("value"), valueString);
            element.call<void>("addEventListener", emscripten::val("change"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;

        case QAccessible::PageTabList:{
            element = document.call<emscripten::val>("createElement", std::string("div"));
            element.call<void>("setAttribute", std::string("role"), std::string("tablist"));
            QString idName = iface->text(QAccessible::Name).replace(" ", "_");
            idName += "_tabList";
            element.call<void>("setAttribute", std::string("id"), idName.toStdString());

            for (int i = 0; i < iface->childCount();  ++i) {
                if (iface->child(i)->role() == QAccessible::PageTab){
                    emscripten::val elementTab = emscripten::val::undefined();
                    elementTab = ensureHtmlElement(iface->child(i));
                    elementTab.call<void>("setAttribute", std::string("aria-owns"), idName.toStdString());
                    setHtmlElementGeometry(iface->child(i));
                }
            }
        } break;

        case QAccessible::PageTab:{
            element =   document.call<emscripten::val>("createElement", std::string("button"));
            element.call<void>("setAttribute", std::string("role"), std::string("tab"));
            QString text = iface->text(QAccessible::Name);
            element.call<void>("setAttribute", std::string("title"), text.toStdString());
            element.call<void>("addEventListener", emscripten::val("click"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;

        case QAccessible::ScrollBar: {
            element = document.call<emscripten::val>("createElement", std::string("div"));
            element.call<void>("setAttribute", std::string("role"), std::string("scrollbar"));
            std::string valueString = iface->valueInterface()->currentValue().toString().toStdString();
            element.call<void>("setAttribute", std::string("aria-valuenow"), valueString);
            element.call<void>("addEventListener", emscripten::val("change"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;

        case QAccessible::StaticText: {
            element = document.call<emscripten::val>("createElement", std::string("textarea"));
            element.call<void>("setAttribute", std::string("readonly"), std::string("true"));

        } break;
        case QAccessible::Dialog: {
            element = document.call<emscripten::val>("createElement", std::string("dialog"));
        }break;
        case QAccessible::ToolBar:
        case QAccessible::ButtonMenu: {
            element = document.call<emscripten::val>("createElement", std::string("div"));
            QString text = iface->text(QAccessible::Name);

            element.call<void>("setAttribute", std::string("role"), std::string("widget"));
            element.call<void>("setAttribute", std::string("title"), text.toStdString());
            element.call<void>("addEventListener", emscripten::val("click"),
                               emscripten::val::module_property("qtEventReceived"), true);
        }break;
        case QAccessible::MenuBar:
        case QAccessible::PopupMenu: {
          element = document.call<emscripten::val>("createElement",std::string("div"));
          QString text = iface->text(QAccessible::Name);
          element.call<void>("setAttribute", std::string("role"), std::string("widget"));
          element.call<void>("setAttribute", std::string("title"), text.toStdString());
          for (int i = 0; i < iface->childCount(); ++i) {
              ensureHtmlElement(iface->child(i));
              setHtmlElementTextName(iface->child(i));
              setHtmlElementGeometry(iface->child(i));
          }
        }break;
        case QAccessible::EditableText: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            element.call<void>("setAttribute", std::string("type"),std::string("text"));
            element.call<void>("addEventListener", emscripten::val("input"),
                               emscripten::val::module_property("qtEventReceived"), true);
        } break;
        default:
            qCDebug(lcQpaAccessibility) << "TODO: createHtmlElement() handle" << iface->role();
            element = document.call<emscripten::val>("createElement", std::string("div"));
        }

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
    emscripten::val container = getContainer(iface);

    if (container.isUndefined()) {
        qCDebug(lcQpaAccessibility) << "TODO: setHtmlElementVisibility: unable to find html container for element" << iface;
        return;
    }

    container.call<void>("appendChild", element);

    element.set("ariaHidden", !visible); // ariaHidden mean completely hidden; maybe some sort of soft-hidden should be used.
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
    QString text = iface->text(QAccessible::Name);
    element.set("innerHTML", text.toStdString()); // FIXME: use something else than innerHTML
}

void QWasmAccessibility::setHtmlElementTextNameLE(QAccessibleInterface *iface) {
    emscripten::val element = ensureHtmlElement(iface);
    QString text = iface->text(QAccessible::Name);
    element.call<void>("setAttribute", std::string("name"), text.toStdString());
    QString value = iface->text(QAccessible::Value);
    element.set("innerHTML", value.toStdString());
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

void QWasmAccessibility::handleLineEditUpdate(QAccessibleEvent *event) {

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

        if (actionNames.contains(QAccessibleActionInterface::pressAction())) {

            iface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());

        } else if (actionNames.contains(QAccessibleActionInterface::toggleAction())) {

            iface->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());

        } else if (actionNames.contains(QAccessibleActionInterface::increaseAction()) ||
                   actionNames.contains(QAccessibleActionInterface::decreaseAction())) {

            QString val = QString::fromStdString(event["target"]["value"].as<std::string>());

            iface->valueInterface()->setCurrentValue(val.toInt());

        } else if (eventType == "input") {

            if (iface->editableTextInterface()) {
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
        bool checkedString = accessible->state().checked ? true : false;
        element.call<void>("setAttribute", std::string("checked"), checkedString);
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
      element.call<void>("setAttribute", std::string("title"), text.toStdString());
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
    case QAccessible::NameChanged:
    case QAccessible::MenuStart  ://"TODO: To implement later
    case QAccessible::PopupMenuStart://"TODO: To implement later
    case QAccessible::StateChanged:{
      emscripten::val element = ensureHtmlElement(iface);
      element.call<void>("setAttribute", std::string("title"), text.toStdString());
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

    // Create html element for the interface, sync up properties.
    ensureHtmlElement(iface);
    const bool visible = !iface->state().invisible;
    setHtmlElementVisibility(iface, visible);
    setHtmlElementGeometry(iface);
    setHtmlElementTextName(iface);

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
        std::string checkedString = accessible->state().checked ? "true" : "false";
        element.call<void>("setAttribute", std::string("checked"), checkedString);
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
        element.call<void>("setAttribute", std::string("value"), valueString);
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
        element.call<void>("setAttribute", std::string("value"), valueString);
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
        element.call<void>("setAttribute", std::string("aria-valuenow"), valueString);
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
    case QAccessible::ObjectShow:

        setHtmlElementVisibility(iface, true);

        // Sync up properties on show;
        setHtmlElementGeometry(iface);
        setHtmlElementTextName(iface);

        return;
    break;
    case QAccessible::ObjectHide:
        setHtmlElementVisibility(iface, false);
        return;
    break;
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

void QWasmAccessibility::onHtmlEventReceived(emscripten::val event)
{
    static_cast<QWasmAccessibility *>(QWasmIntegration::get()->accessibility())->handleEventFromHtmlElement(event);
}

EMSCRIPTEN_BINDINGS(qtButtonEvent) {
    function("qtEventReceived", &QWasmAccessibility::onHtmlEventReceived);
}

#endif // QT_CONFIG(accessibility)
