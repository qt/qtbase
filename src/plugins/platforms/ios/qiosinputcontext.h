/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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

    bool isValid() const override { return true; }

    void showInputPanel() override;
    void hideInputPanel() override;

    bool isInputPanelVisible() const override;
    bool isAnimating() const override;
    QRectF keyboardRect() const override;

    void update(Qt::InputMethodQueries) override;
    void reset() override;
    void commit() override;

    QLocale locale() const override;

    void clearCurrentFocusObject();

    void setFocusObject(QObject *object) override;
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
