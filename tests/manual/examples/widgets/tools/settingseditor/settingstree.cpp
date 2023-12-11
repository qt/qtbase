// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "settingstree.h"
#include "variantdelegate.h"

#include <QApplication>
#include <QHeaderView>
#include <QScreen>
#include <QSettings>

SettingsTree::SettingsTree(QWidget *parent)
    : QTreeWidget(parent),
      m_typeChecker(new TypeChecker)
{
    setItemDelegate(new VariantDelegate(m_typeChecker, this));

    setHeaderLabels({tr("Setting"), tr("Type"), tr("Value")});
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(2, QHeaderView::Stretch);

    refreshTimer.setInterval(2000);

    groupIcon.addPixmap(style()->standardPixmap(QStyle::SP_DirClosedIcon),
                        QIcon::Normal, QIcon::Off);
    groupIcon.addPixmap(style()->standardPixmap(QStyle::SP_DirOpenIcon),
                        QIcon::Normal, QIcon::On);
    keyIcon.addPixmap(style()->standardPixmap(QStyle::SP_FileIcon));

    connect(&refreshTimer, &QTimer::timeout, this, &SettingsTree::maybeRefresh);
}

SettingsTree::~SettingsTree() = default;

void SettingsTree::setSettingsObject(const SettingsPtr &newSettings)
{
    settings = newSettings;
    clear();

    if (settings.isNull()) {
        refreshTimer.stop();
    } else {
        refresh();
        if (autoRefresh)
            refreshTimer.start();
    }
}

QSize SettingsTree::sizeHint() const
{
    const QRect availableGeometry = screen()->availableGeometry();
    return QSize(availableGeometry.width() * 2 / 3, availableGeometry.height() * 2 / 3);
}

void SettingsTree::setAutoRefresh(bool autoRefresh)
{
    this->autoRefresh = autoRefresh;
    if (!settings.isNull()) {
        if (autoRefresh) {
            maybeRefresh();
            refreshTimer.start();
        } else {
            refreshTimer.stop();
        }
    }
}

void SettingsTree::setFallbacksEnabled(bool enabled)
{
    if (!settings.isNull()) {
        settings->setFallbacksEnabled(enabled);
        refresh();
    }
}

void SettingsTree::maybeRefresh()
{
    if (state() != EditingState)
        refresh();
}

void SettingsTree::refresh()
{
    if (settings.isNull())
        return;

    disconnect(this, &QTreeWidget::itemChanged,
               this, &SettingsTree::updateSetting);

    settings->sync();
    updateChildItems(nullptr);

    connect(this, &QTreeWidget::itemChanged,
            this, &SettingsTree::updateSetting);
}

bool SettingsTree::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        if (isActiveWindow() && autoRefresh)
            maybeRefresh();
    }
    return QTreeWidget::event(event);
}

void SettingsTree::updateSetting(QTreeWidgetItem *item)
{
    QString key = item->text(0);
    QTreeWidgetItem *ancestor = item->parent();
    while (ancestor) {
        key.prepend(ancestor->text(0) + QLatin1Char('/'));
        ancestor = ancestor->parent();
    }

    settings->setValue(key, item->data(2, Qt::UserRole));
    if (autoRefresh)
        refresh();
}

void SettingsTree::updateChildItems(QTreeWidgetItem *parent)
{
    int dividerIndex = 0;

    const QStringList childGroups = settings->childGroups();
    for (const QString &group : childGroups) {
        QTreeWidgetItem *child;
        int childIndex = findChild(parent, group, dividerIndex);
        if (childIndex != -1) {
            child = childAt(parent, childIndex);
            child->setText(1, QString());
            child->setText(2, QString());
            child->setData(2, Qt::UserRole, QVariant());
            moveItemForward(parent, childIndex, dividerIndex);
        } else {
            child = createItem(group, parent, dividerIndex);
        }
        child->setIcon(0, groupIcon);
        ++dividerIndex;

        settings->beginGroup(group);
        updateChildItems(child);
        settings->endGroup();
    }

    const QStringList childKeys = settings->childKeys();
    for (const QString &key : childKeys) {
        QTreeWidgetItem *child;
        int childIndex = findChild(parent, key, 0);

        if (childIndex == -1 || childIndex >= dividerIndex) {
            if (childIndex != -1) {
                child = childAt(parent, childIndex);
                for (int i = 0; i < child->childCount(); ++i)
                    delete childAt(child, i);
                moveItemForward(parent, childIndex, dividerIndex);
            } else {
                child = createItem(key, parent, dividerIndex);
            }
            child->setIcon(0, keyIcon);
            ++dividerIndex;
        } else {
            child = childAt(parent, childIndex);
        }

        QVariant value = settings->value(key);
        if (value.userType() == QMetaType::UnknownType) {
            child->setText(1, "Invalid");
        } else {
            if (value.typeId() == QMetaType::QString) {
                const QString stringValue = value.toString();
                if (m_typeChecker->boolExp.match(stringValue).hasMatch()) {
                    value.setValue(stringValue.compare("true", Qt::CaseInsensitive) == 0);
                } else if (m_typeChecker->signedIntegerExp.match(stringValue).hasMatch())
                    value.setValue(stringValue.toInt());
            }

            child->setText(1, value.typeName());
        }
        child->setText(2, VariantDelegate::displayText(value));
        child->setData(2, Qt::UserRole, value);
    }

    while (dividerIndex < childCount(parent))
        delete childAt(parent, dividerIndex);
}

QTreeWidgetItem *SettingsTree::createItem(const QString &text,
                                          QTreeWidgetItem *parent, int index)
{
    QTreeWidgetItem *after = nullptr;
    if (index != 0)
        after = childAt(parent, index - 1);

    QTreeWidgetItem *item;
    if (parent)
        item = new QTreeWidgetItem(parent, after);
    else
        item = new QTreeWidgetItem(this, after);

    item->setText(0, text);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    return item;
}

QTreeWidgetItem *SettingsTree::childAt(QTreeWidgetItem *parent, int index) const
{
    return (parent ? parent->child(index) : topLevelItem(index));
}

int SettingsTree::childCount(QTreeWidgetItem *parent) const
{
    return (parent ? parent->childCount() : topLevelItemCount());
}

int SettingsTree::findChild(QTreeWidgetItem *parent, const QString &text,
                            int startIndex) const
{
    for (int i = startIndex; i < childCount(parent); ++i) {
        if (childAt(parent, i)->text(0) == text)
            return i;
    }
    return -1;
}

void SettingsTree::moveItemForward(QTreeWidgetItem *parent, int oldIndex,
                                   int newIndex)
{
    for (int i = 0; i < oldIndex - newIndex; ++i)
        delete childAt(parent, newIndex);
}
