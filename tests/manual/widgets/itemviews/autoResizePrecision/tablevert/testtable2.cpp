/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets/QtWidgets>

const int columnCount = 1500;

class TableDialog : public QDialog
{
    Q_OBJECT
public:
    TableDialog() : model(2, columnCount) { create(); }
    void create()
    {
        resize(1200, 400);
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
        spinPrecision->setMaximum(columnCount + 2);

        QFont f = QApplication::font();
        for (int u = 0; u < columnCount; ++u) {
            int size = 10 + (u % 63);
            f.setPixelSize(size);
            QString col;
            if (u % 50 < 25)
                col = QChar::fromLatin1('a' + (u % 25));
            else
                col = QChar::fromLatin1('A' + (u % 25));

            int v = columnCount - u - 1;
            model.setData(model.index(0, u), col);
            model.setData(model.index(1, v), col);

            model.setData(model.index(0, u), f,  Qt::FontRole);
            model.setData(model.index(1, v), f,  Qt::FontRole);
        }
        tableView->setModel(&model);

        for (int u = 0; u < columnCount; ++ u)
            tableView->horizontalHeader()->resizeSection(u, 60);

        // Make last index in first row a bit special
        f.setPixelSize(96);
        model.setData(model.index(0, columnCount - 1), f,  Qt::FontRole);
        model.setData(model.index(0, columnCount - 1), QString::fromLatin1("qI"));
        tableView->horizontalHeader()->resizeSection(columnCount - 1, 140);

        tableView->verticalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tableView->verticalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        spinPrecision->setValue(tableView->verticalHeader()->resizeContentsPrecision());
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
    tableView->verticalHeader()->setResizeContentsPrecision(newval);
    tableView->resizeRowsToContents();
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TableDialog d1;
    d1.show();
    app.exec();
}

#include "testtable2.moc"
