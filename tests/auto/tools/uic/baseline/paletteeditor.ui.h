/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'paletteeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
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
            qdesigner_internal__PaletteEditor->setObjectName("qdesigner_internal__PaletteEditor");
        qdesigner_internal__PaletteEditor->resize(365, 409);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
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
        vboxLayout->setObjectName("vboxLayout");
        advancedBox = new QGroupBox(qdesigner_internal__PaletteEditor);
        advancedBox->setObjectName("advancedBox");
        advancedBox->setMinimumSize(QSize(0, 0));
        advancedBox->setMaximumSize(QSize(16777215, 16777215));
        gridLayout = new QGridLayout(advancedBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName("gridLayout");
        buildButton = new QtColorButton(advancedBox);
        buildButton->setObjectName("buildButton");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Ignored);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(buildButton->sizePolicy().hasHeightForWidth());
        buildButton->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(buildButton, 0, 1, 1, 1);

        paletteView = new QTreeView(advancedBox);
        paletteView->setObjectName("paletteView");
        paletteView->setMinimumSize(QSize(0, 200));

        gridLayout->addWidget(paletteView, 1, 0, 1, 4);

        detailsRadio = new QRadioButton(advancedBox);
        detailsRadio->setObjectName("detailsRadio");

        gridLayout->addWidget(detailsRadio, 0, 3, 1, 1);

        computeRadio = new QRadioButton(advancedBox);
        computeRadio->setObjectName("computeRadio");
        computeRadio->setChecked(true);

        gridLayout->addWidget(computeRadio, 0, 2, 1, 1);

        label = new QLabel(advancedBox);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);


        vboxLayout->addWidget(advancedBox);

        GroupBox126 = new QGroupBox(qdesigner_internal__PaletteEditor);
        GroupBox126->setObjectName("GroupBox126");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(GroupBox126->sizePolicy().hasHeightForWidth());
        GroupBox126->setSizePolicy(sizePolicy2);
        gridLayout1 = new QGridLayout(GroupBox126);
#ifndef Q_OS_MAC
        gridLayout1->setSpacing(6);
#endif
        gridLayout1->setContentsMargins(8, 8, 8, 8);
        gridLayout1->setObjectName("gridLayout1");
        disabledRadio = new QRadioButton(GroupBox126);
        disabledRadio->setObjectName("disabledRadio");

        gridLayout1->addWidget(disabledRadio, 0, 2, 1, 1);

        inactiveRadio = new QRadioButton(GroupBox126);
        inactiveRadio->setObjectName("inactiveRadio");

        gridLayout1->addWidget(inactiveRadio, 0, 1, 1, 1);

        activeRadio = new QRadioButton(GroupBox126);
        activeRadio->setObjectName("activeRadio");
        activeRadio->setChecked(true);

        gridLayout1->addWidget(activeRadio, 0, 0, 1, 1);

        previewFrame = new qdesigner_internal::PreviewFrame(GroupBox126);
        previewFrame->setObjectName("previewFrame");
        sizePolicy.setHeightForWidth(previewFrame->sizePolicy().hasHeightForWidth());
        previewFrame->setSizePolicy(sizePolicy);

        gridLayout1->addWidget(previewFrame, 1, 0, 1, 3);


        vboxLayout->addWidget(GroupBox126);

        buttonBox = new QDialogButtonBox(qdesigner_internal__PaletteEditor);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(qdesigner_internal__PaletteEditor);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, qdesigner_internal__PaletteEditor, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, qdesigner_internal__PaletteEditor, qOverload<>(&QDialog::reject));

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
