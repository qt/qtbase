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
        if (msg.arguments().count())
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
        switch (value.type())
        {
        case QVariant::Invalid:
            emit signal();
            break;
        case QVariant::Int:
            emit signal(value.toInt());
            break;
        case QVariant::String:
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
        case 3:
            if3 = new Interface3(this);
        case 2:
            if2 = new Interface2(this);
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