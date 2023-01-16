// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qpa/qplatforminputcontext.h>

class PlatformInputContext : public QPlatformInputContext
{
public:
    PlatformInputContext() :
        m_animating(false),
        m_visible(false),
        m_updateCallCount(0),
        m_resetCallCount(0),
        m_commitCallCount(0),
        m_localeCallCount(0),
        m_inputDirectionCallCount(0),
        m_lastQueries(Qt::ImhNone),
        m_action(QInputMethod::Click),
        m_cursorPosition(0),
        m_setFocusObjectCallCount(0)
    {}

    virtual QRectF keyboardRect() const override { return m_keyboardRect; }
    virtual bool isAnimating() const override { return m_animating; }
    virtual void reset() override { m_resetCallCount++; }
    virtual void commit() override
    {
        m_commitCallCount++;
        if (m_commitString.isEmpty())
            return;
        QInputMethodEvent commitEvent;
        commitEvent.setCommitString(m_commitString);
        if (qGuiApp->focusObject())
            qGuiApp->sendEvent(qGuiApp->focusObject(), &commitEvent);
        else
            qWarning("Test input context to commit without focused object");
    }
    void setCommitString(const QString &commitString)
    {
        m_commitString = commitString;
    }

    virtual void update(Qt::InputMethodQueries queries) override
    {
        m_updateCallCount++;
        m_lastQueries = queries;
    }
    virtual void invokeAction(QInputMethod::Action action, int cursorPosition) override
    {
        m_action = action;
        m_cursorPosition = cursorPosition;
    }
    virtual void showInputPanel() override
    {
        m_visible = true;
    }
    virtual void hideInputPanel() override
    {
        m_visible = false;
    }
    virtual bool isInputPanelVisible() const override
    {
        return m_visible;
    }
    virtual QLocale locale() const override
    {
        m_localeCallCount++;
        return QLocale::c();
    }
    virtual Qt::LayoutDirection inputDirection() const override
    {
        m_inputDirectionCallCount++;
        return Qt::LeftToRight;
    }
    virtual void setFocusObject(QObject *object) override
    {
        Q_UNUSED(object);
        m_setFocusObjectCallCount++;
    }

    bool m_animating;
    bool m_visible;
    int m_updateCallCount;
    int m_resetCallCount;
    int m_commitCallCount;
    QString m_commitString;
    mutable int m_localeCallCount;
    mutable int m_inputDirectionCallCount;
    Qt::InputMethodQueries m_lastQueries;
    QInputMethod::Action m_action;
    int m_cursorPosition;
    QRectF m_keyboardRect;
    int m_setFocusObjectCallCount;
};
