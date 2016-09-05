/********************************************************************************
** Form generated from reading UI file 'embeddeddialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef EMBEDDEDDIALOG_H
#define EMBEDDEDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHeaderView>
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
            embeddedDialog->setObjectName(QStringLiteral("embeddedDialog"));
        embeddedDialog->resize(407, 134);
        formLayout = new QFormLayout(embeddedDialog);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        label = new QLabel(embeddedDialog);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        layoutDirection = new QComboBox(embeddedDialog);
        layoutDirection->setObjectName(QStringLiteral("layoutDirection"));

        formLayout->setWidget(0, QFormLayout::FieldRole, layoutDirection);

        label_2 = new QLabel(embeddedDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        fontComboBox = new QFontComboBox(embeddedDialog);
        fontComboBox->setObjectName(QStringLiteral("fontComboBox"));

        formLayout->setWidget(1, QFormLayout::FieldRole, fontComboBox);

        label_3 = new QLabel(embeddedDialog);
        label_3->setObjectName(QStringLiteral("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        style = new QComboBox(embeddedDialog);
        style->setObjectName(QStringLiteral("style"));

        formLayout->setWidget(2, QFormLayout::FieldRole, style);

        label_4 = new QLabel(embeddedDialog);
        label_4->setObjectName(QStringLiteral("label_4"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        spacing = new QSlider(embeddedDialog);
        spacing->setObjectName(QStringLiteral("spacing"));
        spacing->setOrientation(Qt::Horizontal);

        formLayout->setWidget(3, QFormLayout::FieldRole, spacing);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(layoutDirection);
        label_2->setBuddy(fontComboBox);
        label_3->setBuddy(style);
        label_4->setBuddy(spacing);
#endif // QT_NO_SHORTCUT

        retranslateUi(embeddedDialog);

        QMetaObject::connectSlotsByName(embeddedDialog);
    } // setupUi

    void retranslateUi(QDialog *embeddedDialog)
    {
        embeddedDialog->setWindowTitle(QApplication::translate("embeddedDialog", "Embedded Dialog", Q_NULLPTR));
        label->setText(QApplication::translate("embeddedDialog", "Layout Direction:", Q_NULLPTR));
        layoutDirection->clear();
        layoutDirection->insertItems(0, QStringList()
         << QApplication::translate("embeddedDialog", "Left to Right", Q_NULLPTR)
         << QApplication::translate("embeddedDialog", "Right to Left", Q_NULLPTR)
        );
        label_2->setText(QApplication::translate("embeddedDialog", "Select Font:", Q_NULLPTR));
        label_3->setText(QApplication::translate("embeddedDialog", "Style:", Q_NULLPTR));
        label_4->setText(QApplication::translate("embeddedDialog", "Layout spacing:", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class embeddedDialog: public Ui_embeddedDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // EMBEDDEDDIALOG_H
