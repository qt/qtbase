/********************************************************************************
** Form generated from reading UI file 'topicchooser.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
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

QT_BEGIN_NAMESPACE

class Ui_TopicChooser
{
public:
    QVBoxLayout *unnamed;
    QLabel *label;
    QListWidget *listWidget;
    QHBoxLayout *unnamed1;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonDisplay;
    QPushButton *buttonCancel;

    void setupUi(QDialog *TopicChooser)
    {
        if (TopicChooser->objectName().isEmpty())
            TopicChooser->setObjectName("TopicChooser");
        TopicChooser->resize(391, 223);
        TopicChooser->setSizeGripEnabled(true);
        unnamed = new QVBoxLayout(TopicChooser);
#ifndef Q_OS_MAC
        unnamed->setSpacing(6);
#endif
        unnamed->setContentsMargins(11, 11, 11, 11);
        unnamed->setObjectName("unnamed");
        label = new QLabel(TopicChooser);
        label->setObjectName("label");

        unnamed->addWidget(label);

        listWidget = new QListWidget(TopicChooser);
        listWidget->setObjectName("listWidget");

        unnamed->addWidget(listWidget);

        unnamed1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        unnamed1->setSpacing(6);
#endif
        unnamed1->setContentsMargins(0, 0, 0, 0);
        unnamed1->setObjectName("unnamed1");
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        unnamed1->addItem(Horizontal_Spacing2);

        buttonDisplay = new QPushButton(TopicChooser);
        buttonDisplay->setObjectName("buttonDisplay");
        buttonDisplay->setAutoDefault(true);

        unnamed1->addWidget(buttonDisplay);

        buttonCancel = new QPushButton(TopicChooser);
        buttonCancel->setObjectName("buttonCancel");
        buttonCancel->setAutoDefault(true);

        unnamed1->addWidget(buttonCancel);


        unnamed->addLayout(unnamed1);

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
