// Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets/QtWidgets>

const int rowCount = 2000;

class TableDialog : public QDialog
{
    Q_OBJECT
public:
    TableDialog() : model(rowCount, 3) { create(); }
    void create()
    {
        resize(1000, 233);
        gridLayout = new QGridLayout(this);
        tableView = new QTableView(this);

        gridLayout->addWidget(tableView, 0, 0, 2, 1);
        spinPrecision = new QSpinBox(this);
        gridLayout->addWidget(spinPrecision, 0, 1, 1, 1);
        verticalSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
        gridLayout->addItem(verticalSpacer, 1, 1, 1, 1);

        QString ii = QString::fromLatin1("ii");
        QStringList is;
        spinPrecision->setMinimum(-1);
        spinPrecision->setMaximum(rowCount + 2);
        for (int u = 0; u < rowCount; ++u) {
            if (u % 25 == 0)
                ii += QString::fromLatin1("i");
            else
                ii[ii.length() - 1] = QChar::fromLatin1('a' + (u  % 25));
            ii[ii.length() - 2] = QChar::fromLatin1('i');
            is.append(ii);
        }

        for (int u = 0; u < rowCount; ++u) {
            QString col1;
            col1 = QString::fromLatin1("Row: %1").arg(u);
            model.setData(model.index(u, 0), col1);
            model.setData(model.index(u, 1), is[u]);
            model.setData(model.index(u, 2), is[rowCount - u -1]);
        }
        tableView->setModel(&model);

        tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        spinPrecision->setValue(tableView->horizontalHeader()->resizeContentsPrecision());
        connect(spinPrecision, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)));
    } // setupUi
protected slots:
    void slotValueChanged(int newval);
protected:
    QGridLayout *gridLayout;
    QTableView *tableView;
    QSpinBox *spinPrecision;
    QSpacerItem *verticalSpacer;
    QStandardItemModel model;
};

void TableDialog::slotValueChanged(int newval)
{
    tableView->horizontalHeader()->setResizeContentsPrecision(newval);
    tableView->resizeColumnsToContents();
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TableDialog d1;
    d1.show();
    app.exec();
}

#include "testtable1.moc"
