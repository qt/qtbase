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

#ifndef QACCESSIBLEMENU_H
#define QACCESSIBLEMENU_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qaccessiblewidget.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

#if QT_CONFIG(menu)
class QMenu;
class QMenuBar;
class QAction;

class QAccessibleMenu : public QAccessibleWidget
{
public:
    explicit QAccessibleMenu(QWidget *w);

    int childCount() const override;
    QAccessibleInterface *childAt(int x, int y) const override;

    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override;
    QAccessibleInterface *child(int index) const override;
    QAccessibleInterface *parent() const override;
    int indexOfChild( const QAccessibleInterface *child ) const override;

protected:
    QMenu *menu() const;
};

#if QT_CONFIG(menubar)
class QAccessibleMenuBar : public QAccessibleWidget
{
public:
    explicit QAccessibleMenuBar(QWidget *w);

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;

    int indexOfChild(const QAccessibleInterface *child) const override;

protected:
    QMenuBar *menuBar() const;
};
#endif // QT_CONFIG(menubar)


class QAccessibleMenuItem : public QAccessibleInterface, public QAccessibleActionInterface
{
public:
    explicit QAccessibleMenuItem(QWidget *owner, QAction *w);

    ~QAccessibleMenuItem();
    void *interface_cast(QAccessible::InterfaceType t) override;

    int childCount() const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    bool isValid() const override;
    int indexOfChild(const QAccessibleInterface * child) const override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;
    QObject * object() const override;
    QWindow *window() const override;

    QRect rect() const override;
    QAccessible::Role role() const override;
    void setText(QAccessible::Text t, const QString & text) override;
    QAccessible::State state() const override;
    QString text(QAccessible::Text t) const override;

    // QAccessibleActionInterface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;

    QWidget *owner() const;
protected:
    QAction *action() const;
private:
    QAction *m_action;
    QPointer<QWidget> m_owner; // can hold either QMenu or the QMenuBar that contains the action
};

#endif // QT_CONFIG(menu)

QT_END_NAMESPACE
#endif // QT_NO_ACCESSIBILITY
#endif // QACCESSIBLEMENU_H
