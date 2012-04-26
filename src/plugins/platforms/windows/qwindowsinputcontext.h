/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSINPUTCONTEXT_H
#define QWINDOWSINPUTCONTEXT_H

#include "qtwindows_additional.h"

#include <qpa/qplatforminputcontext.h>

QT_BEGIN_NAMESPACE

class QInputMethodEvent;

class QWindowsInputContext : public QPlatformInputContext
{
    Q_OBJECT

    struct CompositionContext
    {
        CompositionContext();

        HWND hwnd;
        bool haveCaret;
        QString composition;
        int position;
        bool isComposing;
    };
public:
    explicit QWindowsInputContext();
    ~QWindowsInputContext();

    virtual void reset();
    virtual void update(Qt::InputMethodQueries);
    virtual void invokeAction(QInputMethod::Action, int cursorPosition);

    static QWindowsInputContext *instance();

    bool startComposition(HWND hwnd);
    bool composition(HWND hwnd, LPARAM lParam);
    bool endComposition(HWND hwnd);

    int reconvertString(RECONVERTSTRING *reconv);

    bool handleIME_Request(WPARAM wparam, LPARAM lparam, LRESULT *result);

private slots:
    void cursorRectChanged();

private:
    void initContext(HWND hwnd);
    void doneContext();
    void startContextComposition();
    void endContextComposition();

    const DWORD m_WM_MSIME_MOUSE;
    CompositionContext m_compositionContext;
    bool m_endCompositionRecursionGuard;
};

QT_END_NAMESPACE

#endif // QWINDOWSINPUTCONTEXT_H
