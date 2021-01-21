/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPLATFORMINPUTCONTEXT_H
#define QPLATFORMINPUTCONTEXT_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtGui/qinputmethod.h>

QT_BEGIN_NAMESPACE

class QPlatformInputContextPrivate;

class Q_GUI_EXPORT QPlatformInputContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPlatformInputContext)

public:
    enum Capability {
        HiddenTextCapability = 0x1
    };

    QPlatformInputContext();
    ~QPlatformInputContext();

    virtual bool isValid() const;
    virtual bool hasCapability(Capability capability) const;

    virtual void reset();
    virtual void commit();
    virtual void update(Qt::InputMethodQueries);
    virtual void invokeAction(QInputMethod::Action, int cursorPosition);
    virtual bool filterEvent(const QEvent *event);
    virtual QRectF keyboardRect() const;
    void emitKeyboardRectChanged();

    virtual bool isAnimating() const;
    void emitAnimatingChanged();

    virtual void showInputPanel();
    virtual void hideInputPanel();
    virtual bool isInputPanelVisible() const;
    void emitInputPanelVisibleChanged();

    virtual QLocale locale() const;
    void emitLocaleChanged();
    virtual Qt::LayoutDirection inputDirection() const;
    void emitInputDirectionChanged(Qt::LayoutDirection newDirection);

    virtual void setFocusObject(QObject *object);
    bool inputMethodAccepted() const;

    static void setSelectionOnFocusObject(const QPointF &anchorPos, const QPointF &cursorPos);

private:
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QInputMethod;
};

QT_END_NAMESPACE

#endif // QPLATFORMINPUTCONTEXT_H
