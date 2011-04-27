/*
*********************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'previewwidgetbase.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PREVIEWWIDGETBASE_H
#define PREVIEWWIDGETBASE_H

#include <Qt3Support/Q3ButtonGroup>
#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLineEdit>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QScrollBar>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PreviewWidgetBase
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout1;
    Q3ButtonGroup *ButtonGroup1;
    QVBoxLayout *vboxLayout2;
    QRadioButton *RadioButton1;
    QRadioButton *RadioButton2;
    QRadioButton *RadioButton3;
    Q3ButtonGroup *ButtonGroup2;
    QVBoxLayout *vboxLayout3;
    QCheckBox *CheckBox1;
    QCheckBox *CheckBox2;
    QProgressBar *ProgressBar1;
    QVBoxLayout *vboxLayout4;
    QLineEdit *LineEdit1;
    QComboBox *ComboBox1;
    QHBoxLayout *hboxLayout1;
    QSpinBox *SpinBox1;
    QPushButton *PushButton1;
    QScrollBar *ScrollBar1;
    QSlider *Slider1;
    QTextEdit *textView;
    QSpacerItem *Spacer2;

    void setupUi(QWidget *PreviewWidgetBase)
    {
        if (PreviewWidgetBase->objectName().isEmpty())
            PreviewWidgetBase->setObjectName(QString::fromUtf8("PreviewWidgetBase"));
        PreviewWidgetBase->resize(378, 236);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PreviewWidgetBase->sizePolicy().hasHeightForWidth());
        PreviewWidgetBase->setSizePolicy(sizePolicy);
        vboxLayout = new QVBoxLayout(PreviewWidgetBase);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setObjectName(QString::fromUtf8("unnamed"));
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setObjectName(QString::fromUtf8("unnamed"));
        vboxLayout1 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        vboxLayout1->setObjectName(QString::fromUtf8("unnamed"));
        ButtonGroup1 = new Q3ButtonGroup(PreviewWidgetBase);
        ButtonGroup1->setObjectName(QString::fromUtf8("ButtonGroup1"));
        ButtonGroup1->setColumnLayout(0, Qt::Vertical);
#ifndef Q_OS_MAC
        ButtonGroup1->layout()->setSpacing(6);
#endif
        ButtonGroup1->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout2 = new QVBoxLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(ButtonGroup1->layout());
        if (boxlayout)
            boxlayout->addLayout(vboxLayout2);
        vboxLayout2->setAlignment(Qt::AlignTop);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        vboxLayout2->setObjectName(QString::fromUtf8("unnamed"));
        RadioButton1 = new QRadioButton(ButtonGroup1);
        RadioButton1->setObjectName(QString::fromUtf8("RadioButton1"));
        RadioButton1->setChecked(true);

        vboxLayout2->addWidget(RadioButton1);

        RadioButton2 = new QRadioButton(ButtonGroup1);
        RadioButton2->setObjectName(QString::fromUtf8("RadioButton2"));

        vboxLayout2->addWidget(RadioButton2);

        RadioButton3 = new QRadioButton(ButtonGroup1);
        RadioButton3->setObjectName(QString::fromUtf8("RadioButton3"));

        vboxLayout2->addWidget(RadioButton3);


        vboxLayout1->addWidget(ButtonGroup1);

        ButtonGroup2 = new Q3ButtonGroup(PreviewWidgetBase);
        ButtonGroup2->setObjectName(QString::fromUtf8("ButtonGroup2"));
        ButtonGroup2->setColumnLayout(0, Qt::Vertical);
#ifndef Q_OS_MAC
        ButtonGroup2->layout()->setSpacing(6);
#endif
        ButtonGroup2->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout3 = new QVBoxLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(ButtonGroup2->layout());
        if (boxlayout1)
            boxlayout1->addLayout(vboxLayout3);
        vboxLayout3->setAlignment(Qt::AlignTop);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        vboxLayout3->setObjectName(QString::fromUtf8("unnamed"));
        CheckBox1 = new QCheckBox(ButtonGroup2);
        CheckBox1->setObjectName(QString::fromUtf8("CheckBox1"));
        CheckBox1->setChecked(true);

        vboxLayout3->addWidget(CheckBox1);

        CheckBox2 = new QCheckBox(ButtonGroup2);
        CheckBox2->setObjectName(QString::fromUtf8("CheckBox2"));

        vboxLayout3->addWidget(CheckBox2);


        vboxLayout1->addWidget(ButtonGroup2);

        ProgressBar1 = new QProgressBar(PreviewWidgetBase);
        ProgressBar1->setObjectName(QString::fromUtf8("ProgressBar1"));
        ProgressBar1->setValue(50);

        vboxLayout1->addWidget(ProgressBar1);


        hboxLayout->addLayout(vboxLayout1);

        vboxLayout4 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout4->setSpacing(6);
#endif
        vboxLayout4->setContentsMargins(0, 0, 0, 0);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        vboxLayout4->setObjectName(QString::fromUtf8("unnamed"));
        LineEdit1 = new QLineEdit(PreviewWidgetBase);
        LineEdit1->setObjectName(QString::fromUtf8("LineEdit1"));

        vboxLayout4->addWidget(LineEdit1);

        ComboBox1 = new QComboBox(PreviewWidgetBase);
        ComboBox1->setObjectName(QString::fromUtf8("ComboBox1"));

        vboxLayout4->addWidget(ComboBox1);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        hboxLayout1->setObjectName(QString::fromUtf8("unnamed"));
        SpinBox1 = new QSpinBox(PreviewWidgetBase);
        SpinBox1->setObjectName(QString::fromUtf8("SpinBox1"));

        hboxLayout1->addWidget(SpinBox1);

        PushButton1 = new QPushButton(PreviewWidgetBase);
        PushButton1->setObjectName(QString::fromUtf8("PushButton1"));

        hboxLayout1->addWidget(PushButton1);


        vboxLayout4->addLayout(hboxLayout1);

        ScrollBar1 = new QScrollBar(PreviewWidgetBase);
        ScrollBar1->setObjectName(QString::fromUtf8("ScrollBar1"));
        ScrollBar1->setOrientation(Qt::Horizontal);

        vboxLayout4->addWidget(ScrollBar1);

        Slider1 = new QSlider(PreviewWidgetBase);
        Slider1->setObjectName(QString::fromUtf8("Slider1"));
        Slider1->setOrientation(Qt::Horizontal);

        vboxLayout4->addWidget(Slider1);

        textView = new QTextEdit(PreviewWidgetBase);
        textView->setObjectName(QString::fromUtf8("textView"));
        textView->setMaximumSize(QSize(32767, 50));
        textView->setReadOnly(true);

        vboxLayout4->addWidget(textView);


        hboxLayout->addLayout(vboxLayout4);


        vboxLayout->addLayout(hboxLayout);

        Spacer2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(Spacer2);


        retranslateUi(PreviewWidgetBase);

        QMetaObject::connectSlotsByName(PreviewWidgetBase);
    } // setupUi

    void retranslateUi(QWidget *PreviewWidgetBase)
    {
        PreviewWidgetBase->setWindowTitle(QApplication::translate("PreviewWidgetBase", "Preview Window", 0, QApplication::UnicodeUTF8));
        ButtonGroup1->setTitle(QApplication::translate("PreviewWidgetBase", "ButtonGroup", 0, QApplication::UnicodeUTF8));
        RadioButton1->setText(QApplication::translate("PreviewWidgetBase", "RadioButton1", 0, QApplication::UnicodeUTF8));
        RadioButton2->setText(QApplication::translate("PreviewWidgetBase", "RadioButton2", 0, QApplication::UnicodeUTF8));
        RadioButton3->setText(QApplication::translate("PreviewWidgetBase", "RadioButton3", 0, QApplication::UnicodeUTF8));
        ButtonGroup2->setTitle(QApplication::translate("PreviewWidgetBase", "ButtonGroup2", 0, QApplication::UnicodeUTF8));
        CheckBox1->setText(QApplication::translate("PreviewWidgetBase", "CheckBox1", 0, QApplication::UnicodeUTF8));
        CheckBox2->setText(QApplication::translate("PreviewWidgetBase", "CheckBox2", 0, QApplication::UnicodeUTF8));
        LineEdit1->setText(QApplication::translate("PreviewWidgetBase", "LineEdit", 0, QApplication::UnicodeUTF8));
        ComboBox1->clear();
        ComboBox1->insertItems(0, QStringList()
         << QApplication::translate("PreviewWidgetBase", "ComboBox", 0, QApplication::UnicodeUTF8)
        );
        PushButton1->setText(QApplication::translate("PreviewWidgetBase", "PushButton", 0, QApplication::UnicodeUTF8));
        textView->setText(QApplication::translate("PreviewWidgetBase", "<p>\n"
"<a href=\"http://qt.nokia.com\">http://qt.nokia.com</a>\n"
"</p>\n"
"<p>\n"
"<a href=\"http://www.kde.org\">http://www.kde.org</a>\n"
"</p>", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PreviewWidgetBase: public Ui_PreviewWidgetBase {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PREVIEWWIDGETBASE_H
