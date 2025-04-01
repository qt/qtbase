// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSqlRelationalDelegate
    \inmodule QtSql
    \brief The QSqlRelationalDelegate class provides a delegate that is used to
    display and edit data from a QSqlRelationalTableModel.

    Unlike the default delegate, QSqlRelationalDelegate provides a
    combobox for fields that are foreign keys into other tables. To
    use the class, simply call QAbstractItemView::setItemDelegate()
    on the view with an instance of QSqlRelationalDelegate:

    \snippet relationaltablemodel/relationaltablemodel.cpp 4

    The \l{relationaltablemodel}{Relational Table Model} example
    (shown below) illustrates how to use QSqlRelationalDelegate in
    conjunction with QSqlRelationalTableModel to provide tables with
    foreign key support.

    \image relationaltable.png {The user is able to edit a foreign key in a relational table}

    \sa QSqlRelationalTableModel, {Model/View Programming}
*/


/*!
    \fn QSqlRelationalDelegate::QSqlRelationalDelegate(QObject *parent)

    Constructs a QSqlRelationalDelegate object with the given \a
    parent.
*/

/*!
    \fn QSqlRelationalDelegate::~QSqlRelationalDelegate()

    Destroys the QSqlRelationalDelegate object and frees any
    allocated resources.
*/

/*!
    \fn QWidget *QSqlRelationalDelegate::createEditor(QWidget *parent,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index) const
    \reimp
*/

/*!
    \fn void QSqlRelationalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                                  const QModelIndex &index) const
    \reimp
*/

QT_END_NAMESPACE
