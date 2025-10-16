// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMACCESIBILITY_H
#define QWASMACCESIBILITY_H

#include <QtCore/qtconfigmacros.h>
#include <QtGui/qtguiglobal.h>

#if QT_CONFIG(accessibility)

#include <QtCore/qhash.h>
#include <private/qstdweb_p.h>
#include <qpa/qplatformaccessibility.h>

#include <emscripten/val.h>
#include <QLoggingCategory>

#include <map>
#include <emscripten/bind.h>

Q_DECLARE_LOGGING_CATEGORY(lcQpaAccessibility)

class QWasmAccessibility : public QPlatformAccessibility
{
public:
    QWasmAccessibility();
    ~QWasmAccessibility();

    static QWasmAccessibility* get();

    static void addAccessibilityEnableButton(QWindow *window);
    static void onShowWindow(QWindow *);
    static void onRemoveWindow(QWindow *);
    static bool isEnabled();

private:
    void addAccessibilityEnableButtonImpl(QWindow *window);
    void enableAccessibility();
    void onShowWindowImpl(QWindow *);
    void onRemoveWindowImpl(QWindow *);

    emscripten::val getA11yContainer(QWindow *window);
    emscripten::val getA11yContainer(QAccessibleInterface *iface);
    emscripten::val getDescribedByContainer(QWindow *window);
    emscripten::val getDescribedByContainer(QAccessibleInterface *iface);
    emscripten::val getElementContainer(QWindow *window);
    emscripten::val getElementContainer(QAccessibleInterface *iface);
    emscripten::val getDocument(const emscripten::val &container);
    emscripten::val getDocument(QAccessibleInterface *iface);
    QWindow *getWindow(QAccessibleInterface *iface);

    emscripten::val createHtmlElement(QAccessibleInterface *iface);
    void destroyHtmlElement(QAccessibleInterface *iface);
    emscripten::val ensureHtmlElement(QAccessibleInterface *iface);
    void setHtmlElementVisibility(QAccessibleInterface *iface, bool visible);
    void setHtmlElementGeometry(QAccessibleInterface *iface);
    void setHtmlElementGeometry(emscripten::val element, QRect geometry);
    void setHtmlElementTextName(QAccessibleInterface *iface);
    void setHtmlElementTextNameLE(QAccessibleInterface *iface);
    void setHtmlElementFocus(QAccessibleInterface *iface);

    void handleStaticTextUpdate(QAccessibleEvent *event);
    void handleButtonUpdate(QAccessibleEvent *event);
    void handleCheckBoxUpdate(QAccessibleEvent *event);
    void handleDialogUpdate(QAccessibleEvent *event);
    void handleMenuUpdate(QAccessibleEvent *event);
    void handleToolUpdate(QAccessibleEvent *event);
    void handleLineEditUpdate(QAccessibleEvent *event);
    void handleRadioButtonUpdate(QAccessibleEvent *event);
    void handleSpinBoxUpdate(QAccessibleEvent *event);
    void handlePageTabUpdate(QAccessibleEvent *event);
    void handleSliderUpdate(QAccessibleEvent *event);
    void handleScrollBarUpdate(QAccessibleEvent *event);
    void handlePageTabListUpdate(QAccessibleEvent *event);
    void handleIdentifierUpdate(QAccessibleInterface *iface);
    void handleDescriptionChanged(QAccessibleInterface *iface);

    void handleEventFromHtmlElement(const emscripten::val event);

    void populateAccessibilityTree(QAccessibleInterface *iface);
    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    void setRootObject(QObject *o) override;
    void initialize() override;
    void cleanup() override;

    void setAttribute(emscripten::val element, const std::string &attr, const std::string &val);
    void setAttribute(emscripten::val element, const std::string &attr, const char *val);
    void setAttribute(emscripten::val element, const std::string &attr, bool val);

    void setProperty(emscripten::val element, const std::string &attr, const std::string &val);
    void setProperty(emscripten::val element, const std::string &attr, const char *val);
    void setProperty(emscripten::val element, const std::string &attr, bool val);

    void addEventListener(emscripten::val element, const char *eventType);

public: // public for EMSCRIPTEN_BINDINGS
    static void onHtmlEventReceived(emscripten::val event);

private:
    static QWasmAccessibility *s_instance;
    QObject *m_rootObject = nullptr;
    bool m_accessibilityEnabled = false;
    std::map<QWindow *, std::tuple<emscripten::val, std::shared_ptr<qstdweb::EventCallback>>> m_enableButtons;
    QHash<QAccessibleInterface *, emscripten::val> m_elements;
    int m_eventHandlerIndex;
};

#endif // QT_CONFIG(accessibility)

#endif
