// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONNECTIONWIDGET_H
#define CONNECTIONWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QSqlDatabase)

class ConnectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionWidget(QWidget *parent = nullptr);
    ~ConnectionWidget() override;

    QSqlDatabase currentDatabase() const;

signals:
    void tableActivated(const QString &table);
    void metaDataRequested(const QString &tableName);

public slots:
    void refresh();
    void showMetaData();
    void onItemActivated(QTreeWidgetItem *item);
    void onCurrentItemChanged(QTreeWidgetItem *current);

private:
    void setActive(QTreeWidgetItem *);

    QTreeWidget *tree;
    QAction *metaDataAction;
    QString activeDb;
};

#endif
