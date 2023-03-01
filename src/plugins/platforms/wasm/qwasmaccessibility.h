// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMACCESIBILITY_H
#define QWASMACCESIBILITY_H

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
    static void removeAccessibilityEnableButton(QWindow *window);

private:
    void addAccessibilityEnableButtonImpl(QWindow *window);
    void removeAccessibilityEnableButtonImpl(QWindow *window);
    void enableAccessibility();

    static emscripten::val getContainer(QWindow *window);
    static emscripten::val getContainer(QAccessibleInterface *iface);
    static emscripten::val getDocument(const emscripten::val &container);
    static emscripten::val getDocument(QAccessibleInterface *iface);
    static QWindow *getWindow(QAccessibleInterface *iface);

    emscripten::val createHtmlElement(QAccessibleInterface *iface);
    void destroyHtmlElement(QAccessibleInterface *iface);
    emscripten::val ensureHtmlElement(QAccessibleInterface *iface);
    void setHtmlElementVisibility(QAccessibleInterface *iface, bool visible);
    void setHtmlElementGeometry(QAccessibleInterface *iface);
    void setHtmlElementGeometry(emscripten::val element, QRect geometry);
    void setHtmlElementTextName(QAccessibleInterface *iface);
    void setHtmlElementTextNameLE(QAccessibleInterface *iface);

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

    void handleEventFromHtmlElement(const emscripten::val event);

    void populateAccessibilityTree(QAccessibleInterface *iface);
    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    void setRootObject(QObject *o) override;
    void initialize() override;
    void cleanup() override;

public: // public for EMSCRIPTEN_BINDINGS
    static void onHtmlEventReceived(emscripten::val event);

private:
    static QWasmAccessibility *s_instance;
    QObject *m_rootObject = nullptr;
    bool m_accessibilityEnabled = false;
    std::map<QWindow *, std::tuple<emscripten::val, std::shared_ptr<qstdweb::EventCallback>>> m_enableButtons;
    QHash<QAccessibleInterface *, emscripten::val> m_elements;

};

#endif // QT_CONFIG(accessibility)

#endif
