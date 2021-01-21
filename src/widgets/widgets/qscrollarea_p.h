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

#ifndef QSCROLLAREA_P_H
#define QSCROLLAREA_P_H

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

#include "private/qabstractscrollarea_p.h"
#include <QtWidgets/qscrollbar.h>

QT_REQUIRE_CONFIG(scrollarea);

QT_BEGIN_NAMESPACE

class QScrollAreaPrivate: public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QScrollArea)

public:
    QScrollAreaPrivate(): resizable(false) {}
    void updateScrollBars();
    void updateWidgetPosition();
    QPointer<QWidget> widget;
    mutable QSize widgetSize;
    bool resizable;
    Qt::Alignment alignment;
};

QT_END_NAMESPACE

#endif
