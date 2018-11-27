/********************************************************************************
** Form generated from reading UI file 'topicchooser.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TOPICCHOOSER_H
#define TOPICCHOOSER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TopicChooser
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *label;
    QListWidget *listWidget;
    QWidget *Layout16;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonDisplay;
    QPushButton *buttonCancel;

    void setupUi(QDialog *TopicChooser)
    {
        if (TopicChooser->objectName().isEmpty())
            TopicChooser->setObjectName(QString::fromUtf8("TopicChooser"));
        TopicChooser->resize(391, 223);
        TopicChooser->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(TopicChooser);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setObjectName(QString::fromUtf8("unnamed"));
        label = new QLabel(TopicChooser);
        label->setObjectName(QString::fromUtf8("label"));

        vboxLayout->addWidget(label);

        listWidget = new QListWidget(TopicChooser);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        vboxLayout->addWidget(listWidget);

        Layout16 = new QWidget(TopicChooser);
        Layout16->setObjectName(QString::fromUtf8("Layout16"));
        hboxLayout = new QHBoxLayout(Layout16);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setObjectName(QString::fromUtf8("unnamed"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        buttonDisplay = new QPushButton(Layout16);
        buttonDisplay->setObjectName(QString::fromUtf8("buttonDisplay"));
        buttonDisplay->setAutoDefault(true);

        hboxLayout->addWidget(buttonDisplay);

        buttonCancel = new QPushButton(Layout16);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout->addWidget(buttonCancel);


        vboxLayout->addWidget(Layout16);

#if QT_CONFIG(shortcut)
        label->setBuddy(listWidget);
#endif // QT_CONFIG(shortcut)

        retranslateUi(TopicChooser);

        buttonDisplay->setDefault(true);


        QMetaObject::connectSlotsByName(TopicChooser);
    } // setupUi

    void retranslateUi(QDialog *TopicChooser)
    {
        TopicChooser->setWindowTitle(QCoreApplication::translate("TopicChooser", "Choose Topic", nullptr));
        label->setText(QCoreApplication::translate("TopicChooser", "&Topics", nullptr));
        buttonDisplay->setText(QCoreApplication::translate("TopicChooser", "&Display", nullptr));
        buttonCancel->setText(QCoreApplication::translate("TopicChooser", "&Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TopicChooser: public Ui_TopicChooser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TOPICCHOOSER_H
