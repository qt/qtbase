// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [16]
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT name, salary FROM employee");
    model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, tr("Salary"));
//! [17]
    QTableView *view = new QTableView;
//! [17] //! [18]
    view->setModel(model);
//! [18] //! [19]
    view->show();
//! [16] //! [19] //! [20]
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
//! [20]
    }

    QSqlQueryModel model;
    model.setQuery("SELECT name, salary FROM employee");
    int salary = model.record(4).value("salary").toInt();
    Q_UNUSED(salary);

    {
    int salary = model.data(model.index(4, 1)).toInt();
    Q_UNUSED(salary);
    }

    for (int row = 0; row < model.rowCount(); ++row) {
        for (int col = 0; col < model.columnCount(); ++col) {
            qDebug() << model.data(model.index(row, col));
        }
    }
}

class MyModel : public QSqlQueryModel
{
public:
    QVariant data(const QModelIndex &item, int role) const override;
    void fetchModel();

    int m_specialColumnNo;
};

QVariant MyModel::data(const QModelIndex &item, int role) const
{
    if (item.column() == m_specialColumnNo) {
        // handle column separately
    }
    return QSqlQueryModel::data(item, role);
}

void QSqlTableModel_snippets()
{
//! [24]
    QSqlTableModel *model = new QSqlTableModel;
    model->setTable("employee");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, tr("Salary"));

    QTableView *view = new QTableView;
    view->setModel(model);
    view->hideColumn(0); // don't show the ID
    view->show();
//! [24]
}


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QSqlDatabase_snippets();
    QSqlField_snippets();
    QSqlQuery_snippets();
    QSqlQueryModel_snippets();
    QSqlTableModel_snippets();

    XyzDriver driver;
    XyzResult result(&driver);
}
