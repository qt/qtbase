/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#ifndef QDESKTOPWIDGET_P_H
#define QDESKTOPWIDGET_P_H

#include "QDesktopWidget"
#include "private/qwidget_p.h"

#include <QtCore/qalgorithms.h>

QT_BEGIN_NAMESPACE

class QDesktopScreenWidget : public QWidget {
    Q_OBJECT
public:
    explicit QDesktopScreenWidget(QScreen *screen, const QRect &geometry);

    int screenNumber() const;
    void setScreenGeometry(const QRect &geometry);

    QScreen *screen() const { return m_screen.data(); }
    QRect screenGeometry() const { return m_geometry; }

private:
    // The widget updates its screen and geometry automatically. We need to save them separately
    // to detect changes, and trigger the appropriate signals.
    const QPointer<QScreen> m_screen;
    QRect m_geometry;
};

class QDesktopWidgetPrivate : public QWidgetPrivate {
    Q_DECLARE_PUBLIC(QDesktopWidget)

public:
    ~QDesktopWidgetPrivate() { qDeleteAll(screens); }
    void _q_updateScreens();
    void _q_availableGeometryChanged();
    QDesktopScreenWidget *widgetForScreen(QScreen *qScreen) const;

    QList<QDesktopScreenWidget *> screens;
};

QT_END_NAMESPACE

#endif // QDESKTOPWIDGET_QPA_P_H
