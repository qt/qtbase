// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "shortcuteditorwidget.h"

#include "shortcuteditordelegate.h"
#include "shortcuteditormodel.h"

#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

//! [0]
ShortcutEditorWidget::ShortcutEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    m_model = new ShortcutEditorModel(this);
    m_delegate = new ShortcutEditorDelegate(this);
    m_view = new QTreeView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegateForColumn(static_cast<int>(Column::Shortcut), m_delegate);
    m_view->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_view->setAllColumnsShowFocus(true);
    m_view->header()->resizeSection(0, 250);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    setLayout(layout);

    m_model->setActions();
}
//! [0]
