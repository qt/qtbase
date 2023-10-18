// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QTableView>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QItemEditorFactory>
#include <QDoubleSpinBox>

#include "localewidget.h"
#include "localemodel.h"

class DoubleEditorCreator : public QItemEditorCreatorBase
{
public:
    QWidget *createWidget(QWidget *parent) const {
        QDoubleSpinBox *w = new QDoubleSpinBox(parent);
        w->setDecimals(4);
        w->setRange(-10000.0, 10000.0);
        return w;
    }
    virtual QByteArray valuePropertyName() const {
        return QByteArray("value");
    }
};

class EditorFactory : public QItemEditorFactory
{
public:
    EditorFactory() {
        static DoubleEditorCreator double_editor_creator;
        registerEditor(QVariant::Double, &double_editor_creator);
    }
};

LocaleWidget::LocaleWidget(QWidget *parent)
    : QWidget(parent)
{
    m_model = new LocaleModel(this);
    m_view = new QTableView(this);

    QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(m_view->itemDelegate());
    Q_ASSERT(delegate != 0);
    static EditorFactory editor_factory;
    delegate->setItemEditorFactory(&editor_factory);

    m_view->setModel(m_model);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_view);
}
