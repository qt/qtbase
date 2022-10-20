/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWIDGETRESIZEHANDLER_P_H
#define QWIDGETRESIZEHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtCore/qobject.h"
#include "QtCore/qpoint.h"

QT_REQUIRE_CONFIG(resizehandler);

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QKeyEvent;

class QWidgetResizeHandler : public QObject
{
    Q_OBJECT

public:
    explicit QWidgetResizeHandler(QWidget *parent, QWidget *cw = nullptr);
    void setEnabled(bool b);
    bool isEnabled() const;

    bool isButtonDown() const { return buttonDown; }

    void setExtraHeight(int h) { extrahei = h; }

    void setFrameWidth(int w) { fw = w; }

    void doResize();

Q_SIGNALS:
    void activate();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);

private:
    Q_DISABLE_COPY_MOVE(QWidgetResizeHandler)

    enum MousePosition {
        Nowhere,
        TopLeft, BottomRight, BottomLeft, TopRight,
        Top, Bottom, Left, Right,
        Center
    };

    QWidget *widget;
    QWidget *childWidget;
    QPoint moveOffset;
    QPoint invertedMoveOffset;
    MousePosition mode;
    int fw;
    int extrahei;
    int range;
    uint buttonDown     :1;
    uint active         :1;
    uint enabled        :1;

    void setMouseCursor(MousePosition m);
    bool isResizing() const {
        return active && mode != Center;
    }
};

QT_END_NAMESPACE

#endif // QWIDGETRESIZEHANDLER_P_H
