// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTOOLTIP_H
#define QTOOLTIP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

#include <chrono>

QT_REQUIRE_CONFIG(tooltip);
QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QToolTip
{
    QToolTip() = delete;
public:
    static void showText(const QPoint &pos, const QString &text, QWidget *w,
                         const QRect &rect, int msecDisplayTime);
    static void showText(QPoint pos, const QString &text, QWidget *w = nullptr,
                         const QRect &rect = {},
                         std::chrono::milliseconds displayTime = std::chrono::milliseconds(-1));
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
