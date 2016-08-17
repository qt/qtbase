/********************************************************************************
** Form generated from reading UI file 'newdynamicpropertydialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef NEWDYNAMICPROPERTYDIALOG_H
#define NEWDYNAMICPROPERTYDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_NewDynamicPropertyDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLineEdit *m_lineEdit;
    QLabel *label;
    QHBoxLayout *horizontalLayout;
    QComboBox *m_comboBox;
    QSpacerItem *horizontalSpacer;
    QLabel *label_2;
    QSpacerItem *spacerItem;
    QDialogButtonBox *m_buttonBox;

    void setupUi(QDialog *qdesigner_internal__NewDynamicPropertyDialog)
    {
        if (qdesigner_internal__NewDynamicPropertyDialog->objectName().isEmpty())
            qdesigner_internal__NewDynamicPropertyDialog->setObjectName(QStringLiteral("qdesigner_internal__NewDynamicPropertyDialog"));
        qdesigner_internal__NewDynamicPropertyDialog->resize(340, 118);
        verticalLayout = new QVBoxLayout(qdesigner_internal__NewDynamicPropertyDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        m_lineEdit = new QLineEdit(qdesigner_internal__NewDynamicPropertyDialog);
        m_lineEdit->setObjectName(QStringLiteral("m_lineEdit"));
        m_lineEdit->setMinimumSize(QSize(220, 0));

        formLayout->setWidget(0, QFormLayout::FieldRole, m_lineEdit);

        label = new QLabel(qdesigner_internal__NewDynamicPropertyDialog);
        label->setObjectName(QStringLiteral("label"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        m_comboBox = new QComboBox(qdesigner_internal__NewDynamicPropertyDialog);
        m_comboBox->setObjectName(QStringLiteral("m_comboBox"));

        horizontalLayout->addWidget(m_comboBox);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        formLayout->setLayout(1, QFormLayout::FieldRole, horizontalLayout);

        label_2 = new QLabel(qdesigner_internal__NewDynamicPropertyDialog);
        label_2->setObjectName(QStringLiteral("label_2"));
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);


        verticalLayout->addLayout(formLayout);

        spacerItem = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(spacerItem);

        m_buttonBox = new QDialogButtonBox(qdesigner_internal__NewDynamicPropertyDialog);
        m_buttonBox->setObjectName(QStringLiteral("m_buttonBox"));
        m_buttonBox->setOrientation(Qt::Horizontal);
        m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        m_buttonBox->setCenterButtons(false);

        verticalLayout->addWidget(m_buttonBox);


        retranslateUi(qdesigner_internal__NewDynamicPropertyDialog);

        QMetaObject::connectSlotsByName(qdesigner_internal__NewDynamicPropertyDialog);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__NewDynamicPropertyDialog)
    {
        qdesigner_internal__NewDynamicPropertyDialog->setWindowTitle(QApplication::translate("qdesigner_internal::NewDynamicPropertyDialog", "Create Dynamic Property", Q_NULLPTR));
        label->setText(QApplication::translate("qdesigner_internal::NewDynamicPropertyDialog", "Property Name", Q_NULLPTR));
        label_2->setText(QApplication::translate("qdesigner_internal::NewDynamicPropertyDialog", "Property Type", Q_NULLPTR));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class NewDynamicPropertyDialog: public Ui_NewDynamicPropertyDialog {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // NEWDYNAMICPROPERTYDIALOG_H
