/****************************************************************************
**
** Copyright (C) 2013 Thorbjørn Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

class KeepSizeExampleDlg : public QDialog
{
    Q_OBJECT
public:
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout;
    QCheckBox *checkBox;
    QCheckBox *checkBox2;
    QCheckBox *checkBox3;
    QCheckBox *checkBox4;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout2;
    QRadioButton *radioButton;
    QRadioButton *radioButton2;
    QRadioButton *radioButton3;
    QTableView *tableView;
    QPushButton *pushButton;
    QSpacerItem *horizontalSpacer;

    KeepSizeExampleDlg()
    {
        QWidget *form = this;
        form->resize(408, 295);
        gridLayout = new QGridLayout(form);
        horizontalLayout = new QHBoxLayout();
        verticalLayout = new QVBoxLayout();
        checkBox = new QCheckBox(form);
        verticalLayout->addWidget(checkBox);
        checkBox2 = new QCheckBox(form);
        verticalLayout->addWidget(checkBox2);
        checkBox3 = new QCheckBox(form);
        verticalLayout->addWidget(checkBox3);
        checkBox4 = new QCheckBox(form);
        verticalLayout->addWidget(checkBox4);
        horizontalLayout->addLayout(verticalLayout);
        groupBox = new QGroupBox(form);
        verticalLayout2 = new QVBoxLayout(groupBox);
        radioButton = new QRadioButton(groupBox);
        verticalLayout2->addWidget(radioButton);
        radioButton2 = new QRadioButton(groupBox);
        verticalLayout2->addWidget(radioButton2);
        radioButton3 = new QRadioButton(groupBox);
        verticalLayout2->addWidget(radioButton3);
        horizontalLayout->addWidget(groupBox);
        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 2);
        tableView = new QTableView(form);
        gridLayout->addWidget(tableView, 1, 0, 1, 2);
        pushButton = new QPushButton(form);
        gridLayout->addWidget(pushButton, 2, 0, 1, 1);
        horizontalSpacer = new QSpacerItem(340, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        gridLayout->addItem(horizontalSpacer, 2, 1, 1, 1);
        checkBox->setText(QString::fromUtf8("CheckBox1"));
        checkBox2->setText(QString::fromUtf8("CheckBox2"));
        checkBox3->setText(QString::fromUtf8("CheckBox - for client A only"));
        checkBox4->setText(QString::fromUtf8("CheckBox - also for client A"));
        groupBox->setTitle(QString::fromUtf8("Mode"));
        radioButton->setText(QString::fromUtf8("Mode 1"));
        radioButton2->setText(QString::fromUtf8("Mode 2"));
        radioButton3->setText(QString::fromUtf8("Mode 3"));
        pushButton->setText(QString::fromUtf8("&Hide/Show"));

        QObject::connect(pushButton, SIGNAL(clicked()), this, SLOT(showOrHide()));
    }

    protected slots:
    void showOrHide()
    {
        if (checkBox3->isVisible()) {
            checkBox3->hide();
            checkBox4->hide();
        } else {
            checkBox3->show();
            checkBox4->show();
        }
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KeepSizeExampleDlg d;
    QSizePolicy policyKeepSpace = d.checkBox3->sizePolicy();
    policyKeepSpace.setRetainSizeWhenHidden(true);
    d.checkBox3->setSizePolicy(policyKeepSpace);
    d.checkBox4->setSizePolicy(policyKeepSpace);
    d.show();
    app.exec();
}

#include "main.moc"
