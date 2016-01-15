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
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
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
            Config->setObjectName(QStringLiteral("Config"));
        Config->resize(600, 650);
        Config->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(Config);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        ButtonGroup1 = new QGroupBox(Config);
        ButtonGroup1->setObjectName(QStringLiteral("ButtonGroup1"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ButtonGroup1->sizePolicy().hasHeightForWidth());
        ButtonGroup1->setSizePolicy(sizePolicy);
        vboxLayout1 = new QVBoxLayout(ButtonGroup1);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        size_176_220 = new QRadioButton(ButtonGroup1);
        size_176_220->setObjectName(QStringLiteral("size_176_220"));

        vboxLayout1->addWidget(size_176_220);

        size_240_320 = new QRadioButton(ButtonGroup1);
        size_240_320->setObjectName(QStringLiteral("size_240_320"));

        vboxLayout1->addWidget(size_240_320);

        size_320_240 = new QRadioButton(ButtonGroup1);
        size_320_240->setObjectName(QStringLiteral("size_320_240"));

        vboxLayout1->addWidget(size_320_240);

        size_640_480 = new QRadioButton(ButtonGroup1);
        size_640_480->setObjectName(QStringLiteral("size_640_480"));

        vboxLayout1->addWidget(size_640_480);

        size_800_600 = new QRadioButton(ButtonGroup1);
        size_800_600->setObjectName(QStringLiteral("size_800_600"));

        vboxLayout1->addWidget(size_800_600);

        size_1024_768 = new QRadioButton(ButtonGroup1);
        size_1024_768->setObjectName(QStringLiteral("size_1024_768"));

        vboxLayout1->addWidget(size_1024_768);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        size_custom = new QRadioButton(ButtonGroup1);
        size_custom->setObjectName(QStringLiteral("size_custom"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(size_custom->sizePolicy().hasHeightForWidth());
        size_custom->setSizePolicy(sizePolicy1);

        hboxLayout1->addWidget(size_custom);

        size_width = new QSpinBox(ButtonGroup1);
        size_width->setObjectName(QStringLiteral("size_width"));
        size_width->setMinimum(1);
        size_width->setMaximum(1280);
        size_width->setSingleStep(16);
        size_width->setValue(400);

        hboxLayout1->addWidget(size_width);

        size_height = new QSpinBox(ButtonGroup1);
        size_height->setObjectName(QStringLiteral("size_height"));
        size_height->setMinimum(1);
        size_height->setMaximum(1024);
        size_height->setSingleStep(16);
        size_height->setValue(300);

        hboxLayout1->addWidget(size_height);


        vboxLayout1->addLayout(hboxLayout1);


        hboxLayout->addWidget(ButtonGroup1);

        ButtonGroup2 = new QGroupBox(Config);
        ButtonGroup2->setObjectName(QStringLiteral("ButtonGroup2"));
        vboxLayout2 = new QVBoxLayout(ButtonGroup2);
        vboxLayout2->setSpacing(6);
        vboxLayout2->setContentsMargins(11, 11, 11, 11);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        vboxLayout2->setContentsMargins(11, 11, 11, 11);
        depth_1 = new QRadioButton(ButtonGroup2);
        depth_1->setObjectName(QStringLiteral("depth_1"));

        vboxLayout2->addWidget(depth_1);

        depth_4gray = new QRadioButton(ButtonGroup2);
        depth_4gray->setObjectName(QStringLiteral("depth_4gray"));

        vboxLayout2->addWidget(depth_4gray);

        depth_8 = new QRadioButton(ButtonGroup2);
        depth_8->setObjectName(QStringLiteral("depth_8"));

        vboxLayout2->addWidget(depth_8);

        depth_12 = new QRadioButton(ButtonGroup2);
        depth_12->setObjectName(QStringLiteral("depth_12"));

        vboxLayout2->addWidget(depth_12);

        depth_15 = new QRadioButton(ButtonGroup2);
        depth_15->setObjectName(QStringLiteral("depth_15"));

        vboxLayout2->addWidget(depth_15);

        depth_16 = new QRadioButton(ButtonGroup2);
        depth_16->setObjectName(QStringLiteral("depth_16"));

        vboxLayout2->addWidget(depth_16);

        depth_18 = new QRadioButton(ButtonGroup2);
        depth_18->setObjectName(QStringLiteral("depth_18"));

        vboxLayout2->addWidget(depth_18);

        depth_24 = new QRadioButton(ButtonGroup2);
        depth_24->setObjectName(QStringLiteral("depth_24"));

        vboxLayout2->addWidget(depth_24);

        depth_32 = new QRadioButton(ButtonGroup2);
        depth_32->setObjectName(QStringLiteral("depth_32"));

        vboxLayout2->addWidget(depth_32);

        depth_32_argb = new QRadioButton(ButtonGroup2);
        depth_32_argb->setObjectName(QStringLiteral("depth_32_argb"));

        vboxLayout2->addWidget(depth_32_argb);


        hboxLayout->addWidget(ButtonGroup2);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QStringLiteral("hboxLayout2"));
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        TextLabel1_3 = new QLabel(Config);
        TextLabel1_3->setObjectName(QStringLiteral("TextLabel1_3"));

        hboxLayout2->addWidget(TextLabel1_3);

        skin = new QComboBox(Config);
        skin->setObjectName(QStringLiteral("skin"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(skin->sizePolicy().hasHeightForWidth());
        skin->setSizePolicy(sizePolicy2);

        hboxLayout2->addWidget(skin);


        vboxLayout->addLayout(hboxLayout2);

        touchScreen = new QCheckBox(Config);
        touchScreen->setObjectName(QStringLiteral("touchScreen"));

        vboxLayout->addWidget(touchScreen);

        lcdScreen = new QCheckBox(Config);
        lcdScreen->setObjectName(QStringLiteral("lcdScreen"));

        vboxLayout->addWidget(lcdScreen);

        spacerItem = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);

        TextLabel1 = new QLabel(Config);
        TextLabel1->setObjectName(QStringLiteral("TextLabel1"));
        sizePolicy.setHeightForWidth(TextLabel1->sizePolicy().hasHeightForWidth());
        TextLabel1->setSizePolicy(sizePolicy);
        TextLabel1->setWordWrap(true);

        vboxLayout->addWidget(TextLabel1);

        GroupBox1 = new QGroupBox(Config);
        GroupBox1->setObjectName(QStringLiteral("GroupBox1"));
        gridLayout = new QGridLayout(GroupBox1);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        TextLabel3 = new QLabel(GroupBox1);
        TextLabel3->setObjectName(QStringLiteral("TextLabel3"));

        gridLayout->addWidget(TextLabel3, 6, 0, 1, 1);

        bslider = new QSlider(GroupBox1);
        bslider->setObjectName(QStringLiteral("bslider"));
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
        blabel->setObjectName(QStringLiteral("blabel"));

        gridLayout->addWidget(blabel, 6, 2, 1, 1);

        TextLabel2 = new QLabel(GroupBox1);
        TextLabel2->setObjectName(QStringLiteral("TextLabel2"));

        gridLayout->addWidget(TextLabel2, 4, 0, 1, 1);

        gslider = new QSlider(GroupBox1);
        gslider->setObjectName(QStringLiteral("gslider"));
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
        glabel->setObjectName(QStringLiteral("glabel"));

        gridLayout->addWidget(glabel, 4, 2, 1, 1);

        TextLabel7 = new QLabel(GroupBox1);
        TextLabel7->setObjectName(QStringLiteral("TextLabel7"));

        gridLayout->addWidget(TextLabel7, 0, 0, 1, 1);

        TextLabel8 = new QLabel(GroupBox1);
        TextLabel8->setObjectName(QStringLiteral("TextLabel8"));

        gridLayout->addWidget(TextLabel8, 0, 2, 1, 1);

        gammaslider = new QSlider(GroupBox1);
        gammaslider->setObjectName(QStringLiteral("gammaslider"));
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
        TextLabel1_2->setObjectName(QStringLiteral("TextLabel1_2"));

        gridLayout->addWidget(TextLabel1_2, 2, 0, 1, 1);

        rlabel = new QLabel(GroupBox1);
        rlabel->setObjectName(QStringLiteral("rlabel"));

        gridLayout->addWidget(rlabel, 2, 2, 1, 1);

        rslider = new QSlider(GroupBox1);
        rslider->setObjectName(QStringLiteral("rslider"));
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
        PushButton3->setObjectName(QStringLiteral("PushButton3"));

        gridLayout->addWidget(PushButton3, 8, 0, 1, 3);

        MyCustomWidget1 = new GammaView(GroupBox1);
        MyCustomWidget1->setObjectName(QStringLiteral("MyCustomWidget1"));

        gridLayout->addWidget(MyCustomWidget1, 0, 3, 9, 1);


        vboxLayout->addWidget(GroupBox1);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(6);
        hboxLayout3->setObjectName(QStringLiteral("hboxLayout3"));
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(spacerItem1);

        buttonOk = new QPushButton(Config);
        buttonOk->setObjectName(QStringLiteral("buttonOk"));
        buttonOk->setAutoDefault(true);

        hboxLayout3->addWidget(buttonOk);

        buttonCancel = new QPushButton(Config);
        buttonCancel->setObjectName(QStringLiteral("buttonCancel"));
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
        Config->setWindowTitle(QApplication::translate("Config", "Configure", 0));
        ButtonGroup1->setTitle(QApplication::translate("Config", "Size", 0));
        size_176_220->setText(QApplication::translate("Config", "176x220 \"SmartPhone\"", 0));
        size_240_320->setText(QApplication::translate("Config", "240x320 \"PDA\"", 0));
        size_320_240->setText(QApplication::translate("Config", "320x240 \"TV\" / \"QVGA\"", 0));
        size_640_480->setText(QApplication::translate("Config", "640x480 \"VGA\"", 0));
        size_800_600->setText(QApplication::translate("Config", "800x600", 0));
        size_1024_768->setText(QApplication::translate("Config", "1024x768", 0));
        size_custom->setText(QApplication::translate("Config", "Custom", 0));
        ButtonGroup2->setTitle(QApplication::translate("Config", "Depth", 0));
        depth_1->setText(QApplication::translate("Config", "1 bit monochrome", 0));
        depth_4gray->setText(QApplication::translate("Config", "4 bit grayscale", 0));
        depth_8->setText(QApplication::translate("Config", "8 bit", 0));
        depth_12->setText(QApplication::translate("Config", "12 (16) bit", 0));
        depth_15->setText(QApplication::translate("Config", "15 bit", 0));
        depth_16->setText(QApplication::translate("Config", "16 bit", 0));
        depth_18->setText(QApplication::translate("Config", "18 bit", 0));
        depth_24->setText(QApplication::translate("Config", "24 bit", 0));
        depth_32->setText(QApplication::translate("Config", "32 bit", 0));
        depth_32_argb->setText(QApplication::translate("Config", "32 bit ARGB", 0));
        TextLabel1_3->setText(QApplication::translate("Config", "Skin", 0));
        skin->clear();
        skin->insertItems(0, QStringList()
         << QApplication::translate("Config", "None", 0)
        );
        touchScreen->setText(QApplication::translate("Config", "Emulate touch screen (no mouse move)", 0));
        lcdScreen->setText(QApplication::translate("Config", "Emulate LCD screen (Only with fixed zoom of 3.0 times magnification)", 0));
        TextLabel1->setText(QApplication::translate("Config", "<p>Note that any applications using the virtual framebuffer will be terminated if you change the Size or Depth <i>above</i>. You may freely modify the Gamma <i>below</i>.", 0));
        GroupBox1->setTitle(QApplication::translate("Config", "Gamma", 0));
        TextLabel3->setText(QApplication::translate("Config", "Blue", 0));
        blabel->setText(QApplication::translate("Config", "1.0", 0));
        TextLabel2->setText(QApplication::translate("Config", "Green", 0));
        glabel->setText(QApplication::translate("Config", "1.0", 0));
        TextLabel7->setText(QApplication::translate("Config", "All", 0));
        TextLabel8->setText(QApplication::translate("Config", "1.0", 0));
        TextLabel1_2->setText(QApplication::translate("Config", "Red", 0));
        rlabel->setText(QApplication::translate("Config", "1.0", 0));
        PushButton3->setText(QApplication::translate("Config", "Set all to 1.0", 0));
        buttonOk->setText(QApplication::translate("Config", "&OK", 0));
        buttonCancel->setText(QApplication::translate("Config", "&Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class Config: public Ui_Config {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CONFIG_H
