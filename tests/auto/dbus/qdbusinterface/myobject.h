// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <QObject>
#include <QDBusMessage>
#include <QDBusContext>
#include <QDBusConnection>

class MyObject: public QObject, protected QDBusContext
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
        Q_ASSERT(QDBusContext::calledFromDBus());
        ++callCount;
        callArgs = msg.arguments();

        msg.setDelayedReply(true);
        if (!QDBusContext::connection().send(msg.createReply(callArgs)))
            exit(1);
    }

public slots:

    void ping(QDBusMessage msg)
    {
        Q_ASSERT(QDBusContext::calledFromDBus());
        ++callCount;
        callArgs = msg.arguments();

        msg.setDelayedReply(true);
        if (!QDBusContext::connection().send(msg.createReply(callArgs)))
            exit(1);
    }
};

#endif // INTERFACE_H
