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
** Form generated from reading UI file 'paletteeditor.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include "previewframe.h"
#include "qtcolorbutton.h"

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_PaletteEditor
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *advancedBox;
    QGridLayout *gridLayout;
    QtColorButton *buildButton;
    QTreeView *paletteView;
    QRadioButton *detailsRadio;
    QRadioButton *computeRadio;
    QLabel *label;
    QGroupBox *GroupBox126;
    QGridLayout *gridLayout1;
    QRadioButton *disabledRadio;
    QRadioButton *inactiveRadio;
    QRadioButton *activeRadio;
    qdesigner_internal::PreviewFrame *previewFrame;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__PaletteEditor)
    {
        if (qdesigner_internal__PaletteEditor->objectName().isEmpty())
            qdesigner_internal__PaletteEditor->setObjectName(QString::fromUtf8("qdesigner_internal__PaletteEditor"));
        qdesigner_internal__PaletteEditor->resize(365, 409);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qdesigner_internal__PaletteEditor->sizePolicy().hasHeightForWidth());
        qdesigner_internal__PaletteEditor->setSizePolicy(sizePolicy);
        vboxLayout = new QVBoxLayout(qdesigner_internal__PaletteEditor);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        advancedBox = new QGroupBox(qdesigner_internal__PaletteEditor);
        advancedBox->setObjectName(QString::fromUtf8("advancedBox"));
        advancedBox->setMinimumSize(QSize(0, 0));
        advancedBox->setMaximumSize(QSize(16777215, 16777215));
        gridLayout = new QGridLayout(advancedBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        buildButton = new QtColorButton(advancedBox);
        buildButton->setObjectName(QString::fromUtf8("buildButton"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Ignored);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(buildButton->sizePolicy().hasHeightForWidth());
        buildButton->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(buildButton, 0, 1, 1, 1);

        paletteView = new QTreeView(advancedBox);
        paletteView->setObjectName(QString::fromUtf8("paletteView"));
        paletteView->setMinimumSize(QSize(0, 200));

        gridLayout->addWidget(paletteView, 1, 0, 1, 4);

        detailsRadio = new QRadioButton(advancedBox);
        detailsRadio->setObjectName(QString::fromUtf8("detailsRadio"));

        gridLayout->addWidget(detailsRadio, 0, 3, 1, 1);

        computeRadio = new QRadioButton(advancedBox);
        computeRadio->setObjectName(QString::fromUtf8("computeRadio"));
        computeRadio->setChecked(true);

        gridLayout->addWidget(computeRadio, 0, 2, 1, 1);

        label = new QLabel(advancedBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);


        vboxLayout->addWidget(advancedBox);

        GroupBox126 = new QGroupBox(qdesigner_internal__PaletteEditor);
        GroupBox126->setObjectName(QString::fromUtf8("GroupBox126"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(GroupBox126->sizePolicy().hasHeightForWidth());
        GroupBox126->setSizePolicy(sizePolicy2);
        gridLayout1 = new QGridLayout(GroupBox126);
#ifndef Q_OS_MAC
        gridLayout1->setSpacing(6);
#endif
        gridLayout1->setContentsMargins(8, 8, 8, 8);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        disabledRadio = new QRadioButton(GroupBox126);
        disabledRadio->setObjectName(QString::fromUtf8("disabledRadio"));

        gridLayout1->addWidget(disabledRadio, 0, 2, 1, 1);

        inactiveRadio = new QRadioButton(GroupBox126);
        inactiveRadio->setObjectName(QString::fromUtf8("inactiveRadio"));

        gridLayout1->addWidget(inactiveRadio, 0, 1, 1, 1);

        activeRadio = new QRadioButton(GroupBox126);
        activeRadio->setObjectName(QString::fromUtf8("activeRadio"));
        activeRadio->setChecked(true);

        gridLayout1->addWidget(activeRadio, 0, 0, 1, 1);

        previewFrame = new qdesigner_internal::PreviewFrame(GroupBox126);
        previewFrame->setObjectName(QString::fromUtf8("previewFrame"));
        sizePolicy.setHeightForWidth(previewFrame->sizePolicy().hasHeightForWidth());
        previewFrame->setSizePolicy(sizePolicy);

        gridLayout1->addWidget(previewFrame, 1, 0, 1, 3);


        vboxLayout->addWidget(GroupBox126);

        buttonBox = new QDialogButtonBox(qdesigner_internal__PaletteEditor);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(qdesigner_internal__PaletteEditor);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__PaletteEditor, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__PaletteEditor, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__PaletteEditor);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__PaletteEditor)
    {
        qdesigner_internal__PaletteEditor->setWindowTitle(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Edit Palette", nullptr));
        advancedBox->setTitle(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Tune Palette", nullptr));
        buildButton->setText(QString());
        detailsRadio->setText(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Show Details", nullptr));
        computeRadio->setText(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Compute Details", nullptr));
        label->setText(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Quick", nullptr));
        GroupBox126->setTitle(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Preview", nullptr));
        disabledRadio->setText(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Disabled", nullptr));
        inactiveRadio->setText(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Inactive", nullptr));
        activeRadio->setText(QCoreApplication::translate("qdesigner_internal::PaletteEditor", "Active", nullptr));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class PaletteEditor: public Ui_PaletteEditor {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // PALETTEEDITOR_H
