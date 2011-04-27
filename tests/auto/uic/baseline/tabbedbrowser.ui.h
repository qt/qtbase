/*
*********************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'tabbedbrowser.ui'
**
** Created: Fri Sep 4 10:17:15 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TABBEDBROWSER_H
#define TABBEDBROWSER_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TabbedBrowser
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *tab;
    QWidget *frontpage;
    QGridLayout *gridLayout;
    QFrame *frameFind;
    QHBoxLayout *hboxLayout;
    QToolButton *toolClose;
    QLineEdit *editFind;
    QToolButton *toolPrevious;
    QToolButton *toolNext;
    QCheckBox *checkCase;
    QCheckBox *checkWholeWords;
    QLabel *labelWrapped;
    QSpacerItem *spacerItem;

    void setupUi(QWidget *TabbedBrowser)
    {
        if (TabbedBrowser->objectName().isEmpty())
            TabbedBrowser->setObjectName(QString::fromUtf8("TabbedBrowser"));
        TabbedBrowser->resize(710, 664);
        vboxLayout = new QVBoxLayout(TabbedBrowser);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tab = new QTabWidget(TabbedBrowser);
        tab->setObjectName(QString::fromUtf8("tab"));
        frontpage = new QWidget();
        frontpage->setObjectName(QString::fromUtf8("frontpage"));
        gridLayout = new QGridLayout(frontpage);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        tab->addTab(frontpage, QString());

        vboxLayout->addWidget(tab);

        frameFind = new QFrame(TabbedBrowser);
        frameFind->setObjectName(QString::fromUtf8("frameFind"));
        frameFind->setFrameShape(QFrame::StyledPanel);
        frameFind->setFrameShadow(QFrame::Raised);
        hboxLayout = new QHBoxLayout(frameFind);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        toolClose = new QToolButton(frameFind);
        toolClose->setObjectName(QString::fromUtf8("toolClose"));
        const QIcon icon = QIcon(QString::fromUtf8(":/trolltech/assistant/images/close.png"));
        toolClose->setIcon(icon);
        toolClose->setAutoRaise(true);

        hboxLayout->addWidget(toolClose);

        editFind = new QLineEdit(frameFind);
        editFind->setObjectName(QString::fromUtf8("editFind"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(editFind->sizePolicy().hasHeightForWidth());
        editFind->setSizePolicy(sizePolicy);
        editFind->setMinimumSize(QSize(150, 0));

        hboxLayout->addWidget(editFind);

        toolPrevious = new QToolButton(frameFind);
        toolPrevious->setObjectName(QString::fromUtf8("toolPrevious"));
        const QIcon icon1 = QIcon(QString::fromUtf8(":/trolltech/assistant/images/win/previous.png"));
        toolPrevious->setIcon(icon1);
        toolPrevious->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolPrevious->setAutoRaise(true);

        hboxLayout->addWidget(toolPrevious);

        toolNext = new QToolButton(frameFind);
        toolNext->setObjectName(QString::fromUtf8("toolNext"));
        toolNext->setMinimumSize(QSize(0, 0));
        const QIcon icon2 = QIcon(QString::fromUtf8(":/trolltech/assistant/images/win/next.png"));
        toolNext->setIcon(icon2);
        toolNext->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolNext->setAutoRaise(true);
        toolNext->setArrowType(Qt::NoArrow);

        hboxLayout->addWidget(toolNext);

        checkCase = new QCheckBox(frameFind);
        checkCase->setObjectName(QString::fromUtf8("checkCase"));

        hboxLayout->addWidget(checkCase);

        checkWholeWords = new QCheckBox(frameFind);
        checkWholeWords->setObjectName(QString::fromUtf8("checkWholeWords"));

        hboxLayout->addWidget(checkWholeWords);

        labelWrapped = new QLabel(frameFind);
        labelWrapped->setObjectName(QString::fromUtf8("labelWrapped"));
        labelWrapped->setMinimumSize(QSize(0, 20));
        labelWrapped->setMaximumSize(QSize(105, 20));
        labelWrapped->setTextFormat(Qt::RichText);
        labelWrapped->setScaledContents(true);
        labelWrapped->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        hboxLayout->addWidget(labelWrapped);

        spacerItem = new QSpacerItem(81, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addWidget(frameFind);


        retranslateUi(TabbedBrowser);

        QMetaObject::connectSlotsByName(TabbedBrowser);
    } // setupUi

    void retranslateUi(QWidget *TabbedBrowser)
    {
        TabbedBrowser->setWindowTitle(QApplication::translate("TabbedBrowser", "TabbedBrowser", 0, QApplication::UnicodeUTF8));
        tab->setTabText(tab->indexOf(frontpage), QApplication::translate("TabbedBrowser", "Untitled", 0, QApplication::UnicodeUTF8));
        toolClose->setText(QString());
        toolPrevious->setText(QApplication::translate("TabbedBrowser", "Previous", 0, QApplication::UnicodeUTF8));
        toolNext->setText(QApplication::translate("TabbedBrowser", "Next", 0, QApplication::UnicodeUTF8));
        checkCase->setText(QApplication::translate("TabbedBrowser", "Case Sensitive", 0, QApplication::UnicodeUTF8));
        checkWholeWords->setText(QApplication::translate("TabbedBrowser", "Whole words", 0, QApplication::UnicodeUTF8));
        labelWrapped->setText(QApplication::translate("TabbedBrowser", "<img src=\":/trolltech/assistant/images/wrap.png\">&nbsp;Search wrapped", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class TabbedBrowser: public Ui_TabbedBrowser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TABBEDBROWSER_H
