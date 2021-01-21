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

#ifndef QMOUSEEVENTTRANSITION_H
#define QMOUSEEVENTTRANSITION_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qeventtransition.h>

QT_REQUIRE_CONFIG(qeventtransition);

QT_BEGIN_NAMESPACE

class QMouseEventTransitionPrivate;
class QPainterPath;
class Q_WIDGETS_EXPORT QMouseEventTransition : public QEventTransition
{
    Q_OBJECT
    Q_PROPERTY(Qt::MouseButton button READ button WRITE setButton)
    Q_PROPERTY(Qt::KeyboardModifiers modifierMask READ modifierMask WRITE setModifierMask)
public:
    QMouseEventTransition(QState *sourceState = nullptr);
    QMouseEventTransition(QObject *object, QEvent::Type type,
                          Qt::MouseButton button, QState *sourceState = nullptr);
    ~QMouseEventTransition();

    Qt::MouseButton button() const;
    void setButton(Qt::MouseButton button);

    Qt::KeyboardModifiers modifierMask() const;
    void setModifierMask(Qt::KeyboardModifiers modifiers);

    QPainterPath hitTestPath() const;
    void setHitTestPath(const QPainterPath &path);

protected:
    void onTransition(QEvent *event) override;
    bool eventTest(QEvent *event) override;

private:
    Q_DISABLE_COPY(QMouseEventTransition)
    Q_DECLARE_PRIVATE(QMouseEventTransition)
};

QT_END_NAMESPACE

#endif
