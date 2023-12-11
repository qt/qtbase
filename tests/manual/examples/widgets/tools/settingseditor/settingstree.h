// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SETTINGSTREE_H
#define SETTINGSTREE_H

#include <QIcon>
#include <QTimer>
#include <QTreeWidget>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

struct TypeChecker;

class SettingsTree : public QTreeWidget
{
    Q_OBJECT

public:
    using SettingsPtr = QSharedPointer<QSettings>;
    using TypeCheckerPtr = QSharedPointer<TypeChecker>;

    SettingsTree(QWidget *parent = nullptr);
    ~SettingsTree();

    void setSettingsObject(const SettingsPtr &settings);
    QSize sizeHint() const override;

public slots:
    void setAutoRefresh(bool autoRefresh);
    void setFallbacksEnabled(bool enabled);
    void maybeRefresh();
    void refresh();

protected:
    bool event(QEvent *event) override;

private slots:
    void updateSetting(QTreeWidgetItem *item);

private:
    void updateChildItems(QTreeWidgetItem *parent);
    QTreeWidgetItem *createItem(const QString &text, QTreeWidgetItem *parent,
                                int index);
    QTreeWidgetItem *childAt(QTreeWidgetItem *parent, int index) const;
    int childCount(QTreeWidgetItem *parent) const;
    int findChild(QTreeWidgetItem *parent, const QString &text, int startIndex) const;
    void moveItemForward(QTreeWidgetItem *parent, int oldIndex, int newIndex);

    SettingsPtr settings;
    TypeCheckerPtr m_typeChecker;
    QTimer refreshTimer;
    QIcon groupIcon;
    QIcon keyIcon;
    bool autoRefresh = false;
};

#endif
