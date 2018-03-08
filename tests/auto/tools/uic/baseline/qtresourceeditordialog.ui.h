/********************************************************************************
** Form generated from reading UI file 'qtresourceeditordialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTRESOURCEEDITORDIALOG_H
#define QTRESOURCEEDITORDIALOG_H

#include <QtCore/QVariant>
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
            QtResourceEditorDialog->setObjectName(QStringLiteral("QtResourceEditorDialog"));
        QtResourceEditorDialog->resize(469, 317);
        verticalLayout = new QVBoxLayout(QtResourceEditorDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        splitter = new QSplitter(QtResourceEditorDialog);
        splitter->setObjectName(QStringLiteral("splitter"));
        splitter->setOrientation(Qt::Horizontal);
        layoutWidget = new QWidget(splitter);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        gridLayout = new QGridLayout(layoutWidget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        qrcFileList = new QListWidget(layoutWidget);
        qrcFileList->setObjectName(QStringLiteral("qrcFileList"));
        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qrcFileList->sizePolicy().hasHeightForWidth());
        qrcFileList->setSizePolicy(sizePolicy);

        gridLayout->addWidget(qrcFileList, 0, 0, 1, 4);

        newQrcButton = new QToolButton(layoutWidget);
        newQrcButton->setObjectName(QStringLiteral("newQrcButton"));

        gridLayout->addWidget(newQrcButton, 1, 0, 1, 1);

        removeQrcButton = new QToolButton(layoutWidget);
        removeQrcButton->setObjectName(QStringLiteral("removeQrcButton"));

        gridLayout->addWidget(removeQrcButton, 1, 2, 1, 1);

        spacerItem = new QSpacerItem(21, 20, QSizePolicy::Ignored, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 3, 1, 1);

        importQrcButton = new QToolButton(layoutWidget);
        importQrcButton->setObjectName(QStringLiteral("importQrcButton"));

        gridLayout->addWidget(importQrcButton, 1, 1, 1, 1);

        splitter->addWidget(layoutWidget);
        widget = new QWidget(splitter);
        widget->setObjectName(QStringLiteral("widget"));
        gridLayout1 = new QGridLayout(widget);
        gridLayout1->setObjectName(QStringLiteral("gridLayout1"));
        gridLayout1->setContentsMargins(0, 0, 0, 0);
        resourceTreeView = new QTreeView(widget);
        resourceTreeView->setObjectName(QStringLiteral("resourceTreeView"));

        gridLayout1->addWidget(resourceTreeView, 0, 0, 1, 4);

        newResourceButton = new QToolButton(widget);
        newResourceButton->setObjectName(QStringLiteral("newResourceButton"));

        gridLayout1->addWidget(newResourceButton, 1, 0, 1, 1);

        addResourceButton = new QToolButton(widget);
        addResourceButton->setObjectName(QStringLiteral("addResourceButton"));

        gridLayout1->addWidget(addResourceButton, 1, 1, 1, 1);

        removeResourceButton = new QToolButton(widget);
        removeResourceButton->setObjectName(QStringLiteral("removeResourceButton"));

        gridLayout1->addWidget(removeResourceButton, 1, 2, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(horizontalSpacer, 1, 3, 1, 1);

        splitter->addWidget(widget);

        verticalLayout->addWidget(splitter);

        buttonBox = new QDialogButtonBox(QtResourceEditorDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(QtResourceEditorDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), QtResourceEditorDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), QtResourceEditorDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(QtResourceEditorDialog);
    } // setupUi

    void retranslateUi(QDialog *QtResourceEditorDialog)
    {
        QtResourceEditorDialog->setWindowTitle(QApplication::translate("QtResourceEditorDialog", "Dialog", nullptr));
#ifndef QT_NO_TOOLTIP
        newQrcButton->setToolTip(QApplication::translate("QtResourceEditorDialog", "New File", nullptr));
#endif // QT_NO_TOOLTIP
        newQrcButton->setText(QApplication::translate("QtResourceEditorDialog", "N", nullptr));
#ifndef QT_NO_TOOLTIP
        removeQrcButton->setToolTip(QApplication::translate("QtResourceEditorDialog", "Remove File", nullptr));
#endif // QT_NO_TOOLTIP
        removeQrcButton->setText(QApplication::translate("QtResourceEditorDialog", "R", nullptr));
        importQrcButton->setText(QApplication::translate("QtResourceEditorDialog", "I", nullptr));
#ifndef QT_NO_TOOLTIP
        newResourceButton->setToolTip(QApplication::translate("QtResourceEditorDialog", "New Resource", nullptr));
#endif // QT_NO_TOOLTIP
        newResourceButton->setText(QApplication::translate("QtResourceEditorDialog", "N", nullptr));
        addResourceButton->setText(QApplication::translate("QtResourceEditorDialog", "A", nullptr));
#ifndef QT_NO_TOOLTIP
        removeResourceButton->setToolTip(QApplication::translate("QtResourceEditorDialog", "Remove Resource or File", nullptr));
#endif // QT_NO_TOOLTIP
        removeResourceButton->setText(QApplication::translate("QtResourceEditorDialog", "R", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtResourceEditorDialog: public Ui_QtResourceEditorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTRESOURCEEDITORDIALOG_H
