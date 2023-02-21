// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>

#include "chat.h"
#include "chat_adaptor.h"
#include "chat_interface.h"

ChatMainWindow::ChatMainWindow()
{
    setupUi(this);
    sendButton->setEnabled(false);

    connect(messageLineEdit, &QLineEdit::textChanged,
            this, &ChatMainWindow::textChangedSlot);
    connect(sendButton, &QPushButton::clicked,
            this, &ChatMainWindow::sendClickedSlot);
    connect(actionChangeNickname, &QAction::triggered,
            this, &ChatMainWindow::changeNickname);
    connect(actionAboutQt, &QAction::triggered,
            this, &ChatMainWindow::aboutQt);
    connect(qApp, &QApplication::lastWindowClosed,
            this, &ChatMainWindow::exiting);

    // add our D-Bus interface and connect to D-Bus
    new ChatAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/", this);

    org::example::chat *iface;
    iface = new org::example::chat(QString(), QString(), QDBusConnection::sessionBus(), this);
    QDBusConnection::sessionBus().connect(QString(), QString(), "org.example.chat", "message", this, SLOT(messageSlot(QString,QString)));
    connect(iface, &org::example::chat::action,
            this, &ChatMainWindow::actionSlot);

    if (!changeNickname(true))
        QMetaObject::invokeMethod(qApp, &QApplication::quit, Qt::QueuedConnection);
}

ChatMainWindow::~ChatMainWindow()
{
}

void ChatMainWindow::rebuildHistory()
{
    QString history = m_messages.join( QLatin1String("\n" ) );
    chatHistory->setPlainText(history);
}

void ChatMainWindow::messageSlot(const QString &nickname, const QString &text)
{
    QString msg( QLatin1String("<%1> %2") );
    msg = msg.arg(nickname, text);
    m_messages.append(msg);

    if (m_messages.count() > 100)
        m_messages.removeFirst();
    rebuildHistory();
}

void ChatMainWindow::actionSlot(const QString &nickname, const QString &text)
{
    QString msg( QLatin1String("* %1 %2") );
    msg = msg.arg(nickname, text);
    m_messages.append(msg);

    if (m_messages.count() > 100)
        m_messages.removeFirst();
    rebuildHistory();
}

void ChatMainWindow::textChangedSlot(const QString &newText)
{
    sendButton->setEnabled(!newText.isEmpty());
}

void ChatMainWindow::sendClickedSlot()
{
    QDBusMessage msg = QDBusMessage::createSignal("/", "org.example.chat", "message");
    msg << m_nickname << messageLineEdit->text();
    QDBusConnection::sessionBus().send(msg);
    messageLineEdit->setText(QString());
}

bool ChatMainWindow::changeNickname(bool initial)
{
    auto newNickname = QInputDialog::getText(this, tr("Set nickname"), tr("New nickname:"));
    newNickname = newNickname.trimmed();

    if (!newNickname.isEmpty()) {
        auto old = m_nickname;
        m_nickname = newNickname;

        if (initial)
            emit action(m_nickname, tr("joins the chat"));
        else
            emit action(old, tr("is now known as %1").arg(m_nickname));
        return true;
    }

    return false;
}

void ChatMainWindow::aboutQt()
{
    QMessageBox::aboutQt(this);
}

void ChatMainWindow::exiting()
{
    emit action(m_nickname, QLatin1String("leaves the chat"));
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");
        return 1;
    }

    ChatMainWindow chat;
    chat.show();
    return app.exec();
}
