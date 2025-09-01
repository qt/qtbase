/********************************************************************************
** Form generated from reading UI file 'imagedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef IMAGEDIALOG_H
#define IMAGEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ImageDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGridLayout *gridLayout;
    QLabel *widthLabel;
    QLabel *heightLabel;
    QComboBox *colorDepthCombo;
    QLineEdit *nameLineEdit;
    QSpinBox *spinBox;
    QSpinBox *spinBox_2;
    QLabel *nameLabel;
    QLabel *colorDepthLabel;
    QSpacerItem *spacerItem;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem1;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *ImageDialog)
    {
        if (ImageDialog->objectName().isEmpty())
            ImageDialog->setObjectName("ImageDialog");
        ImageDialog->resize(320, 204);
        vboxLayout = new QVBoxLayout(ImageDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName("vboxLayout");
        gridLayout = new QGridLayout();
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(1, 1, 1, 1);
        gridLayout->setObjectName("gridLayout");
        widthLabel = new QLabel(ImageDialog);
        widthLabel->setObjectName("widthLabel");
        widthLabel->setFrameShape(QFrame::Shape::NoFrame);
        widthLabel->setFrameShadow(QFrame::Shadow::Plain);
        widthLabel->setTextFormat(Qt::TextFormat::AutoText);

        gridLayout->addWidget(widthLabel, 1, 0, 1, 1);

        heightLabel = new QLabel(ImageDialog);
        heightLabel->setObjectName("heightLabel");
        heightLabel->setFrameShape(QFrame::Shape::NoFrame);
        heightLabel->setFrameShadow(QFrame::Shadow::Plain);
        heightLabel->setTextFormat(Qt::TextFormat::AutoText);

        gridLayout->addWidget(heightLabel, 2, 0, 1, 1);

        colorDepthCombo = new QComboBox(ImageDialog);
        colorDepthCombo->setObjectName("colorDepthCombo");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(colorDepthCombo->sizePolicy().hasHeightForWidth());
        colorDepthCombo->setSizePolicy(sizePolicy);
        colorDepthCombo->setInsertPolicy(QComboBox::InsertPolicy::InsertAtBottom);

        gridLayout->addWidget(colorDepthCombo, 3, 1, 1, 1);

        nameLineEdit = new QLineEdit(ImageDialog);
        nameLineEdit->setObjectName("nameLineEdit");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(nameLineEdit->sizePolicy().hasHeightForWidth());
        nameLineEdit->setSizePolicy(sizePolicy1);
        nameLineEdit->setEchoMode(QLineEdit::EchoMode::Normal);

        gridLayout->addWidget(nameLineEdit, 0, 1, 1, 1);

        spinBox = new QSpinBox(ImageDialog);
        spinBox->setObjectName("spinBox");
        sizePolicy.setHeightForWidth(spinBox->sizePolicy().hasHeightForWidth());
        spinBox->setSizePolicy(sizePolicy);
        spinBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::UpDownArrows);
        spinBox->setMinimum(1);
        spinBox->setMaximum(1024);
        spinBox->setValue(32);

        gridLayout->addWidget(spinBox, 1, 1, 1, 1);

        spinBox_2 = new QSpinBox(ImageDialog);
        spinBox_2->setObjectName("spinBox_2");
        sizePolicy.setHeightForWidth(spinBox_2->sizePolicy().hasHeightForWidth());
        spinBox_2->setSizePolicy(sizePolicy);
        spinBox_2->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::UpDownArrows);
        spinBox_2->setMinimum(1);
        spinBox_2->setMaximum(1024);
        spinBox_2->setValue(32);

        gridLayout->addWidget(spinBox_2, 2, 1, 1, 1);

        nameLabel = new QLabel(ImageDialog);
        nameLabel->setObjectName("nameLabel");
        nameLabel->setFrameShape(QFrame::Shape::NoFrame);
        nameLabel->setFrameShadow(QFrame::Shadow::Plain);
        nameLabel->setTextFormat(Qt::TextFormat::AutoText);

        gridLayout->addWidget(nameLabel, 0, 0, 1, 1);

        colorDepthLabel = new QLabel(ImageDialog);
        colorDepthLabel->setObjectName("colorDepthLabel");
        colorDepthLabel->setFrameShape(QFrame::Shape::NoFrame);
        colorDepthLabel->setFrameShadow(QFrame::Shadow::Plain);
        colorDepthLabel->setTextFormat(Qt::TextFormat::AutoText);

        gridLayout->addWidget(colorDepthLabel, 3, 0, 1, 1);


        vboxLayout->addLayout(gridLayout);

        spacerItem = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        vboxLayout->addItem(spacerItem);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName("hboxLayout");
        spacerItem1 = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout->addItem(spacerItem1);

        okButton = new QPushButton(ImageDialog);
        okButton->setObjectName("okButton");

        hboxLayout->addWidget(okButton);

        cancelButton = new QPushButton(ImageDialog);
        cancelButton->setObjectName("cancelButton");

        hboxLayout->addWidget(cancelButton);


        vboxLayout->addLayout(hboxLayout);

        QWidget::setTabOrder(nameLineEdit, spinBox);
        QWidget::setTabOrder(spinBox, spinBox_2);
        QWidget::setTabOrder(spinBox_2, colorDepthCombo);
        QWidget::setTabOrder(colorDepthCombo, okButton);
        QWidget::setTabOrder(okButton, cancelButton);

        retranslateUi(ImageDialog);
        QObject::connect(nameLineEdit, &QLineEdit::returnPressed, okButton, qOverload<>(&QPushButton::animateClick));

        QMetaObject::connectSlotsByName(ImageDialog);
    } // setupUi

    void retranslateUi(QDialog *ImageDialog)
    {
        ImageDialog->setWindowTitle(QCoreApplication::translate("ImageDialog", "Create Image", nullptr));
        widthLabel->setText(QCoreApplication::translate("ImageDialog", "Width:", nullptr));
        heightLabel->setText(QCoreApplication::translate("ImageDialog", "Height:", nullptr));
        nameLineEdit->setText(QCoreApplication::translate("ImageDialog", "Untitled image", nullptr));
        nameLabel->setText(QCoreApplication::translate("ImageDialog", "Name:", nullptr));
        colorDepthLabel->setText(QCoreApplication::translate("ImageDialog", "Color depth:", nullptr));
        okButton->setText(QCoreApplication::translate("ImageDialog", "OK", nullptr));
        cancelButton->setText(QCoreApplication::translate("ImageDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImageDialog: public Ui_ImageDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // IMAGEDIALOG_H
