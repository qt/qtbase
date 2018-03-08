/********************************************************************************
** Form generated from reading UI file 'imagedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
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

    void setupUi(QDialog *dialog)
    {
        if (dialog->objectName().isEmpty())
            dialog->setObjectName(QStringLiteral("dialog"));
        dialog->setObjectName(QStringLiteral("ImageDialog"));
        dialog->resize(320, 180);
        vboxLayout = new QVBoxLayout(dialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setObjectName(QStringLiteral(""));
        gridLayout = new QGridLayout();
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(1, 1, 1, 1);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setObjectName(QStringLiteral(""));
        widthLabel = new QLabel(dialog);
        widthLabel->setObjectName(QStringLiteral("widthLabel"));
        widthLabel->setGeometry(QRect(1, 27, 67, 22));
        widthLabel->setFrameShape(QFrame::NoFrame);
        widthLabel->setFrameShadow(QFrame::Plain);
        widthLabel->setTextFormat(Qt::AutoText);

        gridLayout->addWidget(widthLabel, 1, 0, 1, 1);

        heightLabel = new QLabel(dialog);
        heightLabel->setObjectName(QStringLiteral("heightLabel"));
        heightLabel->setGeometry(QRect(1, 55, 67, 22));
        heightLabel->setFrameShape(QFrame::NoFrame);
        heightLabel->setFrameShadow(QFrame::Plain);
        heightLabel->setTextFormat(Qt::AutoText);

        gridLayout->addWidget(heightLabel, 2, 0, 1, 1);

        colorDepthCombo = new QComboBox(dialog);
        colorDepthCombo->setObjectName(QStringLiteral("colorDepthCombo"));
        colorDepthCombo->setGeometry(QRect(74, 83, 227, 22));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(colorDepthCombo->sizePolicy().hasHeightForWidth());
        colorDepthCombo->setSizePolicy(sizePolicy);
        colorDepthCombo->setInsertPolicy(QComboBox::InsertAtBottom);

        gridLayout->addWidget(colorDepthCombo, 3, 1, 1, 1);

        nameLineEdit = new QLineEdit(dialog);
        nameLineEdit->setObjectName(QStringLiteral("nameLineEdit"));
        nameLineEdit->setGeometry(QRect(74, 83, 227, 22));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(nameLineEdit->sizePolicy().hasHeightForWidth());
        nameLineEdit->setSizePolicy(sizePolicy1);
        nameLineEdit->setEchoMode(QLineEdit::Normal);

        gridLayout->addWidget(nameLineEdit, 0, 1, 1, 1);

        spinBox = new QSpinBox(dialog);
        spinBox->setObjectName(QStringLiteral("spinBox"));
        spinBox->setGeometry(QRect(74, 1, 227, 20));
        sizePolicy.setHeightForWidth(spinBox->sizePolicy().hasHeightForWidth());
        spinBox->setSizePolicy(sizePolicy);
        spinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        spinBox->setValue(32);
        spinBox->setMaximum(1024);
        spinBox->setMinimum(1);

        gridLayout->addWidget(spinBox, 1, 1, 1, 1);

        spinBox_2 = new QSpinBox(dialog);
        spinBox_2->setObjectName(QStringLiteral("spinBox_2"));
        spinBox_2->setGeometry(QRect(74, 27, 227, 22));
        sizePolicy.setHeightForWidth(spinBox_2->sizePolicy().hasHeightForWidth());
        spinBox_2->setSizePolicy(sizePolicy);
        spinBox_2->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        spinBox_2->setValue(32);
        spinBox_2->setMaximum(1024);
        spinBox_2->setMinimum(1);

        gridLayout->addWidget(spinBox_2, 2, 1, 1, 1);

        nameLabel = new QLabel(dialog);
        nameLabel->setObjectName(QStringLiteral("nameLabel"));
        nameLabel->setGeometry(QRect(1, 1, 67, 20));
        nameLabel->setFrameShape(QFrame::NoFrame);
        nameLabel->setFrameShadow(QFrame::Plain);
        nameLabel->setTextFormat(Qt::AutoText);

        gridLayout->addWidget(nameLabel, 0, 0, 1, 1);

        colorDepthLabel = new QLabel(dialog);
        colorDepthLabel->setObjectName(QStringLiteral("colorDepthLabel"));
        colorDepthLabel->setGeometry(QRect(1, 83, 67, 22));
        colorDepthLabel->setFrameShape(QFrame::NoFrame);
        colorDepthLabel->setFrameShadow(QFrame::Plain);
        colorDepthLabel->setTextFormat(Qt::AutoText);

        gridLayout->addWidget(colorDepthLabel, 3, 0, 1, 1);


        vboxLayout->addLayout(gridLayout);

        spacerItem = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setObjectName(QStringLiteral(""));
        spacerItem1 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);

        okButton = new QPushButton(dialog);
        okButton->setObjectName(QStringLiteral("okButton"));
        okButton->setGeometry(QRect(135, 1, 80, 24));

        hboxLayout->addWidget(okButton);

        cancelButton = new QPushButton(dialog);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));
        cancelButton->setGeometry(QRect(221, 1, 80, 24));

        hboxLayout->addWidget(cancelButton);


        vboxLayout->addLayout(hboxLayout);

        QWidget::setTabOrder(nameLineEdit, spinBox);
        QWidget::setTabOrder(spinBox, spinBox_2);
        QWidget::setTabOrder(spinBox_2, colorDepthCombo);
        QWidget::setTabOrder(colorDepthCombo, okButton);
        QWidget::setTabOrder(okButton, cancelButton);

        retranslateUi(dialog);
        QObject::connect(nameLineEdit, SIGNAL(returnPressed()), okButton, SLOT(animateClick()));

        QMetaObject::connectSlotsByName(dialog);
    } // setupUi

    void retranslateUi(QDialog *dialog)
    {
        dialog->setWindowTitle(QApplication::translate("ImageDialog", "Create Image", nullptr));
        widthLabel->setText(QApplication::translate("ImageDialog", "Width:", nullptr));
        heightLabel->setText(QApplication::translate("ImageDialog", "Height:", nullptr));
        nameLineEdit->setText(QApplication::translate("ImageDialog", "Untitled image", nullptr));
        nameLabel->setText(QApplication::translate("ImageDialog", "Name:", nullptr));
        colorDepthLabel->setText(QApplication::translate("ImageDialog", "Color depth:", nullptr));
        okButton->setText(QApplication::translate("ImageDialog", "OK", nullptr));
        cancelButton->setText(QApplication::translate("ImageDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImageDialog: public Ui_ImageDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // IMAGEDIALOG_H
