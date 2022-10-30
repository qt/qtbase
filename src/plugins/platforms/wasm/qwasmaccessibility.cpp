// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmaccessibility.h"
#include "qwasmscreen.h"

#include <QtGui/qwindow.h>

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

}

QWasmAccessibility::~QWasmAccessibility()
{

}

emscripten::val QWasmAccessibility::getContainer(QAccessibleInterface *iface)
{
    // Get to QWasmScreen::container(), return undefined element if unable to
    QWindow *window = iface->window();
    if (!window)
        return emscripten::val::undefined();
    QWasmScreen *screen = QWasmScreen::get(window->screen());
    if (!screen)
        return emscripten::val::undefined();
    return screen->container();
}

emscripten::val QWasmAccessibility::getDocument(const emscripten::val &container)
{
    if (container.isUndefined())
        return emscripten::val::undefined();
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
    emscripten::val document = container.isUndefined() ? emscripten::val::global("document") : getDocument(container);

    // Translate the Qt a11y elemen role into html element type + ARIA role.
    // Here we can either create <div> elements with a spesific ARIA role,
    // or create e.g. <button> elements which should have built-in accessibility.
    emscripten::val element = [iface, document] {

        emscripten::val element = emscripten::val::undefined();

        switch (iface->role()) {

        case QAccessible::Button: {
            element = document.call<emscripten::val>("createElement", std::string("button"));
        } break;

        case QAccessible::CheckBox: {
            element = document.call<emscripten::val>("createElement", std::string("input"));
            element.call<void>("setAttribute", std::string("type"), std::string("checkbox"));
        } break;

        default:
            qDebug() << "TODO: createHtmlElement() handle" << iface->role();
            element = document.call<emscripten::val>("createElement", std::string("div"));
            //element.set("AriaRole", "foo");
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
    qDebug() << "TODO destroyHtmlElement";
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
        qDebug() << "TODO: setHtmlElementVisibility: unable to find html container for element" << iface;
        return;
    }

    container.call<void>("appendChild", element);

    element.set("ariaHidden", !visible); // ariaHidden mean completely hidden; maybe some sort of soft-hidden should be used.
}

void QWasmAccessibility::setHtmlElementGeometry(QAccessibleInterface *iface)
{
    emscripten::val element = ensureHtmlElement(iface);
    setHtmlElementGeometry(iface, element);
}

void QWasmAccessibility::setHtmlElementGeometry(QAccessibleInterface *iface, emscripten::val element)
{
    // Position the element using "position: absolute" in order to place
    // it under the corresponding Qt element on the canvas.
    QRect geometry = iface->rect();
    emscripten::val style = element["style"];
    style.set("position", std::string("absolute"));
    style.set("z-index", std::string("-1")); // FIXME: "0" should be sufficient to order beheind the canvas, but isn't
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

void QWasmAccessibility::handleStaticTextUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    default:
        qDebug() << "TODO: implement handleStaticTextUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::handleButtonUpdate(QAccessibleEvent *event)
{
    qDebug() << "TODO: implement handleButtonUpdate for event" << event->type();
}

void QWasmAccessibility::handleCheckBoxUpdate(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::NameChanged: {
        setHtmlElementTextName(event->accessibleInterface());
    } break;
    default:
        qDebug() << "TODO: implement handleCheckBoxUpdate for event" << event->type();
    break;
    }
}

void QWasmAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
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
    default:
        qDebug() << "TODO: implement notifyAccessibilityUpdate for role" << iface->role();
    };
}

void QWasmAccessibility::setRootObject(QObject *o)
{
    qDebug() << "setRootObject" << o;
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
    Q_UNUSED(iface)
}

void QWasmAccessibility::initialize()
{

}

void QWasmAccessibility::cleanup()
{

}
