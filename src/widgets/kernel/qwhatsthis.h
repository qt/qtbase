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

#ifndef QWHATSTHIS_H
#define QWHATSTHIS_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qcursor.h>

QT_REQUIRE_CONFIG(whatsthis);

QT_BEGIN_NAMESPACE

#if QT_CONFIG(action)
class QAction;
#endif // QT_CONFIG(action)

class Q_WIDGETS_EXPORT QWhatsThis
{
    QWhatsThis() = delete;

public:
    static void enterWhatsThisMode();
    static bool inWhatsThisMode();
    static void leaveWhatsThisMode();

    static void showText(const QPoint &pos, const QString &text, QWidget *w = nullptr);
    static void hideText();

#if QT_CONFIG(action)
    static QAction *createAction(QObject *parent = nullptr);
#endif // QT_CONFIG(action)

};

QT_END_NAMESPACE

#endif // QWHATSTHIS_H
