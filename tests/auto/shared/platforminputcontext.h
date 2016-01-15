/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        m_lastEventType(QEvent::None),
        m_setFocusObjectCallCount(0)
    {}

    virtual QRectF keyboardRect() const { return m_keyboardRect; }
    virtual bool isAnimating() const { return m_animating; }
    virtual void reset() { m_resetCallCount++; }
    virtual void commit() {
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

    virtual void update(Qt::InputMethodQueries queries)
    {
        m_updateCallCount++;
        m_lastQueries = queries;
    }
    virtual void invokeAction(QInputMethod::Action action, int cursorPosition)
    {
        m_action = action;
        m_cursorPosition = cursorPosition;
    }
    virtual bool filterEvent(const QEvent *event)
    {
        m_lastEventType = event->type(); return false;
    }
    virtual void showInputPanel()
    {
        m_visible = true;
    }
    virtual void hideInputPanel()
    {
        m_visible = false;
    }
    virtual bool isInputPanelVisible() const
    {
        return m_visible;
    }
    virtual QLocale locale() const
    {
        m_localeCallCount++;
        return QLocale::c();
    }
    virtual Qt::LayoutDirection inputDirection() const
    {
        m_inputDirectionCallCount++;
        return Qt::LeftToRight;
    }
    virtual void setFocusObject(QObject *object)
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
    int m_lastEventType;
    QRectF m_keyboardRect;
    int m_setFocusObjectCallCount;
};
