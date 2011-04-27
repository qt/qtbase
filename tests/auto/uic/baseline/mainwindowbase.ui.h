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
** Form generated from reading UI file 'mainwindowbase.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAINWINDOWBASE_H
#define MAINWINDOWBASE_H

#include <Qt3Support/Q3Frame>
#include <Qt3Support/Q3ListBox>
#include <Qt3Support/Q3MainWindow>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "colorbutton.h"
#include "previewframe.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindowBase
{
public:
    QAction *fileSaveAction;
    QAction *fileExitAction;
    QAction *helpAboutAction;
    QAction *helpAboutQtAction;
    QWidget *widget;
    QHBoxLayout *hboxLayout;
    QTextEdit *helpview;
    QTabWidget *TabWidget3;
    QWidget *tab1;
    QVBoxLayout *vboxLayout;
    QGroupBox *GroupBox40;
    QHBoxLayout *hboxLayout1;
    QLabel *gstylebuddy;
    QComboBox *gstylecombo;
    QGroupBox *groupAutoPalette;
    QHBoxLayout *hboxLayout2;
    QLabel *labelMainColor;
    ColorButton *buttonMainColor;
    QLabel *labelMainColor2;
    ColorButton *buttonMainColor2;
    QSpacerItem *spacerItem;
    QPushButton *btnAdvanced;
    QGroupBox *GroupBox126;
    QGridLayout *gridLayout;
    QLabel *TextLabel1;
    QComboBox *paletteCombo;
    PreviewFrame *previewFrame;
    QWidget *tab2;
    QVBoxLayout *vboxLayout1;
    QGroupBox *GroupBox1;
    QGridLayout *gridLayout1;
    QComboBox *stylecombo;
    QComboBox *familycombo;
    QComboBox *psizecombo;
    QLabel *stylebuddy;
    QLabel *psizebuddy;
    QLabel *familybuddy;
    QLineEdit *samplelineedit;
    QGroupBox *GroupBox2;
    QVBoxLayout *vboxLayout2;
    QHBoxLayout *hboxLayout3;
    QLabel *famsubbuddy;
    QComboBox *familysubcombo;
    QFrame *Line1;
    QLabel *TextLabel5;
    Q3ListBox *sublistbox;
    QHBoxLayout *hboxLayout4;
    QPushButton *PushButton2;
    QPushButton *PushButton3;
    QPushButton *PushButton4;
    QFrame *Line2;
    QHBoxLayout *hboxLayout5;
    QLabel *choosebuddy;
    QComboBox *choosesubcombo;
    QPushButton *PushButton1;
    QWidget *tab;
    QVBoxLayout *vboxLayout3;
    QGroupBox *GroupBox4;
    QGridLayout *gridLayout2;
    QSpinBox *dcispin;
    QLabel *dcibuddy;
    QSpinBox *cfispin;
    QLabel *cfibuddy;
    QSpinBox *wslspin;
    QLabel *wslbuddy;
    QCheckBox *resolvelinks;
    QGroupBox *GroupBox3;
    QVBoxLayout *vboxLayout4;
    QCheckBox *effectcheckbox;
    Q3Frame *effectbase;
    QGridLayout *gridLayout3;
    QLabel *meffectbuddy;
    QLabel *ceffectbuddy;
    QLabel *teffectbuddy;
    QLabel *beffectbuddy;
    QComboBox *menueffect;
    QComboBox *comboeffect;
    QComboBox *tooltipeffect;
    QComboBox *toolboxeffect;
    QGroupBox *GroupBox5;
    QGridLayout *gridLayout4;
    QLabel *swbuddy;
    QLabel *shbuddy;
    QSpinBox *strutwidth;
    QSpinBox *strutheight;
    QCheckBox *rtlExtensions;
    QLabel *inputStyleLabel;
    QComboBox *inputStyle;
    QSpacerItem *spacerItem1;
    QWidget *tab3;
    QVBoxLayout *vboxLayout5;
    QCheckBox *fontembeddingcheckbox;
    QGroupBox *GroupBox10;
    QVBoxLayout *vboxLayout6;
    QGridLayout *gridLayout5;
    QPushButton *PushButton11;
    QPushButton *PushButton13;
    QPushButton *PushButton12;
    Q3ListBox *fontpathlistbox;
    QGridLayout *gridLayout6;
    QSpacerItem *spacerItem2;
    QPushButton *PushButton15;
    QPushButton *PushButton14;
    QLabel *TextLabel15_2;
    QLineEdit *fontpathlineedit;
    QMenuBar *menubar;
    QAction *action;
    QAction *action1;
    QAction *action2;
    QMenu *PopupMenu;
    QAction *action3;
    QAction *action4;
    QAction *action5;
    QMenu *PopupMenu_2;

    void setupUi(Q3MainWindow *MainWindowBase)
    {
        if (MainWindowBase->objectName().isEmpty())
            MainWindowBase->setObjectName(QString::fromUtf8("MainWindowBase"));
        MainWindowBase->resize(724, 615);
        fileSaveAction = new QAction(MainWindowBase);
        fileSaveAction->setObjectName(QString::fromUtf8("fileSaveAction"));
        fileExitAction = new QAction(MainWindowBase);
        fileExitAction->setObjectName(QString::fromUtf8("fileExitAction"));
        helpAboutAction = new QAction(MainWindowBase);
        helpAboutAction->setObjectName(QString::fromUtf8("helpAboutAction"));
        helpAboutQtAction = new QAction(MainWindowBase);
        helpAboutQtAction->setObjectName(QString::fromUtf8("helpAboutQtAction"));
        widget = new QWidget(MainWindowBase);
        widget->setObjectName(QString::fromUtf8("widget"));
        widget->setGeometry(QRect(0, 28, 724, 587));
        hboxLayout = new QHBoxLayout(widget);
        hboxLayout->setSpacing(4);
        hboxLayout->setContentsMargins(8, 8, 8, 8);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        helpview = new QTextEdit(widget);
        helpview->setObjectName(QString::fromUtf8("helpview"));
        helpview->setMinimumSize(QSize(200, 0));
        helpview->setReadOnly(true);

        hboxLayout->addWidget(helpview);

        TabWidget3 = new QTabWidget(widget);
        TabWidget3->setObjectName(QString::fromUtf8("TabWidget3"));
        tab1 = new QWidget();
        tab1->setObjectName(QString::fromUtf8("tab1"));
        vboxLayout = new QVBoxLayout(tab1);
        vboxLayout->setSpacing(4);
        vboxLayout->setContentsMargins(4, 4, 4, 4);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        GroupBox40 = new QGroupBox(tab1);
        GroupBox40->setObjectName(QString::fromUtf8("GroupBox40"));
        hboxLayout1 = new QHBoxLayout(GroupBox40);
        hboxLayout1->setSpacing(4);
        hboxLayout1->setContentsMargins(8, 8, 8, 8);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        gstylebuddy = new QLabel(GroupBox40);
        gstylebuddy->setObjectName(QString::fromUtf8("gstylebuddy"));

        hboxLayout1->addWidget(gstylebuddy);

        gstylecombo = new QComboBox(GroupBox40);
        gstylecombo->setObjectName(QString::fromUtf8("gstylecombo"));

        hboxLayout1->addWidget(gstylecombo);


        vboxLayout->addWidget(GroupBox40);

        groupAutoPalette = new QGroupBox(tab1);
        groupAutoPalette->setObjectName(QString::fromUtf8("groupAutoPalette"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(4));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(groupAutoPalette->sizePolicy().hasHeightForWidth());
        groupAutoPalette->setSizePolicy(sizePolicy);
        hboxLayout2 = new QHBoxLayout(groupAutoPalette);
        hboxLayout2->setSpacing(4);
        hboxLayout2->setContentsMargins(8, 8, 8, 8);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        labelMainColor = new QLabel(groupAutoPalette);
        labelMainColor->setObjectName(QString::fromUtf8("labelMainColor"));

        hboxLayout2->addWidget(labelMainColor);

        buttonMainColor = new ColorButton(groupAutoPalette);
        buttonMainColor->setObjectName(QString::fromUtf8("buttonMainColor"));

        hboxLayout2->addWidget(buttonMainColor);

        labelMainColor2 = new QLabel(groupAutoPalette);
        labelMainColor2->setObjectName(QString::fromUtf8("labelMainColor2"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(labelMainColor2->sizePolicy().hasHeightForWidth());
        labelMainColor2->setSizePolicy(sizePolicy1);
        labelMainColor2->setMinimumSize(QSize(50, 0));
        labelMainColor2->setLineWidth(1);
        labelMainColor2->setMidLineWidth(0);
        labelMainColor2->setAlignment(Qt::AlignVCenter);
        labelMainColor2->setMargin(0);

        hboxLayout2->addWidget(labelMainColor2);

        buttonMainColor2 = new ColorButton(groupAutoPalette);
        buttonMainColor2->setObjectName(QString::fromUtf8("buttonMainColor2"));

        hboxLayout2->addWidget(buttonMainColor2);

        spacerItem = new QSpacerItem(70, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem);

        btnAdvanced = new QPushButton(groupAutoPalette);
        btnAdvanced->setObjectName(QString::fromUtf8("btnAdvanced"));

        hboxLayout2->addWidget(btnAdvanced);


        vboxLayout->addWidget(groupAutoPalette);

        GroupBox126 = new QGroupBox(tab1);
        GroupBox126->setObjectName(QString::fromUtf8("GroupBox126"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(7));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(GroupBox126->sizePolicy().hasHeightForWidth());
        GroupBox126->setSizePolicy(sizePolicy2);
        gridLayout = new QGridLayout(GroupBox126);
        gridLayout->setSpacing(4);
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        TextLabel1 = new QLabel(GroupBox126);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));

        gridLayout->addWidget(TextLabel1, 0, 0, 1, 1);

        paletteCombo = new QComboBox(GroupBox126);
        paletteCombo->setObjectName(QString::fromUtf8("paletteCombo"));

        gridLayout->addWidget(paletteCombo, 0, 1, 1, 1);

        previewFrame = new PreviewFrame(GroupBox126);
        previewFrame->setObjectName(QString::fromUtf8("previewFrame"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(7), static_cast<QSizePolicy::Policy>(7));
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(previewFrame->sizePolicy().hasHeightForWidth());
        previewFrame->setSizePolicy(sizePolicy3);
        previewFrame->setMinimumSize(QSize(410, 260));

        gridLayout->addWidget(previewFrame, 1, 0, 1, 2);


        vboxLayout->addWidget(GroupBox126);

        TabWidget3->addTab(tab1, QString());
        tab2 = new QWidget();
        tab2->setObjectName(QString::fromUtf8("tab2"));
        vboxLayout1 = new QVBoxLayout(tab2);
        vboxLayout1->setSpacing(4);
        vboxLayout1->setContentsMargins(8, 8, 8, 8);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        GroupBox1 = new QGroupBox(tab2);
        GroupBox1->setObjectName(QString::fromUtf8("GroupBox1"));
        gridLayout1 = new QGridLayout(GroupBox1);
        gridLayout1->setSpacing(4);
        gridLayout1->setContentsMargins(8, 8, 8, 8);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        stylecombo = new QComboBox(GroupBox1);
        stylecombo->setObjectName(QString::fromUtf8("stylecombo"));
        stylecombo->setAutoCompletion(true);
        stylecombo->setDuplicatesEnabled(false);

        gridLayout1->addWidget(stylecombo, 1, 1, 1, 1);

        familycombo = new QComboBox(GroupBox1);
        familycombo->setObjectName(QString::fromUtf8("familycombo"));
        familycombo->setAutoCompletion(true);
        familycombo->setDuplicatesEnabled(false);

        gridLayout1->addWidget(familycombo, 0, 1, 1, 1);

        psizecombo = new QComboBox(GroupBox1);
        psizecombo->setObjectName(QString::fromUtf8("psizecombo"));
        psizecombo->setEditable(true);
        psizecombo->setAutoCompletion(true);
        psizecombo->setDuplicatesEnabled(false);

        gridLayout1->addWidget(psizecombo, 2, 1, 1, 1);

        stylebuddy = new QLabel(GroupBox1);
        stylebuddy->setObjectName(QString::fromUtf8("stylebuddy"));

        gridLayout1->addWidget(stylebuddy, 1, 0, 1, 1);

        psizebuddy = new QLabel(GroupBox1);
        psizebuddy->setObjectName(QString::fromUtf8("psizebuddy"));

        gridLayout1->addWidget(psizebuddy, 2, 0, 1, 1);

        familybuddy = new QLabel(GroupBox1);
        familybuddy->setObjectName(QString::fromUtf8("familybuddy"));

        gridLayout1->addWidget(familybuddy, 0, 0, 1, 1);

        samplelineedit = new QLineEdit(GroupBox1);
        samplelineedit->setObjectName(QString::fromUtf8("samplelineedit"));
        samplelineedit->setAlignment(Qt::AlignHCenter);

        gridLayout1->addWidget(samplelineedit, 3, 0, 1, 2);


        vboxLayout1->addWidget(GroupBox1);

        GroupBox2 = new QGroupBox(tab2);
        GroupBox2->setObjectName(QString::fromUtf8("GroupBox2"));
        vboxLayout2 = new QVBoxLayout(GroupBox2);
        vboxLayout2->setSpacing(4);
        vboxLayout2->setContentsMargins(8, 8, 8, 8);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(4);
#ifndef Q_OS_MAC
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        famsubbuddy = new QLabel(GroupBox2);
        famsubbuddy->setObjectName(QString::fromUtf8("famsubbuddy"));

        hboxLayout3->addWidget(famsubbuddy);

        familysubcombo = new QComboBox(GroupBox2);
        familysubcombo->setObjectName(QString::fromUtf8("familysubcombo"));
        familysubcombo->setEditable(true);
        familysubcombo->setAutoCompletion(true);
        familysubcombo->setDuplicatesEnabled(false);

        hboxLayout3->addWidget(familysubcombo);


        vboxLayout2->addLayout(hboxLayout3);

        Line1 = new QFrame(GroupBox2);
        Line1->setObjectName(QString::fromUtf8("Line1"));
        Line1->setFrameShape(QFrame::HLine);
        Line1->setFrameShadow(QFrame::Sunken);
        Line1->setFrameShape(QFrame::HLine);

        vboxLayout2->addWidget(Line1);

        TextLabel5 = new QLabel(GroupBox2);
        TextLabel5->setObjectName(QString::fromUtf8("TextLabel5"));

        vboxLayout2->addWidget(TextLabel5);

        sublistbox = new Q3ListBox(GroupBox2);
        sublistbox->setObjectName(QString::fromUtf8("sublistbox"));

        vboxLayout2->addWidget(sublistbox);

        hboxLayout4 = new QHBoxLayout();
        hboxLayout4->setSpacing(4);
        hboxLayout4->setContentsMargins(0, 0, 0, 0);
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        PushButton2 = new QPushButton(GroupBox2);
        PushButton2->setObjectName(QString::fromUtf8("PushButton2"));

        hboxLayout4->addWidget(PushButton2);

        PushButton3 = new QPushButton(GroupBox2);
        PushButton3->setObjectName(QString::fromUtf8("PushButton3"));

        hboxLayout4->addWidget(PushButton3);

        PushButton4 = new QPushButton(GroupBox2);
        PushButton4->setObjectName(QString::fromUtf8("PushButton4"));

        hboxLayout4->addWidget(PushButton4);


        vboxLayout2->addLayout(hboxLayout4);

        Line2 = new QFrame(GroupBox2);
        Line2->setObjectName(QString::fromUtf8("Line2"));
        Line2->setFrameShape(QFrame::HLine);
        Line2->setFrameShadow(QFrame::Sunken);
        Line2->setFrameShape(QFrame::HLine);

        vboxLayout2->addWidget(Line2);

        hboxLayout5 = new QHBoxLayout();
        hboxLayout5->setSpacing(4);
        hboxLayout5->setContentsMargins(0, 0, 0, 0);
        hboxLayout5->setObjectName(QString::fromUtf8("hboxLayout5"));
        choosebuddy = new QLabel(GroupBox2);
        choosebuddy->setObjectName(QString::fromUtf8("choosebuddy"));

        hboxLayout5->addWidget(choosebuddy);

        choosesubcombo = new QComboBox(GroupBox2);
        choosesubcombo->setObjectName(QString::fromUtf8("choosesubcombo"));
        choosesubcombo->setAutoCompletion(true);
        choosesubcombo->setDuplicatesEnabled(false);

        hboxLayout5->addWidget(choosesubcombo);

        PushButton1 = new QPushButton(GroupBox2);
        PushButton1->setObjectName(QString::fromUtf8("PushButton1"));

        hboxLayout5->addWidget(PushButton1);


        vboxLayout2->addLayout(hboxLayout5);


        vboxLayout1->addWidget(GroupBox2);

        TabWidget3->addTab(tab2, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        vboxLayout3 = new QVBoxLayout(tab);
        vboxLayout3->setSpacing(4);
        vboxLayout3->setContentsMargins(7, 7, 7, 7);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        GroupBox4 = new QGroupBox(tab);
        GroupBox4->setObjectName(QString::fromUtf8("GroupBox4"));
        gridLayout2 = new QGridLayout(GroupBox4);
        gridLayout2->setSpacing(4);
        gridLayout2->setContentsMargins(8, 8, 8, 8);
        gridLayout2->setObjectName(QString::fromUtf8("gridLayout2"));
        dcispin = new QSpinBox(GroupBox4);
        dcispin->setObjectName(QString::fromUtf8("dcispin"));
        dcispin->setMaximum(10000);
        dcispin->setMinimum(10);

        gridLayout2->addWidget(dcispin, 0, 1, 1, 1);

        dcibuddy = new QLabel(GroupBox4);
        dcibuddy->setObjectName(QString::fromUtf8("dcibuddy"));

        gridLayout2->addWidget(dcibuddy, 0, 0, 1, 1);

        cfispin = new QSpinBox(GroupBox4);
        cfispin->setObjectName(QString::fromUtf8("cfispin"));
        cfispin->setMaximum(10000);
        cfispin->setMinimum(9);

        gridLayout2->addWidget(cfispin, 1, 1, 1, 1);

        cfibuddy = new QLabel(GroupBox4);
        cfibuddy->setObjectName(QString::fromUtf8("cfibuddy"));

        gridLayout2->addWidget(cfibuddy, 1, 0, 1, 1);

        wslspin = new QSpinBox(GroupBox4);
        wslspin->setObjectName(QString::fromUtf8("wslspin"));
        wslspin->setMaximum(20);
        wslspin->setMinimum(1);

        gridLayout2->addWidget(wslspin, 2, 1, 1, 1);

        wslbuddy = new QLabel(GroupBox4);
        wslbuddy->setObjectName(QString::fromUtf8("wslbuddy"));

        gridLayout2->addWidget(wslbuddy, 2, 0, 1, 1);

        resolvelinks = new QCheckBox(GroupBox4);
        resolvelinks->setObjectName(QString::fromUtf8("resolvelinks"));

        gridLayout2->addWidget(resolvelinks, 3, 0, 1, 2);


        vboxLayout3->addWidget(GroupBox4);

        GroupBox3 = new QGroupBox(tab);
        GroupBox3->setObjectName(QString::fromUtf8("GroupBox3"));
        vboxLayout4 = new QVBoxLayout(GroupBox3);
        vboxLayout4->setSpacing(4);
        vboxLayout4->setContentsMargins(8, 8, 8, 8);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        effectcheckbox = new QCheckBox(GroupBox3);
        effectcheckbox->setObjectName(QString::fromUtf8("effectcheckbox"));

        vboxLayout4->addWidget(effectcheckbox);

        effectbase = new Q3Frame(GroupBox3);
        effectbase->setObjectName(QString::fromUtf8("effectbase"));
        effectbase->setFrameShape(QFrame::NoFrame);
        effectbase->setFrameShadow(QFrame::Plain);
        gridLayout3 = new QGridLayout(effectbase);
        gridLayout3->setSpacing(4);
        gridLayout3->setContentsMargins(0, 0, 0, 0);
        gridLayout3->setObjectName(QString::fromUtf8("gridLayout3"));
        meffectbuddy = new QLabel(effectbase);
        meffectbuddy->setObjectName(QString::fromUtf8("meffectbuddy"));

        gridLayout3->addWidget(meffectbuddy, 0, 0, 1, 1);

        ceffectbuddy = new QLabel(effectbase);
        ceffectbuddy->setObjectName(QString::fromUtf8("ceffectbuddy"));

        gridLayout3->addWidget(ceffectbuddy, 1, 0, 1, 1);

        teffectbuddy = new QLabel(effectbase);
        teffectbuddy->setObjectName(QString::fromUtf8("teffectbuddy"));

        gridLayout3->addWidget(teffectbuddy, 2, 0, 1, 1);

        beffectbuddy = new QLabel(effectbase);
        beffectbuddy->setObjectName(QString::fromUtf8("beffectbuddy"));

        gridLayout3->addWidget(beffectbuddy, 3, 0, 1, 1);

        menueffect = new QComboBox(effectbase);
        menueffect->setObjectName(QString::fromUtf8("menueffect"));
        menueffect->setAutoCompletion(true);

        gridLayout3->addWidget(menueffect, 0, 1, 1, 1);

        comboeffect = new QComboBox(effectbase);
        comboeffect->setObjectName(QString::fromUtf8("comboeffect"));

        gridLayout3->addWidget(comboeffect, 1, 1, 1, 1);

        tooltipeffect = new QComboBox(effectbase);
        tooltipeffect->setObjectName(QString::fromUtf8("tooltipeffect"));

        gridLayout3->addWidget(tooltipeffect, 2, 1, 1, 1);

        toolboxeffect = new QComboBox(effectbase);
        toolboxeffect->setObjectName(QString::fromUtf8("toolboxeffect"));

        gridLayout3->addWidget(toolboxeffect, 3, 1, 1, 1);


        vboxLayout4->addWidget(effectbase);


        vboxLayout3->addWidget(GroupBox3);

        GroupBox5 = new QGroupBox(tab);
        GroupBox5->setObjectName(QString::fromUtf8("GroupBox5"));
        gridLayout4 = new QGridLayout(GroupBox5);
        gridLayout4->setSpacing(4);
        gridLayout4->setContentsMargins(8, 8, 8, 8);
        gridLayout4->setObjectName(QString::fromUtf8("gridLayout4"));
        swbuddy = new QLabel(GroupBox5);
        swbuddy->setObjectName(QString::fromUtf8("swbuddy"));

        gridLayout4->addWidget(swbuddy, 0, 0, 1, 1);

        shbuddy = new QLabel(GroupBox5);
        shbuddy->setObjectName(QString::fromUtf8("shbuddy"));

        gridLayout4->addWidget(shbuddy, 1, 0, 1, 1);

        strutwidth = new QSpinBox(GroupBox5);
        strutwidth->setObjectName(QString::fromUtf8("strutwidth"));
        strutwidth->setMaximum(1000);

        gridLayout4->addWidget(strutwidth, 0, 1, 1, 1);

        strutheight = new QSpinBox(GroupBox5);
        strutheight->setObjectName(QString::fromUtf8("strutheight"));
        strutheight->setMaximum(1000);

        gridLayout4->addWidget(strutheight, 1, 1, 1, 1);


        vboxLayout3->addWidget(GroupBox5);

        rtlExtensions = new QCheckBox(tab);
        rtlExtensions->setObjectName(QString::fromUtf8("rtlExtensions"));

        vboxLayout3->addWidget(rtlExtensions);

        inputStyleLabel = new QLabel(tab);
        inputStyleLabel->setObjectName(QString::fromUtf8("inputStyleLabel"));

        vboxLayout3->addWidget(inputStyleLabel);

        inputStyle = new QComboBox(tab);
        inputStyle->setObjectName(QString::fromUtf8("inputStyle"));

        vboxLayout3->addWidget(inputStyle);

        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout3->addItem(spacerItem1);

        TabWidget3->addTab(tab, QString());
        tab3 = new QWidget();
        tab3->setObjectName(QString::fromUtf8("tab3"));
        vboxLayout5 = new QVBoxLayout(tab3);
        vboxLayout5->setSpacing(4);
        vboxLayout5->setContentsMargins(8, 8, 8, 8);
        vboxLayout5->setObjectName(QString::fromUtf8("vboxLayout5"));
        fontembeddingcheckbox = new QCheckBox(tab3);
        fontembeddingcheckbox->setObjectName(QString::fromUtf8("fontembeddingcheckbox"));
        fontembeddingcheckbox->setChecked(true);

        vboxLayout5->addWidget(fontembeddingcheckbox);

        GroupBox10 = new QGroupBox(tab3);
        GroupBox10->setObjectName(QString::fromUtf8("GroupBox10"));
        sizePolicy2.setHeightForWidth(GroupBox10->sizePolicy().hasHeightForWidth());
        GroupBox10->setSizePolicy(sizePolicy2);
        vboxLayout6 = new QVBoxLayout(GroupBox10);
        vboxLayout6->setSpacing(4);
        vboxLayout6->setContentsMargins(8, 8, 8, 8);
        vboxLayout6->setObjectName(QString::fromUtf8("vboxLayout6"));
        gridLayout5 = new QGridLayout();
        gridLayout5->setSpacing(4);
#ifndef Q_OS_MAC
        gridLayout5->setContentsMargins(0, 0, 0, 0);
#endif
        gridLayout5->setObjectName(QString::fromUtf8("gridLayout5"));
        PushButton11 = new QPushButton(GroupBox10);
        PushButton11->setObjectName(QString::fromUtf8("PushButton11"));

        gridLayout5->addWidget(PushButton11, 1, 0, 1, 1);

        PushButton13 = new QPushButton(GroupBox10);
        PushButton13->setObjectName(QString::fromUtf8("PushButton13"));

        gridLayout5->addWidget(PushButton13, 1, 2, 1, 1);

        PushButton12 = new QPushButton(GroupBox10);
        PushButton12->setObjectName(QString::fromUtf8("PushButton12"));

        gridLayout5->addWidget(PushButton12, 1, 1, 1, 1);

        fontpathlistbox = new Q3ListBox(GroupBox10);
        fontpathlistbox->setObjectName(QString::fromUtf8("fontpathlistbox"));

        gridLayout5->addWidget(fontpathlistbox, 0, 0, 1, 3);


        vboxLayout6->addLayout(gridLayout5);

        gridLayout6 = new QGridLayout();
        gridLayout6->setSpacing(4);
        gridLayout6->setContentsMargins(0, 0, 0, 0);
        gridLayout6->setObjectName(QString::fromUtf8("gridLayout6"));
        spacerItem2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);

        gridLayout6->addItem(spacerItem2, 2, 0, 1, 1);

        PushButton15 = new QPushButton(GroupBox10);
        PushButton15->setObjectName(QString::fromUtf8("PushButton15"));

        gridLayout6->addWidget(PushButton15, 2, 2, 1, 1);

        PushButton14 = new QPushButton(GroupBox10);
        PushButton14->setObjectName(QString::fromUtf8("PushButton14"));

        gridLayout6->addWidget(PushButton14, 2, 1, 1, 1);

        TextLabel15_2 = new QLabel(GroupBox10);
        TextLabel15_2->setObjectName(QString::fromUtf8("TextLabel15_2"));

        gridLayout6->addWidget(TextLabel15_2, 0, 0, 1, 3);

        fontpathlineedit = new QLineEdit(GroupBox10);
        fontpathlineedit->setObjectName(QString::fromUtf8("fontpathlineedit"));

        gridLayout6->addWidget(fontpathlineedit, 1, 0, 1, 3);


        vboxLayout6->addLayout(gridLayout6);


        vboxLayout5->addWidget(GroupBox10);

        TabWidget3->addTab(tab3, QString());

        hboxLayout->addWidget(TabWidget3);

        MainWindowBase->setCentralWidget(widget);
        menubar = new QMenuBar(MainWindowBase);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 724, 27));
        action = new QAction(menubar);
        action->setObjectName(QString::fromUtf8("action"));
        action1 = new QAction(menubar);
        action1->setObjectName(QString::fromUtf8("action1"));
        action2 = new QAction(menubar);
        action2->setObjectName(QString::fromUtf8("action2"));
        PopupMenu = new QMenu(menubar);
        PopupMenu->setObjectName(QString::fromUtf8("PopupMenu"));
        PopupMenu->setGeometry(QRect(0, 0, 800, 480));
        action3 = new QAction(PopupMenu);
        action3->setObjectName(QString::fromUtf8("action3"));
        action4 = new QAction(PopupMenu);
        action4->setObjectName(QString::fromUtf8("action4"));
        action5 = new QAction(PopupMenu);
        action5->setObjectName(QString::fromUtf8("action5"));
        PopupMenu_2 = new QMenu(menubar);
        PopupMenu_2->setObjectName(QString::fromUtf8("PopupMenu_2"));
        PopupMenu_2->setGeometry(QRect(0, 0, 800, 480));
#ifndef QT_NO_SHORTCUT
        gstylebuddy->setBuddy(gstylecombo);
        labelMainColor->setBuddy(buttonMainColor);
        labelMainColor2->setBuddy(buttonMainColor2);
        TextLabel1->setBuddy(paletteCombo);
        stylebuddy->setBuddy(stylecombo);
        psizebuddy->setBuddy(psizecombo);
        familybuddy->setBuddy(familycombo);
        famsubbuddy->setBuddy(familysubcombo);
        choosebuddy->setBuddy(choosesubcombo);
        dcibuddy->setBuddy(dcispin);
        cfibuddy->setBuddy(cfispin);
        wslbuddy->setBuddy(wslspin);
        meffectbuddy->setBuddy(menueffect);
        ceffectbuddy->setBuddy(comboeffect);
        teffectbuddy->setBuddy(tooltipeffect);
        beffectbuddy->setBuddy(toolboxeffect);
        swbuddy->setBuddy(strutwidth);
        shbuddy->setBuddy(strutheight);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(helpview, TabWidget3);
        QWidget::setTabOrder(TabWidget3, familycombo);
        QWidget::setTabOrder(familycombo, stylecombo);
        QWidget::setTabOrder(stylecombo, psizecombo);
        QWidget::setTabOrder(psizecombo, samplelineedit);
        QWidget::setTabOrder(samplelineedit, familysubcombo);
        QWidget::setTabOrder(familysubcombo, PushButton2);
        QWidget::setTabOrder(PushButton2, PushButton3);
        QWidget::setTabOrder(PushButton3, PushButton4);
        QWidget::setTabOrder(PushButton4, choosesubcombo);
        QWidget::setTabOrder(choosesubcombo, PushButton1);
        QWidget::setTabOrder(PushButton1, dcispin);
        QWidget::setTabOrder(dcispin, cfispin);
        QWidget::setTabOrder(cfispin, wslspin);
        QWidget::setTabOrder(wslspin, effectcheckbox);
        QWidget::setTabOrder(effectcheckbox, menueffect);
        QWidget::setTabOrder(menueffect, comboeffect);
        QWidget::setTabOrder(comboeffect, tooltipeffect);
        QWidget::setTabOrder(tooltipeffect, strutwidth);
        QWidget::setTabOrder(strutwidth, strutheight);
        QWidget::setTabOrder(strutheight, sublistbox);

        menubar->addAction(PopupMenu->menuAction());
        menubar->addSeparator();
        menubar->addAction(PopupMenu_2->menuAction());
        PopupMenu->addAction(fileSaveAction);
        PopupMenu->addSeparator();
        PopupMenu->addAction(fileExitAction);
        PopupMenu_2->addAction(helpAboutAction);
        PopupMenu_2->addAction(helpAboutQtAction);

        retranslateUi(MainWindowBase);

        menueffect->setCurrentIndex(0);
        inputStyle->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindowBase);
    } // setupUi

    void retranslateUi(Q3MainWindow *MainWindowBase)
    {
        MainWindowBase->setWindowTitle(QApplication::translate("MainWindowBase", "Qt Configuration", 0, QApplication::UnicodeUTF8));
        fileSaveAction->setText(QApplication::translate("MainWindowBase", "&Save", 0, QApplication::UnicodeUTF8));
        fileSaveAction->setIconText(QApplication::translate("MainWindowBase", "Save", 0, QApplication::UnicodeUTF8));
        fileSaveAction->setShortcut(QApplication::translate("MainWindowBase", "Ctrl+S", 0, QApplication::UnicodeUTF8));
        fileExitAction->setText(QApplication::translate("MainWindowBase", "E&xit", 0, QApplication::UnicodeUTF8));
        fileExitAction->setIconText(QApplication::translate("MainWindowBase", "Exit", 0, QApplication::UnicodeUTF8));
        fileExitAction->setShortcut(QString());
        helpAboutAction->setText(QApplication::translate("MainWindowBase", "&About", 0, QApplication::UnicodeUTF8));
        helpAboutAction->setIconText(QApplication::translate("MainWindowBase", "About", 0, QApplication::UnicodeUTF8));
        helpAboutAction->setShortcut(QString());
        helpAboutQtAction->setText(QApplication::translate("MainWindowBase", "About &Qt", 0, QApplication::UnicodeUTF8));
        helpAboutQtAction->setIconText(QApplication::translate("MainWindowBase", "About Qt", 0, QApplication::UnicodeUTF8));
        GroupBox40->setTitle(QApplication::translate("MainWindowBase", "GUI Style", 0, QApplication::UnicodeUTF8));
        gstylebuddy->setText(QApplication::translate("MainWindowBase", "Select GUI &Style:", 0, QApplication::UnicodeUTF8));
        groupAutoPalette->setTitle(QApplication::translate("MainWindowBase", "Build Palette", 0, QApplication::UnicodeUTF8));
        labelMainColor->setText(QApplication::translate("MainWindowBase", "&3-D Effects:", 0, QApplication::UnicodeUTF8));
        labelMainColor2->setText(QApplication::translate("MainWindowBase", "Window Back&ground:", 0, QApplication::UnicodeUTF8));
        btnAdvanced->setText(QApplication::translate("MainWindowBase", "&Tune Palette...", 0, QApplication::UnicodeUTF8));
        GroupBox126->setTitle(QApplication::translate("MainWindowBase", "Preview", 0, QApplication::UnicodeUTF8));
        TextLabel1->setText(QApplication::translate("MainWindowBase", "Select &Palette:", 0, QApplication::UnicodeUTF8));
        paletteCombo->clear();
        paletteCombo->insertItems(0, QStringList()
         << QApplication::translate("MainWindowBase", "Active Palette", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Inactive Palette", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Disabled Palette", 0, QApplication::UnicodeUTF8)
        );
        TabWidget3->setTabText(TabWidget3->indexOf(tab1), QApplication::translate("MainWindowBase", "Appearance", 0, QApplication::UnicodeUTF8));
        GroupBox1->setTitle(QApplication::translate("MainWindowBase", "Default Font", 0, QApplication::UnicodeUTF8));
        stylebuddy->setText(QApplication::translate("MainWindowBase", "&Style:", 0, QApplication::UnicodeUTF8));
        psizebuddy->setText(QApplication::translate("MainWindowBase", "&Point Size:", 0, QApplication::UnicodeUTF8));
        familybuddy->setText(QApplication::translate("MainWindowBase", "F&amily:", 0, QApplication::UnicodeUTF8));
        samplelineedit->setText(QApplication::translate("MainWindowBase", "Sample Text", 0, QApplication::UnicodeUTF8));
        GroupBox2->setTitle(QApplication::translate("MainWindowBase", "Font Substitution", 0, QApplication::UnicodeUTF8));
        famsubbuddy->setText(QApplication::translate("MainWindowBase", "S&elect or Enter a Family:", 0, QApplication::UnicodeUTF8));
        TextLabel5->setText(QApplication::translate("MainWindowBase", "Current Substitutions:", 0, QApplication::UnicodeUTF8));
        PushButton2->setText(QApplication::translate("MainWindowBase", "Up", 0, QApplication::UnicodeUTF8));
        PushButton3->setText(QApplication::translate("MainWindowBase", "Down", 0, QApplication::UnicodeUTF8));
        PushButton4->setText(QApplication::translate("MainWindowBase", "Remove", 0, QApplication::UnicodeUTF8));
        choosebuddy->setText(QApplication::translate("MainWindowBase", "Select s&ubstitute Family:", 0, QApplication::UnicodeUTF8));
        PushButton1->setText(QApplication::translate("MainWindowBase", "Add", 0, QApplication::UnicodeUTF8));
        TabWidget3->setTabText(TabWidget3->indexOf(tab2), QApplication::translate("MainWindowBase", "Fonts", 0, QApplication::UnicodeUTF8));
        GroupBox4->setTitle(QApplication::translate("MainWindowBase", "Feel Settings", 0, QApplication::UnicodeUTF8));
        dcispin->setSuffix(QApplication::translate("MainWindowBase", " ms", 0, QApplication::UnicodeUTF8));
        dcibuddy->setText(QApplication::translate("MainWindowBase", "&Double Click Interval:", 0, QApplication::UnicodeUTF8));
        cfispin->setSpecialValueText(QApplication::translate("MainWindowBase", "No blinking", 0, QApplication::UnicodeUTF8));
        cfispin->setSuffix(QApplication::translate("MainWindowBase", " ms", 0, QApplication::UnicodeUTF8));
        cfibuddy->setText(QApplication::translate("MainWindowBase", "&Cursor Flash Time:", 0, QApplication::UnicodeUTF8));
        wslspin->setSuffix(QApplication::translate("MainWindowBase", " lines", 0, QApplication::UnicodeUTF8));
        wslbuddy->setText(QApplication::translate("MainWindowBase", "Wheel &Scroll Lines:", 0, QApplication::UnicodeUTF8));
        resolvelinks->setText(QApplication::translate("MainWindowBase", "Resolve symlinks in URLs", 0, QApplication::UnicodeUTF8));
        GroupBox3->setTitle(QApplication::translate("MainWindowBase", "GUI Effects", 0, QApplication::UnicodeUTF8));
        effectcheckbox->setText(QApplication::translate("MainWindowBase", "&Enable", 0, QApplication::UnicodeUTF8));
        effectcheckbox->setShortcut(QApplication::translate("MainWindowBase", "Alt+E", 0, QApplication::UnicodeUTF8));
        meffectbuddy->setText(QApplication::translate("MainWindowBase", "&Menu Effect:", 0, QApplication::UnicodeUTF8));
        ceffectbuddy->setText(QApplication::translate("MainWindowBase", "C&omboBox Effect:", 0, QApplication::UnicodeUTF8));
        teffectbuddy->setText(QApplication::translate("MainWindowBase", "&ToolTip Effect:", 0, QApplication::UnicodeUTF8));
        beffectbuddy->setText(QApplication::translate("MainWindowBase", "Tool&Box Effect:", 0, QApplication::UnicodeUTF8));
        menueffect->clear();
        menueffect->insertItems(0, QStringList()
         << QApplication::translate("MainWindowBase", "Disable", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Animate", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Fade", 0, QApplication::UnicodeUTF8)
        );
        comboeffect->clear();
        comboeffect->insertItems(0, QStringList()
         << QApplication::translate("MainWindowBase", "Disable", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Animate", 0, QApplication::UnicodeUTF8)
        );
        tooltipeffect->clear();
        tooltipeffect->insertItems(0, QStringList()
         << QApplication::translate("MainWindowBase", "Disable", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Animate", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Fade", 0, QApplication::UnicodeUTF8)
        );
        toolboxeffect->clear();
        toolboxeffect->insertItems(0, QStringList()
         << QApplication::translate("MainWindowBase", "Disable", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Animate", 0, QApplication::UnicodeUTF8)
        );
        GroupBox5->setTitle(QApplication::translate("MainWindowBase", "Global Strut", 0, QApplication::UnicodeUTF8));
        swbuddy->setText(QApplication::translate("MainWindowBase", "Minimum &Width:", 0, QApplication::UnicodeUTF8));
        shbuddy->setText(QApplication::translate("MainWindowBase", "Minimum Hei&ght:", 0, QApplication::UnicodeUTF8));
        strutwidth->setSuffix(QApplication::translate("MainWindowBase", " pixels", 0, QApplication::UnicodeUTF8));
        strutheight->setSuffix(QApplication::translate("MainWindowBase", " pixels", 0, QApplication::UnicodeUTF8));
        rtlExtensions->setText(QApplication::translate("MainWindowBase", "Enhanced support for languages written right-to-left", 0, QApplication::UnicodeUTF8));
        inputStyleLabel->setText(QApplication::translate("MainWindowBase", "XIM Input Style:", 0, QApplication::UnicodeUTF8));
        inputStyle->clear();
        inputStyle->insertItems(0, QStringList()
         << QApplication::translate("MainWindowBase", "On The Spot", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Over The Spot", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Off The Spot", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindowBase", "Root", 0, QApplication::UnicodeUTF8)
        );
        TabWidget3->setTabText(TabWidget3->indexOf(tab), QApplication::translate("MainWindowBase", "Interface", 0, QApplication::UnicodeUTF8));
        fontembeddingcheckbox->setText(QApplication::translate("MainWindowBase", "Enable Font embedding", 0, QApplication::UnicodeUTF8));
        GroupBox10->setTitle(QApplication::translate("MainWindowBase", "Font Paths", 0, QApplication::UnicodeUTF8));
        PushButton11->setText(QApplication::translate("MainWindowBase", "Up", 0, QApplication::UnicodeUTF8));
        PushButton13->setText(QApplication::translate("MainWindowBase", "Remove", 0, QApplication::UnicodeUTF8));
        PushButton12->setText(QApplication::translate("MainWindowBase", "Down", 0, QApplication::UnicodeUTF8));
        PushButton15->setText(QApplication::translate("MainWindowBase", "Add", 0, QApplication::UnicodeUTF8));
        PushButton14->setText(QApplication::translate("MainWindowBase", "Browse...", 0, QApplication::UnicodeUTF8));
        TextLabel15_2->setText(QApplication::translate("MainWindowBase", "Press the <b>Browse</b> button or enter a directory and press Enter to add them to the list.", 0, QApplication::UnicodeUTF8));
        TabWidget3->setTabText(TabWidget3->indexOf(tab3), QApplication::translate("MainWindowBase", "Printer", 0, QApplication::UnicodeUTF8));
        PopupMenu->setTitle(QApplication::translate("MainWindowBase", "&File", 0, QApplication::UnicodeUTF8));
        PopupMenu_2->setTitle(QApplication::translate("MainWindowBase", "&Help", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindowBase: public Ui_MainWindowBase {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAINWINDOWBASE_H
