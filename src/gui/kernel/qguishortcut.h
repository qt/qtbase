/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QGUISHORTCUT_H
#define QGUISHORTCUT_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qkeysequence.h>
#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(shortcut);

QT_BEGIN_NAMESPACE

class QGuiShortcutPrivate;
class QWindow;

class Q_GUI_EXPORT QGuiShortcut : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGuiShortcut)
    Q_PROPERTY(QKeySequence key READ key WRITE setKey)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat)
    Q_PROPERTY(Qt::ShortcutContext context READ context WRITE setContext)
public:
    explicit QGuiShortcut(QWindow *parent);
    explicit QGuiShortcut(const QKeySequence& key, QWindow *parent,
                          const char *member = nullptr, const char *ambiguousMember = nullptr,
                          Qt::ShortcutContext context = Qt::WindowShortcut);
    ~QGuiShortcut();

    void setKey(const QKeySequence& key);
    QKeySequence key() const;

    void setEnabled(bool enable);
    bool isEnabled() const;

    void setContext(Qt::ShortcutContext context);
    Qt::ShortcutContext context() const;

    void setAutoRepeat(bool on);
    bool autoRepeat() const;

    int id() const;

Q_SIGNALS:
    void activated();
    void activatedAmbiguously();

protected:
    QGuiShortcut(QGuiShortcutPrivate &dd, QObject *parent);
    QGuiShortcut(QGuiShortcutPrivate &dd, const QKeySequence& key, QObject *parent,
                 const char *member, const char *ambiguousMember,
                 Qt::ShortcutContext context);

    bool event(QEvent *e) override;
};

QT_END_NAMESPACE

#endif // QGUISHORTCUT_H
