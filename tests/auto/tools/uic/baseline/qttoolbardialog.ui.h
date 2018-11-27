/********************************************************************************
** Form generated from reading UI file 'qttoolbardialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTTOOLBARDIALOG_H
#define QTTOOLBARDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_QtToolBarDialog
{
public:
    QGridLayout *gridLayout;
    QTreeWidget *actionTree;
    QLabel *label;
    QHBoxLayout *hboxLayout;
    QLabel *label_2;
    QToolButton *newButton;
    QToolButton *removeButton;
    QToolButton *renameButton;
    QVBoxLayout *vboxLayout;
    QToolButton *upButton;
    QToolButton *leftButton;
    QToolButton *rightButton;
    QToolButton *downButton;
    QSpacerItem *spacerItem;
    QListWidget *currentToolBarList;
    QLabel *label_3;
    QListWidget *toolBarList;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *QtToolBarDialog)
    {
        if (QtToolBarDialog->objectName().isEmpty())
            QtToolBarDialog->setObjectName(QString::fromUtf8("QtToolBarDialog"));
        QtToolBarDialog->resize(583, 508);
        gridLayout = new QGridLayout(QtToolBarDialog);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        actionTree = new QTreeWidget(QtToolBarDialog);
        actionTree->setObjectName(QString::fromUtf8("actionTree"));

        gridLayout->addWidget(actionTree, 1, 0, 3, 1);

        label = new QLabel(QtToolBarDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        label_2 = new QLabel(QtToolBarDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        hboxLayout->addWidget(label_2);

        newButton = new QToolButton(QtToolBarDialog);
        newButton->setObjectName(QString::fromUtf8("newButton"));

        hboxLayout->addWidget(newButton);

        removeButton = new QToolButton(QtToolBarDialog);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));

        hboxLayout->addWidget(removeButton);

        renameButton = new QToolButton(QtToolBarDialog);
        renameButton->setObjectName(QString::fromUtf8("renameButton"));

        hboxLayout->addWidget(renameButton);


        gridLayout->addLayout(hboxLayout, 0, 1, 1, 2);

        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        upButton = new QToolButton(QtToolBarDialog);
        upButton->setObjectName(QString::fromUtf8("upButton"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(upButton->sizePolicy().hasHeightForWidth());
        upButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(upButton);

        leftButton = new QToolButton(QtToolBarDialog);
        leftButton->setObjectName(QString::fromUtf8("leftButton"));
        sizePolicy.setHeightForWidth(leftButton->sizePolicy().hasHeightForWidth());
        leftButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(leftButton);

        rightButton = new QToolButton(QtToolBarDialog);
        rightButton->setObjectName(QString::fromUtf8("rightButton"));
        sizePolicy.setHeightForWidth(rightButton->sizePolicy().hasHeightForWidth());
        rightButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(rightButton);

        downButton = new QToolButton(QtToolBarDialog);
        downButton->setObjectName(QString::fromUtf8("downButton"));
        sizePolicy.setHeightForWidth(downButton->sizePolicy().hasHeightForWidth());
        downButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(downButton);

        spacerItem = new QSpacerItem(29, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);


        gridLayout->addLayout(vboxLayout, 3, 1, 1, 1);

        currentToolBarList = new QListWidget(QtToolBarDialog);
        currentToolBarList->setObjectName(QString::fromUtf8("currentToolBarList"));

        gridLayout->addWidget(currentToolBarList, 3, 2, 1, 1);

        label_3 = new QLabel(QtToolBarDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 2, 1, 1, 2);

        toolBarList = new QListWidget(QtToolBarDialog);
        toolBarList->setObjectName(QString::fromUtf8("toolBarList"));

        gridLayout->addWidget(toolBarList, 1, 1, 1, 2);

        buttonBox = new QDialogButtonBox(QtToolBarDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::RestoreDefaults);

        gridLayout->addWidget(buttonBox, 5, 0, 1, 3);

        QWidget::setTabOrder(newButton, removeButton);
        QWidget::setTabOrder(removeButton, renameButton);
        QWidget::setTabOrder(renameButton, toolBarList);
        QWidget::setTabOrder(toolBarList, upButton);
        QWidget::setTabOrder(upButton, leftButton);
        QWidget::setTabOrder(leftButton, rightButton);
        QWidget::setTabOrder(rightButton, downButton);
        QWidget::setTabOrder(downButton, currentToolBarList);

        retranslateUi(QtToolBarDialog);

        QMetaObject::connectSlotsByName(QtToolBarDialog);
    } // setupUi

    void retranslateUi(QDialog *QtToolBarDialog)
    {
        QtToolBarDialog->setWindowTitle(QCoreApplication::translate("QtToolBarDialog", "Customize Toolbars", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = actionTree->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("QtToolBarDialog", "1", nullptr));
        label->setText(QCoreApplication::translate("QtToolBarDialog", "Actions", nullptr));
        label_2->setText(QCoreApplication::translate("QtToolBarDialog", "Toolbars", nullptr));
#if QT_CONFIG(tooltip)
        newButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Add new toolbar", nullptr));
#endif // QT_CONFIG(tooltip)
        newButton->setText(QCoreApplication::translate("QtToolBarDialog", "New", nullptr));
#if QT_CONFIG(tooltip)
        removeButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Remove selected toolbar", nullptr));
#endif // QT_CONFIG(tooltip)
        removeButton->setText(QCoreApplication::translate("QtToolBarDialog", "Remove", nullptr));
#if QT_CONFIG(tooltip)
        renameButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Rename toolbar", nullptr));
#endif // QT_CONFIG(tooltip)
        renameButton->setText(QCoreApplication::translate("QtToolBarDialog", "Rename", nullptr));
#if QT_CONFIG(tooltip)
        upButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Move action up", nullptr));
#endif // QT_CONFIG(tooltip)
        upButton->setText(QCoreApplication::translate("QtToolBarDialog", "Up", nullptr));
#if QT_CONFIG(tooltip)
        leftButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Remove action from toolbar", nullptr));
#endif // QT_CONFIG(tooltip)
        leftButton->setText(QCoreApplication::translate("QtToolBarDialog", "<-", nullptr));
#if QT_CONFIG(tooltip)
        rightButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Add action to toolbar", nullptr));
#endif // QT_CONFIG(tooltip)
        rightButton->setText(QCoreApplication::translate("QtToolBarDialog", "->", nullptr));
#if QT_CONFIG(tooltip)
        downButton->setToolTip(QCoreApplication::translate("QtToolBarDialog", "Move action down", nullptr));
#endif // QT_CONFIG(tooltip)
        downButton->setText(QCoreApplication::translate("QtToolBarDialog", "Down", nullptr));
        label_3->setText(QCoreApplication::translate("QtToolBarDialog", "Current Toolbar Actions", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtToolBarDialog: public Ui_QtToolBarDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTTOOLBARDIALOG_H
