// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "actionmanager.h"

#include <QAction>
#include <QApplication>
#include <QString>
#include <QVariant>

static const char *kDefaultShortcutPropertyName = "defaultShortcuts";
static const char *kIdPropertyName = "id";
static const char *kAuthorName = "qt";

struct ActionIdentifier {
    QString author;
    QString context;
    QString category;
    QString name;
};

QList<QAction *> ActionManager::registeredActions() const
{
    return m_actions;
}

void ActionManager::registerAction(QAction *action)
{
    action->setProperty(kDefaultShortcutPropertyName, QVariant::fromValue(action->shortcut()));
    m_actions.append(action);
}

void ActionManager::registerAction(QAction *action, const QString &context, const QString &category)
{
    action->setProperty(kIdPropertyName, QVariant::fromValue(ActionIdentifier{
        kAuthorName, context, category, action->text()
    }));
    registerAction(action);
}

QAction *ActionManager::registerAction(const QString &name, const QString &shortcut, const QString &context, const QString &category)
{
    QAction *action = new QAction(name, qApp);
    action->setShortcut(QKeySequence(shortcut));
    registerAction(action, context, category);
    return action;
}

QString ActionManager::contextForAction(QAction *action)
{
    return action->property(kIdPropertyName).value<ActionIdentifier>().context;
}

QString ActionManager::categoryForAction(QAction *action)
{
    return action->property(kIdPropertyName).value<ActionIdentifier>().category;
}
