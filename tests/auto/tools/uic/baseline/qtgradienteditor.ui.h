/*
*********************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
** Form generated from reading UI file 'qtgradienteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTGRADIENTEDITOR_H
#define QTGRADIENTEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "qtcolorbutton.h"
#include "qtcolorline.h"
#include "qtgradientstopswidget.h"
#include "qtgradientwidget.h"

QT_BEGIN_NAMESPACE

class Ui_QtGradientEditor
{
public:
    QFrame *frame;
    QVBoxLayout *vboxLayout;
    QtGradientWidget *gradientWidget;
    QLabel *label1;
    QDoubleSpinBox *spinBox1;
    QLabel *label2;
    QDoubleSpinBox *spinBox2;
    QLabel *label3;
    QDoubleSpinBox *spinBox3;
    QLabel *label4;
    QDoubleSpinBox *spinBox4;
    QLabel *label5;
    QDoubleSpinBox *spinBox5;
    QtGradientStopsWidget *gradientStopsWidget;
    QLabel *zoomLabel;
    QToolButton *zoomAllButton;
    QLabel *positionLabel;
    QLabel *hLabel;
    QFrame *frame_2;
    QHBoxLayout *hboxLayout;
    QtColorLine *hueColorLine;
    QLabel *hueLabel;
    QLabel *sLabel;
    QFrame *frame_5;
    QHBoxLayout *hboxLayout1;
    QtColorLine *saturationColorLine;
    QLabel *saturationLabel;
    QLabel *vLabel;
    QFrame *frame_3;
    QHBoxLayout *hboxLayout2;
    QtColorLine *valueColorLine;
    QLabel *valueLabel;
    QLabel *aLabel;
    QFrame *frame_4;
    QHBoxLayout *hboxLayout3;
    QtColorLine *alphaColorLine;
    QLabel *alphaLabel;
    QComboBox *typeComboBox;
    QComboBox *spreadComboBox;
    QLabel *colorLabel;
    QtColorButton *colorButton;
    QRadioButton *hsvRadioButton;
    QRadioButton *rgbRadioButton;
    QWidget *positionWidget;
    QVBoxLayout *vboxLayout1;
    QDoubleSpinBox *positionSpinBox;
    QWidget *hueWidget;
    QVBoxLayout *vboxLayout2;
    QSpinBox *hueSpinBox;
    QWidget *saturationWidget;
    QVBoxLayout *vboxLayout3;
    QSpinBox *saturationSpinBox;
    QWidget *valueWidget;
    QVBoxLayout *vboxLayout4;
    QSpinBox *valueSpinBox;
    QWidget *alphaWidget;
    QVBoxLayout *vboxLayout5;
    QSpinBox *alphaSpinBox;
    QWidget *zoomWidget;
    QVBoxLayout *vboxLayout6;
    QSpinBox *zoomSpinBox;
    QWidget *line1Widget;
    QVBoxLayout *vboxLayout7;
    QFrame *line1;
    QWidget *line2Widget;
    QVBoxLayout *vboxLayout8;
    QFrame *line2;
    QWidget *zoomButtonsWidget;
    QHBoxLayout *hboxLayout4;
    QToolButton *zoomInButton;
    QToolButton *zoomOutButton;
    QSpacerItem *spacerItem;
    QToolButton *detailsButton;
    QToolButton *linearButton;
    QToolButton *radialButton;
    QToolButton *conicalButton;
    QToolButton *padButton;
    QToolButton *repeatButton;
    QToolButton *reflectButton;

    void setupUi(QWidget *QtGradientEditor)
    {
        if (QtGradientEditor->objectName().isEmpty())
            QtGradientEditor->setObjectName(QString::fromUtf8("QtGradientEditor"));
        QtGradientEditor->resize(364, 518);
        frame = new QFrame(QtGradientEditor);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setGeometry(QRect(10, 69, 193, 150));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frame->sizePolicy().hasHeightForWidth());
        frame->setSizePolicy(sizePolicy);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        vboxLayout = new QVBoxLayout(frame);
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        gradientWidget = new QtGradientWidget(frame);
        gradientWidget->setObjectName(QString::fromUtf8("gradientWidget"));
        sizePolicy.setHeightForWidth(gradientWidget->sizePolicy().hasHeightForWidth());
        gradientWidget->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(gradientWidget);

        label1 = new QLabel(QtGradientEditor);
        label1->setObjectName(QString::fromUtf8("label1"));
        label1->setGeometry(QRect(209, 69, 64, 23));
        spinBox1 = new QDoubleSpinBox(QtGradientEditor);
        spinBox1->setObjectName(QString::fromUtf8("spinBox1"));
        spinBox1->setGeometry(QRect(279, 69, 73, 23));
        spinBox1->setKeyboardTracking(false);
        spinBox1->setDecimals(3);
        spinBox1->setMaximum(1.000000000000000);
        spinBox1->setSingleStep(0.010000000000000);
        label2 = new QLabel(QtGradientEditor);
        label2->setObjectName(QString::fromUtf8("label2"));
        label2->setGeometry(QRect(209, 99, 64, 23));
        spinBox2 = new QDoubleSpinBox(QtGradientEditor);
        spinBox2->setObjectName(QString::fromUtf8("spinBox2"));
        spinBox2->setGeometry(QRect(279, 99, 73, 23));
        spinBox2->setKeyboardTracking(false);
        spinBox2->setDecimals(3);
        spinBox2->setMaximum(1.000000000000000);
        spinBox2->setSingleStep(0.010000000000000);
        label3 = new QLabel(QtGradientEditor);
        label3->setObjectName(QString::fromUtf8("label3"));
        label3->setGeometry(QRect(209, 129, 64, 23));
        spinBox3 = new QDoubleSpinBox(QtGradientEditor);
        spinBox3->setObjectName(QString::fromUtf8("spinBox3"));
        spinBox3->setGeometry(QRect(279, 129, 73, 23));
        spinBox3->setKeyboardTracking(false);
        spinBox3->setDecimals(3);
        spinBox3->setMaximum(1.000000000000000);
        spinBox3->setSingleStep(0.010000000000000);
        label4 = new QLabel(QtGradientEditor);
        label4->setObjectName(QString::fromUtf8("label4"));
        label4->setGeometry(QRect(209, 159, 64, 23));
        spinBox4 = new QDoubleSpinBox(QtGradientEditor);
        spinBox4->setObjectName(QString::fromUtf8("spinBox4"));
        spinBox4->setGeometry(QRect(279, 159, 73, 23));
        spinBox4->setKeyboardTracking(false);
        spinBox4->setDecimals(3);
        spinBox4->setMaximum(1.000000000000000);
        spinBox4->setSingleStep(0.010000000000000);
        label5 = new QLabel(QtGradientEditor);
        label5->setObjectName(QString::fromUtf8("label5"));
        label5->setGeometry(QRect(209, 189, 64, 23));
        spinBox5 = new QDoubleSpinBox(QtGradientEditor);
        spinBox5->setObjectName(QString::fromUtf8("spinBox5"));
        spinBox5->setGeometry(QRect(279, 189, 73, 23));
        spinBox5->setKeyboardTracking(false);
        spinBox5->setDecimals(3);
        spinBox5->setMaximum(1.000000000000000);
        spinBox5->setSingleStep(0.010000000000000);
        gradientStopsWidget = new QtGradientStopsWidget(QtGradientEditor);
        gradientStopsWidget->setObjectName(QString::fromUtf8("gradientStopsWidget"));
        gradientStopsWidget->setGeometry(QRect(10, 225, 193, 67));
        zoomLabel = new QLabel(QtGradientEditor);
        zoomLabel->setObjectName(QString::fromUtf8("zoomLabel"));
        zoomLabel->setGeometry(QRect(209, 231, 64, 23));
        zoomAllButton = new QToolButton(QtGradientEditor);
        zoomAllButton->setObjectName(QString::fromUtf8("zoomAllButton"));
        zoomAllButton->setGeometry(QRect(279, 260, 72, 26));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(zoomAllButton->sizePolicy().hasHeightForWidth());
        zoomAllButton->setSizePolicy(sizePolicy1);
        positionLabel = new QLabel(QtGradientEditor);
        positionLabel->setObjectName(QString::fromUtf8("positionLabel"));
        positionLabel->setGeometry(QRect(209, 304, 64, 23));
        hLabel = new QLabel(QtGradientEditor);
        hLabel->setObjectName(QString::fromUtf8("hLabel"));
        hLabel->setGeometry(QRect(10, 335, 32, 18));
        sizePolicy1.setHeightForWidth(hLabel->sizePolicy().hasHeightForWidth());
        hLabel->setSizePolicy(sizePolicy1);
        frame_2 = new QFrame(QtGradientEditor);
        frame_2->setObjectName(QString::fromUtf8("frame_2"));
        frame_2->setGeometry(QRect(48, 333, 155, 23));
        QSizePolicy sizePolicy2(QSizePolicy::Ignored, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(frame_2->sizePolicy().hasHeightForWidth());
        frame_2->setSizePolicy(sizePolicy2);
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        hboxLayout = new QHBoxLayout(frame_2);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hueColorLine = new QtColorLine(frame_2);
        hueColorLine->setObjectName(QString::fromUtf8("hueColorLine"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(hueColorLine->sizePolicy().hasHeightForWidth());
        hueColorLine->setSizePolicy(sizePolicy3);

        hboxLayout->addWidget(hueColorLine);

        hueLabel = new QLabel(QtGradientEditor);
        hueLabel->setObjectName(QString::fromUtf8("hueLabel"));
        hueLabel->setGeometry(QRect(209, 335, 64, 18));
        sizePolicy1.setHeightForWidth(hueLabel->sizePolicy().hasHeightForWidth());
        hueLabel->setSizePolicy(sizePolicy1);
        sLabel = new QLabel(QtGradientEditor);
        sLabel->setObjectName(QString::fromUtf8("sLabel"));
        sLabel->setGeometry(QRect(10, 364, 32, 18));
        sizePolicy1.setHeightForWidth(sLabel->sizePolicy().hasHeightForWidth());
        sLabel->setSizePolicy(sizePolicy1);
        frame_5 = new QFrame(QtGradientEditor);
        frame_5->setObjectName(QString::fromUtf8("frame_5"));
        frame_5->setGeometry(QRect(48, 362, 155, 23));
        sizePolicy2.setHeightForWidth(frame_5->sizePolicy().hasHeightForWidth());
        frame_5->setSizePolicy(sizePolicy2);
        frame_5->setFrameShape(QFrame::StyledPanel);
        frame_5->setFrameShadow(QFrame::Raised);
        hboxLayout1 = new QHBoxLayout(frame_5);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        saturationColorLine = new QtColorLine(frame_5);
        saturationColorLine->setObjectName(QString::fromUtf8("saturationColorLine"));
        sizePolicy3.setHeightForWidth(saturationColorLine->sizePolicy().hasHeightForWidth());
        saturationColorLine->setSizePolicy(sizePolicy3);

        hboxLayout1->addWidget(saturationColorLine);

        saturationLabel = new QLabel(QtGradientEditor);
        saturationLabel->setObjectName(QString::fromUtf8("saturationLabel"));
        saturationLabel->setGeometry(QRect(209, 364, 64, 18));
        sizePolicy1.setHeightForWidth(saturationLabel->sizePolicy().hasHeightForWidth());
        saturationLabel->setSizePolicy(sizePolicy1);
        vLabel = new QLabel(QtGradientEditor);
        vLabel->setObjectName(QString::fromUtf8("vLabel"));
        vLabel->setGeometry(QRect(10, 393, 32, 18));
        sizePolicy1.setHeightForWidth(vLabel->sizePolicy().hasHeightForWidth());
        vLabel->setSizePolicy(sizePolicy1);
        frame_3 = new QFrame(QtGradientEditor);
        frame_3->setObjectName(QString::fromUtf8("frame_3"));
        frame_3->setGeometry(QRect(48, 391, 155, 23));
        sizePolicy2.setHeightForWidth(frame_3->sizePolicy().hasHeightForWidth());
        frame_3->setSizePolicy(sizePolicy2);
        frame_3->setFrameShape(QFrame::StyledPanel);
        frame_3->setFrameShadow(QFrame::Raised);
        hboxLayout2 = new QHBoxLayout(frame_3);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        valueColorLine = new QtColorLine(frame_3);
        valueColorLine->setObjectName(QString::fromUtf8("valueColorLine"));
        sizePolicy3.setHeightForWidth(valueColorLine->sizePolicy().hasHeightForWidth());
        valueColorLine->setSizePolicy(sizePolicy3);

        hboxLayout2->addWidget(valueColorLine);

        valueLabel = new QLabel(QtGradientEditor);
        valueLabel->setObjectName(QString::fromUtf8("valueLabel"));
        valueLabel->setGeometry(QRect(209, 393, 64, 18));
        sizePolicy1.setHeightForWidth(valueLabel->sizePolicy().hasHeightForWidth());
        valueLabel->setSizePolicy(sizePolicy1);
        aLabel = new QLabel(QtGradientEditor);
        aLabel->setObjectName(QString::fromUtf8("aLabel"));
        aLabel->setGeometry(QRect(10, 422, 32, 18));
        sizePolicy1.setHeightForWidth(aLabel->sizePolicy().hasHeightForWidth());
        aLabel->setSizePolicy(sizePolicy1);
        frame_4 = new QFrame(QtGradientEditor);
        frame_4->setObjectName(QString::fromUtf8("frame_4"));
        frame_4->setGeometry(QRect(48, 420, 155, 23));
        sizePolicy2.setHeightForWidth(frame_4->sizePolicy().hasHeightForWidth());
        frame_4->setSizePolicy(sizePolicy2);
        frame_4->setFrameShape(QFrame::StyledPanel);
        frame_4->setFrameShadow(QFrame::Raised);
        hboxLayout3 = new QHBoxLayout(frame_4);
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        alphaColorLine = new QtColorLine(frame_4);
        alphaColorLine->setObjectName(QString::fromUtf8("alphaColorLine"));
        sizePolicy3.setHeightForWidth(alphaColorLine->sizePolicy().hasHeightForWidth());
        alphaColorLine->setSizePolicy(sizePolicy3);

        hboxLayout3->addWidget(alphaColorLine);

        alphaLabel = new QLabel(QtGradientEditor);
        alphaLabel->setObjectName(QString::fromUtf8("alphaLabel"));
        alphaLabel->setGeometry(QRect(209, 422, 64, 18));
        sizePolicy1.setHeightForWidth(alphaLabel->sizePolicy().hasHeightForWidth());
        alphaLabel->setSizePolicy(sizePolicy1);
        typeComboBox = new QComboBox(QtGradientEditor);
        typeComboBox->setObjectName(QString::fromUtf8("typeComboBox"));
        typeComboBox->setGeometry(QRect(10, 40, 79, 22));
        spreadComboBox = new QComboBox(QtGradientEditor);
        spreadComboBox->setObjectName(QString::fromUtf8("spreadComboBox"));
        spreadComboBox->setGeometry(QRect(96, 40, 72, 22));
        colorLabel = new QLabel(QtGradientEditor);
        colorLabel->setObjectName(QString::fromUtf8("colorLabel"));
        colorLabel->setGeometry(QRect(10, 298, 32, 29));
        QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(colorLabel->sizePolicy().hasHeightForWidth());
        colorLabel->setSizePolicy(sizePolicy4);
        colorButton = new QtColorButton(QtGradientEditor);
        colorButton->setObjectName(QString::fromUtf8("colorButton"));
        colorButton->setGeometry(QRect(48, 300, 26, 25));
        hsvRadioButton = new QRadioButton(QtGradientEditor);
        hsvRadioButton->setObjectName(QString::fromUtf8("hsvRadioButton"));
        hsvRadioButton->setGeometry(QRect(80, 301, 49, 23));
        QSizePolicy sizePolicy5(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(hsvRadioButton->sizePolicy().hasHeightForWidth());
        hsvRadioButton->setSizePolicy(sizePolicy5);
        hsvRadioButton->setChecked(true);
        rgbRadioButton = new QRadioButton(QtGradientEditor);
        rgbRadioButton->setObjectName(QString::fromUtf8("rgbRadioButton"));
        rgbRadioButton->setGeometry(QRect(135, 301, 49, 23));
        sizePolicy5.setHeightForWidth(rgbRadioButton->sizePolicy().hasHeightForWidth());
        rgbRadioButton->setSizePolicy(sizePolicy5);
        positionWidget = new QWidget(QtGradientEditor);
        positionWidget->setObjectName(QString::fromUtf8("positionWidget"));
        positionWidget->setGeometry(QRect(279, 304, 73, 23));
        vboxLayout1 = new QVBoxLayout(positionWidget);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        positionSpinBox = new QDoubleSpinBox(positionWidget);
        positionSpinBox->setObjectName(QString::fromUtf8("positionSpinBox"));
        positionSpinBox->setKeyboardTracking(false);
        positionSpinBox->setDecimals(3);
        positionSpinBox->setMinimum(0.000000000000000);
        positionSpinBox->setMaximum(1.000000000000000);
        positionSpinBox->setSingleStep(0.010000000000000);
        positionSpinBox->setValue(0.000000000000000);

        vboxLayout1->addWidget(positionSpinBox);

        hueWidget = new QWidget(QtGradientEditor);
        hueWidget->setObjectName(QString::fromUtf8("hueWidget"));
        hueWidget->setGeometry(QRect(279, 333, 73, 23));
        vboxLayout2 = new QVBoxLayout(hueWidget);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        hueSpinBox = new QSpinBox(hueWidget);
        hueSpinBox->setObjectName(QString::fromUtf8("hueSpinBox"));
        hueSpinBox->setKeyboardTracking(false);
        hueSpinBox->setMaximum(359);

        vboxLayout2->addWidget(hueSpinBox);

        saturationWidget = new QWidget(QtGradientEditor);
        saturationWidget->setObjectName(QString::fromUtf8("saturationWidget"));
        saturationWidget->setGeometry(QRect(279, 362, 73, 23));
        vboxLayout3 = new QVBoxLayout(saturationWidget);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        vboxLayout3->setContentsMargins(0, 0, 0, 0);
        saturationSpinBox = new QSpinBox(saturationWidget);
        saturationSpinBox->setObjectName(QString::fromUtf8("saturationSpinBox"));
        saturationSpinBox->setKeyboardTracking(false);
        saturationSpinBox->setMaximum(255);

        vboxLayout3->addWidget(saturationSpinBox);

        valueWidget = new QWidget(QtGradientEditor);
        valueWidget->setObjectName(QString::fromUtf8("valueWidget"));
        valueWidget->setGeometry(QRect(279, 391, 73, 23));
        vboxLayout4 = new QVBoxLayout(valueWidget);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        vboxLayout4->setContentsMargins(0, 0, 0, 0);
        valueSpinBox = new QSpinBox(valueWidget);
        valueSpinBox->setObjectName(QString::fromUtf8("valueSpinBox"));
        valueSpinBox->setKeyboardTracking(false);
        valueSpinBox->setMaximum(255);

        vboxLayout4->addWidget(valueSpinBox);

        alphaWidget = new QWidget(QtGradientEditor);
        alphaWidget->setObjectName(QString::fromUtf8("alphaWidget"));
        alphaWidget->setGeometry(QRect(279, 420, 73, 23));
        vboxLayout5 = new QVBoxLayout(alphaWidget);
        vboxLayout5->setObjectName(QString::fromUtf8("vboxLayout5"));
        vboxLayout5->setContentsMargins(0, 0, 0, 0);
        alphaSpinBox = new QSpinBox(alphaWidget);
        alphaSpinBox->setObjectName(QString::fromUtf8("alphaSpinBox"));
        alphaSpinBox->setKeyboardTracking(false);
        alphaSpinBox->setMaximum(255);

        vboxLayout5->addWidget(alphaSpinBox);

        zoomWidget = new QWidget(QtGradientEditor);
        zoomWidget->setObjectName(QString::fromUtf8("zoomWidget"));
        zoomWidget->setGeometry(QRect(279, 231, 73, 23));
        vboxLayout6 = new QVBoxLayout(zoomWidget);
        vboxLayout6->setObjectName(QString::fromUtf8("vboxLayout6"));
        vboxLayout6->setContentsMargins(0, 0, 0, 0);
        zoomSpinBox = new QSpinBox(zoomWidget);
        zoomSpinBox->setObjectName(QString::fromUtf8("zoomSpinBox"));
        zoomSpinBox->setKeyboardTracking(false);
        zoomSpinBox->setMinimum(100);
        zoomSpinBox->setMaximum(10000);
        zoomSpinBox->setSingleStep(100);
        zoomSpinBox->setValue(100);

        vboxLayout6->addWidget(zoomSpinBox);

        line1Widget = new QWidget(QtGradientEditor);
        line1Widget->setObjectName(QString::fromUtf8("line1Widget"));
        line1Widget->setGeometry(QRect(209, 219, 143, 16));
        vboxLayout7 = new QVBoxLayout(line1Widget);
        vboxLayout7->setObjectName(QString::fromUtf8("vboxLayout7"));
        vboxLayout7->setContentsMargins(0, 0, 0, 0);
        line1 = new QFrame(line1Widget);
        line1->setObjectName(QString::fromUtf8("line1"));
        line1->setFrameShape(QFrame::HLine);
        line1->setFrameShadow(QFrame::Sunken);

        vboxLayout7->addWidget(line1);

        line2Widget = new QWidget(QtGradientEditor);
        line2Widget->setObjectName(QString::fromUtf8("line2Widget"));
        line2Widget->setGeometry(QRect(209, 292, 143, 16));
        vboxLayout8 = new QVBoxLayout(line2Widget);
        vboxLayout8->setObjectName(QString::fromUtf8("vboxLayout8"));
        vboxLayout8->setContentsMargins(0, 0, 0, 0);
        line2 = new QFrame(line2Widget);
        line2->setObjectName(QString::fromUtf8("line2"));
        line2->setFrameShape(QFrame::HLine);
        line2->setFrameShadow(QFrame::Sunken);

        vboxLayout8->addWidget(line2);

        zoomButtonsWidget = new QWidget(QtGradientEditor);
        zoomButtonsWidget->setObjectName(QString::fromUtf8("zoomButtonsWidget"));
        zoomButtonsWidget->setGeometry(QRect(209, 260, 64, 26));
        QSizePolicy sizePolicy6(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(zoomButtonsWidget->sizePolicy().hasHeightForWidth());
        zoomButtonsWidget->setSizePolicy(sizePolicy6);
        hboxLayout4 = new QHBoxLayout(zoomButtonsWidget);
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        hboxLayout4->setContentsMargins(0, 0, 0, 0);
        zoomInButton = new QToolButton(zoomButtonsWidget);
        zoomInButton->setObjectName(QString::fromUtf8("zoomInButton"));

        hboxLayout4->addWidget(zoomInButton);

        zoomOutButton = new QToolButton(zoomButtonsWidget);
        zoomOutButton->setObjectName(QString::fromUtf8("zoomOutButton"));

        hboxLayout4->addWidget(zoomOutButton);

        spacerItem = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(spacerItem);

        detailsButton = new QToolButton(QtGradientEditor);
        detailsButton->setObjectName(QString::fromUtf8("detailsButton"));
        detailsButton->setGeometry(QRect(176, 40, 25, 22));
        QSizePolicy sizePolicy7(QSizePolicy::Fixed, QSizePolicy::Ignored);
        sizePolicy7.setHorizontalStretch(0);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(detailsButton->sizePolicy().hasHeightForWidth());
        detailsButton->setSizePolicy(sizePolicy7);
        detailsButton->setCheckable(true);
        detailsButton->setAutoRaise(true);
        linearButton = new QToolButton(QtGradientEditor);
        linearButton->setObjectName(QString::fromUtf8("linearButton"));
        linearButton->setGeometry(QRect(10, 10, 30, 26));
        linearButton->setCheckable(true);
        linearButton->setAutoRaise(true);
        radialButton = new QToolButton(QtGradientEditor);
        radialButton->setObjectName(QString::fromUtf8("radialButton"));
        radialButton->setGeometry(QRect(40, 10, 30, 26));
        radialButton->setCheckable(true);
        radialButton->setAutoRaise(true);
        conicalButton = new QToolButton(QtGradientEditor);
        conicalButton->setObjectName(QString::fromUtf8("conicalButton"));
        conicalButton->setGeometry(QRect(70, 10, 30, 26));
        conicalButton->setCheckable(true);
        conicalButton->setAutoRaise(true);
        padButton = new QToolButton(QtGradientEditor);
        padButton->setObjectName(QString::fromUtf8("padButton"));
        padButton->setGeometry(QRect(110, 10, 30, 26));
        padButton->setCheckable(true);
        padButton->setAutoRaise(true);
        repeatButton = new QToolButton(QtGradientEditor);
        repeatButton->setObjectName(QString::fromUtf8("repeatButton"));
        repeatButton->setGeometry(QRect(140, 10, 30, 26));
        repeatButton->setCheckable(true);
        repeatButton->setAutoRaise(true);
        reflectButton = new QToolButton(QtGradientEditor);
        reflectButton->setObjectName(QString::fromUtf8("reflectButton"));
        reflectButton->setGeometry(QRect(170, 10, 30, 26));
        reflectButton->setCheckable(true);
        reflectButton->setAutoRaise(true);
        QWidget::setTabOrder(typeComboBox, spreadComboBox);
        QWidget::setTabOrder(spreadComboBox, detailsButton);
        QWidget::setTabOrder(detailsButton, spinBox1);
        QWidget::setTabOrder(spinBox1, spinBox2);
        QWidget::setTabOrder(spinBox2, spinBox3);
        QWidget::setTabOrder(spinBox3, spinBox4);
        QWidget::setTabOrder(spinBox4, spinBox5);
        QWidget::setTabOrder(spinBox5, zoomSpinBox);
        QWidget::setTabOrder(zoomSpinBox, zoomInButton);
        QWidget::setTabOrder(zoomInButton, zoomOutButton);
        QWidget::setTabOrder(zoomOutButton, zoomAllButton);
        QWidget::setTabOrder(zoomAllButton, colorButton);
        QWidget::setTabOrder(colorButton, hsvRadioButton);
        QWidget::setTabOrder(hsvRadioButton, rgbRadioButton);
        QWidget::setTabOrder(rgbRadioButton, positionSpinBox);
        QWidget::setTabOrder(positionSpinBox, hueSpinBox);
        QWidget::setTabOrder(hueSpinBox, saturationSpinBox);
        QWidget::setTabOrder(saturationSpinBox, valueSpinBox);
        QWidget::setTabOrder(valueSpinBox, alphaSpinBox);

        retranslateUi(QtGradientEditor);

        QMetaObject::connectSlotsByName(QtGradientEditor);
    } // setupUi

    void retranslateUi(QWidget *QtGradientEditor)
    {
        QtGradientEditor->setWindowTitle(QCoreApplication::translate("QtGradientEditor", "Form", nullptr));
#if QT_CONFIG(tooltip)
        gradientWidget->setToolTip(QCoreApplication::translate("QtGradientEditor", "Gradient Editor", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        gradientWidget->setWhatsThis(QCoreApplication::translate("QtGradientEditor", "This area shows a preview of the gradient being edited. It also allows you to edit parameters specific to the gradient's type such as start and final point, radius, etc. by drag & drop.", nullptr));
#endif // QT_CONFIG(whatsthis)
        label1->setText(QCoreApplication::translate("QtGradientEditor", "1", nullptr));
        label2->setText(QCoreApplication::translate("QtGradientEditor", "2", nullptr));
        label3->setText(QCoreApplication::translate("QtGradientEditor", "3", nullptr));
        label4->setText(QCoreApplication::translate("QtGradientEditor", "4", nullptr));
        label5->setText(QCoreApplication::translate("QtGradientEditor", "5", nullptr));
#if QT_CONFIG(tooltip)
        gradientStopsWidget->setToolTip(QCoreApplication::translate("QtGradientEditor", "Gradient Stops Editor", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        gradientStopsWidget->setWhatsThis(QCoreApplication::translate("QtGradientEditor", "This area allows you to edit gradient stops. Double click on the existing stop handle to duplicate it. Double click outside of the existing stop handles to create a new stop. Drag & drop the handle to reposition it. Use right mouse button to popup context menu with extra actions.", nullptr));
#endif // QT_CONFIG(whatsthis)
        zoomLabel->setText(QCoreApplication::translate("QtGradientEditor", "Zoom", nullptr));
#if QT_CONFIG(tooltip)
        zoomAllButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Reset Zoom", nullptr));
#endif // QT_CONFIG(tooltip)
        zoomAllButton->setText(QCoreApplication::translate("QtGradientEditor", "Reset Zoom", nullptr));
        positionLabel->setText(QCoreApplication::translate("QtGradientEditor", "Position", nullptr));
#if QT_CONFIG(tooltip)
        hLabel->setToolTip(QCoreApplication::translate("QtGradientEditor", "Hue", nullptr));
#endif // QT_CONFIG(tooltip)
        hLabel->setText(QCoreApplication::translate("QtGradientEditor", "H", nullptr));
#if QT_CONFIG(tooltip)
        hueColorLine->setToolTip(QCoreApplication::translate("QtGradientEditor", "Hue", nullptr));
#endif // QT_CONFIG(tooltip)
        hueLabel->setText(QCoreApplication::translate("QtGradientEditor", "Hue", nullptr));
#if QT_CONFIG(tooltip)
        sLabel->setToolTip(QCoreApplication::translate("QtGradientEditor", "Saturation", nullptr));
#endif // QT_CONFIG(tooltip)
        sLabel->setText(QCoreApplication::translate("QtGradientEditor", "S", nullptr));
#if QT_CONFIG(tooltip)
        saturationColorLine->setToolTip(QCoreApplication::translate("QtGradientEditor", "Saturation", nullptr));
#endif // QT_CONFIG(tooltip)
        saturationLabel->setText(QCoreApplication::translate("QtGradientEditor", "Sat", nullptr));
#if QT_CONFIG(tooltip)
        vLabel->setToolTip(QCoreApplication::translate("QtGradientEditor", "Value", nullptr));
#endif // QT_CONFIG(tooltip)
        vLabel->setText(QCoreApplication::translate("QtGradientEditor", "V", nullptr));
#if QT_CONFIG(tooltip)
        valueColorLine->setToolTip(QCoreApplication::translate("QtGradientEditor", "Value", nullptr));
#endif // QT_CONFIG(tooltip)
        valueLabel->setText(QCoreApplication::translate("QtGradientEditor", "Val", nullptr));
#if QT_CONFIG(tooltip)
        aLabel->setToolTip(QCoreApplication::translate("QtGradientEditor", "Alpha", nullptr));
#endif // QT_CONFIG(tooltip)
        aLabel->setText(QCoreApplication::translate("QtGradientEditor", "A", nullptr));
#if QT_CONFIG(tooltip)
        alphaColorLine->setToolTip(QCoreApplication::translate("QtGradientEditor", "Alpha", nullptr));
#endif // QT_CONFIG(tooltip)
        alphaLabel->setText(QCoreApplication::translate("QtGradientEditor", "Alpha", nullptr));
#if QT_CONFIG(tooltip)
        typeComboBox->setToolTip(QCoreApplication::translate("QtGradientEditor", "Type", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        spreadComboBox->setToolTip(QCoreApplication::translate("QtGradientEditor", "Spread", nullptr));
#endif // QT_CONFIG(tooltip)
        colorLabel->setText(QCoreApplication::translate("QtGradientEditor", "Color", nullptr));
#if QT_CONFIG(tooltip)
        colorButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Current stop's color", nullptr));
#endif // QT_CONFIG(tooltip)
        colorButton->setText(QString());
#if QT_CONFIG(tooltip)
        hsvRadioButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Show HSV specification", nullptr));
#endif // QT_CONFIG(tooltip)
        hsvRadioButton->setText(QCoreApplication::translate("QtGradientEditor", "HSV", nullptr));
#if QT_CONFIG(tooltip)
        rgbRadioButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Show RGB specification", nullptr));
#endif // QT_CONFIG(tooltip)
        rgbRadioButton->setText(QCoreApplication::translate("QtGradientEditor", "RGB", nullptr));
#if QT_CONFIG(tooltip)
        positionSpinBox->setToolTip(QCoreApplication::translate("QtGradientEditor", "Current stop's position", nullptr));
#endif // QT_CONFIG(tooltip)
        zoomSpinBox->setSuffix(QCoreApplication::translate("QtGradientEditor", "%", nullptr));
#if QT_CONFIG(tooltip)
        zoomInButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Zoom In", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        zoomOutButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Zoom Out", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        detailsButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Toggle details extension", nullptr));
#endif // QT_CONFIG(tooltip)
        detailsButton->setText(QCoreApplication::translate("QtGradientEditor", ">", nullptr));
#if QT_CONFIG(tooltip)
        linearButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Linear Type", nullptr));
#endif // QT_CONFIG(tooltip)
        linearButton->setText(QCoreApplication::translate("QtGradientEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        radialButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Radial Type", nullptr));
#endif // QT_CONFIG(tooltip)
        radialButton->setText(QCoreApplication::translate("QtGradientEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        conicalButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Conical Type", nullptr));
#endif // QT_CONFIG(tooltip)
        conicalButton->setText(QCoreApplication::translate("QtGradientEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        padButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Pad Spread", nullptr));
#endif // QT_CONFIG(tooltip)
        padButton->setText(QCoreApplication::translate("QtGradientEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        repeatButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Repeat Spread", nullptr));
#endif // QT_CONFIG(tooltip)
        repeatButton->setText(QCoreApplication::translate("QtGradientEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        reflectButton->setToolTip(QCoreApplication::translate("QtGradientEditor", "Reflect Spread", nullptr));
#endif // QT_CONFIG(tooltip)
        reflectButton->setText(QCoreApplication::translate("QtGradientEditor", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtGradientEditor: public Ui_QtGradientEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTGRADIENTEDITOR_H
