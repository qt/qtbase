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

#ifndef QDYNAMICTOOLBARSEPARATOR_P_H
#define QDYNAMICTOOLBARSEPARATOR_P_H

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
#include "QtWidgets/qwidget.h"

QT_REQUIRE_CONFIG(toolbar);

QT_BEGIN_NAMESPACE

class QStyleOption;
class QToolBar;

class QToolBarSeparator : public QWidget
{
    Q_OBJECT
    Qt::Orientation orient;

public:
    explicit QToolBarSeparator(QToolBar *parent);

    Qt::Orientation orientation() const;

    QSize sizeHint() const override;

    void paintEvent(QPaintEvent *) override;
    void initStyleOption(QStyleOption *option) const;

public Q_SLOTS:
    void setOrientation(Qt::Orientation orientation);
};

QT_END_NAMESPACE

#endif // QDYNAMICTOOLBARSEPARATOR_P_H
