/********************************************************************************
** Form generated from reading UI file 'qtresourceeditordialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTRESOURCEEDITORDIALOG_H
#define QTRESOURCEEDITORDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QtResourceEditorDialog
{
public:
    QVBoxLayout *verticalLayout;
    QSplitter *splitter;
    QWidget *layoutWidget;
    QGridLayout *gridLayout;
    QListWidget *qrcFileList;
    QToolButton *newQrcButton;
    QToolButton *removeQrcButton;
    QSpacerItem *spacerItem;
    QToolButton *importQrcButton;
    QWidget *widget;
    QGridLayout *gridLayout1;
    QTreeView *resourceTreeView;
    QToolButton *newResourceButton;
    QToolButton *addResourceButton;
    QToolButton *removeResourceButton;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *QtResourceEditorDialog)
    {
        if (QtResourceEditorDialog->objectName().isEmpty())
            QtResourceEditorDialog->setObjectName("QtResourceEditorDialog");
        QtResourceEditorDialog->resize(469, 317);
        verticalLayout = new QVBoxLayout(QtResourceEditorDialog);
        verticalLayout->setObjectName("verticalLayout");
        splitter = new QSplitter(QtResourceEditorDialog);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Horizontal);
        layoutWidget = new QWidget(splitter);
        layoutWidget->setObjectName("layoutWidget");
        gridLayout = new QGridLayout(layoutWidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(0, 0, 0, 0);
        qrcFileList = new QListWidget(layoutWidget);
        qrcFileList->setObjectName("qrcFileList");
        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qrcFileList->sizePolicy().hasHeightForWidth());
        qrcFileList->setSizePolicy(sizePolicy);

        gridLayout->addWidget(qrcFileList, 0, 0, 1, 4);

        newQrcButton = new QToolButton(layoutWidget);
        newQrcButton->setObjectName("newQrcButton");

        gridLayout->addWidget(newQrcButton, 1, 0, 1, 1);

        removeQrcButton = new QToolButton(layoutWidget);
        removeQrcButton->setObjectName("removeQrcButton");

        gridLayout->addWidget(removeQrcButton, 1, 2, 1, 1);

        spacerItem = new QSpacerItem(21, 20, QSizePolicy::Ignored, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 3, 1, 1);

        importQrcButton = new QToolButton(layoutWidget);
        importQrcButton->setObjectName("importQrcButton");

        gridLayout->addWidget(importQrcButton, 1, 1, 1, 1);

        splitter->addWidget(layoutWidget);
        widget = new QWidget(splitter);
        widget->setObjectName("widget");
        gridLayout1 = new QGridLayout(widget);
        gridLayout1->setObjectName("gridLayout1");
        gridLayout1->setContentsMargins(0, 0, 0, 0);
        resourceTreeView = new QTreeView(widget);
        resourceTreeView->setObjectName("resourceTreeView");

        gridLayout1->addWidget(resourceTreeView, 0, 0, 1, 4);

        newResourceButton = new QToolButton(widget);
        newResourceButton->setObjectName("newResourceButton");

        gridLayout1->addWidget(newResourceButton, 1, 0, 1, 1);

        addResourceButton = new QToolButton(widget);
        addResourceButton->setObjectName("addResourceButton");

        gridLayout1->addWidget(addResourceButton, 1, 1, 1, 1);

        removeResourceButton = new QToolButton(widget);
        removeResourceButton->setObjectName("removeResourceButton");

        gridLayout1->addWidget(removeResourceButton, 1, 2, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(horizontalSpacer, 1, 3, 1, 1);

        splitter->addWidget(widget);

        verticalLayout->addWidget(splitter);

        buttonBox = new QDialogButtonBox(QtResourceEditorDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(QtResourceEditorDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, QtResourceEditorDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, QtResourceEditorDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(QtResourceEditorDialog);
    } // setupUi

    void retranslateUi(QDialog *QtResourceEditorDialog)
    {
        QtResourceEditorDialog->setWindowTitle(QCoreApplication::translate("QtResourceEditorDialog", "Dialog", nullptr));
#if QT_CONFIG(tooltip)
        newQrcButton->setToolTip(QCoreApplication::translate("QtResourceEditorDialog", "New File", nullptr));
#endif // QT_CONFIG(tooltip)
        newQrcButton->setText(QCoreApplication::translate("QtResourceEditorDialog", "N", nullptr));
#if QT_CONFIG(tooltip)
        removeQrcButton->setToolTip(QCoreApplication::translate("QtResourceEditorDialog", "Remove File", nullptr));
#endif // QT_CONFIG(tooltip)
        removeQrcButton->setText(QCoreApplication::translate("QtResourceEditorDialog", "R", nullptr));
        importQrcButton->setText(QCoreApplication::translate("QtResourceEditorDialog", "I", nullptr));
#if QT_CONFIG(tooltip)
        newResourceButton->setToolTip(QCoreApplication::translate("QtResourceEditorDialog", "New Resource", nullptr));
#endif // QT_CONFIG(tooltip)
        newResourceButton->setText(QCoreApplication::translate("QtResourceEditorDialog", "N", nullptr));
        addResourceButton->setText(QCoreApplication::translate("QtResourceEditorDialog", "A", nullptr));
#if QT_CONFIG(tooltip)
        removeResourceButton->setToolTip(QCoreApplication::translate("QtResourceEditorDialog", "Remove Resource or File", nullptr));
#endif // QT_CONFIG(tooltip)
        removeResourceButton->setText(QCoreApplication::translate("QtResourceEditorDialog", "R", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtResourceEditorDialog: public Ui_QtResourceEditorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTRESOURCEEDITORDIALOG_H
