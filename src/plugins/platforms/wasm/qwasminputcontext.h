// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMINPUTCONTEXT_H
#define QWASMINPUTCONTEXT_H


#include <qpa/qplatforminputcontext.h>
#include <QtCore/qpointer.h>
#include <private/qstdweb_p.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>

QT_BEGIN_NAMESPACE

class QWasmInputContext : public QPlatformInputContext
{
    Q_DISABLE_COPY(QWasmInputContext)
    Q_OBJECT
public:
    explicit QWasmInputContext();
    ~QWasmInputContext() override;

    void update(Qt::InputMethodQueries) override;

    void showInputPanel() override;
    void hideInputPanel() override;
    bool isValid() const override { return true; }

    void focusWindowChanged(QWindow *focusWindow);
    void inputStringChanged(QString &, int eventType, QWasmInputContext *context);

private:
    emscripten::val inputHandlerElementForFocusedWindow();

    bool m_inputPanelVisible = false;

    QPointer<QWindow> m_focusWindow;
    emscripten::val m_inputElement = emscripten::val::null();
    std::unique_ptr<qstdweb::EventCallback> m_blurEventHandler;
    std::unique_ptr<qstdweb::EventCallback> m_inputEventHandler;
    static int inputMethodKeyboardCallback(int eventType,
                                       const EmscriptenKeyboardEvent *keyEvent, void *userData);
    bool inputPanelIsOpen = false;
};

QT_END_NAMESPACE

#endif // QWASMINPUTCONTEXT_H
