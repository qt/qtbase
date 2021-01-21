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

#ifndef QFRAME_P_H
#define QFRAME_P_H

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
#include "private/qwidget_p.h"
#include "qframe.h"

QT_BEGIN_NAMESPACE

// ### unexport this class when and if QAbstractScrollAreaPrivate is unexported
class Q_WIDGETS_EXPORT QFramePrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QFrame)
public:
    QFramePrivate();
    ~QFramePrivate();

    void        updateFrameWidth();
    void        updateStyledFrameWidths();

    QRect       frect;
    int         frameStyle;
    short       lineWidth;
    short       midLineWidth;
    short       frameWidth;
    short       leftFrameWidth, rightFrameWidth;
    short       topFrameWidth, bottomFrameWidth;

    inline void init();

};

QT_END_NAMESPACE

#endif // QFRAME_P_H
