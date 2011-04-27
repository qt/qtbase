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
** Form generated from reading UI file 'paletteeditoradvancedbase.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PALETTEEDITORADVANCEDBASE_H
#define PALETTEEDITORADVANCEDBASE_H

#include <Qt3Support/Q3ButtonGroup>
#include <Qt3Support/Q3GroupBox>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "colorbutton.h"

QT_BEGIN_NAMESPACE

class Ui_PaletteEditorAdvancedBase
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QLabel *TextLabel1;
    QComboBox *paletteCombo;
    Q3ButtonGroup *ButtonGroup1;
    QVBoxLayout *vboxLayout1;
    QCheckBox *checkBuildInactive;
    QCheckBox *checkBuildDisabled;
    Q3GroupBox *groupCentral;
    QVBoxLayout *vboxLayout2;
    QComboBox *comboCentral;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *Horizontal_Spacing1;
    QLabel *labelCentral;
    ColorButton *buttonCentral;
    Q3GroupBox *groupEffect;
    QVBoxLayout *vboxLayout3;
    QHBoxLayout *hboxLayout2;
    QCheckBox *checkBuildEffect;
    QComboBox *comboEffect;
    QHBoxLayout *hboxLayout3;
    QSpacerItem *Horizontal_Spacing3;
    QLabel *labelEffect;
    ColorButton *buttonEffect;
    QHBoxLayout *hboxLayout4;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *PaletteEditorAdvancedBase)
    {
        if (PaletteEditorAdvancedBase->objectName().isEmpty())
            PaletteEditorAdvancedBase->setObjectName(QString::fromUtf8("PaletteEditorAdvancedBase"));
        PaletteEditorAdvancedBase->setEnabled(true);
        PaletteEditorAdvancedBase->resize(295, 346);
        PaletteEditorAdvancedBase->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(PaletteEditorAdvancedBase);
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
        TextLabel1 = new QLabel(PaletteEditorAdvancedBase);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));

        hboxLayout->addWidget(TextLabel1);

        paletteCombo = new QComboBox(PaletteEditorAdvancedBase);
        paletteCombo->setObjectName(QString::fromUtf8("paletteCombo"));

        hboxLayout->addWidget(paletteCombo);


        vboxLayout->addLayout(hboxLayout);

        ButtonGroup1 = new Q3ButtonGroup(PaletteEditorAdvancedBase);
        ButtonGroup1->setObjectName(QString::fromUtf8("ButtonGroup1"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(4));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ButtonGroup1->sizePolicy().hasHeightForWidth());
        ButtonGroup1->setSizePolicy(sizePolicy);
        ButtonGroup1->setColumnLayout(0, Qt::Vertical);
#ifndef Q_OS_MAC
        ButtonGroup1->layout()->setSpacing(6);
#endif
        ButtonGroup1->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout1 = new QVBoxLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(ButtonGroup1->layout());
        if (boxlayout)
            boxlayout->addLayout(vboxLayout1);
        vboxLayout1->setAlignment(Qt::AlignTop);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        vboxLayout1->setObjectName(QString::fromUtf8("unnamed"));
        checkBuildInactive = new QCheckBox(ButtonGroup1);
        checkBuildInactive->setObjectName(QString::fromUtf8("checkBuildInactive"));
        checkBuildInactive->setChecked(true);

        vboxLayout1->addWidget(checkBuildInactive);

        checkBuildDisabled = new QCheckBox(ButtonGroup1);
        checkBuildDisabled->setObjectName(QString::fromUtf8("checkBuildDisabled"));
        checkBuildDisabled->setChecked(true);

        vboxLayout1->addWidget(checkBuildDisabled);


        vboxLayout->addWidget(ButtonGroup1);

        groupCentral = new Q3GroupBox(PaletteEditorAdvancedBase);
        groupCentral->setObjectName(QString::fromUtf8("groupCentral"));
        groupCentral->setColumnLayout(0, Qt::Vertical);
#ifndef Q_OS_MAC
        groupCentral->layout()->setSpacing(6);
#endif
        groupCentral->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout2 = new QVBoxLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(groupCentral->layout());
        if (boxlayout1)
            boxlayout1->addLayout(vboxLayout2);
        vboxLayout2->setAlignment(Qt::AlignTop);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        vboxLayout2->setObjectName(QString::fromUtf8("unnamed"));
        comboCentral = new QComboBox(groupCentral);
        comboCentral->setObjectName(QString::fromUtf8("comboCentral"));

        vboxLayout2->addWidget(comboCentral);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        hboxLayout1->setObjectName(QString::fromUtf8("unnamed"));
        Horizontal_Spacing1 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(Horizontal_Spacing1);

        labelCentral = new QLabel(groupCentral);
        labelCentral->setObjectName(QString::fromUtf8("labelCentral"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(labelCentral->sizePolicy().hasHeightForWidth());
        labelCentral->setSizePolicy(sizePolicy1);
        labelCentral->setMinimumSize(QSize(0, 0));

        hboxLayout1->addWidget(labelCentral);

        buttonCentral = new ColorButton(groupCentral);
        buttonCentral->setObjectName(QString::fromUtf8("buttonCentral"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(buttonCentral->sizePolicy().hasHeightForWidth());
        buttonCentral->setSizePolicy(sizePolicy2);
        buttonCentral->setFocusPolicy(Qt::TabFocus);

        hboxLayout1->addWidget(buttonCentral);


        vboxLayout2->addLayout(hboxLayout1);


        vboxLayout->addWidget(groupCentral);

        groupEffect = new Q3GroupBox(PaletteEditorAdvancedBase);
        groupEffect->setObjectName(QString::fromUtf8("groupEffect"));
        groupEffect->setColumnLayout(0, Qt::Vertical);
#ifndef Q_OS_MAC
        groupEffect->layout()->setSpacing(6);
#endif
        groupEffect->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout3 = new QVBoxLayout();
        QBoxLayout *boxlayout2 = qobject_cast<QBoxLayout *>(groupEffect->layout());
        if (boxlayout2)
            boxlayout2->addLayout(vboxLayout3);
        vboxLayout3->setAlignment(Qt::AlignTop);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        vboxLayout3->setObjectName(QString::fromUtf8("unnamed"));
        hboxLayout2 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        hboxLayout2->setObjectName(QString::fromUtf8("unnamed"));
        checkBuildEffect = new QCheckBox(groupEffect);
        checkBuildEffect->setObjectName(QString::fromUtf8("checkBuildEffect"));
        checkBuildEffect->setChecked(true);

        hboxLayout2->addWidget(checkBuildEffect);

        comboEffect = new QComboBox(groupEffect);
        comboEffect->setObjectName(QString::fromUtf8("comboEffect"));

        hboxLayout2->addWidget(comboEffect);


        vboxLayout3->addLayout(hboxLayout2);

        hboxLayout3 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout3->setSpacing(6);
#endif
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        hboxLayout3->setObjectName(QString::fromUtf8("unnamed"));
        Horizontal_Spacing3 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(Horizontal_Spacing3);

        labelEffect = new QLabel(groupEffect);
        labelEffect->setObjectName(QString::fromUtf8("labelEffect"));
        sizePolicy1.setHeightForWidth(labelEffect->sizePolicy().hasHeightForWidth());
        labelEffect->setSizePolicy(sizePolicy1);
        labelEffect->setMinimumSize(QSize(0, 0));

        hboxLayout3->addWidget(labelEffect);

        buttonEffect = new ColorButton(groupEffect);
        buttonEffect->setObjectName(QString::fromUtf8("buttonEffect"));
        sizePolicy2.setHeightForWidth(buttonEffect->sizePolicy().hasHeightForWidth());
        buttonEffect->setSizePolicy(sizePolicy2);
        buttonEffect->setFocusPolicy(Qt::TabFocus);

        hboxLayout3->addWidget(buttonEffect);


        vboxLayout3->addLayout(hboxLayout3);


        vboxLayout->addWidget(groupEffect);

        hboxLayout4 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout4->setSpacing(6);
#endif
        hboxLayout4->setContentsMargins(0, 0, 0, 0);
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        hboxLayout4->setObjectName(QString::fromUtf8("unnamed"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(PaletteEditorAdvancedBase);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout4->addWidget(buttonOk);

        buttonCancel = new QPushButton(PaletteEditorAdvancedBase);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout4->addWidget(buttonCancel);


        vboxLayout->addLayout(hboxLayout4);

#ifndef QT_NO_SHORTCUT
        TextLabel1->setBuddy(paletteCombo);
        labelCentral->setBuddy(buttonCentral);
        labelEffect->setBuddy(buttonEffect);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(buttonOk, buttonCancel);
        QWidget::setTabOrder(buttonCancel, paletteCombo);
        QWidget::setTabOrder(paletteCombo, checkBuildInactive);
        QWidget::setTabOrder(checkBuildInactive, checkBuildDisabled);
        QWidget::setTabOrder(checkBuildDisabled, comboCentral);
        QWidget::setTabOrder(comboCentral, buttonCentral);
        QWidget::setTabOrder(buttonCentral, checkBuildEffect);
        QWidget::setTabOrder(checkBuildEffect, comboEffect);
        QWidget::setTabOrder(comboEffect, buttonEffect);

        retranslateUi(PaletteEditorAdvancedBase);

        QMetaObject::connectSlotsByName(PaletteEditorAdvancedBase);
    } // setupUi

    void retranslateUi(QDialog *PaletteEditorAdvancedBase)
    {
        PaletteEditorAdvancedBase->setWindowTitle(QApplication::translate("PaletteEditorAdvancedBase", "Tune Palette", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        PaletteEditorAdvancedBase->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "<b>Edit Palette</b><p>Change the palette of the current widget or form.</p><p>Use a generated palette or select colors for each color group and each color role.</p><p>The palette can be tested with different widget layouts in the preview section.</p>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        TextLabel1->setText(QApplication::translate("PaletteEditorAdvancedBase", "Select &Palette:", 0, QApplication::UnicodeUTF8));
        paletteCombo->clear();
        paletteCombo->insertItems(0, QStringList()
         << QApplication::translate("PaletteEditorAdvancedBase", "Active Palette", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Inactive Palette", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Disabled Palette", 0, QApplication::UnicodeUTF8)
        );
        ButtonGroup1->setTitle(QApplication::translate("PaletteEditorAdvancedBase", "Auto", 0, QApplication::UnicodeUTF8));
        checkBuildInactive->setText(QApplication::translate("PaletteEditorAdvancedBase", "Build inactive palette from active", 0, QApplication::UnicodeUTF8));
        checkBuildDisabled->setText(QApplication::translate("PaletteEditorAdvancedBase", "Build disabled palette from active", 0, QApplication::UnicodeUTF8));
        groupCentral->setTitle(QApplication::translate("PaletteEditorAdvancedBase", "Central color &roles", 0, QApplication::UnicodeUTF8));
        comboCentral->clear();
        comboCentral->insertItems(0, QStringList()
         << QApplication::translate("PaletteEditorAdvancedBase", "Window", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "WindowText", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Button", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Base", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Text", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "BrightText", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "ButtonText", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Highlight", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "HighlightedText", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        comboCentral->setProperty("toolTip", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Choose central color role", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        comboCentral->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "<b>Select a color role.</b><p>Available central roles are: <ul> <li>Window - general background color.</li> <li>WindowText - general foreground color. </li> <li>Base - used as background color for e.g. text entry widgets, usually white or another light color. </li> <li>Text - the foreground color used with Base. Usually this is the same as WindowText, in what case it must provide good contrast both with Window and Base. </li> <li>Button - general button background color, where buttons need a background different from Window, as in the Macintosh style. </li> <li>ButtonText - a foreground color used with the Button color. </li> <li>Highlight - a color to indicate a selected or highlighted item. </li> <li>HighlightedText - a text color that contrasts to Highlight. </li> <li>BrightText - a text color that is very different from WindowText and contrasts well with e.g. black. </li> </ul> </p>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        labelCentral->setText(QApplication::translate("PaletteEditorAdvancedBase", "&Select Color:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        buttonCentral->setProperty("toolTip", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Choose a color", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        buttonCentral->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Choose a color for the selected central color role.", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        groupEffect->setTitle(QApplication::translate("PaletteEditorAdvancedBase", "3-D shadow &effects", 0, QApplication::UnicodeUTF8));
        checkBuildEffect->setText(QApplication::translate("PaletteEditorAdvancedBase", "Build &from button color", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        checkBuildEffect->setProperty("toolTip", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Generate shadings", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        checkBuildEffect->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Check to let 3D-effect colors be calculated from button-color.", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        comboEffect->clear();
        comboEffect->insertItems(0, QStringList()
         << QApplication::translate("PaletteEditorAdvancedBase", "Light", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Midlight", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Mid", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Dark", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PaletteEditorAdvancedBase", "Shadow", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        comboEffect->setProperty("toolTip", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Choose 3D-effect color role", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        comboEffect->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "<b>Select a color role.</b><p>Available effect roles are: <ul> <li>Light - lighter than Button color. </li> <li>Midlight - between Button and Light. </li> <li>Mid - between Button and Dark. </li> <li>Dark - darker than Button. </li> <li>Shadow - a very dark color. </li> </ul>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        labelEffect->setText(QApplication::translate("PaletteEditorAdvancedBase", "Select Co&lor:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        buttonEffect->setProperty("toolTip", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Choose a color", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        buttonEffect->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Choose a color for the selected effect color role.", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        buttonOk->setText(QApplication::translate("PaletteEditorAdvancedBase", "OK", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        buttonOk->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Close dialog and apply all changes.", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        buttonCancel->setText(QApplication::translate("PaletteEditorAdvancedBase", "Cancel", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        buttonCancel->setProperty("whatsThis", QVariant(QApplication::translate("PaletteEditorAdvancedBase", "Close dialog and discard all changes.", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
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
    class PaletteEditorAdvancedBase: public Ui_PaletteEditorAdvancedBase {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PALETTEEDITORADVANCEDBASE_H
