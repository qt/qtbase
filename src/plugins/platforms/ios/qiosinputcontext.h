/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIOSINPUTCONTEXT_H
#define QIOSINPUTCONTEXT_H

#include <UIKit/UIKit.h>

#include <QtGui/qevent.h>
#include <QtGui/qtransform.h>
#include <qpa/qplatforminputcontext.h>

const char kImePlatformDataInputView[] = "inputView";
const char kImePlatformDataInputAccessoryView[] = "inputAccessoryView";
const char kImePlatformDataReturnKeyType[] = "returnKeyType";

QT_BEGIN_NAMESPACE

@class QIOSKeyboardListener;
@class QIOSTextInputResponder;

struct ImeState
{
    ImeState() : currentState(0), focusObject(0) {}
    Qt::InputMethodQueries update(Qt::InputMethodQueries properties);
    QInputMethodQueryEvent currentState;
    QObject *focusObject;
};

class QIOSInputContext : public QPlatformInputContext
{
public:
    QIOSInputContext();
    ~QIOSInputContext();

    QRectF keyboardRect() const;

    void showInputPanel();
    void hideInputPanel();

    void clearCurrentFocusObject();

    bool isInputPanelVisible() const;
    void setFocusObject(QObject *object);

    void focusWindowChanged(QWindow *focusWindow);
    void cursorRectangleChanged();
    void scrollToCursor();
    void scroll(int y);

    void update(Qt::InputMethodQueries);
    void reset();
    void commit();

    const ImeState &imeState() { return m_imeState; };
    bool inputMethodAccepted() const;

    static QIOSInputContext *instance();

private:
    QIOSKeyboardListener *m_keyboardListener;
    QIOSTextInputResponder *m_textResponder;
    ImeState m_imeState;
};

QT_END_NAMESPACE

#endif
