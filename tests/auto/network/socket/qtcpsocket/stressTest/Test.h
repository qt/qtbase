// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TEST_H
#define TEST_H

//------------------------------------------------------------------------------

#include <QTcpServer>
#include <QTcpSocket>

//------------------------------------------------------------------------------
class My4Socket : public QTcpSocket
{
    Q_OBJECT
public:
    My4Socket(QObject *parent);

    void sendTest(quint32 num);
    bool safeShutDown;

private slots:
    void read();
    void closed();
};

//------------------------------------------------------------------------------
class My4Server : public QTcpServer
{
    Q_OBJECT
public:
    My4Server(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socket) override;

private slots:
    void stopServer();

private:
    My4Socket *m_socket;
};

//------------------------------------------------------------------------------
class Test : public QObject
{
    Q_OBJECT

public:
    enum Type {
        Qt4Client,
        Qt4Server,
    };
    Test(Type type);
};

//------------------------------------------------------------------------------
#endif  // TEST_H
