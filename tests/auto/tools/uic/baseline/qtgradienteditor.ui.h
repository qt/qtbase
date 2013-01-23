/*
*********************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'qtgradienteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTGRADIENTEDITOR_H
#define QTGRADIENTEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
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
            QtGradientEditor->setObjectName(QStringLiteral("QtGradientEditor"));
        QtGradientEditor->resize(364, 518);
        frame = new QFrame(QtGradientEditor);
        frame->setObjectName(QStringLiteral("frame"));
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
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        gradientWidget = new QtGradientWidget(frame);
        gradientWidget->setObjectName(QStringLiteral("gradientWidget"));
        sizePolicy.setHeightForWidth(gradientWidget->sizePolicy().hasHeightForWidth());
        gradientWidget->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(gradientWidget);

        label1 = new QLabel(QtGradientEditor);
        label1->setObjectName(QStringLiteral("label1"));
        label1->setGeometry(QRect(209, 69, 64, 23));
        spinBox1 = new QDoubleSpinBox(QtGradientEditor);
        spinBox1->setObjectName(QStringLiteral("spinBox1"));
        spinBox1->setGeometry(QRect(279, 69, 73, 23));
        spinBox1->setKeyboardTracking(false);
        spinBox1->setDecimals(3);
        spinBox1->setMaximum(1);
        spinBox1->setSingleStep(0.01);
        label2 = new QLabel(QtGradientEditor);
        label2->setObjectName(QStringLiteral("label2"));
        label2->setGeometry(QRect(209, 99, 64, 23));
        spinBox2 = new QDoubleSpinBox(QtGradientEditor);
        spinBox2->setObjectName(QStringLiteral("spinBox2"));
        spinBox2->setGeometry(QRect(279, 99, 73, 23));
        spinBox2->setKeyboardTracking(false);
        spinBox2->setDecimals(3);
        spinBox2->setMaximum(1);
        spinBox2->setSingleStep(0.01);
        label3 = new QLabel(QtGradientEditor);
        label3->setObjectName(QStringLiteral("label3"));
        label3->setGeometry(QRect(209, 129, 64, 23));
        spinBox3 = new QDoubleSpinBox(QtGradientEditor);
        spinBox3->setObjectName(QStringLiteral("spinBox3"));
        spinBox3->setGeometry(QRect(279, 129, 73, 23));
        spinBox3->setKeyboardTracking(false);
        spinBox3->setDecimals(3);
        spinBox3->setMaximum(1);
        spinBox3->setSingleStep(0.01);
        label4 = new QLabel(QtGradientEditor);
        label4->setObjectName(QStringLiteral("label4"));
        label4->setGeometry(QRect(209, 159, 64, 23));
        spinBox4 = new QDoubleSpinBox(QtGradientEditor);
        spinBox4->setObjectName(QStringLiteral("spinBox4"));
        spinBox4->setGeometry(QRect(279, 159, 73, 23));
        spinBox4->setKeyboardTracking(false);
        spinBox4->setDecimals(3);
        spinBox4->setMaximum(1);
        spinBox4->setSingleStep(0.01);
        label5 = new QLabel(QtGradientEditor);
        label5->setObjectName(QStringLiteral("label5"));
        label5->setGeometry(QRect(209, 189, 64, 23));
        spinBox5 = new QDoubleSpinBox(QtGradientEditor);
        spinBox5->setObjectName(QStringLiteral("spinBox5"));
        spinBox5->setGeometry(QRect(279, 189, 73, 23));
        spinBox5->setKeyboardTracking(false);
        spinBox5->setDecimals(3);
        spinBox5->setMaximum(1);
        spinBox5->setSingleStep(0.01);
        gradientStopsWidget = new QtGradientStopsWidget(QtGradientEditor);
        gradientStopsWidget->setObjectName(QStringLiteral("gradientStopsWidget"));
        gradientStopsWidget->setGeometry(QRect(10, 225, 193, 67));
        zoomLabel = new QLabel(QtGradientEditor);
        zoomLabel->setObjectName(QStringLiteral("zoomLabel"));
        zoomLabel->setGeometry(QRect(209, 231, 64, 23));
        zoomAllButton = new QToolButton(QtGradientEditor);
        zoomAllButton->setObjectName(QStringLiteral("zoomAllButton"));
        zoomAllButton->setGeometry(QRect(279, 260, 72, 26));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(zoomAllButton->sizePolicy().hasHeightForWidth());
        zoomAllButton->setSizePolicy(sizePolicy1);
        positionLabel = new QLabel(QtGradientEditor);
        positionLabel->setObjectName(QStringLiteral("positionLabel"));
        positionLabel->setGeometry(QRect(209, 304, 64, 23));
        hLabel = new QLabel(QtGradientEditor);
        hLabel->setObjectName(QStringLiteral("hLabel"));
        hLabel->setGeometry(QRect(10, 335, 32, 18));
        sizePolicy1.setHeightForWidth(hLabel->sizePolicy().hasHeightForWidth());
        hLabel->setSizePolicy(sizePolicy1);
        frame_2 = new QFrame(QtGradientEditor);
        frame_2->setObjectName(QStringLiteral("frame_2"));
        frame_2->setGeometry(QRect(48, 333, 155, 23));
        QSizePolicy sizePolicy2(QSizePolicy::Ignored, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(frame_2->sizePolicy().hasHeightForWidth());
        frame_2->setSizePolicy(sizePolicy2);
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        hboxLayout = new QHBoxLayout(frame_2);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hueColorLine = new QtColorLine(frame_2);
        hueColorLine->setObjectName(QStringLiteral("hueColorLine"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(hueColorLine->sizePolicy().hasHeightForWidth());
        hueColorLine->setSizePolicy(sizePolicy3);

        hboxLayout->addWidget(hueColorLine);

        hueLabel = new QLabel(QtGradientEditor);
        hueLabel->setObjectName(QStringLiteral("hueLabel"));
        hueLabel->setGeometry(QRect(209, 335, 64, 18));
        sizePolicy1.setHeightForWidth(hueLabel->sizePolicy().hasHeightForWidth());
        hueLabel->setSizePolicy(sizePolicy1);
        sLabel = new QLabel(QtGradientEditor);
        sLabel->setObjectName(QStringLiteral("sLabel"));
        sLabel->setGeometry(QRect(10, 364, 32, 18));
        sizePolicy1.setHeightForWidth(sLabel->sizePolicy().hasHeightForWidth());
        sLabel->setSizePolicy(sizePolicy1);
        frame_5 = new QFrame(QtGradientEditor);
        frame_5->setObjectName(QStringLiteral("frame_5"));
        frame_5->setGeometry(QRect(48, 362, 155, 23));
        sizePolicy2.setHeightForWidth(frame_5->sizePolicy().hasHeightForWidth());
        frame_5->setSizePolicy(sizePolicy2);
        frame_5->setFrameShape(QFrame::StyledPanel);
        frame_5->setFrameShadow(QFrame::Raised);
        hboxLayout1 = new QHBoxLayout(frame_5);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        saturationColorLine = new QtColorLine(frame_5);
        saturationColorLine->setObjectName(QStringLiteral("saturationColorLine"));
        sizePolicy3.setHeightForWidth(saturationColorLine->sizePolicy().hasHeightForWidth());
        saturationColorLine->setSizePolicy(sizePolicy3);

        hboxLayout1->addWidget(saturationColorLine);

        saturationLabel = new QLabel(QtGradientEditor);
        saturationLabel->setObjectName(QStringLiteral("saturationLabel"));
        saturationLabel->setGeometry(QRect(209, 364, 64, 18));
        sizePolicy1.setHeightForWidth(saturationLabel->sizePolicy().hasHeightForWidth());
        saturationLabel->setSizePolicy(sizePolicy1);
        vLabel = new QLabel(QtGradientEditor);
        vLabel->setObjectName(QStringLiteral("vLabel"));
        vLabel->setGeometry(QRect(10, 393, 32, 18));
        sizePolicy1.setHeightForWidth(vLabel->sizePolicy().hasHeightForWidth());
        vLabel->setSizePolicy(sizePolicy1);
        frame_3 = new QFrame(QtGradientEditor);
        frame_3->setObjectName(QStringLiteral("frame_3"));
        frame_3->setGeometry(QRect(48, 391, 155, 23));
        sizePolicy2.setHeightForWidth(frame_3->sizePolicy().hasHeightForWidth());
        frame_3->setSizePolicy(sizePolicy2);
        frame_3->setFrameShape(QFrame::StyledPanel);
        frame_3->setFrameShadow(QFrame::Raised);
        hboxLayout2 = new QHBoxLayout(frame_3);
        hboxLayout2->setObjectName(QStringLiteral("hboxLayout2"));
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        valueColorLine = new QtColorLine(frame_3);
        valueColorLine->setObjectName(QStringLiteral("valueColorLine"));
        sizePolicy3.setHeightForWidth(valueColorLine->sizePolicy().hasHeightForWidth());
        valueColorLine->setSizePolicy(sizePolicy3);

        hboxLayout2->addWidget(valueColorLine);

        valueLabel = new QLabel(QtGradientEditor);
        valueLabel->setObjectName(QStringLiteral("valueLabel"));
        valueLabel->setGeometry(QRect(209, 393, 64, 18));
        sizePolicy1.setHeightForWidth(valueLabel->sizePolicy().hasHeightForWidth());
        valueLabel->setSizePolicy(sizePolicy1);
        aLabel = new QLabel(QtGradientEditor);
        aLabel->setObjectName(QStringLiteral("aLabel"));
        aLabel->setGeometry(QRect(10, 422, 32, 18));
        sizePolicy1.setHeightForWidth(aLabel->sizePolicy().hasHeightForWidth());
        aLabel->setSizePolicy(sizePolicy1);
        frame_4 = new QFrame(QtGradientEditor);
        frame_4->setObjectName(QStringLiteral("frame_4"));
        frame_4->setGeometry(QRect(48, 420, 155, 23));
        sizePolicy2.setHeightForWidth(frame_4->sizePolicy().hasHeightForWidth());
        frame_4->setSizePolicy(sizePolicy2);
        frame_4->setFrameShape(QFrame::StyledPanel);
        frame_4->setFrameShadow(QFrame::Raised);
        hboxLayout3 = new QHBoxLayout(frame_4);
        hboxLayout3->setObjectName(QStringLiteral("hboxLayout3"));
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        alphaColorLine = new QtColorLine(frame_4);
        alphaColorLine->setObjectName(QStringLiteral("alphaColorLine"));
        sizePolicy3.setHeightForWidth(alphaColorLine->sizePolicy().hasHeightForWidth());
        alphaColorLine->setSizePolicy(sizePolicy3);

        hboxLayout3->addWidget(alphaColorLine);

        alphaLabel = new QLabel(QtGradientEditor);
        alphaLabel->setObjectName(QStringLiteral("alphaLabel"));
        alphaLabel->setGeometry(QRect(209, 422, 64, 18));
        sizePolicy1.setHeightForWidth(alphaLabel->sizePolicy().hasHeightForWidth());
        alphaLabel->setSizePolicy(sizePolicy1);
        typeComboBox = new QComboBox(QtGradientEditor);
        typeComboBox->setObjectName(QStringLiteral("typeComboBox"));
        typeComboBox->setGeometry(QRect(10, 40, 79, 22));
        spreadComboBox = new QComboBox(QtGradientEditor);
        spreadComboBox->setObjectName(QStringLiteral("spreadComboBox"));
        spreadComboBox->setGeometry(QRect(96, 40, 72, 22));
        colorLabel = new QLabel(QtGradientEditor);
        colorLabel->setObjectName(QStringLiteral("colorLabel"));
        colorLabel->setGeometry(QRect(10, 298, 32, 29));
        QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(colorLabel->sizePolicy().hasHeightForWidth());
        colorLabel->setSizePolicy(sizePolicy4);
        colorButton = new QtColorButton(QtGradientEditor);
        colorButton->setObjectName(QStringLiteral("colorButton"));
        colorButton->setGeometry(QRect(48, 300, 26, 25));
        hsvRadioButton = new QRadioButton(QtGradientEditor);
        hsvRadioButton->setObjectName(QStringLiteral("hsvRadioButton"));
        hsvRadioButton->setGeometry(QRect(80, 301, 49, 23));
        QSizePolicy sizePolicy5(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(hsvRadioButton->sizePolicy().hasHeightForWidth());
        hsvRadioButton->setSizePolicy(sizePolicy5);
        hsvRadioButton->setChecked(true);
        rgbRadioButton = new QRadioButton(QtGradientEditor);
        rgbRadioButton->setObjectName(QStringLiteral("rgbRadioButton"));
        rgbRadioButton->setGeometry(QRect(135, 301, 49, 23));
        sizePolicy5.setHeightForWidth(rgbRadioButton->sizePolicy().hasHeightForWidth());
        rgbRadioButton->setSizePolicy(sizePolicy5);
        positionWidget = new QWidget(QtGradientEditor);
        positionWidget->setObjectName(QStringLiteral("positionWidget"));
        positionWidget->setGeometry(QRect(279, 304, 73, 23));
        vboxLayout1 = new QVBoxLayout(positionWidget);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        positionSpinBox = new QDoubleSpinBox(positionWidget);
        positionSpinBox->setObjectName(QStringLiteral("positionSpinBox"));
        positionSpinBox->setKeyboardTracking(false);
        positionSpinBox->setDecimals(3);
        positionSpinBox->setMinimum(0);
        positionSpinBox->setMaximum(1);
        positionSpinBox->setSingleStep(0.01);
        positionSpinBox->setValue(0);

        vboxLayout1->addWidget(positionSpinBox);

        hueWidget = new QWidget(QtGradientEditor);
        hueWidget->setObjectName(QStringLiteral("hueWidget"));
        hueWidget->setGeometry(QRect(279, 333, 73, 23));
        vboxLayout2 = new QVBoxLayout(hueWidget);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        hueSpinBox = new QSpinBox(hueWidget);
        hueSpinBox->setObjectName(QStringLiteral("hueSpinBox"));
        hueSpinBox->setKeyboardTracking(false);
        hueSpinBox->setMaximum(359);

        vboxLayout2->addWidget(hueSpinBox);

        saturationWidget = new QWidget(QtGradientEditor);
        saturationWidget->setObjectName(QStringLiteral("saturationWidget"));
        saturationWidget->setGeometry(QRect(279, 362, 73, 23));
        vboxLayout3 = new QVBoxLayout(saturationWidget);
        vboxLayout3->setObjectName(QStringLiteral("vboxLayout3"));
        vboxLayout3->setContentsMargins(0, 0, 0, 0);
        saturationSpinBox = new QSpinBox(saturationWidget);
        saturationSpinBox->setObjectName(QStringLiteral("saturationSpinBox"));
        saturationSpinBox->setKeyboardTracking(false);
        saturationSpinBox->setMaximum(255);

        vboxLayout3->addWidget(saturationSpinBox);

        valueWidget = new QWidget(QtGradientEditor);
        valueWidget->setObjectName(QStringLiteral("valueWidget"));
        valueWidget->setGeometry(QRect(279, 391, 73, 23));
        vboxLayout4 = new QVBoxLayout(valueWidget);
        vboxLayout4->setObjectName(QStringLiteral("vboxLayout4"));
        vboxLayout4->setContentsMargins(0, 0, 0, 0);
        valueSpinBox = new QSpinBox(valueWidget);
        valueSpinBox->setObjectName(QStringLiteral("valueSpinBox"));
        valueSpinBox->setKeyboardTracking(false);
        valueSpinBox->setMaximum(255);

        vboxLayout4->addWidget(valueSpinBox);

        alphaWidget = new QWidget(QtGradientEditor);
        alphaWidget->setObjectName(QStringLiteral("alphaWidget"));
        alphaWidget->setGeometry(QRect(279, 420, 73, 23));
        vboxLayout5 = new QVBoxLayout(alphaWidget);
        vboxLayout5->setObjectName(QStringLiteral("vboxLayout5"));
        vboxLayout5->setContentsMargins(0, 0, 0, 0);
        alphaSpinBox = new QSpinBox(alphaWidget);
        alphaSpinBox->setObjectName(QStringLiteral("alphaSpinBox"));
        alphaSpinBox->setKeyboardTracking(false);
        alphaSpinBox->setMaximum(255);

        vboxLayout5->addWidget(alphaSpinBox);

        zoomWidget = new QWidget(QtGradientEditor);
        zoomWidget->setObjectName(QStringLiteral("zoomWidget"));
        zoomWidget->setGeometry(QRect(279, 231, 73, 23));
        vboxLayout6 = new QVBoxLayout(zoomWidget);
        vboxLayout6->setObjectName(QStringLiteral("vboxLayout6"));
        vboxLayout6->setContentsMargins(0, 0, 0, 0);
        zoomSpinBox = new QSpinBox(zoomWidget);
        zoomSpinBox->setObjectName(QStringLiteral("zoomSpinBox"));
        zoomSpinBox->setKeyboardTracking(false);
        zoomSpinBox->setMinimum(100);
        zoomSpinBox->setMaximum(10000);
        zoomSpinBox->setSingleStep(100);
        zoomSpinBox->setValue(100);

        vboxLayout6->addWidget(zoomSpinBox);

        line1Widget = new QWidget(QtGradientEditor);
        line1Widget->setObjectName(QStringLiteral("line1Widget"));
        line1Widget->setGeometry(QRect(209, 219, 143, 16));
        vboxLayout7 = new QVBoxLayout(line1Widget);
        vboxLayout7->setObjectName(QStringLiteral("vboxLayout7"));
        vboxLayout7->setContentsMargins(0, 0, 0, 0);
        line1 = new QFrame(line1Widget);
        line1->setObjectName(QStringLiteral("line1"));
        line1->setFrameShape(QFrame::HLine);
        line1->setFrameShadow(QFrame::Sunken);

        vboxLayout7->addWidget(line1);

        line2Widget = new QWidget(QtGradientEditor);
        line2Widget->setObjectName(QStringLiteral("line2Widget"));
        line2Widget->setGeometry(QRect(209, 292, 143, 16));
        vboxLayout8 = new QVBoxLayout(line2Widget);
        vboxLayout8->setObjectName(QStringLiteral("vboxLayout8"));
        vboxLayout8->setContentsMargins(0, 0, 0, 0);
        line2 = new QFrame(line2Widget);
        line2->setObjectName(QStringLiteral("line2"));
        line2->setFrameShape(QFrame::HLine);
        line2->setFrameShadow(QFrame::Sunken);

        vboxLayout8->addWidget(line2);

        zoomButtonsWidget = new QWidget(QtGradientEditor);
        zoomButtonsWidget->setObjectName(QStringLiteral("zoomButtonsWidget"));
        zoomButtonsWidget->setGeometry(QRect(209, 260, 64, 26));
        QSizePolicy sizePolicy6(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(zoomButtonsWidget->sizePolicy().hasHeightForWidth());
        zoomButtonsWidget->setSizePolicy(sizePolicy6);
        hboxLayout4 = new QHBoxLayout(zoomButtonsWidget);
        hboxLayout4->setObjectName(QStringLiteral("hboxLayout4"));
        hboxLayout4->setContentsMargins(0, 0, 0, 0);
        zoomInButton = new QToolButton(zoomButtonsWidget);
        zoomInButton->setObjectName(QStringLiteral("zoomInButton"));

        hboxLayout4->addWidget(zoomInButton);

        zoomOutButton = new QToolButton(zoomButtonsWidget);
        zoomOutButton->setObjectName(QStringLiteral("zoomOutButton"));

        hboxLayout4->addWidget(zoomOutButton);

        spacerItem = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(spacerItem);

        detailsButton = new QToolButton(QtGradientEditor);
        detailsButton->setObjectName(QStringLiteral("detailsButton"));
        detailsButton->setGeometry(QRect(176, 40, 25, 22));
        QSizePolicy sizePolicy7(QSizePolicy::Fixed, QSizePolicy::Ignored);
        sizePolicy7.setHorizontalStretch(0);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(detailsButton->sizePolicy().hasHeightForWidth());
        detailsButton->setSizePolicy(sizePolicy7);
        detailsButton->setCheckable(true);
        detailsButton->setAutoRaise(true);
        linearButton = new QToolButton(QtGradientEditor);
        linearButton->setObjectName(QStringLiteral("linearButton"));
        linearButton->setGeometry(QRect(10, 10, 30, 26));
        linearButton->setCheckable(true);
        linearButton->setAutoRaise(true);
        radialButton = new QToolButton(QtGradientEditor);
        radialButton->setObjectName(QStringLiteral("radialButton"));
        radialButton->setGeometry(QRect(40, 10, 30, 26));
        radialButton->setCheckable(true);
        radialButton->setAutoRaise(true);
        conicalButton = new QToolButton(QtGradientEditor);
        conicalButton->setObjectName(QStringLiteral("conicalButton"));
        conicalButton->setGeometry(QRect(70, 10, 30, 26));
        conicalButton->setCheckable(true);
        conicalButton->setAutoRaise(true);
        padButton = new QToolButton(QtGradientEditor);
        padButton->setObjectName(QStringLiteral("padButton"));
        padButton->setGeometry(QRect(110, 10, 30, 26));
        padButton->setCheckable(true);
        padButton->setAutoRaise(true);
        repeatButton = new QToolButton(QtGradientEditor);
        repeatButton->setObjectName(QStringLiteral("repeatButton"));
        repeatButton->setGeometry(QRect(140, 10, 30, 26));
        repeatButton->setCheckable(true);
        repeatButton->setAutoRaise(true);
        reflectButton = new QToolButton(QtGradientEditor);
        reflectButton->setObjectName(QStringLiteral("reflectButton"));
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
        QtGradientEditor->setWindowTitle(QApplication::translate("QtGradientEditor", "Form", 0));
#ifndef QT_NO_TOOLTIP
        gradientWidget->setToolTip(QApplication::translate("QtGradientEditor", "Gradient Editor", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        gradientWidget->setWhatsThis(QApplication::translate("QtGradientEditor", "This area shows a preview of the gradient being edited. It also allows you to edit parameters specific to the gradient's type such as start and final point, radius, etc. by drag & drop.", 0));
#endif // QT_NO_WHATSTHIS
        label1->setText(QApplication::translate("QtGradientEditor", "1", 0));
        label2->setText(QApplication::translate("QtGradientEditor", "2", 0));
        label3->setText(QApplication::translate("QtGradientEditor", "3", 0));
        label4->setText(QApplication::translate("QtGradientEditor", "4", 0));
        label5->setText(QApplication::translate("QtGradientEditor", "5", 0));
#ifndef QT_NO_TOOLTIP
        gradientStopsWidget->setToolTip(QApplication::translate("QtGradientEditor", "Gradient Stops Editor", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        gradientStopsWidget->setWhatsThis(QApplication::translate("QtGradientEditor", "This area allows you to edit gradient stops. Double click on the existing stop handle to duplicate it. Double click outside of the existing stop handles to create a new stop. Drag & drop the handle to reposition it. Use right mouse button to popup context menu with extra actions.", 0));
#endif // QT_NO_WHATSTHIS
        zoomLabel->setText(QApplication::translate("QtGradientEditor", "Zoom", 0));
#ifndef QT_NO_TOOLTIP
        zoomAllButton->setToolTip(QApplication::translate("QtGradientEditor", "Reset Zoom", 0));
#endif // QT_NO_TOOLTIP
        zoomAllButton->setText(QApplication::translate("QtGradientEditor", "Reset Zoom", 0));
        positionLabel->setText(QApplication::translate("QtGradientEditor", "Position", 0));
#ifndef QT_NO_TOOLTIP
        hLabel->setToolTip(QApplication::translate("QtGradientEditor", "Hue", 0));
#endif // QT_NO_TOOLTIP
        hLabel->setText(QApplication::translate("QtGradientEditor", "H", 0));
#ifndef QT_NO_TOOLTIP
        hueColorLine->setToolTip(QApplication::translate("QtGradientEditor", "Hue", 0));
#endif // QT_NO_TOOLTIP
        hueLabel->setText(QApplication::translate("QtGradientEditor", "Hue", 0));
#ifndef QT_NO_TOOLTIP
        sLabel->setToolTip(QApplication::translate("QtGradientEditor", "Saturation", 0));
#endif // QT_NO_TOOLTIP
        sLabel->setText(QApplication::translate("QtGradientEditor", "S", 0));
#ifndef QT_NO_TOOLTIP
        saturationColorLine->setToolTip(QApplication::translate("QtGradientEditor", "Saturation", 0));
#endif // QT_NO_TOOLTIP
        saturationLabel->setText(QApplication::translate("QtGradientEditor", "Sat", 0));
#ifndef QT_NO_TOOLTIP
        vLabel->setToolTip(QApplication::translate("QtGradientEditor", "Value", 0));
#endif // QT_NO_TOOLTIP
        vLabel->setText(QApplication::translate("QtGradientEditor", "V", 0));
#ifndef QT_NO_TOOLTIP
        valueColorLine->setToolTip(QApplication::translate("QtGradientEditor", "Value", 0));
#endif // QT_NO_TOOLTIP
        valueLabel->setText(QApplication::translate("QtGradientEditor", "Val", 0));
#ifndef QT_NO_TOOLTIP
        aLabel->setToolTip(QApplication::translate("QtGradientEditor", "Alpha", 0));
#endif // QT_NO_TOOLTIP
        aLabel->setText(QApplication::translate("QtGradientEditor", "A", 0));
#ifndef QT_NO_TOOLTIP
        alphaColorLine->setToolTip(QApplication::translate("QtGradientEditor", "Alpha", 0));
#endif // QT_NO_TOOLTIP
        alphaLabel->setText(QApplication::translate("QtGradientEditor", "Alpha", 0));
#ifndef QT_NO_TOOLTIP
        typeComboBox->setToolTip(QApplication::translate("QtGradientEditor", "Type", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        spreadComboBox->setToolTip(QApplication::translate("QtGradientEditor", "Spread", 0));
#endif // QT_NO_TOOLTIP
        colorLabel->setText(QApplication::translate("QtGradientEditor", "Color", 0));
#ifndef QT_NO_TOOLTIP
        colorButton->setToolTip(QApplication::translate("QtGradientEditor", "Current stop's color", 0));
#endif // QT_NO_TOOLTIP
        colorButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        hsvRadioButton->setToolTip(QApplication::translate("QtGradientEditor", "Show HSV specification", 0));
#endif // QT_NO_TOOLTIP
        hsvRadioButton->setText(QApplication::translate("QtGradientEditor", "HSV", 0));
#ifndef QT_NO_TOOLTIP
        rgbRadioButton->setToolTip(QApplication::translate("QtGradientEditor", "Show RGB specification", 0));
#endif // QT_NO_TOOLTIP
        rgbRadioButton->setText(QApplication::translate("QtGradientEditor", "RGB", 0));
#ifndef QT_NO_TOOLTIP
        positionSpinBox->setToolTip(QApplication::translate("QtGradientEditor", "Current stop's position", 0));
#endif // QT_NO_TOOLTIP
        zoomSpinBox->setSuffix(QApplication::translate("QtGradientEditor", "%", 0));
#ifndef QT_NO_TOOLTIP
        zoomInButton->setToolTip(QApplication::translate("QtGradientEditor", "Zoom In", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        zoomOutButton->setToolTip(QApplication::translate("QtGradientEditor", "Zoom Out", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        detailsButton->setToolTip(QApplication::translate("QtGradientEditor", "Toggle details extension", 0));
#endif // QT_NO_TOOLTIP
        detailsButton->setText(QApplication::translate("QtGradientEditor", ">", 0));
#ifndef QT_NO_TOOLTIP
        linearButton->setToolTip(QApplication::translate("QtGradientEditor", "Linear Type", 0));
#endif // QT_NO_TOOLTIP
        linearButton->setText(QApplication::translate("QtGradientEditor", "...", 0));
#ifndef QT_NO_TOOLTIP
        radialButton->setToolTip(QApplication::translate("QtGradientEditor", "Radial Type", 0));
#endif // QT_NO_TOOLTIP
        radialButton->setText(QApplication::translate("QtGradientEditor", "...", 0));
#ifndef QT_NO_TOOLTIP
        conicalButton->setToolTip(QApplication::translate("QtGradientEditor", "Conical Type", 0));
#endif // QT_NO_TOOLTIP
        conicalButton->setText(QApplication::translate("QtGradientEditor", "...", 0));
#ifndef QT_NO_TOOLTIP
        padButton->setToolTip(QApplication::translate("QtGradientEditor", "Pad Spread", 0));
#endif // QT_NO_TOOLTIP
        padButton->setText(QApplication::translate("QtGradientEditor", "...", 0));
#ifndef QT_NO_TOOLTIP
        repeatButton->setToolTip(QApplication::translate("QtGradientEditor", "Repeat Spread", 0));
#endif // QT_NO_TOOLTIP
        repeatButton->setText(QApplication::translate("QtGradientEditor", "...", 0));
#ifndef QT_NO_TOOLTIP
        reflectButton->setToolTip(QApplication::translate("QtGradientEditor", "Reflect Spread", 0));
#endif // QT_NO_TOOLTIP
        reflectButton->setText(QApplication::translate("QtGradientEditor", "...", 0));
    } // retranslateUi

};

namespace Ui {
    class QtGradientEditor: public Ui_QtGradientEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTGRADIENTEDITOR_H
