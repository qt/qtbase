/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
