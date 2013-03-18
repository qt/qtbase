/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QACCESSIBLEWIDGET_H
#define QACCESSIBLEWIDGET_H

#include <QtGui/qaccessibleobject.h>
#include <QtGui/private/qaccessible2_p.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_ACCESSIBILITY

class QAccessibleWidgetPrivate;

class Q_WIDGETS_EXPORT QAccessibleWidget : public QAccessibleObject, public QAccessibleActionInterface
{
public:
    explicit QAccessibleWidget(QWidget *o, QAccessible::Role r = QAccessible::Client, const QString& name = QString());
    bool isValid() const;

    QWindow *window() const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> > relations(QAccessible::Relation match = QAccessible::AllRelations) const;
    QAccessibleInterface *focusChild() const;

    QRect rect() const;

    QAccessibleInterface *parent() const;
    QAccessibleInterface *child(int index) const;

    QString text(QAccessible::Text t) const;
    QAccessible::Role role() const;
    QAccessible::State state() const;

    QColor foregroundColor() const;
    QColor backgroundColor() const;

    void *interface_cast(QAccessible::InterfaceType t);

    // QAccessibleActionInterface
    QStringList actionNames() const;
    void doAction(const QString &actionName);
    QStringList keyBindingsForAction(const QString &actionName) const;
protected:
    ~QAccessibleWidget();
    QWidget *widget() const;
    QObject *parentObject() const;

    void addControllingSignal(const QString &signal);

private:
    QAccessibleWidgetPrivate *d;
    Q_DISABLE_COPY(QAccessibleWidget)
};


#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QACCESSIBLEWIDGET_H
