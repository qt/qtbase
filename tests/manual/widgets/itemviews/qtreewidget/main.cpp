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

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QDialog>
#include <QApplication>
#include <QHeaderView>

class ExampleDlg : public QDialog
{
    Q_OBJECT
public:
    QVBoxLayout *groupLayout;
    QVBoxLayout *dialogLayout;
    QTreeWidget *treeWidget;
    QGroupBox   *groupBox;
    QRadioButton *radioFirstName;
    QRadioButton *radioLastName;
    QRadioButton *radioDeveloperNo;
    QRadioButton *radioTitle;

    ExampleDlg() : QDialog(0)
    {
        dialogLayout = new QVBoxLayout(this);
        treeWidget = new QTreeWidget(this);
        dialogLayout->addWidget(treeWidget);

        groupBox = new QGroupBox(this);
        groupLayout = new QVBoxLayout(groupBox);
        radioFirstName = new QRadioButton("First Name", groupBox);
        groupLayout->addWidget(radioFirstName);
        radioLastName = new QRadioButton("Last Name", groupBox);
        groupLayout->addWidget(radioLastName);
        radioDeveloperNo = new QRadioButton("Developer No.", groupBox);
        groupLayout->addWidget(radioDeveloperNo);
        radioTitle = new QRadioButton("Title", groupBox);
        groupLayout->addWidget(radioTitle);
        dialogLayout->addWidget(groupBox);

        QStringList item1sl("Barry");
        item1sl.append("Butter");
        item1sl.append("12199");
        item1sl.append("Key Maintainer");
        QTreeWidgetItem *item1 = new QTreeWidgetItem(treeWidget, item1sl);

        QStringList item2sl("Cordon");
        item2sl.append("Rampsey");
        item2sl.append("59299");
        item2sl.append("Maintainer");
        QTreeWidgetItem *item2 = new QTreeWidgetItem(item1, item2sl);

        QStringList item3sl("Samuel le");
        item3sl.append("Smackson");
        item3sl.append("708");
        item3sl.append("Contributer");
        /* QTreeWidgetItem *item3 = */ new QTreeWidgetItem(item2, item3sl);

        QStringList item4sl("Georg");
        item4sl.append("Ambush");
        item4sl.append("86999");
        item4sl.append("Area Maintainer");
        QTreeWidgetItem *item4 = new QTreeWidgetItem(item1, item4sl);

        QStringList item5sl("Arne");
        item5sl.append("Strassenleger");
        item5sl.append("338999");
        item5sl.append("Approver");
        /* QTreeWidgetItem *item4 =*/ new QTreeWidgetItem(item4, item5sl);

        treeWidget->setColumnCount(item2sl.size());
        QStringList itemInfo("First Name");
        itemInfo.append("Last Name");
        itemInfo.append("Developer No.");
        // Developer no. could also have been social security number og some other id.
        itemInfo.append("Title");
        treeWidget->setHeaderLabels(itemInfo);
        radioLastName->setChecked(true);

        connect(radioFirstName, SIGNAL(toggled(bool)), this, SLOT(fixDataInTree(bool)));
        connect(radioLastName, SIGNAL(toggled(bool)), this, SLOT(fixDataInTree(bool)));
        connect(radioDeveloperNo, SIGNAL(toggled(bool)), this, SLOT(fixDataInTree(bool)));
        connect(radioTitle, SIGNAL(toggled(bool)), this, SLOT(fixDataInTree(bool)));
        treeWidget->setTreePosition(-1);
        treeWidget->header()->swapSections(0, 1);
    }

    protected slots:
    void fixDataInTree(bool checked)
    {
        if (!checked)
            return;
        int colInTree = 0; // first Name
        if (radioLastName->isChecked())
            colInTree = 1;
        if (radioDeveloperNo->isChecked())
            colInTree = 2;
        if (radioTitle->isChecked())
            colInTree = 3;
        treeWidget->header()->swapSections(0, treeWidget->header()->visualIndex(colInTree));
    }
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ExampleDlg d;
    d.show();
    app.exec();
}

#include "main.moc"
