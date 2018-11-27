/********************************************************************************
** Form generated from reading UI file 'embeddeddialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef EMBEDDEDDIALOG_H
#define EMBEDDEDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>

QT_BEGIN_NAMESPACE

class Ui_embeddedDialog
{
public:
    QFormLayout *formLayout;
    QLabel *label;
    QComboBox *layoutDirection;
    QLabel *label_2;
    QFontComboBox *fontComboBox;
    QLabel *label_3;
    QComboBox *style;
    QLabel *label_4;
    QSlider *spacing;

    void setupUi(QDialog *embeddedDialog)
    {
        if (embeddedDialog->objectName().isEmpty())
            embeddedDialog->setObjectName(QString::fromUtf8("embeddedDialog"));
        embeddedDialog->resize(407, 134);
        formLayout = new QFormLayout(embeddedDialog);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(embeddedDialog);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        layoutDirection = new QComboBox(embeddedDialog);
        layoutDirection->addItem(QString());
        layoutDirection->addItem(QString());
        layoutDirection->setObjectName(QString::fromUtf8("layoutDirection"));

        formLayout->setWidget(0, QFormLayout::FieldRole, layoutDirection);

        label_2 = new QLabel(embeddedDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        fontComboBox = new QFontComboBox(embeddedDialog);
        fontComboBox->setObjectName(QString::fromUtf8("fontComboBox"));

        formLayout->setWidget(1, QFormLayout::FieldRole, fontComboBox);

        label_3 = new QLabel(embeddedDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        style = new QComboBox(embeddedDialog);
        style->setObjectName(QString::fromUtf8("style"));

        formLayout->setWidget(2, QFormLayout::FieldRole, style);

        label_4 = new QLabel(embeddedDialog);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        spacing = new QSlider(embeddedDialog);
        spacing->setObjectName(QString::fromUtf8("spacing"));
        spacing->setOrientation(Qt::Horizontal);

        formLayout->setWidget(3, QFormLayout::FieldRole, spacing);

#if QT_CONFIG(shortcut)
        label->setBuddy(layoutDirection);
        label_2->setBuddy(fontComboBox);
        label_3->setBuddy(style);
        label_4->setBuddy(spacing);
#endif // QT_CONFIG(shortcut)

        retranslateUi(embeddedDialog);

        QMetaObject::connectSlotsByName(embeddedDialog);
    } // setupUi

    void retranslateUi(QDialog *embeddedDialog)
    {
        embeddedDialog->setWindowTitle(QCoreApplication::translate("embeddedDialog", "Embedded Dialog", nullptr));
        label->setText(QCoreApplication::translate("embeddedDialog", "Layout Direction:", nullptr));
        layoutDirection->setItemText(0, QCoreApplication::translate("embeddedDialog", "Left to Right", nullptr));
        layoutDirection->setItemText(1, QCoreApplication::translate("embeddedDialog", "Right to Left", nullptr));

        label_2->setText(QCoreApplication::translate("embeddedDialog", "Select Font:", nullptr));
        label_3->setText(QCoreApplication::translate("embeddedDialog", "Style:", nullptr));
        label_4->setText(QCoreApplication::translate("embeddedDialog", "Layout spacing:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class embeddedDialog: public Ui_embeddedDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // EMBEDDEDDIALOG_H
