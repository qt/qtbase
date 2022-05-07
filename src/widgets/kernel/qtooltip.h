/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QTOOLTIP_H
#define QTOOLTIP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

QT_REQUIRE_CONFIG(tooltip);
QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QToolTip
{
    QToolTip() = delete;
public:
    static void showText(const QPoint &pos, const QString &text,
                         QWidget *w = nullptr, const QRect &rect = {}, int msecShowTime = -1);
    static inline void hideText() { showText(QPoint(), QString()); }

    static bool isVisible();
    static QString text();

    static QPalette palette();
    static void setPalette(const QPalette &);
    static QFont font();
    static void setFont(const QFont &);
};

QT_END_NAMESPACE

#endif // QTOOLTIP_H
