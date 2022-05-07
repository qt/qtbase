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

#ifndef QACTION_P_H
#define QACTION_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qaction.h>
#include <QtGui/qfont.h>
#if QT_CONFIG(shortcut)
#  include <QtGui/private/qshortcutmap_p.h>
#endif
#include "private/qobject_p.h"

QT_REQUIRE_CONFIG(action);

QT_BEGIN_NAMESPACE

class QShortcutMap;

class Q_GUI_EXPORT QActionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAction)
public:
    QActionPrivate();
    ~QActionPrivate();

    virtual void destroy();

#if QT_CONFIG(shortcut)
    virtual QShortcutMap::ContextMatcher contextMatcher() const;
#endif

    static QActionPrivate *get(QAction *q)
    {
        return q->d_func();
    }

    bool setEnabled(bool enable, bool byGroup);
    void setVisible(bool b);

    QPointer<QActionGroup> group;
    QString text;
    QString iconText;
    QIcon icon;
    QString tooltip;
    QString statustip;
    QString whatsthis;
#if QT_CONFIG(shortcut)
    QList<QKeySequence> shortcuts;
#endif
    QVariant userData;

    QObjectList associatedObjects;
    virtual QObject *menu() const;
    virtual void setMenu(QObject *menu);

#if QT_CONFIG(shortcut)
    QList<int> shortcutIds;
    Qt::ShortcutContext shortcutContext = Qt::WindowShortcut;
    uint autorepeat : 1;
#endif
    QFont font;
    uint enabled : 1, explicitEnabled : 1, explicitEnabledValue : 1;
    uint visible : 1, forceInvisible : 1;
    uint checkable : 1;
    uint checked : 1;
    uint separator : 1;
    uint fontSet : 1;

    int iconVisibleInMenu : 2;  // Only has values -1, 0, and 1
    int shortcutVisibleInContextMenu : 2; // Only has values -1, 0, and 1

    QAction::MenuRole menuRole = QAction::TextHeuristicRole;
    QAction::Priority priority = QAction::NormalPriority;

#if QT_CONFIG(shortcut)
    void redoGrab(QShortcutMap &map);
    void redoGrabAlternate(QShortcutMap &map);
    void setShortcutEnabled(bool enable, QShortcutMap &map);
#endif // QT_NO_SHORTCUT

    bool showStatusText(QObject *widget, const QString &str);
    void sendDataChanged();
};

QT_END_NAMESPACE

#endif // QACTION_P_H
