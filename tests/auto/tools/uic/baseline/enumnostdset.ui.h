/********************************************************************************
** Form generated from reading UI file 'enumnostdset.ui'
**
** Created by: Qt User Interface Compiler version 5.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ENUMNOSTDSET_H
#define ENUMNOSTDSET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include "worldtimeclock.h"

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    WorldTimeClock *worldTimeClock;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QStringLiteral("Form"));
        Form->resize(400, 300);
        worldTimeClock = new WorldTimeClock(Form);
        worldTimeClock->setObjectName(QStringLiteral("worldTimeClock"));
        worldTimeClock->setGeometry(QRect(100, 100, 100, 100));
        worldTimeClock->setProperty("penStyle", QVariant::fromValue(Qt::DashDotLine));

        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QApplication::translate("Form", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ENUMNOSTDSET_H
