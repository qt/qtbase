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

#ifndef QSCROLLBAR_P_H
#define QSCROLLBAR_P_H

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
#include "private/qabstractslider_p.h"
#include "qstyle.h"

QT_REQUIRE_CONFIG(scrollbar);

QT_BEGIN_NAMESPACE

class QScrollBarPrivate : public QAbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(QScrollBar)
public:
    QStyle::SubControl pressedControl;
    bool pointerOutsidePressedControl;

    int clickOffset, snapBackPosition;

    void activateControl(uint control, int threshold = 500);
    void stopRepeatAction();
    int pixelPosToRangeValue(int pos) const;
    void init();
    bool updateHoverControl(const QPoint &pos);
    QStyle::SubControl newHoverControl(const QPoint &pos);

    QStyle::SubControl hoverControl;
    QRect hoverRect;

    bool transient;
    void setTransient(bool value);

    bool flashed;
    int flashTimer;
    void flash();
};

QT_END_NAMESPACE

#endif // QSCROLLBAR_P_H
