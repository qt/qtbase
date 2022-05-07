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

#ifndef QSHORTCUT_P_H
#define QSHORTCUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qshortcut.h"
#include <QtGui/qkeysequence.h>

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qobject_p.h>

#include <private/qshortcutmap_p.h>


QT_BEGIN_NAMESPACE

class QShortcutMap;

/*
    \internal
    Private data accessed through d-pointer.
*/
class Q_GUI_EXPORT QShortcutPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QShortcut)
public:
    QShortcutPrivate() = default;

    virtual QShortcutMap::ContextMatcher contextMatcher() const;
    virtual bool handleWhatsThis() { return false; }

    QList<QKeySequence> sc_sequences;
    QString sc_whatsthis;
    Qt::ShortcutContext sc_context = Qt::WindowShortcut;
    bool sc_enabled = true;
    bool sc_autorepeat = true;
    QList<int> sc_ids;
    void redoGrab(QShortcutMap &map);
};

QT_END_NAMESPACE

#endif // QSHORTCUT_P_H
