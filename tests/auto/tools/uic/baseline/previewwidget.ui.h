/*
*********************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the autotests of the Qt Toolkit.
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
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'previewwidget.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_PreviewWidget
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *vboxLayout;
    QLineEdit *LineEdit1;
    QComboBox *ComboBox1;
    QHBoxLayout *hboxLayout;
    QSpinBox *SpinBox1;
    QPushButton *PushButton1;
    QScrollBar *ScrollBar1;
    QSlider *Slider1;
    QListWidget *listWidget;
    QSpacerItem *spacerItem;
    QProgressBar *ProgressBar1;
    QGroupBox *ButtonGroup2;
    QVBoxLayout *vboxLayout1;
    QCheckBox *CheckBox1;
    QCheckBox *CheckBox2;
    QGroupBox *ButtonGroup1;
    QVBoxLayout *vboxLayout2;
    QRadioButton *RadioButton1;
    QRadioButton *RadioButton2;
    QRadioButton *RadioButton3;

    void setupUi(QWidget *qdesigner_internal__PreviewWidget)
    {
        if (qdesigner_internal__PreviewWidget->objectName().isEmpty())
            qdesigner_internal__PreviewWidget->setObjectName(QStringLiteral("qdesigner_internal__PreviewWidget"));
        qdesigner_internal__PreviewWidget->resize(471, 251);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qdesigner_internal__PreviewWidget->sizePolicy().hasHeightForWidth());
        qdesigner_internal__PreviewWidget->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(qdesigner_internal__PreviewWidget);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        LineEdit1 = new QLineEdit(qdesigner_internal__PreviewWidget);
        LineEdit1->setObjectName(QStringLiteral("LineEdit1"));

        vboxLayout->addWidget(LineEdit1);

        ComboBox1 = new QComboBox(qdesigner_internal__PreviewWidget);
        ComboBox1->setObjectName(QStringLiteral("ComboBox1"));

        vboxLayout->addWidget(ComboBox1);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        SpinBox1 = new QSpinBox(qdesigner_internal__PreviewWidget);
        SpinBox1->setObjectName(QStringLiteral("SpinBox1"));

        hboxLayout->addWidget(SpinBox1);

        PushButton1 = new QPushButton(qdesigner_internal__PreviewWidget);
        PushButton1->setObjectName(QStringLiteral("PushButton1"));

        hboxLayout->addWidget(PushButton1);


        vboxLayout->addLayout(hboxLayout);

        ScrollBar1 = new QScrollBar(qdesigner_internal__PreviewWidget);
        ScrollBar1->setObjectName(QStringLiteral("ScrollBar1"));
        ScrollBar1->setOrientation(Qt::Horizontal);

        vboxLayout->addWidget(ScrollBar1);

        Slider1 = new QSlider(qdesigner_internal__PreviewWidget);
        Slider1->setObjectName(QStringLiteral("Slider1"));
        Slider1->setOrientation(Qt::Horizontal);

        vboxLayout->addWidget(Slider1);

        listWidget = new QListWidget(qdesigner_internal__PreviewWidget);
        listWidget->setObjectName(QStringLiteral("listWidget"));
        listWidget->setMaximumSize(QSize(32767, 50));

        vboxLayout->addWidget(listWidget);


        gridLayout->addLayout(vboxLayout, 0, 1, 3, 1);

        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 3, 0, 1, 2);

        ProgressBar1 = new QProgressBar(qdesigner_internal__PreviewWidget);
        ProgressBar1->setObjectName(QStringLiteral("ProgressBar1"));
        ProgressBar1->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(ProgressBar1, 2, 0, 1, 1);

        ButtonGroup2 = new QGroupBox(qdesigner_internal__PreviewWidget);
        ButtonGroup2->setObjectName(QStringLiteral("ButtonGroup2"));
        vboxLayout1 = new QVBoxLayout(ButtonGroup2);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        CheckBox1 = new QCheckBox(ButtonGroup2);
        CheckBox1->setObjectName(QStringLiteral("CheckBox1"));
        CheckBox1->setChecked(true);

        vboxLayout1->addWidget(CheckBox1);

        CheckBox2 = new QCheckBox(ButtonGroup2);
        CheckBox2->setObjectName(QStringLiteral("CheckBox2"));

        vboxLayout1->addWidget(CheckBox2);


        gridLayout->addWidget(ButtonGroup2, 1, 0, 1, 1);

        ButtonGroup1 = new QGroupBox(qdesigner_internal__PreviewWidget);
        ButtonGroup1->setObjectName(QStringLiteral("ButtonGroup1"));
        vboxLayout2 = new QVBoxLayout(ButtonGroup1);
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout2->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        RadioButton1 = new QRadioButton(ButtonGroup1);
        RadioButton1->setObjectName(QStringLiteral("RadioButton1"));
        RadioButton1->setChecked(true);

        vboxLayout2->addWidget(RadioButton1);

        RadioButton2 = new QRadioButton(ButtonGroup1);
        RadioButton2->setObjectName(QStringLiteral("RadioButton2"));

        vboxLayout2->addWidget(RadioButton2);

        RadioButton3 = new QRadioButton(ButtonGroup1);
        RadioButton3->setObjectName(QStringLiteral("RadioButton3"));

        vboxLayout2->addWidget(RadioButton3);


        gridLayout->addWidget(ButtonGroup1, 0, 0, 1, 1);


        retranslateUi(qdesigner_internal__PreviewWidget);

        QMetaObject::connectSlotsByName(qdesigner_internal__PreviewWidget);
    } // setupUi

    void retranslateUi(QWidget *qdesigner_internal__PreviewWidget)
    {
        qdesigner_internal__PreviewWidget->setWindowTitle(QApplication::translate("qdesigner_internal::PreviewWidget", "Preview Window", 0));
        LineEdit1->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "LineEdit", 0));
        ComboBox1->clear();
        ComboBox1->insertItems(0, QStringList()
         << QApplication::translate("qdesigner_internal::PreviewWidget", "ComboBox", 0)
        );
        PushButton1->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "PushButton", 0));
        ButtonGroup2->setTitle(QApplication::translate("qdesigner_internal::PreviewWidget", "ButtonGroup2", 0));
        CheckBox1->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "CheckBox1", 0));
        CheckBox2->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "CheckBox2", 0));
        ButtonGroup1->setTitle(QApplication::translate("qdesigner_internal::PreviewWidget", "ButtonGroup", 0));
        RadioButton1->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "RadioButton1", 0));
        RadioButton2->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "RadioButton2", 0));
        RadioButton3->setText(QApplication::translate("qdesigner_internal::PreviewWidget", "RadioButton3", 0));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class PreviewWidget: public Ui_PreviewWidget {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // PREVIEWWIDGET_H
