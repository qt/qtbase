/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QACTION_WIDGETS_P_H
#define QACTION_WIDGETS_P_H

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

#include <QtGui/private/qaction_p.h>
#if QT_CONFIG(menu)
#include <QtWidgets/qmenu.h>
#endif

QT_REQUIRE_CONFIG(action);

QT_BEGIN_NAMESPACE

class QShortcutMap;

class Q_WIDGETS_EXPORT QtWidgetsActionPrivate : public QActionPrivate
{
    Q_DECLARE_PUBLIC(QAction)
public:
    QtWidgetsActionPrivate() = default;
    ~QtWidgetsActionPrivate();

    void destroy() override;

#if QT_CONFIG(shortcut)
    QShortcutMap::ContextMatcher contextMatcher() const override;
#endif

#if QT_CONFIG(menu)
    QPointer<QMenu> m_menu;

    QObject *menu() const override;
    void setMenu(QObject *menu) override;
#endif
};

QT_END_NAMESPACE

#endif // QACTION_WIDGETS_P_H
