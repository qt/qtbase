/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
