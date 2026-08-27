// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEWINDOW_P_H
#define QACCESSIBLEWINDOW_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qaccessibleobject.h>

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

class QWindow;

class Q_GUI_EXPORT QAccessibleWindow : public QAccessibleObject
{
public:
    explicit QAccessibleWindow(QWindow *window);

    QWindow *window() const override;

    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    QAccessibleInterface *child(int index) const override;
    QAccessibleInterface *parent() const override;
    QAccessibleInterface *focusChild() const override;

    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;

protected:
    QList<QAccessibleInterface *> childInterfaces() const;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QACCESSIBLEWINDOW_P_H
