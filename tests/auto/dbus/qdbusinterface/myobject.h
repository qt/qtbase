/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>


class MyObject: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.QtDBus.MyObject")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.qtproject.QtDBus.MyObject\" >\n"
"    <property access=\"readwrite\" type=\"i\" name=\"prop1\" />\n"
"    <property name=\"complexProp\" type=\"ai\" access=\"readwrite\">\n"
"      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"QList&lt;int&gt;\"/>\n"
"    </property>\n"
"    <signal name=\"somethingHappened\" >\n"
"      <arg direction=\"out\" type=\"s\" />\n"
"    </signal>\n"
"    <method name=\"ping\" >\n"
"      <arg direction=\"in\" type=\"v\" name=\"ping\" />\n"
"      <arg direction=\"out\" type=\"v\" name=\"ping\" />\n"
"    </method>\n"
"    <method name=\"ping_invokable\" >\n"
"      <arg direction=\"in\" type=\"v\" name=\"ping_invokable\" />\n"
"      <arg direction=\"out\" type=\"v\" name=\"ping_invokable\" />\n"
"    </method>\n"
"    <method name=\"ping\" >\n"
"      <arg direction=\"in\" type=\"v\" name=\"ping1\" />\n"
"      <arg direction=\"in\" type=\"v\" name=\"ping2\" />\n"
"      <arg direction=\"out\" type=\"v\" name=\"pong1\" />\n"
"      <arg direction=\"out\" type=\"v\" name=\"pong2\" />\n"
"    </method>\n"
"    <method name=\"ping_invokable\" >\n"
"      <arg direction=\"in\" type=\"v\" name=\"ping1_invokable\" />\n"
"      <arg direction=\"in\" type=\"v\" name=\"ping2_invokable\" />\n"
"      <arg direction=\"out\" type=\"v\" name=\"pong1_invokable\" />\n"
"      <arg direction=\"out\" type=\"v\" name=\"pong2_invokable\" />\n"
"    </method>\n"
"    <method name=\"ping\" >\n"
"      <arg direction=\"in\" type=\"ai\" name=\"ping\" />\n"
"      <arg direction=\"out\" type=\"ai\" name=\"ping\" />\n"
"      <annotation name=\"org.qtproject.QtDBus.QtTypeName.In0\" value=\"QList&lt;int&gt;\"/>\n"
"      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QList&lt;int&gt;\"/>\n"
"    </method>\n"
"    <method name=\"ping_invokable\" >\n"
"      <arg direction=\"in\" type=\"ai\" name=\"ping_invokable\" />\n"
"      <arg direction=\"out\" type=\"ai\" name=\"ping_invokable\" />\n"
"      <annotation name=\"org.qtproject.QtDBus.QtTypeName.In0\" value=\"QList&lt;int&gt;\"/>\n"
"      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QList&lt;int&gt;\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1)
    Q_PROPERTY(QList<int> complexProp READ complexProp WRITE setComplexProp)

public:
    static int callCount;
    static QVariantList callArgs;
    MyObject()
    {
        QObject *subObject = new QObject(this);
        subObject->setObjectName("subObject");
    }

    int m_prop1;
    int prop1() const
    {
        ++callCount;
        return m_prop1;
    }
    void setProp1(int value)
    {
        ++callCount;
        m_prop1 = value;
    }

    QList<int> m_complexProp;
    QList<int> complexProp() const
    {
        ++callCount;
        return m_complexProp;
    }
    void setComplexProp(const QList<int> &value)
    {
        ++callCount;
        m_complexProp = value;
    }

    Q_INVOKABLE void ping_invokable(QDBusMessage msg)
    {
        QDBusConnection sender = QDBusConnection::sender();
        if (!sender.isConnected())
            exit(1);

        ++callCount;
        callArgs = msg.arguments();

        msg.setDelayedReply(true);
        if (!sender.send(msg.createReply(callArgs)))
            exit(1);
    }

public slots:

    void ping(QDBusMessage msg)
    {
        QDBusConnection sender = QDBusConnection::sender();
        if (!sender.isConnected())
            exit(1);

        ++callCount;
        callArgs = msg.arguments();

        msg.setDelayedReply(true);
        if (!sender.send(msg.createReply(callArgs)))
            exit(1);
    }
};

#endif // INTERFACE_H
