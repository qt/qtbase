/********************************************************************************
** Form generated from reading UI file 'stylesheeteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef STYLESHEETEDITOR_H
#define STYLESHEETEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_StyleSheetEditor
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;
    QComboBox *styleSheetCombo;
    QSpacerItem *spacerItem2;
    QComboBox *styleCombo;
    QLabel *label_7;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem3;
    QPushButton *applyButton;
    QTextEdit *styleTextEdit;
    QLabel *label_8;

    void setupUi(QWidget *StyleSheetEditor)
    {
        if (StyleSheetEditor->objectName().isEmpty())
            StyleSheetEditor->setObjectName(QStringLiteral("StyleSheetEditor"));
        StyleSheetEditor->resize(445, 289);
        gridLayout = new QGridLayout(StyleSheetEditor);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        spacerItem = new QSpacerItem(32, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 0, 6, 1, 1);

        spacerItem1 = new QSpacerItem(32, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 0, 0, 1, 1);

        styleSheetCombo = new QComboBox(StyleSheetEditor);
        styleSheetCombo->addItem(QString());
        styleSheetCombo->addItem(QString());
        styleSheetCombo->addItem(QString());
        styleSheetCombo->setObjectName(QStringLiteral("styleSheetCombo"));

        gridLayout->addWidget(styleSheetCombo, 0, 5, 1, 1);

        spacerItem2 = new QSpacerItem(10, 16, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 0, 3, 1, 1);

        styleCombo = new QComboBox(StyleSheetEditor);
        styleCombo->setObjectName(QStringLiteral("styleCombo"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(styleCombo->sizePolicy().hasHeightForWidth());
        styleCombo->setSizePolicy(sizePolicy);

        gridLayout->addWidget(styleCombo, 0, 2, 1, 1);

        label_7 = new QLabel(StyleSheetEditor);
        label_7->setObjectName(QStringLiteral("label_7"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_7->sizePolicy().hasHeightForWidth());
        label_7->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(label_7, 0, 1, 1, 1);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        spacerItem3 = new QSpacerItem(321, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem3);

        applyButton = new QPushButton(StyleSheetEditor);
        applyButton->setObjectName(QStringLiteral("applyButton"));
        applyButton->setEnabled(false);

        hboxLayout->addWidget(applyButton);


        gridLayout->addLayout(hboxLayout, 2, 0, 1, 7);

        styleTextEdit = new QTextEdit(StyleSheetEditor);
        styleTextEdit->setObjectName(QStringLiteral("styleTextEdit"));

        gridLayout->addWidget(styleTextEdit, 1, 0, 1, 7);

        label_8 = new QLabel(StyleSheetEditor);
        label_8->setObjectName(QStringLiteral("label_8"));
        sizePolicy1.setHeightForWidth(label_8->sizePolicy().hasHeightForWidth());
        label_8->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(label_8, 0, 4, 1, 1);


        retranslateUi(StyleSheetEditor);

        QMetaObject::connectSlotsByName(StyleSheetEditor);
    } // setupUi

    void retranslateUi(QWidget *StyleSheetEditor)
    {
        StyleSheetEditor->setWindowTitle(QApplication::translate("StyleSheetEditor", "Style Editor", nullptr));
        styleSheetCombo->setItemText(0, QApplication::translate("StyleSheetEditor", "Default", nullptr));
        styleSheetCombo->setItemText(1, QApplication::translate("StyleSheetEditor", "Coffee", nullptr));
        styleSheetCombo->setItemText(2, QApplication::translate("StyleSheetEditor", "Pagefold", nullptr));

        label_7->setText(QApplication::translate("StyleSheetEditor", "Style:", nullptr));
        applyButton->setText(QApplication::translate("StyleSheetEditor", "&Apply", nullptr));
        label_8->setText(QApplication::translate("StyleSheetEditor", "Style Sheet:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class StyleSheetEditor: public Ui_StyleSheetEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // STYLESHEETEDITOR_H
