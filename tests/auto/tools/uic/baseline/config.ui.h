/*
*********************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'config.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include "gammaview.h"

QT_BEGIN_NAMESPACE

class Ui_Config
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QGroupBox *ButtonGroup1;
    QVBoxLayout *vboxLayout1;
    QRadioButton *size_176_220;
    QRadioButton *size_240_320;
    QRadioButton *size_320_240;
    QRadioButton *size_640_480;
    QRadioButton *size_800_600;
    QRadioButton *size_1024_768;
    QHBoxLayout *hboxLayout1;
    QRadioButton *size_custom;
    QSpinBox *size_width;
    QSpinBox *size_height;
    QGroupBox *ButtonGroup2;
    QVBoxLayout *vboxLayout2;
    QRadioButton *depth_1;
    QRadioButton *depth_4gray;
    QRadioButton *depth_8;
    QRadioButton *depth_12;
    QRadioButton *depth_15;
    QRadioButton *depth_16;
    QRadioButton *depth_18;
    QRadioButton *depth_24;
    QRadioButton *depth_32;
    QRadioButton *depth_32_argb;
    QHBoxLayout *hboxLayout2;
    QLabel *TextLabel1_3;
    QComboBox *skin;
    QCheckBox *touchScreen;
    QCheckBox *lcdScreen;
    QSpacerItem *spacerItem;
    QLabel *TextLabel1;
    QGroupBox *GroupBox1;
    QGridLayout *gridLayout;
    QLabel *TextLabel3;
    QSlider *bslider;
    QLabel *blabel;
    QLabel *TextLabel2;
    QSlider *gslider;
    QLabel *glabel;
    QLabel *TextLabel7;
    QLabel *TextLabel8;
    QSlider *gammaslider;
    QLabel *TextLabel1_2;
    QLabel *rlabel;
    QSlider *rslider;
    QPushButton *PushButton3;
    GammaView *MyCustomWidget1;
    QHBoxLayout *hboxLayout3;
    QSpacerItem *spacerItem1;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *Config)
    {
        if (Config->objectName().isEmpty())
            Config->setObjectName(QString::fromUtf8("Config"));
        Config->resize(600, 650);
        Config->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(Config);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        ButtonGroup1 = new QGroupBox(Config);
        ButtonGroup1->setObjectName(QString::fromUtf8("ButtonGroup1"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ButtonGroup1->sizePolicy().hasHeightForWidth());
        ButtonGroup1->setSizePolicy(sizePolicy);
        vboxLayout1 = new QVBoxLayout(ButtonGroup1);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        size_176_220 = new QRadioButton(ButtonGroup1);
        size_176_220->setObjectName(QString::fromUtf8("size_176_220"));

        vboxLayout1->addWidget(size_176_220);

        size_240_320 = new QRadioButton(ButtonGroup1);
        size_240_320->setObjectName(QString::fromUtf8("size_240_320"));

        vboxLayout1->addWidget(size_240_320);

        size_320_240 = new QRadioButton(ButtonGroup1);
        size_320_240->setObjectName(QString::fromUtf8("size_320_240"));

        vboxLayout1->addWidget(size_320_240);

        size_640_480 = new QRadioButton(ButtonGroup1);
        size_640_480->setObjectName(QString::fromUtf8("size_640_480"));

        vboxLayout1->addWidget(size_640_480);

        size_800_600 = new QRadioButton(ButtonGroup1);
        size_800_600->setObjectName(QString::fromUtf8("size_800_600"));

        vboxLayout1->addWidget(size_800_600);

        size_1024_768 = new QRadioButton(ButtonGroup1);
        size_1024_768->setObjectName(QString::fromUtf8("size_1024_768"));

        vboxLayout1->addWidget(size_1024_768);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        size_custom = new QRadioButton(ButtonGroup1);
        size_custom->setObjectName(QString::fromUtf8("size_custom"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(size_custom->sizePolicy().hasHeightForWidth());
        size_custom->setSizePolicy(sizePolicy1);

        hboxLayout1->addWidget(size_custom);

        size_width = new QSpinBox(ButtonGroup1);
        size_width->setObjectName(QString::fromUtf8("size_width"));
        size_width->setMinimum(1);
        size_width->setMaximum(1280);
        size_width->setSingleStep(16);
        size_width->setValue(400);

        hboxLayout1->addWidget(size_width);

        size_height = new QSpinBox(ButtonGroup1);
        size_height->setObjectName(QString::fromUtf8("size_height"));
        size_height->setMinimum(1);
        size_height->setMaximum(1024);
        size_height->setSingleStep(16);
        size_height->setValue(300);

        hboxLayout1->addWidget(size_height);


        vboxLayout1->addLayout(hboxLayout1);


        hboxLayout->addWidget(ButtonGroup1);

        ButtonGroup2 = new QGroupBox(Config);
        ButtonGroup2->setObjectName(QString::fromUtf8("ButtonGroup2"));
        vboxLayout2 = new QVBoxLayout(ButtonGroup2);
        vboxLayout2->setSpacing(6);
        vboxLayout2->setContentsMargins(11, 11, 11, 11);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        vboxLayout2->setContentsMargins(11, 11, 11, 11);
        depth_1 = new QRadioButton(ButtonGroup2);
        depth_1->setObjectName(QString::fromUtf8("depth_1"));

        vboxLayout2->addWidget(depth_1);

        depth_4gray = new QRadioButton(ButtonGroup2);
        depth_4gray->setObjectName(QString::fromUtf8("depth_4gray"));

        vboxLayout2->addWidget(depth_4gray);

        depth_8 = new QRadioButton(ButtonGroup2);
        depth_8->setObjectName(QString::fromUtf8("depth_8"));

        vboxLayout2->addWidget(depth_8);

        depth_12 = new QRadioButton(ButtonGroup2);
        depth_12->setObjectName(QString::fromUtf8("depth_12"));

        vboxLayout2->addWidget(depth_12);

        depth_15 = new QRadioButton(ButtonGroup2);
        depth_15->setObjectName(QString::fromUtf8("depth_15"));

        vboxLayout2->addWidget(depth_15);

        depth_16 = new QRadioButton(ButtonGroup2);
        depth_16->setObjectName(QString::fromUtf8("depth_16"));

        vboxLayout2->addWidget(depth_16);

        depth_18 = new QRadioButton(ButtonGroup2);
        depth_18->setObjectName(QString::fromUtf8("depth_18"));

        vboxLayout2->addWidget(depth_18);

        depth_24 = new QRadioButton(ButtonGroup2);
        depth_24->setObjectName(QString::fromUtf8("depth_24"));

        vboxLayout2->addWidget(depth_24);

        depth_32 = new QRadioButton(ButtonGroup2);
        depth_32->setObjectName(QString::fromUtf8("depth_32"));

        vboxLayout2->addWidget(depth_32);

        depth_32_argb = new QRadioButton(ButtonGroup2);
        depth_32_argb->setObjectName(QString::fromUtf8("depth_32_argb"));

        vboxLayout2->addWidget(depth_32_argb);


        hboxLayout->addWidget(ButtonGroup2);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        TextLabel1_3 = new QLabel(Config);
        TextLabel1_3->setObjectName(QString::fromUtf8("TextLabel1_3"));

        hboxLayout2->addWidget(TextLabel1_3);

        skin = new QComboBox(Config);
        skin->addItem(QString());
        skin->setObjectName(QString::fromUtf8("skin"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(skin->sizePolicy().hasHeightForWidth());
        skin->setSizePolicy(sizePolicy2);

        hboxLayout2->addWidget(skin);


        vboxLayout->addLayout(hboxLayout2);

        touchScreen = new QCheckBox(Config);
        touchScreen->setObjectName(QString::fromUtf8("touchScreen"));

        vboxLayout->addWidget(touchScreen);

        lcdScreen = new QCheckBox(Config);
        lcdScreen->setObjectName(QString::fromUtf8("lcdScreen"));

        vboxLayout->addWidget(lcdScreen);

        spacerItem = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);

        TextLabel1 = new QLabel(Config);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));
        sizePolicy.setHeightForWidth(TextLabel1->sizePolicy().hasHeightForWidth());
        TextLabel1->setSizePolicy(sizePolicy);
        TextLabel1->setWordWrap(true);

        vboxLayout->addWidget(TextLabel1);

        GroupBox1 = new QGroupBox(Config);
        GroupBox1->setObjectName(QString::fromUtf8("GroupBox1"));
        gridLayout = new QGridLayout(GroupBox1);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        TextLabel3 = new QLabel(GroupBox1);
        TextLabel3->setObjectName(QString::fromUtf8("TextLabel3"));

        gridLayout->addWidget(TextLabel3, 6, 0, 1, 1);

        bslider = new QSlider(GroupBox1);
        bslider->setObjectName(QString::fromUtf8("bslider"));
        QPalette palette;
        QBrush brush(QColor(128, 128, 128, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush1(QColor(0, 0, 255, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Button, brush1);
        QBrush brush2(QColor(127, 127, 255, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Light, brush2);
        QBrush brush3(QColor(38, 38, 255, 255));
        brush3.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Midlight, brush3);
        QBrush brush4(QColor(0, 0, 127, 255));
        brush4.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Dark, brush4);
        QBrush brush5(QColor(0, 0, 170, 255));
        brush5.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Mid, brush5);
        QBrush brush6(QColor(0, 0, 0, 255));
        brush6.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Text, brush6);
        QBrush brush7(QColor(255, 255, 255, 255));
        brush7.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::BrightText, brush7);
        palette.setBrush(QPalette::Active, QPalette::ButtonText, brush);
        palette.setBrush(QPalette::Active, QPalette::Base, brush7);
        QBrush brush8(QColor(220, 220, 220, 255));
        brush8.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Window, brush8);
        palette.setBrush(QPalette::Active, QPalette::Shadow, brush6);
        QBrush brush9(QColor(10, 95, 137, 255));
        brush9.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Highlight, brush9);
        palette.setBrush(QPalette::Active, QPalette::HighlightedText, brush7);
        palette.setBrush(QPalette::Active, QPalette::Link, brush6);
        palette.setBrush(QPalette::Active, QPalette::LinkVisited, brush6);
        QBrush brush10(QColor(232, 232, 232, 255));
        brush10.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::AlternateBase, brush10);
        palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Button, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::Light, brush2);
        palette.setBrush(QPalette::Inactive, QPalette::Midlight, brush3);
        palette.setBrush(QPalette::Inactive, QPalette::Dark, brush4);
        palette.setBrush(QPalette::Inactive, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Inactive, QPalette::Text, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::BrightText, brush7);
        palette.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush7);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush8);
        palette.setBrush(QPalette::Inactive, QPalette::Shadow, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::Highlight, brush9);
        palette.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush7);
        palette.setBrush(QPalette::Inactive, QPalette::Link, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::LinkVisited, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush10);
        palette.setBrush(QPalette::Disabled, QPalette::WindowText, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Button, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Light, brush2);
        palette.setBrush(QPalette::Disabled, QPalette::Midlight, brush3);
        palette.setBrush(QPalette::Disabled, QPalette::Dark, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Disabled, QPalette::Text, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::BrightText, brush7);
        palette.setBrush(QPalette::Disabled, QPalette::ButtonText, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush7);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush8);
        palette.setBrush(QPalette::Disabled, QPalette::Shadow, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::Highlight, brush9);
        palette.setBrush(QPalette::Disabled, QPalette::HighlightedText, brush7);
        palette.setBrush(QPalette::Disabled, QPalette::Link, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::LinkVisited, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush10);
        bslider->setPalette(palette);
        bslider->setMaximum(400);
        bslider->setValue(100);
        bslider->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(bslider, 6, 1, 1, 1);

        blabel = new QLabel(GroupBox1);
        blabel->setObjectName(QString::fromUtf8("blabel"));

        gridLayout->addWidget(blabel, 6, 2, 1, 1);

        TextLabel2 = new QLabel(GroupBox1);
        TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));

        gridLayout->addWidget(TextLabel2, 4, 0, 1, 1);

        gslider = new QSlider(GroupBox1);
        gslider->setObjectName(QString::fromUtf8("gslider"));
        QPalette palette1;
        palette1.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush11(QColor(0, 255, 0, 255));
        brush11.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Button, brush11);
        QBrush brush12(QColor(127, 255, 127, 255));
        brush12.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Light, brush12);
        QBrush brush13(QColor(38, 255, 38, 255));
        brush13.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Midlight, brush13);
        QBrush brush14(QColor(0, 127, 0, 255));
        brush14.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Dark, brush14);
        QBrush brush15(QColor(0, 170, 0, 255));
        brush15.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Mid, brush15);
        palette1.setBrush(QPalette::Active, QPalette::Text, brush6);
        palette1.setBrush(QPalette::Active, QPalette::BrightText, brush7);
        palette1.setBrush(QPalette::Active, QPalette::ButtonText, brush);
        palette1.setBrush(QPalette::Active, QPalette::Base, brush7);
        palette1.setBrush(QPalette::Active, QPalette::Window, brush8);
        palette1.setBrush(QPalette::Active, QPalette::Shadow, brush6);
        palette1.setBrush(QPalette::Active, QPalette::Highlight, brush9);
        palette1.setBrush(QPalette::Active, QPalette::HighlightedText, brush7);
        palette1.setBrush(QPalette::Active, QPalette::Link, brush6);
        palette1.setBrush(QPalette::Active, QPalette::LinkVisited, brush6);
        palette1.setBrush(QPalette::Active, QPalette::AlternateBase, brush10);
        palette1.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Button, brush11);
        palette1.setBrush(QPalette::Inactive, QPalette::Light, brush12);
        palette1.setBrush(QPalette::Inactive, QPalette::Midlight, brush13);
        palette1.setBrush(QPalette::Inactive, QPalette::Dark, brush14);
        palette1.setBrush(QPalette::Inactive, QPalette::Mid, brush15);
        palette1.setBrush(QPalette::Inactive, QPalette::Text, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::BrightText, brush7);
        palette1.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Base, brush7);
        palette1.setBrush(QPalette::Inactive, QPalette::Window, brush8);
        palette1.setBrush(QPalette::Inactive, QPalette::Shadow, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::Highlight, brush9);
        palette1.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush7);
        palette1.setBrush(QPalette::Inactive, QPalette::Link, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::LinkVisited, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush10);
        palette1.setBrush(QPalette::Disabled, QPalette::WindowText, brush);
        palette1.setBrush(QPalette::Disabled, QPalette::Button, brush11);
        palette1.setBrush(QPalette::Disabled, QPalette::Light, brush12);
        palette1.setBrush(QPalette::Disabled, QPalette::Midlight, brush13);
        palette1.setBrush(QPalette::Disabled, QPalette::Dark, brush14);
        palette1.setBrush(QPalette::Disabled, QPalette::Mid, brush15);
        palette1.setBrush(QPalette::Disabled, QPalette::Text, brush6);
        palette1.setBrush(QPalette::Disabled, QPalette::BrightText, brush7);
        palette1.setBrush(QPalette::Disabled, QPalette::ButtonText, brush);
        palette1.setBrush(QPalette::Disabled, QPalette::Base, brush7);
        palette1.setBrush(QPalette::Disabled, QPalette::Window, brush8);
        palette1.setBrush(QPalette::Disabled, QPalette::Shadow, brush6);
        palette1.setBrush(QPalette::Disabled, QPalette::Highlight, brush9);
        palette1.setBrush(QPalette::Disabled, QPalette::HighlightedText, brush7);
        palette1.setBrush(QPalette::Disabled, QPalette::Link, brush6);
        palette1.setBrush(QPalette::Disabled, QPalette::LinkVisited, brush6);
        palette1.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush10);
        gslider->setPalette(palette1);
        gslider->setMaximum(400);
        gslider->setValue(100);
        gslider->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(gslider, 4, 1, 1, 1);

        glabel = new QLabel(GroupBox1);
        glabel->setObjectName(QString::fromUtf8("glabel"));

        gridLayout->addWidget(glabel, 4, 2, 1, 1);

        TextLabel7 = new QLabel(GroupBox1);
        TextLabel7->setObjectName(QString::fromUtf8("TextLabel7"));

        gridLayout->addWidget(TextLabel7, 0, 0, 1, 1);

        TextLabel8 = new QLabel(GroupBox1);
        TextLabel8->setObjectName(QString::fromUtf8("TextLabel8"));

        gridLayout->addWidget(TextLabel8, 0, 2, 1, 1);

        gammaslider = new QSlider(GroupBox1);
        gammaslider->setObjectName(QString::fromUtf8("gammaslider"));
        QPalette palette2;
        palette2.setBrush(QPalette::Active, QPalette::WindowText, brush);
        palette2.setBrush(QPalette::Active, QPalette::Button, brush7);
        palette2.setBrush(QPalette::Active, QPalette::Light, brush7);
        palette2.setBrush(QPalette::Active, QPalette::Midlight, brush7);
        QBrush brush16(QColor(127, 127, 127, 255));
        brush16.setStyle(Qt::SolidPattern);
        palette2.setBrush(QPalette::Active, QPalette::Dark, brush16);
        QBrush brush17(QColor(170, 170, 170, 255));
        brush17.setStyle(Qt::SolidPattern);
        palette2.setBrush(QPalette::Active, QPalette::Mid, brush17);
        palette2.setBrush(QPalette::Active, QPalette::Text, brush6);
        palette2.setBrush(QPalette::Active, QPalette::BrightText, brush7);
        palette2.setBrush(QPalette::Active, QPalette::ButtonText, brush);
        palette2.setBrush(QPalette::Active, QPalette::Base, brush7);
        palette2.setBrush(QPalette::Active, QPalette::Window, brush8);
        palette2.setBrush(QPalette::Active, QPalette::Shadow, brush6);
        palette2.setBrush(QPalette::Active, QPalette::Highlight, brush9);
        palette2.setBrush(QPalette::Active, QPalette::HighlightedText, brush7);
        palette2.setBrush(QPalette::Active, QPalette::Link, brush6);
        palette2.setBrush(QPalette::Active, QPalette::LinkVisited, brush6);
        palette2.setBrush(QPalette::Active, QPalette::AlternateBase, brush10);
        palette2.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette2.setBrush(QPalette::Inactive, QPalette::Button, brush7);
        palette2.setBrush(QPalette::Inactive, QPalette::Light, brush7);
        palette2.setBrush(QPalette::Inactive, QPalette::Midlight, brush7);
        palette2.setBrush(QPalette::Inactive, QPalette::Dark, brush16);
        palette2.setBrush(QPalette::Inactive, QPalette::Mid, brush17);
        palette2.setBrush(QPalette::Inactive, QPalette::Text, brush6);
        palette2.setBrush(QPalette::Inactive, QPalette::BrightText, brush7);
        palette2.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
        palette2.setBrush(QPalette::Inactive, QPalette::Base, brush7);
        palette2.setBrush(QPalette::Inactive, QPalette::Window, brush8);
        palette2.setBrush(QPalette::Inactive, QPalette::Shadow, brush6);
        palette2.setBrush(QPalette::Inactive, QPalette::Highlight, brush9);
        palette2.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush7);
        palette2.setBrush(QPalette::Inactive, QPalette::Link, brush6);
        palette2.setBrush(QPalette::Inactive, QPalette::LinkVisited, brush6);
        palette2.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush10);
        palette2.setBrush(QPalette::Disabled, QPalette::WindowText, brush);
        palette2.setBrush(QPalette::Disabled, QPalette::Button, brush7);
        palette2.setBrush(QPalette::Disabled, QPalette::Light, brush7);
        palette2.setBrush(QPalette::Disabled, QPalette::Midlight, brush7);
        palette2.setBrush(QPalette::Disabled, QPalette::Dark, brush16);
        palette2.setBrush(QPalette::Disabled, QPalette::Mid, brush17);
        palette2.setBrush(QPalette::Disabled, QPalette::Text, brush6);
        palette2.setBrush(QPalette::Disabled, QPalette::BrightText, brush7);
        palette2.setBrush(QPalette::Disabled, QPalette::ButtonText, brush);
        palette2.setBrush(QPalette::Disabled, QPalette::Base, brush7);
        palette2.setBrush(QPalette::Disabled, QPalette::Window, brush8);
        palette2.setBrush(QPalette::Disabled, QPalette::Shadow, brush6);
        palette2.setBrush(QPalette::Disabled, QPalette::Highlight, brush9);
        palette2.setBrush(QPalette::Disabled, QPalette::HighlightedText, brush7);
        palette2.setBrush(QPalette::Disabled, QPalette::Link, brush6);
        palette2.setBrush(QPalette::Disabled, QPalette::LinkVisited, brush6);
        palette2.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush10);
        gammaslider->setPalette(palette2);
        gammaslider->setMaximum(400);
        gammaslider->setValue(100);
        gammaslider->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(gammaslider, 0, 1, 1, 1);

        TextLabel1_2 = new QLabel(GroupBox1);
        TextLabel1_2->setObjectName(QString::fromUtf8("TextLabel1_2"));

        gridLayout->addWidget(TextLabel1_2, 2, 0, 1, 1);

        rlabel = new QLabel(GroupBox1);
        rlabel->setObjectName(QString::fromUtf8("rlabel"));

        gridLayout->addWidget(rlabel, 2, 2, 1, 1);

        rslider = new QSlider(GroupBox1);
        rslider->setObjectName(QString::fromUtf8("rslider"));
        QPalette palette3;
        palette3.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush18(QColor(255, 0, 0, 255));
        brush18.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Button, brush18);
        QBrush brush19(QColor(255, 127, 127, 255));
        brush19.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Light, brush19);
        QBrush brush20(QColor(255, 38, 38, 255));
        brush20.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Midlight, brush20);
        QBrush brush21(QColor(127, 0, 0, 255));
        brush21.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Dark, brush21);
        QBrush brush22(QColor(170, 0, 0, 255));
        brush22.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Mid, brush22);
        palette3.setBrush(QPalette::Active, QPalette::Text, brush6);
        palette3.setBrush(QPalette::Active, QPalette::BrightText, brush7);
        palette3.setBrush(QPalette::Active, QPalette::ButtonText, brush);
        palette3.setBrush(QPalette::Active, QPalette::Base, brush7);
        palette3.setBrush(QPalette::Active, QPalette::Window, brush8);
        palette3.setBrush(QPalette::Active, QPalette::Shadow, brush6);
        palette3.setBrush(QPalette::Active, QPalette::Highlight, brush9);
        palette3.setBrush(QPalette::Active, QPalette::HighlightedText, brush7);
        palette3.setBrush(QPalette::Active, QPalette::Link, brush6);
        palette3.setBrush(QPalette::Active, QPalette::LinkVisited, brush6);
        palette3.setBrush(QPalette::Active, QPalette::AlternateBase, brush10);
        palette3.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Button, brush18);
        palette3.setBrush(QPalette::Inactive, QPalette::Light, brush19);
        palette3.setBrush(QPalette::Inactive, QPalette::Midlight, brush20);
        palette3.setBrush(QPalette::Inactive, QPalette::Dark, brush21);
        palette3.setBrush(QPalette::Inactive, QPalette::Mid, brush22);
        palette3.setBrush(QPalette::Inactive, QPalette::Text, brush6);
        palette3.setBrush(QPalette::Inactive, QPalette::BrightText, brush7);
        palette3.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Base, brush7);
        palette3.setBrush(QPalette::Inactive, QPalette::Window, brush8);
        palette3.setBrush(QPalette::Inactive, QPalette::Shadow, brush6);
        palette3.setBrush(QPalette::Inactive, QPalette::Highlight, brush9);
        palette3.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush7);
        palette3.setBrush(QPalette::Inactive, QPalette::Link, brush6);
        palette3.setBrush(QPalette::Inactive, QPalette::LinkVisited, brush6);
        palette3.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush10);
        palette3.setBrush(QPalette::Disabled, QPalette::WindowText, brush);
        palette3.setBrush(QPalette::Disabled, QPalette::Button, brush18);
        palette3.setBrush(QPalette::Disabled, QPalette::Light, brush19);
        palette3.setBrush(QPalette::Disabled, QPalette::Midlight, brush20);
        palette3.setBrush(QPalette::Disabled, QPalette::Dark, brush21);
        palette3.setBrush(QPalette::Disabled, QPalette::Mid, brush22);
        palette3.setBrush(QPalette::Disabled, QPalette::Text, brush6);
        palette3.setBrush(QPalette::Disabled, QPalette::BrightText, brush7);
        palette3.setBrush(QPalette::Disabled, QPalette::ButtonText, brush);
        palette3.setBrush(QPalette::Disabled, QPalette::Base, brush7);
        palette3.setBrush(QPalette::Disabled, QPalette::Window, brush8);
        palette3.setBrush(QPalette::Disabled, QPalette::Shadow, brush6);
        palette3.setBrush(QPalette::Disabled, QPalette::Highlight, brush9);
        palette3.setBrush(QPalette::Disabled, QPalette::HighlightedText, brush7);
        palette3.setBrush(QPalette::Disabled, QPalette::Link, brush6);
        palette3.setBrush(QPalette::Disabled, QPalette::LinkVisited, brush6);
        palette3.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush10);
        rslider->setPalette(palette3);
        rslider->setMaximum(400);
        rslider->setValue(100);
        rslider->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(rslider, 2, 1, 1, 1);

        PushButton3 = new QPushButton(GroupBox1);
        PushButton3->setObjectName(QString::fromUtf8("PushButton3"));

        gridLayout->addWidget(PushButton3, 8, 0, 1, 3);

        MyCustomWidget1 = new GammaView(GroupBox1);
        MyCustomWidget1->setObjectName(QString::fromUtf8("MyCustomWidget1"));

        gridLayout->addWidget(MyCustomWidget1, 0, 3, 9, 1);


        vboxLayout->addWidget(GroupBox1);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(6);
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(spacerItem1);

        buttonOk = new QPushButton(Config);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);

        hboxLayout3->addWidget(buttonOk);

        buttonCancel = new QPushButton(Config);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout3->addWidget(buttonCancel);


        vboxLayout->addLayout(hboxLayout3);


        retranslateUi(Config);
        QObject::connect(size_width, SIGNAL(valueChanged(int)), size_custom, SLOT(click()));
        QObject::connect(size_height, SIGNAL(valueChanged(int)), size_custom, SLOT(click()));

        buttonOk->setDefault(true);


        QMetaObject::connectSlotsByName(Config);
    } // setupUi

    void retranslateUi(QDialog *Config)
    {
        Config->setWindowTitle(QCoreApplication::translate("Config", "Configure", nullptr));
        ButtonGroup1->setTitle(QCoreApplication::translate("Config", "Size", nullptr));
        size_176_220->setText(QCoreApplication::translate("Config", "176x220 \"SmartPhone\"", nullptr));
        size_240_320->setText(QCoreApplication::translate("Config", "240x320 \"PDA\"", nullptr));
        size_320_240->setText(QCoreApplication::translate("Config", "320x240 \"TV\" / \"QVGA\"", nullptr));
        size_640_480->setText(QCoreApplication::translate("Config", "640x480 \"VGA\"", nullptr));
        size_800_600->setText(QCoreApplication::translate("Config", "800x600", nullptr));
        size_1024_768->setText(QCoreApplication::translate("Config", "1024x768", nullptr));
        size_custom->setText(QCoreApplication::translate("Config", "Custom", nullptr));
        ButtonGroup2->setTitle(QCoreApplication::translate("Config", "Depth", nullptr));
        depth_1->setText(QCoreApplication::translate("Config", "1 bit monochrome", nullptr));
        depth_4gray->setText(QCoreApplication::translate("Config", "4 bit grayscale", nullptr));
        depth_8->setText(QCoreApplication::translate("Config", "8 bit", nullptr));
        depth_12->setText(QCoreApplication::translate("Config", "12 (16) bit", nullptr));
        depth_15->setText(QCoreApplication::translate("Config", "15 bit", nullptr));
        depth_16->setText(QCoreApplication::translate("Config", "16 bit", nullptr));
        depth_18->setText(QCoreApplication::translate("Config", "18 bit", nullptr));
        depth_24->setText(QCoreApplication::translate("Config", "24 bit", nullptr));
        depth_32->setText(QCoreApplication::translate("Config", "32 bit", nullptr));
        depth_32_argb->setText(QCoreApplication::translate("Config", "32 bit ARGB", nullptr));
        TextLabel1_3->setText(QCoreApplication::translate("Config", "Skin", nullptr));
        skin->setItemText(0, QCoreApplication::translate("Config", "None", nullptr));

        touchScreen->setText(QCoreApplication::translate("Config", "Emulate touch screen (no mouse move)", nullptr));
        lcdScreen->setText(QCoreApplication::translate("Config", "Emulate LCD screen (Only with fixed zoom of 3.0 times magnification)", nullptr));
        TextLabel1->setText(QCoreApplication::translate("Config", "<p>Note that any applications using the virtual framebuffer will be terminated if you change the Size or Depth <i>above</i>. You may freely modify the Gamma <i>below</i>.", nullptr));
        GroupBox1->setTitle(QCoreApplication::translate("Config", "Gamma", nullptr));
        TextLabel3->setText(QCoreApplication::translate("Config", "Blue", nullptr));
        blabel->setText(QCoreApplication::translate("Config", "1.0", nullptr));
        TextLabel2->setText(QCoreApplication::translate("Config", "Green", nullptr));
        glabel->setText(QCoreApplication::translate("Config", "1.0", nullptr));
        TextLabel7->setText(QCoreApplication::translate("Config", "All", nullptr));
        TextLabel8->setText(QCoreApplication::translate("Config", "1.0", nullptr));
        TextLabel1_2->setText(QCoreApplication::translate("Config", "Red", nullptr));
        rlabel->setText(QCoreApplication::translate("Config", "1.0", nullptr));
        PushButton3->setText(QCoreApplication::translate("Config", "Set all to 1.0", nullptr));
        buttonOk->setText(QCoreApplication::translate("Config", "&OK", nullptr));
        buttonCancel->setText(QCoreApplication::translate("Config", "&Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Config: public Ui_Config {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CONFIG_H
