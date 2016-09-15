/********************************************************************************
** Form generated from reading UI file 'chatdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ChatDialog
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QTextEdit *textEdit;
    QListWidget *listWidget;
    QHBoxLayout *hboxLayout1;
    QLabel *label;
    QLineEdit *lineEdit;

    void setupUi(QDialog *ChatDialog)
    {
        if (ChatDialog->objectName().isEmpty())
            ChatDialog->setObjectName(QStringLiteral("ChatDialog"));
        ChatDialog->resize(513, 349);
        vboxLayout = new QVBoxLayout(ChatDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        textEdit = new QTextEdit(ChatDialog);
        textEdit->setObjectName(QStringLiteral("textEdit"));
        textEdit->setFocusPolicy(Qt::NoFocus);
        textEdit->setReadOnly(true);

        hboxLayout->addWidget(textEdit);

        listWidget = new QListWidget(ChatDialog);
        listWidget->setObjectName(QStringLiteral("listWidget"));
        listWidget->setMaximumSize(QSize(180, 16777215));
        listWidget->setFocusPolicy(Qt::NoFocus);

        hboxLayout->addWidget(listWidget);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        label = new QLabel(ChatDialog);
        label->setObjectName(QStringLiteral("label"));

        hboxLayout1->addWidget(label);

        lineEdit = new QLineEdit(ChatDialog);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));

        hboxLayout1->addWidget(lineEdit);


        vboxLayout->addLayout(hboxLayout1);


        retranslateUi(ChatDialog);

        QMetaObject::connectSlotsByName(ChatDialog);
    } // setupUi

    void retranslateUi(QDialog *ChatDialog)
    {
        ChatDialog->setWindowTitle(QApplication::translate("ChatDialog", "Chat", Q_NULLPTR));
        label->setText(QApplication::translate("ChatDialog", "Message:", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ChatDialog: public Ui_ChatDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CHATDIALOG_H
