/********************************************************************************
** Form generated from reading UI file 'chatmainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CHATMAINWINDOW_H
#define CHATMAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ChatMainWindow
{
public:
    QAction *actionQuit;
    QAction *actionAboutQt;
    QAction *actionChangeNickname;
    QWidget *centralwidget;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout;
    QTextBrowser *chatHistory;
    QHBoxLayout *hboxLayout1;
    QLabel *label;
    QLineEdit *messageLineEdit;
    QPushButton *sendButton;
    QMenuBar *menubar;
    QMenu *menuQuit;
    QMenu *menuFile;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *ChatMainWindow)
    {
        if (ChatMainWindow->objectName().isEmpty())
            ChatMainWindow->setObjectName(QStringLiteral("ChatMainWindow"));
        ChatMainWindow->resize(800, 600);
        actionQuit = new QAction(ChatMainWindow);
        actionQuit->setObjectName(QStringLiteral("actionQuit"));
        actionAboutQt = new QAction(ChatMainWindow);
        actionAboutQt->setObjectName(QStringLiteral("actionAboutQt"));
        actionChangeNickname = new QAction(ChatMainWindow);
        actionChangeNickname->setObjectName(QStringLiteral("actionChangeNickname"));
        centralwidget = new QWidget(ChatMainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        hboxLayout = new QHBoxLayout(centralwidget);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        chatHistory = new QTextBrowser(centralwidget);
        chatHistory->setObjectName(QStringLiteral("chatHistory"));
        chatHistory->setAcceptDrops(false);
        chatHistory->setAcceptRichText(true);

        vboxLayout->addWidget(chatHistory);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        label = new QLabel(centralwidget);
        label->setObjectName(QStringLiteral("label"));

        hboxLayout1->addWidget(label);

        messageLineEdit = new QLineEdit(centralwidget);
        messageLineEdit->setObjectName(QStringLiteral("messageLineEdit"));

        hboxLayout1->addWidget(messageLineEdit);

        sendButton = new QPushButton(centralwidget);
        sendButton->setObjectName(QStringLiteral("sendButton"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(sendButton->sizePolicy().hasHeightForWidth());
        sendButton->setSizePolicy(sizePolicy);

        hboxLayout1->addWidget(sendButton);


        vboxLayout->addLayout(hboxLayout1);


        hboxLayout->addLayout(vboxLayout);

        ChatMainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(ChatMainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 31));
        menuQuit = new QMenu(menubar);
        menuQuit->setObjectName(QStringLiteral("menuQuit"));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        ChatMainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(ChatMainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        ChatMainWindow->setStatusBar(statusbar);
#ifndef QT_NO_SHORTCUT
        label->setBuddy(messageLineEdit);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(chatHistory, messageLineEdit);
        QWidget::setTabOrder(messageLineEdit, sendButton);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuQuit->menuAction());
        menuQuit->addAction(actionAboutQt);
        menuFile->addAction(actionChangeNickname);
        menuFile->addSeparator();
        menuFile->addAction(actionQuit);

        retranslateUi(ChatMainWindow);
        QObject::connect(messageLineEdit, SIGNAL(returnPressed()), sendButton, SLOT(animateClick()));
        QObject::connect(actionQuit, SIGNAL(triggered(bool)), ChatMainWindow, SLOT(close()));

        QMetaObject::connectSlotsByName(ChatMainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *ChatMainWindow)
    {
        ChatMainWindow->setWindowTitle(QApplication::translate("ChatMainWindow", "Qt D-Bus Chat", Q_NULLPTR));
        actionQuit->setText(QApplication::translate("ChatMainWindow", "Quit", Q_NULLPTR));
        actionQuit->setShortcut(QApplication::translate("ChatMainWindow", "Ctrl+Q", Q_NULLPTR));
        actionAboutQt->setText(QApplication::translate("ChatMainWindow", "About Qt...", Q_NULLPTR));
        actionChangeNickname->setText(QApplication::translate("ChatMainWindow", "Change nickname...", Q_NULLPTR));
        actionChangeNickname->setShortcut(QApplication::translate("ChatMainWindow", "Ctrl+N", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        chatHistory->setToolTip(QApplication::translate("ChatMainWindow", "Messages sent and received from other users", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        label->setText(QApplication::translate("ChatMainWindow", "Message:", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        sendButton->setToolTip(QApplication::translate("ChatMainWindow", "Sends a message to other people", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        sendButton->setWhatsThis(QString());
#endif // QT_NO_WHATSTHIS
        sendButton->setText(QApplication::translate("ChatMainWindow", "Send", Q_NULLPTR));
        menuQuit->setTitle(QApplication::translate("ChatMainWindow", "Help", Q_NULLPTR));
        menuFile->setTitle(QApplication::translate("ChatMainWindow", "File", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ChatMainWindow: public Ui_ChatMainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CHATMAINWINDOW_H
