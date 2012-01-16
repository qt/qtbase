/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QTableView>
#include <QVBoxLayout>
#include <QItemDelegate>
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

    QItemDelegate *delegate = qobject_cast<QItemDelegate*>(m_view->itemDelegate());
    Q_ASSERT(delegate != 0);
    static EditorFactory editor_factory;
    delegate->setItemEditorFactory(&editor_factory);

    m_view->setModel(m_model);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_view);
}
