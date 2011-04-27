/*
*********************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** Form generated from reading UI file 'config_fromuic3.ui'
**
** Created: Thu Dec 17 12:48:42 2009
**      by: Qt User Interface Compiler version 4.6.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CONFIG_FROMUIC3_H
#define CONFIG_FROMUIC3_H

#include <Qt3Support/Q3ButtonGroup>
#include <Qt3Support/Q3GroupBox>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QVBoxLayout>
#include "gammaview.h"

QT_BEGIN_NAMESPACE

class Ui_Config
{
public:
    QGridLayout *gridLayout;
    Q3ButtonGroup *ButtonGroup2;
    QRadioButton *depth_1;
    QRadioButton *depth_4gray;
    QRadioButton *depth_8;
    QRadioButton *depth_12;
    QRadioButton *depth_16;
    QRadioButton *depth_32;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
    QCheckBox *touchScreen;
    Q3GroupBox *GroupBox1;
    QGridLayout *gridLayout1;
    QLabel *TextLabel3;
    QSlider *bslider;
    QLabel *blabel;
    QSpacerItem *Spacer3;
    QLabel *TextLabel2;
    QSlider *gslider;
    QLabel *glabel;
    QLabel *TextLabel7;
    QLabel *TextLabel8;
    QSlider *gammaslider;
    QLabel *TextLabel1_2;
    QLabel *rlabel;
    QSlider *rslider;
    QSpacerItem *Spacer2;
    QSpacerItem *Spacer4;
    QPushButton *PushButton3;
    GammaView *MyCustomWidget1;
    QSpacerItem *Spacer5;
    Q3ButtonGroup *ButtonGroup1;
    QVBoxLayout *vboxLayout;
    QRadioButton *size_240_320;
    QRadioButton *size_320_240;
    QRadioButton *size_640_480;
    QHBoxLayout *hboxLayout1;
    QRadioButton *size_custom;
    QSpinBox *size_width;
    QSpinBox *size_height;
    QHBoxLayout *hboxLayout2;
    QRadioButton *size_skin;
    QComboBox *skin;
    QLabel *TextLabel1;
    QRadioButton *test_for_useless_buttongroupId;

    void setupUi(QDialog *Config)
    {
        if (Config->objectName().isEmpty())
            Config->setObjectName(QString::fromUtf8("Config"));
        Config->resize(481, 645);
        Config->setWindowIcon(QPixmap(QString::fromUtf8("logo.png")));
        Config->setSizeGripEnabled(true);
        gridLayout = new QGridLayout(Config);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        ButtonGroup2 = new Q3ButtonGroup(Config);
        ButtonGroup2->setObjectName(QString::fromUtf8("ButtonGroup2"));
        depth_1 = new QRadioButton(ButtonGroup2);
        depth_1->setObjectName(QString::fromUtf8("depth_1"));
        depth_1->setGeometry(QRect(11, 19, 229, 19));
        depth_4gray = new QRadioButton(ButtonGroup2);
        depth_4gray->setObjectName(QString::fromUtf8("depth_4gray"));
        depth_4gray->setGeometry(QRect(11, 44, 229, 19));
        depth_8 = new QRadioButton(ButtonGroup2);
        depth_8->setObjectName(QString::fromUtf8("depth_8"));
        depth_8->setGeometry(QRect(11, 69, 229, 19));
        depth_12 = new QRadioButton(ButtonGroup2);
        depth_12->setObjectName(QString::fromUtf8("depth_12"));
        depth_12->setGeometry(QRect(11, 94, 229, 19));
        depth_16 = new QRadioButton(ButtonGroup2);
        depth_16->setObjectName(QString::fromUtf8("depth_16"));
        depth_16->setGeometry(QRect(11, 119, 229, 19));
        depth_32 = new QRadioButton(ButtonGroup2);
        depth_32->setObjectName(QString::fromUtf8("depth_32"));
        depth_32->setGeometry(QRect(11, 144, 229, 19));

        gridLayout->addWidget(ButtonGroup2, 0, 1, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(Config);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout->addWidget(buttonOk);

        buttonCancel = new QPushButton(Config);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout->addWidget(buttonCancel);


        gridLayout->addLayout(hboxLayout, 4, 0, 1, 2);

        touchScreen = new QCheckBox(Config);
        touchScreen->setObjectName(QString::fromUtf8("touchScreen"));

        gridLayout->addWidget(touchScreen, 2, 0, 1, 2);

        GroupBox1 = new Q3GroupBox(Config);
        GroupBox1->setObjectName(QString::fromUtf8("GroupBox1"));
        GroupBox1->setColumnLayout(0, Qt::Vertical);
        GroupBox1->layout()->setSpacing(6);
        GroupBox1->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout1 = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(GroupBox1->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout1);
        gridLayout1->setAlignment(Qt::AlignTop);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        TextLabel3 = new QLabel(GroupBox1);
        TextLabel3->setObjectName(QString::fromUtf8("TextLabel3"));
        TextLabel3->setWordWrap(false);

        gridLayout1->addWidget(TextLabel3, 6, 0, 1, 1);

        bslider = new QSlider(GroupBox1);
        bslider->setObjectName(QString::fromUtf8("bslider"));
        QPalette palette;
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(1), QColor(0, 0, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(2), QColor(127, 127, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(3), QColor(63, 63, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(4), QColor(0, 0, 127));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(5), QColor(0, 0, 170));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(1), QColor(0, 0, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(2), QColor(127, 127, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(3), QColor(38, 38, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(4), QColor(0, 0, 127));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(5), QColor(0, 0, 170));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(0), QColor(128, 128, 128));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(1), QColor(0, 0, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(2), QColor(127, 127, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(3), QColor(38, 38, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(4), QColor(0, 0, 127));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(5), QColor(0, 0, 170));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(8), QColor(128, 128, 128));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        bslider->setPalette(palette);
        bslider->setMaximum(400);
        bslider->setValue(100);
        bslider->setOrientation(Qt::Horizontal);

        gridLayout1->addWidget(bslider, 6, 1, 1, 1);

        blabel = new QLabel(GroupBox1);
        blabel->setObjectName(QString::fromUtf8("blabel"));
        blabel->setWordWrap(false);

        gridLayout1->addWidget(blabel, 6, 2, 1, 1);

        Spacer3 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(Spacer3, 5, 1, 1, 1);

        TextLabel2 = new QLabel(GroupBox1);
        TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));
        TextLabel2->setWordWrap(false);

        gridLayout1->addWidget(TextLabel2, 4, 0, 1, 1);

        gslider = new QSlider(GroupBox1);
        gslider->setObjectName(QString::fromUtf8("gslider"));
        QPalette palette1;
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(1), QColor(0, 255, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(2), QColor(127, 255, 127));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(3), QColor(63, 255, 63));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(4), QColor(0, 127, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(5), QColor(0, 170, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette1.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(1), QColor(0, 255, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(2), QColor(127, 255, 127));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(3), QColor(38, 255, 38));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(4), QColor(0, 127, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(5), QColor(0, 170, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette1.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(0), QColor(128, 128, 128));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(1), QColor(0, 255, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(2), QColor(127, 255, 127));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(3), QColor(38, 255, 38));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(4), QColor(0, 127, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(5), QColor(0, 170, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(8), QColor(128, 128, 128));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette1.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        gslider->setPalette(palette1);
        gslider->setMaximum(400);
        gslider->setValue(100);
        gslider->setOrientation(Qt::Horizontal);

        gridLayout1->addWidget(gslider, 4, 1, 1, 1);

        glabel = new QLabel(GroupBox1);
        glabel->setObjectName(QString::fromUtf8("glabel"));
        glabel->setWordWrap(false);

        gridLayout1->addWidget(glabel, 4, 2, 1, 1);

        TextLabel7 = new QLabel(GroupBox1);
        TextLabel7->setObjectName(QString::fromUtf8("TextLabel7"));
        TextLabel7->setWordWrap(false);

        gridLayout1->addWidget(TextLabel7, 0, 0, 1, 1);

        TextLabel8 = new QLabel(GroupBox1);
        TextLabel8->setObjectName(QString::fromUtf8("TextLabel8"));
        TextLabel8->setWordWrap(false);

        gridLayout1->addWidget(TextLabel8, 0, 2, 1, 1);

        gammaslider = new QSlider(GroupBox1);
        gammaslider->setObjectName(QString::fromUtf8("gammaslider"));
        QPalette palette2;
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(1), QColor(255, 255, 255));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(2), QColor(255, 255, 255));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(3), QColor(255, 255, 255));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(4), QColor(127, 127, 127));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(5), QColor(170, 170, 170));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette2.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(1), QColor(255, 255, 255));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(2), QColor(255, 255, 255));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(3), QColor(255, 255, 255));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(4), QColor(127, 127, 127));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(5), QColor(170, 170, 170));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette2.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(0), QColor(128, 128, 128));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(1), QColor(255, 255, 255));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(2), QColor(255, 255, 255));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(3), QColor(255, 255, 255));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(4), QColor(127, 127, 127));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(5), QColor(170, 170, 170));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(8), QColor(128, 128, 128));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette2.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        gammaslider->setPalette(palette2);
        gammaslider->setMaximum(400);
        gammaslider->setValue(100);
        gammaslider->setOrientation(Qt::Horizontal);

        gridLayout1->addWidget(gammaslider, 0, 1, 1, 1);

        TextLabel1_2 = new QLabel(GroupBox1);
        TextLabel1_2->setObjectName(QString::fromUtf8("TextLabel1_2"));
        TextLabel1_2->setWordWrap(false);

        gridLayout1->addWidget(TextLabel1_2, 2, 0, 1, 1);

        rlabel = new QLabel(GroupBox1);
        rlabel->setObjectName(QString::fromUtf8("rlabel"));
        rlabel->setWordWrap(false);

        gridLayout1->addWidget(rlabel, 2, 2, 1, 1);

        rslider = new QSlider(GroupBox1);
        rslider->setObjectName(QString::fromUtf8("rslider"));
        QPalette palette3;
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(1), QColor(255, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(2), QColor(255, 127, 127));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(3), QColor(255, 63, 63));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(4), QColor(127, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(5), QColor(170, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette3.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(1), QColor(255, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(2), QColor(255, 127, 127));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(3), QColor(255, 38, 38));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(4), QColor(127, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(5), QColor(170, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette3.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(0), QColor(128, 128, 128));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(1), QColor(255, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(2), QColor(255, 127, 127));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(3), QColor(255, 38, 38));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(4), QColor(127, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(5), QColor(170, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(7), QColor(255, 255, 255));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(8), QColor(128, 128, 128));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(10), QColor(220, 220, 220));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(12), QColor(10, 95, 137));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(14), QColor(0, 0, 0));
        palette3.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(15), QColor(0, 0, 0));
        rslider->setPalette(palette3);
        rslider->setMaximum(400);
        rslider->setValue(100);
        rslider->setOrientation(Qt::Horizontal);

        gridLayout1->addWidget(rslider, 2, 1, 1, 1);

        Spacer2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(Spacer2, 3, 1, 1, 1);

        Spacer4 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(Spacer4, 1, 1, 1, 1);

        PushButton3 = new QPushButton(GroupBox1);
        PushButton3->setObjectName(QString::fromUtf8("PushButton3"));

        gridLayout1->addWidget(PushButton3, 8, 0, 1, 3);

        MyCustomWidget1 = new GammaView(GroupBox1);
        MyCustomWidget1->setObjectName(QString::fromUtf8("MyCustomWidget1"));

        gridLayout1->addWidget(MyCustomWidget1, 0, 3, 9, 1);

        Spacer5 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(Spacer5, 7, 1, 1, 1);


        gridLayout->addWidget(GroupBox1, 3, 0, 1, 2);

        ButtonGroup1 = new Q3ButtonGroup(Config);
        ButtonGroup1->setObjectName(QString::fromUtf8("ButtonGroup1"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ButtonGroup1->sizePolicy().hasHeightForWidth());
        ButtonGroup1->setSizePolicy(sizePolicy);
        ButtonGroup1->setColumnLayout(0, Qt::Vertical);
        ButtonGroup1->layout()->setSpacing(6);
        ButtonGroup1->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout = new QVBoxLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(ButtonGroup1->layout());
        if (boxlayout1)
            boxlayout1->addLayout(vboxLayout);
        vboxLayout->setAlignment(Qt::AlignTop);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        size_240_320 = new QRadioButton(ButtonGroup1);
        size_240_320->setObjectName(QString::fromUtf8("size_240_320"));

        vboxLayout->addWidget(size_240_320);

        size_320_240 = new QRadioButton(ButtonGroup1);
        size_320_240->setObjectName(QString::fromUtf8("size_320_240"));

        vboxLayout->addWidget(size_320_240);

        size_640_480 = new QRadioButton(ButtonGroup1);
        size_640_480->setObjectName(QString::fromUtf8("size_640_480"));

        vboxLayout->addWidget(size_640_480);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        size_custom = new QRadioButton(ButtonGroup1);
        size_custom->setObjectName(QString::fromUtf8("size_custom"));

        hboxLayout1->addWidget(size_custom);

        size_width = new QSpinBox(ButtonGroup1);
        size_width->setObjectName(QString::fromUtf8("size_width"));
        size_width->setMaximum(1280);
        size_width->setMinimum(1);
        size_width->setSingleStep(16);
        size_width->setValue(400);

        hboxLayout1->addWidget(size_width);

        size_height = new QSpinBox(ButtonGroup1);
        size_height->setObjectName(QString::fromUtf8("size_height"));
        size_height->setMaximum(1024);
        size_height->setMinimum(1);
        size_height->setSingleStep(16);
        size_height->setValue(300);

        hboxLayout1->addWidget(size_height);


        vboxLayout->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        size_skin = new QRadioButton(ButtonGroup1);
        size_skin->setObjectName(QString::fromUtf8("size_skin"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(size_skin->sizePolicy().hasHeightForWidth());
        size_skin->setSizePolicy(sizePolicy1);

        hboxLayout2->addWidget(size_skin);

        skin = new QComboBox(ButtonGroup1);
        skin->setObjectName(QString::fromUtf8("skin"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(skin->sizePolicy().hasHeightForWidth());
        skin->setSizePolicy(sizePolicy2);

        hboxLayout2->addWidget(skin);


        vboxLayout->addLayout(hboxLayout2);


        gridLayout->addWidget(ButtonGroup1, 0, 0, 1, 1);

        TextLabel1 = new QLabel(Config);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));
        TextLabel1->setWordWrap(false);

        gridLayout->addWidget(TextLabel1, 1, 0, 1, 2);

        test_for_useless_buttongroupId = new QRadioButton(Config);
        test_for_useless_buttongroupId->setObjectName(QString::fromUtf8("test_for_useless_buttongroupId"));

        gridLayout->addWidget(test_for_useless_buttongroupId, 0, 0, 1, 1);


        retranslateUi(Config);
        QObject::connect(buttonOk, SIGNAL(clicked()), Config, SLOT(accept()));
        QObject::connect(buttonCancel, SIGNAL(clicked()), Config, SLOT(reject()));

        QMetaObject::connectSlotsByName(Config);
    } // setupUi

    void retranslateUi(QDialog *Config)
    {
        Config->setWindowTitle(QApplication::translate("Config", "Configure", 0, QApplication::UnicodeUTF8));
        ButtonGroup2->setTitle(QApplication::translate("Config", "Depth", 0, QApplication::UnicodeUTF8));
        depth_1->setText(QApplication::translate("Config", "1 bit monochrome", 0, QApplication::UnicodeUTF8));
        depth_4gray->setText(QApplication::translate("Config", "4 bit grayscale", 0, QApplication::UnicodeUTF8));
        depth_8->setText(QApplication::translate("Config", "8 bit", 0, QApplication::UnicodeUTF8));
        depth_12->setText(QApplication::translate("Config", "12 (16) bit", 0, QApplication::UnicodeUTF8));
        depth_16->setText(QApplication::translate("Config", "16 bit", 0, QApplication::UnicodeUTF8));
        depth_32->setText(QApplication::translate("Config", "32 bit", 0, QApplication::UnicodeUTF8));
        buttonOk->setText(QApplication::translate("Config", "&OK", 0, QApplication::UnicodeUTF8));
        buttonCancel->setText(QApplication::translate("Config", "&Cancel", 0, QApplication::UnicodeUTF8));
        touchScreen->setText(QApplication::translate("Config", "Emulate touch screen (no mouse move).", 0, QApplication::UnicodeUTF8));
        GroupBox1->setTitle(QApplication::translate("Config", "Gamma", 0, QApplication::UnicodeUTF8));
        TextLabel3->setText(QApplication::translate("Config", "Blue", 0, QApplication::UnicodeUTF8));
        blabel->setText(QApplication::translate("Config", "1.0", 0, QApplication::UnicodeUTF8));
        TextLabel2->setText(QApplication::translate("Config", "Green", 0, QApplication::UnicodeUTF8));
        glabel->setText(QApplication::translate("Config", "1.0", 0, QApplication::UnicodeUTF8));
        TextLabel7->setText(QApplication::translate("Config", "All", 0, QApplication::UnicodeUTF8));
        TextLabel8->setText(QApplication::translate("Config", "1.0", 0, QApplication::UnicodeUTF8));
        TextLabel1_2->setText(QApplication::translate("Config", "Red", 0, QApplication::UnicodeUTF8));
        rlabel->setText(QApplication::translate("Config", "1.0", 0, QApplication::UnicodeUTF8));
        PushButton3->setText(QApplication::translate("Config", "Set all to 1.0", 0, QApplication::UnicodeUTF8));
        ButtonGroup1->setTitle(QApplication::translate("Config", "Size", 0, QApplication::UnicodeUTF8));
        size_240_320->setText(QApplication::translate("Config", "240x320 \"PDA\"", 0, QApplication::UnicodeUTF8));
        size_320_240->setText(QApplication::translate("Config", "320x240 \"TV\"", 0, QApplication::UnicodeUTF8));
        size_640_480->setText(QApplication::translate("Config", "640x480 \"VGA\"", 0, QApplication::UnicodeUTF8));
        size_custom->setText(QApplication::translate("Config", "Custom", 0, QApplication::UnicodeUTF8));
        size_skin->setText(QApplication::translate("Config", "Skin", 0, QApplication::UnicodeUTF8));
        skin->clear();
        skin->insertItems(0, QStringList()
         << QApplication::translate("Config", "pda.skin", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Config", "ipaq.skin", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Config", "qpe.skin", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Config", "cassiopeia.skin", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Config", "other.skin", 0, QApplication::UnicodeUTF8)
        );
        TextLabel1->setText(QApplication::translate("Config", "<p>Note that any applications using the virtual framebuffer will be terminated if you change the Size or Depth <i>above</i>. You may freely modify the Gamma <i>below</i>.", 0, QApplication::UnicodeUTF8));
        test_for_useless_buttongroupId->setText(QApplication::translate("Config", "Test", 0, QApplication::UnicodeUTF8));
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    /* XPM */
    static const char* const image0_data[] = { 
"22 22 2 1",
". c None",
"# c #a4c610",
"........######........",
".....###########......",
"....##############....",
"...################...",
"..######......######..",
"..#####........#####..",
".#####.......#..#####.",
".####.......###..####.",
"####.......#####..####",
"####......#####...####",
"####....#######...####",
"####....######....####",
"####...########...####",
".####.##########..####",
".####..####.#########.",
".#####..##...########.",
"..#####.......#######.",
"..######......######..",
"...###################",
"....##################",
"......###########.###.",
"........######.....#.."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class Config: public Ui_Config {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CONFIG_FROMUIC3_H
