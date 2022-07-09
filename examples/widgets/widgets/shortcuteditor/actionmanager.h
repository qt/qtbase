// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

class ActionManager
{
public:
    ActionManager() = default;
    ~ActionManager() = default;

    QList<QAction*> registeredActions() const;

    void registerAction(QAction *action);
    void registerAction(QAction *action, const QString &context, const QString &category);
    QAction *registerAction(const QString &name, const QString &shortcut, const QString &context, const QString &category);

    QString contextForAction(QAction *action);
    QString categoryForAction(QAction *action);

private:
    QList<QAction *> m_actions;
};

#endif
