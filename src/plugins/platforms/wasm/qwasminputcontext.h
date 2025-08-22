// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMINPUTCONTEXT_H
#define QWASMINPUTCONTEXT_H


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

    void update(Qt::InputMethodQueries) override;

    void showInputPanel() override;
    void hideInputPanel() override;
    bool isValid() const override { return true; }

    const QString preeditString() { return m_preeditString; }
    void setPreeditString(QString preeditStr, int replaceSize);
    void insertPreedit();
    void commitPreeditAndClear();

    void insertText(QString inputStr, bool replace = false);

    bool usingTextInput() const { return m_inputMethodAccepted; }
    void setFocusObject(QObject *object) override;

    void inputCallback(emscripten::val event);
    void compositionEndCallback(emscripten::val event);
    void compositionStartCallback(emscripten::val event);
    void compositionUpdateCallback(emscripten::val event);

    void updateGeometry();

    bool isActive() const {
        return m_focusObject && m_inputMethodAccepted;
    }

private:
    void updateInputElement();

private:
    QString m_preeditString;
    int m_replaceSize = 0;

    bool m_inputMethodAccepted = false;
    QObject *m_focusObject = nullptr;
    emscripten::val m_inputElement = emscripten::val::null();
};

QT_END_NAMESPACE

#endif // QWASMINPUTCONTEXT_H
