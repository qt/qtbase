/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QKEYEVENTTRANSITION_H
#define QKEYEVENTTRANSITION_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qeventtransition.h>

QT_REQUIRE_CONFIG(qeventtransition);

QT_BEGIN_NAMESPACE

class QKeyEventTransitionPrivate;
class Q_WIDGETS_EXPORT QKeyEventTransition : public QEventTransition
{
    Q_OBJECT
    Q_PROPERTY(int key READ key WRITE setKey)
    Q_PROPERTY(Qt::KeyboardModifiers modifierMask READ modifierMask WRITE setModifierMask)
public:
    QKeyEventTransition(QState *sourceState = nullptr);
    QKeyEventTransition(QObject *object, QEvent::Type type, int key,
                        QState *sourceState = nullptr);
    ~QKeyEventTransition();

    int key() const;
    void setKey(int key);

    Qt::KeyboardModifiers modifierMask() const;
    void setModifierMask(Qt::KeyboardModifiers modifiers);

protected:
    void onTransition(QEvent *event) override;
    bool eventTest(QEvent *event) override;

private:
    Q_DISABLE_COPY(QKeyEventTransition)
    Q_DECLARE_PRIVATE(QKeyEventTransition)
};

QT_END_NAMESPACE

#endif
