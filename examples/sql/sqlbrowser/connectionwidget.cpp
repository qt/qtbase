/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "connectionwidget.h"

#include <QtWidgets>
#include <QtSql>

ConnectionWidget::ConnectionWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    tree = new QTreeWidget(this);
    tree->setObjectName(QLatin1String("tree"));
    tree->setHeaderLabels(QStringList(tr("database")));
    tree->header()->setSectionResizeMode(QHeaderView::Stretch);
    QAction *refreshAction = new QAction(tr("Refresh"), tree);
    metaDataAction = new QAction(tr("Show Schema"), tree);
    connect(refreshAction, SIGNAL(triggered()), SLOT(refresh()));
    connect(metaDataAction, SIGNAL(triggered()), SLOT(showMetaData()));
    tree->addAction(refreshAction);
    tree->addAction(metaDataAction);
    tree->setContextMenuPolicy(Qt::ActionsContextMenu);

    layout->addWidget(tree);

    QMetaObject::connectSlotsByName(this);
}

ConnectionWidget::~ConnectionWidget()
{
}

static QString qDBCaption(const QSqlDatabase &db)
{
    QString nm = db.driverName();
    nm.append(QLatin1Char(':'));
    if (!db.userName().isEmpty())
        nm.append(db.userName()).append(QLatin1Char('@'));
    nm.append(db.databaseName());
    return nm;
}

void ConnectionWidget::refresh()
{
    tree->clear();
    QStringList connectionNames = QSqlDatabase::connectionNames();

    bool gotActiveDb = false;
    for (int i = 0; i < connectionNames.count(); ++i) {
        QTreeWidgetItem *root = new QTreeWidgetItem(tree);
        QSqlDatabase db = QSqlDatabase::database(connectionNames.at(i), false);
        root->setText(0, qDBCaption(db));
        if (connectionNames.at(i) == activeDb) {
            gotActiveDb = true;
            setActive(root);
        }
        if (db.isOpen()) {
            QStringList tables = db.tables();
            for (int t = 0; t < tables.count(); ++t) {
                QTreeWidgetItem *table = new QTreeWidgetItem(root);
                table->setText(0, tables.at(t));
            }
        }
    }
    if (!gotActiveDb) {
        activeDb = connectionNames.value(0);
        setActive(tree->topLevelItem(0));
    }

    tree->doItemsLayout(); // HACK
}

QSqlDatabase ConnectionWidget::currentDatabase() const
{
    return QSqlDatabase::database(activeDb);
}

static void qSetBold(QTreeWidgetItem *item, bool bold)
{
    QFont font = item->font(0);
    font.setBold(bold);
    item->setFont(0, font);
}

void ConnectionWidget::setActive(QTreeWidgetItem *item)
{
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (tree->topLevelItem(i)->font(0).bold())
            qSetBold(tree->topLevelItem(i), false);
    }

    if (!item)
        return;

    qSetBold(item, true);
    activeDb = QSqlDatabase::connectionNames().value(tree->indexOfTopLevelItem(item));
}

void ConnectionWidget::on_tree_itemActivated(QTreeWidgetItem *item, int /* column */)
{

    if (!item)
        return;

    if (!item->parent()) {
        setActive(item);
    } else {
        setActive(item->parent());
        emit tableActivated(item->text(0));
    }
}

void ConnectionWidget::showMetaData()
{
    QTreeWidgetItem *cItem = tree->currentItem();
    if (!cItem || !cItem->parent())
        return;
    setActive(cItem->parent());
    emit metaDataRequested(cItem->text(0));
}

void ConnectionWidget::on_tree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    metaDataAction->setEnabled(current && current->parent());
}

