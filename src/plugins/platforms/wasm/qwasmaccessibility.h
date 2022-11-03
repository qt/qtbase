// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMACCESIBILITY_H
#define QWASMACCESIBILITY_H

#include <QtCore/qhash.h>
#include <qpa/qplatformaccessibility.h>

#include <emscripten/val.h>

Q_DECLARE_LOGGING_CATEGORY(lcQpaAccessibility)

class QWasmAccessibility : public QPlatformAccessibility
{
public:
    QWasmAccessibility();
    ~QWasmAccessibility();

    static emscripten::val getContainer(QAccessibleInterface *iface);
    static emscripten::val getDocument(const emscripten::val &container);
    static emscripten::val getDocument(QAccessibleInterface *iface);

    emscripten::val createHtmlElement(QAccessibleInterface *iface);
    void destroyHtmlElement(QAccessibleInterface *iface);
    emscripten::val ensureHtmlElement(QAccessibleInterface *iface);
    void setHtmlElementVisibility(QAccessibleInterface *iface, bool visible);
    void setHtmlElementGeometry(QAccessibleInterface *iface);
    void setHtmlElementGeometry(QAccessibleInterface *iface, emscripten::val element);
    void setHtmlElementTextName(QAccessibleInterface *iface);

    void handleStaticTextUpdate(QAccessibleEvent *event);
    void handleButtonUpdate(QAccessibleEvent *event);
    void handleCheckBoxUpdate(QAccessibleEvent *event);

    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    void setRootObject(QObject *o) override;
    void initialize() override;
    void cleanup() override;

private:
    QHash<QAccessibleInterface *, emscripten::val> m_elements;

};

#endif
