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

#ifndef QWIDGETACTION_P_H
#define QWIDGETACTION_P_H

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
#include "private/qaction_p.h"

QT_BEGIN_NAMESPACE

class QWidgetActionPrivate : public QActionPrivate
{
    Q_DECLARE_PUBLIC(QWidgetAction)
public:
    inline QWidgetActionPrivate() : defaultWidgetInUse(false), autoCreated(false) {}
    QPointer<QWidget> defaultWidget;
    QList<QWidget *> createdWidgets;
    uint defaultWidgetInUse : 1;
    uint autoCreated : 1; // created by QToolBar::addWidget and the like

    inline void _q_widgetDestroyed(QObject *o) {
        createdWidgets.removeAll(static_cast<QWidget *>(o));
    }
};

QT_END_NAMESPACE

#endif
