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

    connect(messageLineEdit, &QLineEdit::textChanged, this,
            [this](const QString &newText) { sendButton->setEnabled(!newText.isEmpty()); });
    connect(sendButton, &QPushButton::clicked, this, [this]() {
        emit message(m_nickname, messageLineEdit->text());
        messageLineEdit->clear();
    });
    connect(actionChangeNickname, &QAction::triggered,
            this, &ChatMainWindow::changeNickname);
    connect(actionAboutQt, &QAction::triggered, this, [this]() { QMessageBox::aboutQt(this); });
    connect(qApp, &QApplication::lastWindowClosed, this,
            [this]() { emit action(m_nickname, tr("leaves the chat")); });

    // add our D-Bus interface and connect to D-Bus
    new ChatAdaptor(this);

    auto connection = QDBusConnection::sessionBus();
    connection.registerObject("/", this);

    using org::example::chat;

    auto *iface = new chat({}, {}, connection, this);
    connect(iface, &chat::message, this, [this](const QString &nickname, const QString &text) {
        displayMessage(tr("<%1> %2").arg(nickname, text));
    });
    connect(iface, &chat::action, this, [this](const QString &nickname, const QString &text) {
        displayMessage(tr("* %1 %2").arg(nickname, text));
    });

    if (!changeNickname(true))
        QMetaObject::invokeMethod(qApp, &QApplication::quit, Qt::QueuedConnection);
}

void ChatMainWindow::displayMessage(const QString &message)
{
    m_messages.append(message);

    if (m_messages.count() > 100)
        m_messages.removeFirst();

    auto history = m_messages.join(QLatin1String("\n"));
    chatHistory->setPlainText(history);
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
