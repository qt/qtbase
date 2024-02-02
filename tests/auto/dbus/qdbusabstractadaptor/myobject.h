// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <QObject>
#include <QVariant>
#include <QUrl>
#include <QDBusMessage>
#include <QDBusAbstractAdaptor>

extern const char *slotSpy;
extern QString valueSpy;

class QDBusSignalSpy: public QObject
{
    Q_OBJECT

public slots:
    void slot(const QDBusMessage &msg)
    {
        ++count;
        interface = msg.interface();
        name = msg.member();
        signature = msg.signature();
        path = msg.path();
        value.clear();
        if (msg.arguments().size())
            value = msg.arguments().at(0);
    }

public:
    QDBusSignalSpy() : count(0) { }

    int count;
    QString interface;
    QString name;
    QString signature;
    QString path;
    QVariant value;
};

class Interface1: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.Interface1")
public:
    Interface1(QObject *parent) : QDBusAbstractAdaptor(parent)
    { }
};

class Interface2: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.Interface2")
    Q_PROPERTY(QString prop1 READ prop1)
    Q_PROPERTY(QString prop2 READ prop2 WRITE setProp2 SCRIPTABLE true)
    Q_PROPERTY(QUrl nonDBusProperty READ nonDBusProperty)
public:
    Interface2(QObject *parent) : QDBusAbstractAdaptor(parent)
    { setAutoRelaySignals(true); }

    QString prop1() const
    { return QLatin1String("QString Interface2::prop1() const"); }

    QString prop2() const
    { return QLatin1String("QString Interface2::prop2() const"); }

    void setProp2(const QString &value)
    {
        slotSpy = "void Interface2::setProp2(const QString &)";
        valueSpy = value;
    }

    QUrl nonDBusProperty() const
    { return QUrl(); }

    void emitSignal(const QString &, const QVariant &)
    { emit signal(); }

public slots:
    void method()
    {
        slotSpy = "void Interface2::method()";
    }

    Q_SCRIPTABLE void scriptableMethod()
    {
        slotSpy = "void Interface2::scriptableMethod()";
    }

signals:
    void signal();
};

class Interface3: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.Interface3")
    Q_PROPERTY(QString prop1 READ prop1)
    Q_PROPERTY(QString prop2 READ prop2 WRITE setProp2)
    Q_PROPERTY(QString interface3prop READ interface3prop)
public:
    Interface3(QObject *parent) : QDBusAbstractAdaptor(parent)
    { setAutoRelaySignals(true); }

    QString prop1() const
    { return QLatin1String("QString Interface3::prop1() const"); }

    QString prop2() const
    { return QLatin1String("QString Interface3::prop2() const"); }

    void setProp2(const QString &value)
    {
        slotSpy = "void Interface3::setProp2(const QString &)";
        valueSpy = value;
    }

    QString interface3prop() const
    { return QLatin1String("QString Interface3::interface3prop() const"); }

    void emitSignal(const QString &name, const QVariant &value)
    {
        if (name == "signalVoid")
            emit signalVoid();
        else if (name == "signalInt")
            emit signalInt(value.toInt());
        else if (name == "signalString")
            emit signalString(value.toString());
    }

public slots:
    void methodVoid() { slotSpy = "void Interface3::methodVoid()"; }
    void methodInt(int) { slotSpy = "void Interface3::methodInt(int)"; }
    void methodString(QString) { slotSpy = "void Interface3::methodString(QString)"; }

    int methodStringString(const QString &s, QString &out)
    {
        slotSpy = "int Interface3::methodStringString(const QString &, QString &)";
        out = s;
        return 42;
    }

signals:
    void signalVoid();
    void signalInt(int);
    void signalString(const QString &);
};

class Interface4: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.Interface4")
    Q_PROPERTY(QString prop1 READ prop1)
    Q_PROPERTY(QString prop2 READ prop2 WRITE setProp2)
    Q_PROPERTY(QString interface4prop READ interface4prop)
public:
    Interface4(QObject *parent) : QDBusAbstractAdaptor(parent)
    { setAutoRelaySignals(true); }

    QString prop1() const
    { return QLatin1String("QString Interface4::prop1() const"); }

    QString prop2() const
    { return QLatin1String("QString Interface4::prop2() const"); }

    QString interface4prop() const
    { return QLatin1String("QString Interface4::interface4prop() const"); }

    void setProp2(const QString &value)
    {
        slotSpy = "void Interface4::setProp2(const QString &)";
        valueSpy = value;
    }

    void emitSignal(const QString &, const QVariant &value)
    {
        switch (value.metaType().id())
        {
        case QMetaType::UnknownType:
            emit signal();
            break;
        case QMetaType::Int:
            emit signal(value.toInt());
            break;
        case QMetaType::QString:
            emit signal(value.toString());
            break;
        default:
            break;
        }
    }

public slots:
    void method() { slotSpy = "void Interface4::method()"; }
    void method(int) { slotSpy = "void Interface4::method(int)"; }
    void method(QString) { slotSpy = "void Interface4::method(QString)"; }

signals:
    void signal();
    void signal(int);
    void signal(const QString &);
};

class MyObject: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.MyObject")
public:
    Interface1 *if1;
    Interface2 *if2;
    Interface3 *if3;
    Interface4 *if4;

    MyObject(int n = 4)
        : if1(0), if2(0), if3(0), if4(0)
    {
        switch (n)
        {
        case 4:
            if4 = new Interface4(this);
            Q_FALLTHROUGH();
        case 3:
            if3 = new Interface3(this);
            Q_FALLTHROUGH();
        case 2:
            if2 = new Interface2(this);
            Q_FALLTHROUGH();
        case 1:
            if1 = new Interface1(this);
        }
    }

    void emitSignal(const QString &name, const QVariant &value)
    {
        if (name == "scriptableSignalVoid")
            emit scriptableSignalVoid();
        else if (name == "scriptableSignalInt")
            emit scriptableSignalInt(value.toInt());
        else if (name == "scriptableSignalString")
            emit scriptableSignalString(value.toString());
        else if (name == "nonScriptableSignalVoid")
            emit nonScriptableSignalVoid();
    }

signals:
    Q_SCRIPTABLE void scriptableSignalVoid();
    Q_SCRIPTABLE void scriptableSignalInt(int);
    Q_SCRIPTABLE void scriptableSignalString(QString);
    void nonScriptableSignalVoid();
};

#endif // MYOBJECT_H
