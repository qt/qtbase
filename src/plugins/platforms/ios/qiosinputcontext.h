/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIOSINPUTCONTEXT_H
#define QIOSINPUTCONTEXT_H

#include <UIKit/UIKit.h>

#include <QtCore/qlocale.h>
#include <QtGui/qevent.h>
#include <QtGui/qtransform.h>
#include <qpa/qplatforminputcontext.h>

const char kImePlatformDataInputView[] = "inputView";
const char kImePlatformDataInputAccessoryView[] = "inputAccessoryView";
const char kImePlatformDataHideShortcutsBar[] = "hideShortcutsBar";
const char kImePlatformDataReturnKeyType[] = "returnKeyType";

@class QIOSLocaleListener;
@class QIOSKeyboardListener;
@class QIOSTextInputResponder;
@protocol KeyboardState;

QT_BEGIN_NAMESPACE

struct KeyboardState
{
    KeyboardState() :
        keyboardVisible(false), keyboardAnimating(false),
        animationDuration(0), animationCurve(UIViewAnimationCurve(-1))
        {}

    bool keyboardVisible;
    bool keyboardAnimating;
    QRectF keyboardRect;
    CGRect keyboardEndRect;
    NSTimeInterval animationDuration;
    UIViewAnimationCurve animationCurve;
};

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

    bool isValid() const Q_DECL_OVERRIDE { return true; }

    void showInputPanel() Q_DECL_OVERRIDE;
    void hideInputPanel() Q_DECL_OVERRIDE;

    bool isInputPanelVisible() const Q_DECL_OVERRIDE;
    bool isAnimating() const Q_DECL_OVERRIDE;
    QRectF keyboardRect() const Q_DECL_OVERRIDE;

    void update(Qt::InputMethodQueries) Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void commit() Q_DECL_OVERRIDE;

    QLocale locale() const Q_DECL_OVERRIDE;

    void clearCurrentFocusObject();

    void setFocusObject(QObject *object) Q_DECL_OVERRIDE;
    void focusWindowChanged(QWindow *focusWindow);

    void scrollToCursor();
    void scroll(int y);

    void updateKeyboardState(NSNotification *notification = 0);

    const ImeState &imeState() { return m_imeState; };
    const KeyboardState &keyboardState() { return m_keyboardState; };

    bool inputMethodAccepted() const;

    static QIOSInputContext *instance();

private:
    UIView* scrollableRootView();

    QIOSLocaleListener *m_localeListener;
    QIOSKeyboardListener *m_keyboardHideGesture;
    QIOSTextInputResponder *m_textResponder;
    KeyboardState m_keyboardState;
    ImeState m_imeState;
};

QT_END_NAMESPACE

#endif
