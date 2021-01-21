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

#ifndef QABSTRACTITEMDELEGATE_P_H
#define QABSTRACTITEMDELEGATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qabstractitemdelegate.h"
#include <private/qobject_p.h>

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QAbstractItemDelegatePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemDelegate)
public:
    explicit QAbstractItemDelegatePrivate();

    bool editorEventFilter(QObject *object, QEvent *event);
    bool tryFixup(QWidget *editor);
    QString textForRole(Qt::ItemDataRole role, const QVariant &value, const QLocale &locale, int precision = 6) const;
    void _q_commitDataAndCloseEditor(QWidget *editor);
};

QT_END_NAMESPACE

#endif // QABSTRACTITEMDELEGATE_P_H
