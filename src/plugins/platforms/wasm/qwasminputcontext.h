// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMINPUTCONTEXT_H
#define QWASMINPUTCONTEXT_H

#include "qwasmwindow.h"

#include <qpa/qplatforminputcontext.h>
#include <private/qstdweb_p.h>
#include <QtCore/qloggingcategory.h>

#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcQpaWasmInputContext)

class QWasmInputContext : public QPlatformInputContext
{
    Q_DISABLE_COPY(QWasmInputContext)
    Q_OBJECT
public:
    explicit QWasmInputContext();
    ~QWasmInputContext() override;

    bool isValid() const override { return true; }
    void update(Qt::InputMethodQueries) override;
    void showInputPanel() override;
    void hideInputPanel() override;
    void setFocusObject(QObject *object) override;

    const QString preeditString() { return m_preeditString; }
    void setPreeditString(QString preeditStr);
    void insertPreedit(int repalcementLength = 0);
    void commitPreeditAndClear();

    void commitText(const QString &text, int replaceFrom = 0, int replaceLength = 0);

    void focusOnFocusWindow(QWasmWindow::FocusTarget target);

    void inputCallback(emscripten::val event);
    void compositionEndCallback(emscripten::val event);
    void compositionStartCallback(emscripten::val event);
    void compositionUpdateCallback(emscripten::val event);
    void beforeInputCallback(emscripten::val event);

    bool isActive() { return inputMethodAccepted(); }
    bool m_ignoreNextInput = false;

private:
    void updateInputElement();

    struct CompositionRange
    {
        int start = 0;
        int end = 0;
        bool isValid = false;
    };

    CompositionRange getFirstRange() const;

    struct ResolvedRange
    {
        int replaceFrom = 0;
        int replaceLength = 0;
    };

    ResolvedRange resolveRange(const CompositionRange &range) const;

    QString m_preeditString;

    QObject *m_focusObject = nullptr;
    emscripten::val m_inputElement = emscripten::val::null();
    emscripten::val m_compositionRange = emscripten::val::null();
};

QT_END_NAMESPACE

#endif // QWASMINPUTCONTEXT_H
