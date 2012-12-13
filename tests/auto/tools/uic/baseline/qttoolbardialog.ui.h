/********************************************************************************
** Form generated from reading UI file 'qttoolbardialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTTOOLBARDIALOG_H
#define QTTOOLBARDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
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
            QtToolBarDialog->setObjectName(QStringLiteral("QtToolBarDialog"));
        QtToolBarDialog->resize(583, 508);
        gridLayout = new QGridLayout(QtToolBarDialog);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        actionTree = new QTreeWidget(QtToolBarDialog);
        actionTree->setObjectName(QStringLiteral("actionTree"));

        gridLayout->addWidget(actionTree, 1, 0, 3, 1);

        label = new QLabel(QtToolBarDialog);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        label_2 = new QLabel(QtToolBarDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        hboxLayout->addWidget(label_2);

        newButton = new QToolButton(QtToolBarDialog);
        newButton->setObjectName(QStringLiteral("newButton"));

        hboxLayout->addWidget(newButton);

        removeButton = new QToolButton(QtToolBarDialog);
        removeButton->setObjectName(QStringLiteral("removeButton"));

        hboxLayout->addWidget(removeButton);

        renameButton = new QToolButton(QtToolBarDialog);
        renameButton->setObjectName(QStringLiteral("renameButton"));

        hboxLayout->addWidget(renameButton);


        gridLayout->addLayout(hboxLayout, 0, 1, 1, 2);

        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        upButton = new QToolButton(QtToolBarDialog);
        upButton->setObjectName(QStringLiteral("upButton"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(upButton->sizePolicy().hasHeightForWidth());
        upButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(upButton);

        leftButton = new QToolButton(QtToolBarDialog);
        leftButton->setObjectName(QStringLiteral("leftButton"));
        sizePolicy.setHeightForWidth(leftButton->sizePolicy().hasHeightForWidth());
        leftButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(leftButton);

        rightButton = new QToolButton(QtToolBarDialog);
        rightButton->setObjectName(QStringLiteral("rightButton"));
        sizePolicy.setHeightForWidth(rightButton->sizePolicy().hasHeightForWidth());
        rightButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(rightButton);

        downButton = new QToolButton(QtToolBarDialog);
        downButton->setObjectName(QStringLiteral("downButton"));
        sizePolicy.setHeightForWidth(downButton->sizePolicy().hasHeightForWidth());
        downButton->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(downButton);

        spacerItem = new QSpacerItem(29, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);


        gridLayout->addLayout(vboxLayout, 3, 1, 1, 1);

        currentToolBarList = new QListWidget(QtToolBarDialog);
        currentToolBarList->setObjectName(QStringLiteral("currentToolBarList"));

        gridLayout->addWidget(currentToolBarList, 3, 2, 1, 1);

        label_3 = new QLabel(QtToolBarDialog);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 2, 1, 1, 2);

        toolBarList = new QListWidget(QtToolBarDialog);
        toolBarList->setObjectName(QStringLiteral("toolBarList"));

        gridLayout->addWidget(toolBarList, 1, 1, 1, 2);

        buttonBox = new QDialogButtonBox(QtToolBarDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
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
        QtToolBarDialog->setWindowTitle(QApplication::translate("QtToolBarDialog", "Customize Toolbars", 0));
        QTreeWidgetItem *___qtreewidgetitem = actionTree->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("QtToolBarDialog", "1", 0));
        label->setText(QApplication::translate("QtToolBarDialog", "Actions", 0));
        label_2->setText(QApplication::translate("QtToolBarDialog", "Toolbars", 0));
#ifndef QT_NO_TOOLTIP
        newButton->setToolTip(QApplication::translate("QtToolBarDialog", "Add new toolbar", 0));
#endif // QT_NO_TOOLTIP
        newButton->setText(QApplication::translate("QtToolBarDialog", "New", 0));
#ifndef QT_NO_TOOLTIP
        removeButton->setToolTip(QApplication::translate("QtToolBarDialog", "Remove selected toolbar", 0));
#endif // QT_NO_TOOLTIP
        removeButton->setText(QApplication::translate("QtToolBarDialog", "Remove", 0));
#ifndef QT_NO_TOOLTIP
        renameButton->setToolTip(QApplication::translate("QtToolBarDialog", "Rename toolbar", 0));
#endif // QT_NO_TOOLTIP
        renameButton->setText(QApplication::translate("QtToolBarDialog", "Rename", 0));
#ifndef QT_NO_TOOLTIP
        upButton->setToolTip(QApplication::translate("QtToolBarDialog", "Move action up", 0));
#endif // QT_NO_TOOLTIP
        upButton->setText(QApplication::translate("QtToolBarDialog", "Up", 0));
#ifndef QT_NO_TOOLTIP
        leftButton->setToolTip(QApplication::translate("QtToolBarDialog", "Remove action from toolbar", 0));
#endif // QT_NO_TOOLTIP
        leftButton->setText(QApplication::translate("QtToolBarDialog", "<-", 0));
#ifndef QT_NO_TOOLTIP
        rightButton->setToolTip(QApplication::translate("QtToolBarDialog", "Add action to toolbar", 0));
#endif // QT_NO_TOOLTIP
        rightButton->setText(QApplication::translate("QtToolBarDialog", "->", 0));
#ifndef QT_NO_TOOLTIP
        downButton->setToolTip(QApplication::translate("QtToolBarDialog", "Move action down", 0));
#endif // QT_NO_TOOLTIP
        downButton->setText(QApplication::translate("QtToolBarDialog", "Down", 0));
        label_3->setText(QApplication::translate("QtToolBarDialog", "Current Toolbar Actions", 0));
    } // retranslateUi

};

namespace Ui {
    class QtToolBarDialog: public Ui_QtToolBarDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTTOOLBARDIALOG_H
