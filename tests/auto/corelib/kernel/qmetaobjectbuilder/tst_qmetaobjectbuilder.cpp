/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/qlocale.h>
#include <private/qmetaobjectbuilder_p.h>

class tst_QMetaObjectBuilder : public QObject
{
    Q_OBJECT
private slots:
    void create();
    void className();
    void superClass();
    void flags();
    void method();
    void slot();
    void signal();
    void constructor();
    void property();
    void variantProperty();
    void notifySignal();
    void enumerator();
    void classInfo();
    void relatedMetaObject();
    void staticMetacall();
    void copyMetaObject();
    void serialize();
    void relocatableData();
    void removeNotifySignal();

    void usage_signal();
    void usage_property();
    void usage_slot();
    void usage_method();
    void usage_constructor();
    void usage_connect();
    void usage_templateConnect();

    void classNameFirstInStringData();

private:
    static bool checkForSideEffects
        (const QMetaObjectBuilder& builder,
         QMetaObjectBuilder::AddMembers members);
    static bool sameMetaObject
        (const QMetaObject *meta1, const QMetaObject *meta2);
};

// Dummy class that has something of every type of thing moc can generate.
class SomethingOfEverything : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("ci_foo", "ABC")
    Q_CLASSINFO("ci_bar", "DEF")
    Q_PROPERTY(QString prop READ prop WRITE setProp NOTIFY propChanged)
    Q_PROPERTY(QString prop2 READ prop WRITE setProp)
    Q_PROPERTY(QString revisionProp READ prop WRITE setProp REVISION 42)
    Q_PROPERTY(SomethingEnum eprop READ eprop)
    Q_PROPERTY(SomethingFlagEnum fprop READ fprop)
    Q_PROPERTY(QLocale::Language language READ language)
    Q_ENUMS(SomethingEnum)
    Q_FLAGS(SomethingFlagEnum)
public:
    Q_INVOKABLE SomethingOfEverything() {}
    ~SomethingOfEverything() {}

    enum SomethingEnum
    {
        GHI,
        JKL = 10
    };

    enum SomethingFlagEnum
    {
        XYZ = 1,
        UVW = 8
    };

    Q_INVOKABLE Q_SCRIPTABLE void method1() {}

    QString prop() const { return QString(); }
    void setProp(const QString& v) { Q_UNUSED(v); }

    SomethingOfEverything::SomethingEnum eprop() const { return GHI; }
    SomethingOfEverything::SomethingFlagEnum fprop() const { return XYZ; }
    QLocale::Language language() const { return QLocale::English; }

public slots:
    void slot1(const QString&) {}
    void slot2(int, const QString&) {}
    Q_REVISION(24) void revisionSlot() {}

private slots:
    void slot3() {}

protected slots:
    Q_SCRIPTABLE void slot4(int) {}
    void slot5(int a, const QString& b) { Q_UNUSED(a); Q_UNUSED(b); }

signals:
    void sig1();
    void sig2(int x, const QString& y);
    void propChanged(const QString&);
};

void tst_QMetaObjectBuilder::create()
{
    QMetaObjectBuilder builder;
    QVERIFY(builder.className().isEmpty());
    QCOMPARE(builder.superClass(), &QObject::staticMetaObject);
    QCOMPARE(builder.methodCount(), 0);
    QCOMPARE(builder.constructorCount(), 0);
    QCOMPARE(builder.propertyCount(), 0);
    QCOMPARE(builder.enumeratorCount(), 0);
    QCOMPARE(builder.classInfoCount(), 0);
    QCOMPARE(builder.relatedMetaObjectCount(), 0);
    QVERIFY(!builder.staticMetacallFunction());
}

void tst_QMetaObjectBuilder::className()
{
    QMetaObjectBuilder builder;

    // Change the class name.
    builder.setClassName("Foo");
    QCOMPARE(builder.className(), QByteArray("Foo"));

    // Change it again.
    builder.setClassName("Bar");
    QCOMPARE(builder.className(), QByteArray("Bar"));

    // Clone the class name off a static QMetaObject.
    builder.addMetaObject(&QObject::staticMetaObject, QMetaObjectBuilder::ClassName);
    QCOMPARE(builder.className(), QByteArray("QObject"));

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::ClassName));
}

void tst_QMetaObjectBuilder::superClass()
{
    QMetaObjectBuilder builder;

    // Change the super class.
    builder.setSuperClass(&QObject::staticMetaObject);
    QCOMPARE(builder.superClass(), &QObject::staticMetaObject);

    // Change it again.
    builder.setSuperClass(&staticMetaObject);
    QCOMPARE(builder.superClass(), &staticMetaObject);

    // Clone the super class off a static QMetaObject.
    builder.addMetaObject(&QObject::staticMetaObject, QMetaObjectBuilder::SuperClass);
    QVERIFY(!builder.superClass());
    builder.addMetaObject(&staticMetaObject, QMetaObjectBuilder::SuperClass);
    QCOMPARE(builder.superClass(), staticMetaObject.superClass());

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::SuperClass));
}

void tst_QMetaObjectBuilder::flags()
{
    QMetaObjectBuilder builder;

    // Check default
    QCOMPARE(builder.flags(), 0);

    // Set flags
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
    QCOMPARE(builder.flags(), QMetaObjectBuilder::DynamicMetaObject);
}

void tst_QMetaObjectBuilder::method()
{
    QMetaObjectBuilder builder;

    // Check null method
    QMetaMethodBuilder nullMethod;
    QCOMPARE(nullMethod.signature(), QByteArray());
    QCOMPARE(nullMethod.methodType(), QMetaMethod::Method);
    QVERIFY(nullMethod.returnType().isEmpty());
    QVERIFY(nullMethod.parameterTypes().isEmpty());
    QVERIFY(nullMethod.parameterNames().isEmpty());
    QVERIFY(nullMethod.tag().isEmpty());
    QCOMPARE(nullMethod.access(), QMetaMethod::Public);
    QCOMPARE(nullMethod.attributes(), 0);
    QCOMPARE(nullMethod.revision(), 0);
    QCOMPARE(nullMethod.index(), 0);

    // Add a method and check its attributes.
    QMetaMethodBuilder method1 = builder.addMethod("foo(const QString&, int)");
    QCOMPARE(method1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(method1.methodType(), QMetaMethod::Method);
    QCOMPARE(method1.returnType(), QByteArray("void"));
    QCOMPARE(method1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QVERIFY(method1.parameterNames().isEmpty());
    QVERIFY(method1.tag().isEmpty());
    QCOMPARE(method1.access(), QMetaMethod::Public);
    QCOMPARE(method1.attributes(), 0);
    QCOMPARE(method1.revision(), 0);
    QCOMPARE(method1.index(), 0);
    QCOMPARE(builder.methodCount(), 1);

    // Add another method and check again.
    QMetaMethodBuilder method2 = builder.addMethod("bar(QString)", "int");
    QCOMPARE(method2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(method2.methodType(), QMetaMethod::Method);
    QCOMPARE(method2.returnType(), QByteArray("int"));
    QCOMPARE(method2.parameterTypes(), QList<QByteArray>() << "QString");
    QVERIFY(method2.parameterNames().isEmpty());
    QVERIFY(method2.tag().isEmpty());
    QCOMPARE(method2.access(), QMetaMethod::Public);
    QCOMPARE(method2.attributes(), 0);
    QCOMPARE(method2.revision(), 0);
    QCOMPARE(method2.index(), 1);
    QCOMPARE(builder.methodCount(), 2);

    // Perform index-based lookup.
    QCOMPARE(builder.indexOfMethod("foo(const QString&, int)"), 0);
    QCOMPARE(builder.indexOfMethod("bar(QString)"), 1);
    QCOMPARE(builder.indexOfMethod("baz()"), -1);

    // Modify the attributes on method1.
    method1.setReturnType("int");
    method1.setParameterNames(QList<QByteArray>() << "a" << "b");
    method1.setTag("tag");
    method1.setAccess(QMetaMethod::Private);
    method1.setAttributes(42);
    method1.setRevision(123);

    // Check that method1 is changed, but method2 is not.
    QCOMPARE(method1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(method1.methodType(), QMetaMethod::Method);
    QCOMPARE(method1.returnType(), QByteArray("int"));
    QCOMPARE(method1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QCOMPARE(method1.parameterNames(), QList<QByteArray>() << "a" << "b");
    QCOMPARE(method1.tag(), QByteArray("tag"));
    QCOMPARE(method1.access(), QMetaMethod::Private);
    QCOMPARE(method1.attributes(), 42);
    QCOMPARE(method1.revision(), 123);
    QCOMPARE(method1.index(), 0);
    QCOMPARE(method2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(method2.methodType(), QMetaMethod::Method);
    QCOMPARE(method2.returnType(), QByteArray("int"));
    QCOMPARE(method2.parameterTypes(), QList<QByteArray>() << "QString");
    QVERIFY(method2.parameterNames().isEmpty());
    QVERIFY(method2.tag().isEmpty());
    QCOMPARE(method2.access(), QMetaMethod::Public);
    QCOMPARE(method2.attributes(), 0);
    QCOMPARE(method2.revision(), 0);
    QCOMPARE(method2.index(), 1);
    QCOMPARE(builder.methodCount(), 2);

    // Modify the attributes on method2.
    method2.setReturnType("QString");
    method2.setParameterNames(QList<QByteArray>() << "c");
    method2.setTag("Q_FOO");
    method2.setAccess(QMetaMethod::Protected);
    method2.setAttributes(24);
    method2.setRevision(321);

    // This time check that only method2 changed.
    QCOMPARE(method1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(method1.methodType(), QMetaMethod::Method);
    QCOMPARE(method1.returnType(), QByteArray("int"));
    QCOMPARE(method1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QCOMPARE(method1.parameterNames(), QList<QByteArray>() << "a" << "b");
    QCOMPARE(method1.tag(), QByteArray("tag"));
    QCOMPARE(method1.access(), QMetaMethod::Private);
    QCOMPARE(method1.attributes(), 42);
    QCOMPARE(method1.revision(), 123);
    QCOMPARE(method1.index(), 0);
    QCOMPARE(method2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(method2.methodType(), QMetaMethod::Method);
    QCOMPARE(method2.returnType(), QByteArray("QString"));
    QCOMPARE(method2.parameterTypes(), QList<QByteArray>() << "QString");
    QCOMPARE(method2.parameterNames(), QList<QByteArray>() << "c");
    QCOMPARE(method2.tag(), QByteArray("Q_FOO"));
    QCOMPARE(method2.access(), QMetaMethod::Protected);
    QCOMPARE(method2.attributes(), 24);
    QCOMPARE(method2.revision(), 321);
    QCOMPARE(method2.index(), 1);
    QCOMPARE(builder.methodCount(), 2);

    // Remove method1 and check that method2 becomes index 0.
    builder.removeMethod(0);
    QCOMPARE(builder.methodCount(), 1);
    method2 = builder.method(0);
    QCOMPARE(method2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(method2.methodType(), QMetaMethod::Method);
    QCOMPARE(method2.returnType(), QByteArray("QString"));
    QCOMPARE(method2.parameterTypes(), QList<QByteArray>() << "QString");
    QCOMPARE(method2.parameterNames(), QList<QByteArray>() << "c");
    QCOMPARE(method2.tag(), QByteArray("Q_FOO"));
    QCOMPARE(method2.access(), QMetaMethod::Protected);
    QCOMPARE(method2.attributes(), 24);
    QCOMPARE(method2.revision(), 321);
    QCOMPARE(method2.index(), 0);

    // Perform index-based lookup again.
    QCOMPARE(builder.indexOfMethod("foo(const QString&, int)"), -1);
    QCOMPARE(builder.indexOfMethod("bar(QString)"), 0);
    QCOMPARE(builder.indexOfMethod("baz()"), -1);
    QCOMPARE(builder.method(0).signature(), QByteArray("bar(QString)"));
    QCOMPARE(builder.method(9).signature(), QByteArray());

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::Methods));
}

void tst_QMetaObjectBuilder::slot()
{
    QMetaObjectBuilder builder;

    // Add a slot and check its attributes.
    QMetaMethodBuilder method1 = builder.addSlot("foo(const QString&, int)");
    QCOMPARE(method1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(method1.methodType(), QMetaMethod::Slot);
    QCOMPARE(method1.returnType(), QByteArray("void"));
    QCOMPARE(method1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QVERIFY(method1.parameterNames().isEmpty());
    QVERIFY(method1.tag().isEmpty());
    QCOMPARE(method1.access(), QMetaMethod::Public);
    QCOMPARE(method1.attributes(), 0);
    QCOMPARE(method1.index(), 0);
    QCOMPARE(builder.methodCount(), 1);

    // Add another slot and check again.
    QMetaMethodBuilder method2 = builder.addSlot("bar(QString)");
    QCOMPARE(method2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(method2.methodType(), QMetaMethod::Slot);
    QCOMPARE(method2.returnType(), QByteArray("void"));
    QCOMPARE(method2.parameterTypes(), QList<QByteArray>() << "QString");
    QVERIFY(method2.parameterNames().isEmpty());
    QVERIFY(method2.tag().isEmpty());
    QCOMPARE(method2.access(), QMetaMethod::Public);
    QCOMPARE(method2.attributes(), 0);
    QCOMPARE(method2.index(), 1);
    QCOMPARE(builder.methodCount(), 2);

    // Perform index-based lookup
    QCOMPARE(builder.indexOfSlot("foo(const QString &, int)"), 0);
    QCOMPARE(builder.indexOfSlot("bar(QString)"), 1);
    QCOMPARE(builder.indexOfSlot("baz()"), -1);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::Methods));
}

void tst_QMetaObjectBuilder::signal()
{
    QMetaObjectBuilder builder;

    // Add a signal and check its attributes.
    QMetaMethodBuilder method1 = builder.addSignal("foo(const QString&, int)");
    QCOMPARE(method1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(method1.methodType(), QMetaMethod::Signal);
    QCOMPARE(method1.returnType(), QByteArray("void"));
    QCOMPARE(method1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QVERIFY(method1.parameterNames().isEmpty());
    QVERIFY(method1.tag().isEmpty());
    QCOMPARE(method1.access(), QMetaMethod::Public);
    QCOMPARE(method1.attributes(), 0);
    QCOMPARE(method1.index(), 0);
    QCOMPARE(builder.methodCount(), 1);

    // Add another signal and check again.
    QMetaMethodBuilder method2 = builder.addSignal("bar(QString)");
    QCOMPARE(method2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(method2.methodType(), QMetaMethod::Signal);
    QCOMPARE(method2.returnType(), QByteArray("void"));
    QCOMPARE(method2.parameterTypes(), QList<QByteArray>() << "QString");
    QVERIFY(method2.parameterNames().isEmpty());
    QVERIFY(method2.tag().isEmpty());
    QCOMPARE(method2.access(), QMetaMethod::Public);
    QCOMPARE(method2.attributes(), 0);
    QCOMPARE(method2.index(), 1);
    QCOMPARE(builder.methodCount(), 2);

    // Perform index-based lookup
    QCOMPARE(builder.indexOfSignal("foo(const QString &, int)"), 0);
    QCOMPARE(builder.indexOfSignal("bar(QString)"), 1);
    QCOMPARE(builder.indexOfSignal("baz()"), -1);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::Methods));
}

void tst_QMetaObjectBuilder::constructor()
{
    QMetaObjectBuilder builder;

    // Add a constructor and check its attributes.
    QMetaMethodBuilder ctor1 = builder.addConstructor("foo(const QString&, int)");
    QCOMPARE(ctor1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(ctor1.methodType(), QMetaMethod::Constructor);
    QVERIFY(ctor1.returnType().isEmpty());
    QCOMPARE(ctor1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QVERIFY(ctor1.parameterNames().isEmpty());
    QVERIFY(ctor1.tag().isEmpty());
    QCOMPARE(ctor1.access(), QMetaMethod::Public);
    QCOMPARE(ctor1.attributes(), 0);
    QCOMPARE(ctor1.index(), 0);
    QCOMPARE(builder.constructorCount(), 1);

    // Add another constructor and check again.
    QMetaMethodBuilder ctor2 = builder.addConstructor("bar(QString)");
    QCOMPARE(ctor2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(ctor2.methodType(), QMetaMethod::Constructor);
    QVERIFY(ctor2.returnType().isEmpty());
    QCOMPARE(ctor2.parameterTypes(), QList<QByteArray>() << "QString");
    QVERIFY(ctor2.parameterNames().isEmpty());
    QVERIFY(ctor2.tag().isEmpty());
    QCOMPARE(ctor2.access(), QMetaMethod::Public);
    QCOMPARE(ctor2.attributes(), 0);
    QCOMPARE(ctor2.index(), 1);
    QCOMPARE(builder.constructorCount(), 2);

    // Perform index-based lookup.
    QCOMPARE(builder.indexOfConstructor("foo(const QString&, int)"), 0);
    QCOMPARE(builder.indexOfConstructor("bar(QString)"), 1);
    QCOMPARE(builder.indexOfConstructor("baz()"), -1);
    QCOMPARE(builder.constructor(1).signature(), QByteArray("bar(QString)"));
    QCOMPARE(builder.constructor(9).signature(), QByteArray());

    // Modify the attributes on ctor1.
    ctor1.setReturnType("int");
    ctor1.setParameterNames(QList<QByteArray>() << "a" << "b");
    ctor1.setTag("tag");
    ctor1.setAccess(QMetaMethod::Private);
    ctor1.setAttributes(42);

    // Check that ctor1 is changed, but ctor2 is not.
    QCOMPARE(ctor1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(ctor1.methodType(), QMetaMethod::Constructor);
    QCOMPARE(ctor1.returnType(), QByteArray("int"));
    QCOMPARE(ctor1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QCOMPARE(ctor1.parameterNames(), QList<QByteArray>() << "a" << "b");
    QCOMPARE(ctor1.tag(), QByteArray("tag"));
    QCOMPARE(ctor1.access(), QMetaMethod::Private);
    QCOMPARE(ctor1.attributes(), 42);
    QCOMPARE(ctor1.index(), 0);
    QCOMPARE(ctor2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(ctor2.methodType(), QMetaMethod::Constructor);
    QVERIFY(ctor2.returnType().isEmpty());
    QCOMPARE(ctor2.parameterTypes(), QList<QByteArray>() << "QString");
    QVERIFY(ctor2.parameterNames().isEmpty());
    QVERIFY(ctor2.tag().isEmpty());
    QCOMPARE(ctor2.access(), QMetaMethod::Public);
    QCOMPARE(ctor2.attributes(), 0);
    QCOMPARE(ctor2.index(), 1);
    QCOMPARE(builder.constructorCount(), 2);

    // Modify the attributes on ctor2.
    ctor2.setReturnType("QString");
    ctor2.setParameterNames(QList<QByteArray>() << "c");
    ctor2.setTag("Q_FOO");
    ctor2.setAccess(QMetaMethod::Protected);
    ctor2.setAttributes(24);

    // This time check that only ctor2 changed.
    QCOMPARE(ctor1.signature(), QByteArray("foo(QString,int)"));
    QCOMPARE(ctor1.methodType(), QMetaMethod::Constructor);
    QCOMPARE(ctor1.returnType(), QByteArray("int"));
    QCOMPARE(ctor1.parameterTypes(), QList<QByteArray>() << "QString" << "int");
    QCOMPARE(ctor1.parameterNames(), QList<QByteArray>() << "a" << "b");
    QCOMPARE(ctor1.tag(), QByteArray("tag"));
    QCOMPARE(ctor1.access(), QMetaMethod::Private);
    QCOMPARE(ctor1.attributes(), 42);
    QCOMPARE(ctor1.index(), 0);
    QCOMPARE(ctor2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(ctor2.methodType(), QMetaMethod::Constructor);
    QCOMPARE(ctor2.returnType(), QByteArray("QString"));
    QCOMPARE(ctor2.parameterTypes(), QList<QByteArray>() << "QString");
    QCOMPARE(ctor2.parameterNames(), QList<QByteArray>() << "c");
    QCOMPARE(ctor2.tag(), QByteArray("Q_FOO"));
    QCOMPARE(ctor2.access(), QMetaMethod::Protected);
    QCOMPARE(ctor2.attributes(), 24);
    QCOMPARE(ctor2.index(), 1);
    QCOMPARE(builder.constructorCount(), 2);

    // Remove ctor1 and check that ctor2 becomes index 0.
    builder.removeConstructor(0);
    QCOMPARE(builder.constructorCount(), 1);
    ctor2 = builder.constructor(0);
    QCOMPARE(ctor2.signature(), QByteArray("bar(QString)"));
    QCOMPARE(ctor2.methodType(), QMetaMethod::Constructor);
    QCOMPARE(ctor2.returnType(), QByteArray("QString"));
    QCOMPARE(ctor2.parameterTypes(), QList<QByteArray>() << "QString");
    QCOMPARE(ctor2.parameterNames(), QList<QByteArray>() << "c");
    QCOMPARE(ctor2.tag(), QByteArray("Q_FOO"));
    QCOMPARE(ctor2.access(), QMetaMethod::Protected);
    QCOMPARE(ctor2.attributes(), 24);
    QCOMPARE(ctor2.index(), 0);

    // Perform index-based lookup again.
    QCOMPARE(builder.indexOfConstructor("foo(const QString&, int)"), -1);
    QCOMPARE(builder.indexOfConstructor("bar(QString)"), 0);
    QCOMPARE(builder.indexOfConstructor("baz()"), -1);

    // Add constructor from prototype
    QMetaMethod prototype = SomethingOfEverything::staticMetaObject.constructor(0);
    QMetaMethodBuilder prototypeConstructor = builder.addMethod(prototype);
    QCOMPARE(builder.constructorCount(), 2);

    QCOMPARE(prototypeConstructor.signature(), QByteArray("SomethingOfEverything()"));
    QCOMPARE(prototypeConstructor.methodType(), QMetaMethod::Constructor);
    QCOMPARE(prototypeConstructor.returnType(), QByteArray());
    QVERIFY(prototypeConstructor.parameterTypes().isEmpty());
    QCOMPARE(prototypeConstructor.access(), QMetaMethod::Public);
    QCOMPARE(prototypeConstructor.index(), 1);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::Constructors));
}

void tst_QMetaObjectBuilder::property()
{
    QMetaObjectBuilder builder;

    // Null property builder
    QMetaPropertyBuilder nullProp;
    QCOMPARE(nullProp.name(), QByteArray());
    QCOMPARE(nullProp.type(), QByteArray());
    QVERIFY(!nullProp.hasNotifySignal());
    QVERIFY(!nullProp.isReadable());
    QVERIFY(!nullProp.isWritable());
    QVERIFY(!nullProp.isResettable());
    QVERIFY(!nullProp.isDesignable());
    QVERIFY(!nullProp.isScriptable());
    QVERIFY(!nullProp.isStored());
    QVERIFY(!nullProp.isEditable());
    QVERIFY(!nullProp.isUser());
    QVERIFY(!nullProp.hasStdCppSet());
    QVERIFY(!nullProp.isEnumOrFlag());
    QVERIFY(!nullProp.isConstant());
    QVERIFY(!nullProp.isFinal());
    QCOMPARE(nullProp.index(), 0);
    QCOMPARE(nullProp.revision(), 0);

    // Add a property and check its attributes.
    QMetaPropertyBuilder prop1 = builder.addProperty("foo", "const QString &");
    QCOMPARE(prop1.name(), QByteArray("foo"));
    QCOMPARE(prop1.type(), QByteArray("QString"));
    QVERIFY(!prop1.hasNotifySignal());
    QVERIFY(prop1.isReadable());
    QVERIFY(prop1.isWritable());
    QVERIFY(!prop1.isResettable());
    QVERIFY(!prop1.isDesignable());
    QVERIFY(prop1.isScriptable());
    QVERIFY(!prop1.isStored());
    QVERIFY(!prop1.isEditable());
    QVERIFY(!prop1.isUser());
    QVERIFY(!prop1.hasStdCppSet());
    QVERIFY(!prop1.isEnumOrFlag());
    QVERIFY(!prop1.isConstant());
    QVERIFY(!prop1.isFinal());
    QCOMPARE(prop1.revision(), 0);
    QCOMPARE(prop1.index(), 0);
    QCOMPARE(builder.propertyCount(), 1);

    // Add another property and check again.
    QMetaPropertyBuilder prop2 = builder.addProperty("bar", "int");
    QCOMPARE(prop2.name(), QByteArray("bar"));
    QCOMPARE(prop2.type(), QByteArray("int"));
    QVERIFY(!prop2.hasNotifySignal());
    QVERIFY(prop2.isReadable());
    QVERIFY(prop2.isWritable());
    QVERIFY(!prop2.isResettable());
    QVERIFY(!prop2.isDesignable());
    QVERIFY(prop2.isScriptable());
    QVERIFY(!prop2.isStored());
    QVERIFY(!prop2.isEditable());
    QVERIFY(!prop2.isUser());
    QVERIFY(!prop2.hasStdCppSet());
    QVERIFY(!prop2.isEnumOrFlag());
    QVERIFY(!prop2.isConstant());
    QVERIFY(!prop2.isFinal());
    QCOMPARE(prop2.revision(), 0);
    QCOMPARE(prop2.index(), 1);
    QCOMPARE(builder.propertyCount(), 2);

    // Perform index-based lookup.
    QCOMPARE(builder.indexOfProperty("foo"), 0);
    QCOMPARE(builder.indexOfProperty("bar"), 1);
    QCOMPARE(builder.indexOfProperty("baz"), -1);
    QCOMPARE(builder.property(1).name(), QByteArray("bar"));
    QCOMPARE(builder.property(9).name(), QByteArray());

    // Modify the attributes on prop1.
    prop1.setReadable(false);
    prop1.setWritable(false);
    prop1.setResettable(true);
    prop1.setDesignable(true);
    prop1.setScriptable(false);
    prop1.setStored(true);
    prop1.setEditable(true);
    prop1.setUser(true);
    prop1.setStdCppSet(true);
    prop1.setEnumOrFlag(true);
    prop1.setConstant(true);
    prop1.setFinal(true);
    prop1.setRevision(123);

    // Check that prop1 is changed, but prop2 is not.
    QCOMPARE(prop1.name(), QByteArray("foo"));
    QCOMPARE(prop1.type(), QByteArray("QString"));
    QVERIFY(!prop1.isReadable());
    QVERIFY(!prop1.isWritable());
    QVERIFY(prop1.isResettable());
    QVERIFY(prop1.isDesignable());
    QVERIFY(!prop1.isScriptable());
    QVERIFY(prop1.isStored());
    QVERIFY(prop1.isEditable());
    QVERIFY(prop1.isUser());
    QVERIFY(prop1.hasStdCppSet());
    QVERIFY(prop1.isEnumOrFlag());
    QVERIFY(prop1.isConstant());
    QVERIFY(prop1.isFinal());
    QCOMPARE(prop1.revision(), 123);
    QVERIFY(prop2.isReadable());
    QVERIFY(prop2.isWritable());
    QCOMPARE(prop2.name(), QByteArray("bar"));
    QCOMPARE(prop2.type(), QByteArray("int"));
    QVERIFY(!prop2.isResettable());
    QVERIFY(!prop2.isDesignable());
    QVERIFY(prop2.isScriptable());
    QVERIFY(!prop2.isStored());
    QVERIFY(!prop2.isEditable());
    QVERIFY(!prop2.isUser());
    QVERIFY(!prop2.hasStdCppSet());
    QVERIFY(!prop2.isEnumOrFlag());
    QVERIFY(!prop2.isConstant());
    QVERIFY(!prop2.isFinal());
    QCOMPARE(prop2.revision(), 0);

    // Remove prop1 and check that prop2 becomes index 0.
    builder.removeProperty(0);
    QCOMPARE(builder.propertyCount(), 1);
    prop2 = builder.property(0);
    QCOMPARE(prop2.name(), QByteArray("bar"));
    QCOMPARE(prop2.type(), QByteArray("int"));
    QVERIFY(!prop2.isResettable());
    QVERIFY(!prop2.isDesignable());
    QVERIFY(prop2.isScriptable());
    QVERIFY(!prop2.isStored());
    QVERIFY(!prop2.isEditable());
    QVERIFY(!prop2.isUser());
    QVERIFY(!prop2.hasStdCppSet());
    QVERIFY(!prop2.isEnumOrFlag());
    QVERIFY(!prop2.isConstant());
    QVERIFY(!prop2.isFinal());
    QCOMPARE(prop2.revision(), 0);
    QCOMPARE(prop2.index(), 0);

    // Perform index-based lookup again.
    QCOMPARE(builder.indexOfProperty("foo"), -1);
    QCOMPARE(builder.indexOfProperty("bar"), 0);
    QCOMPARE(builder.indexOfProperty("baz"), -1);

    // Check for side-effects between the flags on prop2.
    // Setting a flag to true shouldn't set any of the others to true.
    // This checks for cut-and-paste bugs in the implementation where
    // the flag code was pasted but the flag name was not changed.
#define CLEAR_FLAGS() \
        do { \
            prop2.setReadable(false); \
            prop2.setWritable(false); \
            prop2.setResettable(false); \
            prop2.setDesignable(false); \
            prop2.setScriptable(false); \
            prop2.setStored(false); \
            prop2.setEditable(false); \
            prop2.setUser(false); \
            prop2.setStdCppSet(false); \
            prop2.setEnumOrFlag(false); \
            prop2.setConstant(false); \
            prop2.setFinal(false); \
        } while (0)
#define COUNT_FLAGS() \
        ((prop2.isReadable() ? 1 : 0) + \
         (prop2.isWritable() ? 1 : 0) + \
         (prop2.isResettable() ? 1 : 0) + \
         (prop2.isDesignable() ? 1 : 0) + \
         (prop2.isScriptable() ? 1 : 0) + \
         (prop2.isStored() ? 1 : 0) + \
         (prop2.isEditable() ? 1 : 0) + \
         (prop2.isUser() ? 1 : 0) + \
         (prop2.hasStdCppSet() ? 1 : 0) + \
         (prop2.isEnumOrFlag() ? 1 : 0) + \
         (prop2.isConstant() ? 1 : 0) + \
         (prop2.isFinal() ? 1 : 0))
#define CHECK_FLAG(setFunc,isFunc) \
        do { \
            CLEAR_FLAGS(); \
            QCOMPARE(COUNT_FLAGS(), 0); \
            prop2.setFunc(true); \
            QVERIFY(prop2.isFunc()); \
            QCOMPARE(COUNT_FLAGS(), 1); \
        } while (0)
    CHECK_FLAG(setReadable, isReadable);
    CHECK_FLAG(setWritable, isWritable);
    CHECK_FLAG(setResettable, isResettable);
    CHECK_FLAG(setDesignable, isDesignable);
    CHECK_FLAG(setScriptable, isScriptable);
    CHECK_FLAG(setStored, isStored);
    CHECK_FLAG(setEditable, isEditable);
    CHECK_FLAG(setUser, isUser);
    CHECK_FLAG(setStdCppSet, hasStdCppSet);
    CHECK_FLAG(setEnumOrFlag, isEnumOrFlag);
    CHECK_FLAG(setConstant, isConstant);
    CHECK_FLAG(setFinal, isFinal);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::Properties));

    // Add property from prototype
    QMetaProperty prototype = SomethingOfEverything::staticMetaObject.property(1);
    QVERIFY(prototype.name() == QByteArray("prop"));
    QMetaPropertyBuilder prototypeProp = builder.addProperty(prototype);
    QCOMPARE(prototypeProp.name(), QByteArray("prop"));
    QVERIFY(prototypeProp.hasNotifySignal());
    QCOMPARE(prototypeProp.notifySignal().signature(), QByteArray("propChanged(QString)"));
    QCOMPARE(builder.methodCount(), 1);
    QCOMPARE(builder.method(0).signature(), QByteArray("propChanged(QString)"));
}

void tst_QMetaObjectBuilder::variantProperty()
{
    QMetaObjectBuilder builder;
    builder.addProperty("variant", "const QVariant &");
    QMetaObject *meta = builder.toMetaObject();

    QMetaProperty prop = meta->property(meta->propertyOffset());
    QCOMPARE(QMetaType::Type(prop.type()), QMetaType::QVariant);
    QCOMPARE(QMetaType::Type(prop.userType()), QMetaType::QVariant);
    QCOMPARE(QByteArray(prop.typeName()), QByteArray("QVariant"));

    free(meta);
}

void tst_QMetaObjectBuilder::notifySignal()
{
    QMetaObjectBuilder builder;

    QMetaPropertyBuilder prop = builder.addProperty("foo", "const QString &");
    builder.addSlot("setFoo(QString)");
    QMetaMethodBuilder notify = builder.addSignal("fooChanged(QString)");

    QVERIFY(!prop.hasNotifySignal());
    QCOMPARE(prop.notifySignal().index(), 0);

    prop.setNotifySignal(notify);
    QVERIFY(prop.hasNotifySignal());
    QCOMPARE(prop.notifySignal().index(), 1);

    prop.setNotifySignal(QMetaMethodBuilder());
    QVERIFY(!prop.hasNotifySignal());
    QCOMPARE(prop.notifySignal().index(), 0);

    prop.setNotifySignal(notify);
    prop.removeNotifySignal();
    QVERIFY(!prop.hasNotifySignal());
    QCOMPARE(prop.notifySignal().index(), 0);

    QCOMPARE(builder.methodCount(), 2);
    QCOMPARE(builder.propertyCount(), 1);

    // Check that nothing else changed except methods and properties.
    QVERIFY(checkForSideEffects
        (builder, QMetaObjectBuilder::Methods | QMetaObjectBuilder::Properties));
}

void tst_QMetaObjectBuilder::enumerator()
{
    QMetaObjectBuilder builder;

    // Add an enumerator and check its attributes.
    QMetaEnumBuilder enum1 = builder.addEnumerator("foo");
    QCOMPARE(enum1.name(), QByteArray("foo"));
    QVERIFY(!enum1.isFlag());
    QCOMPARE(enum1.keyCount(), 0);
    QCOMPARE(enum1.index(), 0);
    QCOMPARE(builder.enumeratorCount(), 1);

    // Add another enumerator and check again.
    QMetaEnumBuilder enum2 = builder.addEnumerator("bar");
    QCOMPARE(enum2.name(), QByteArray("bar"));
    QVERIFY(!enum2.isFlag());
    QCOMPARE(enum2.keyCount(), 0);
    QCOMPARE(enum2.index(), 1);
    QCOMPARE(builder.enumeratorCount(), 2);

    // Perform index-based lookup.
    QCOMPARE(builder.indexOfEnumerator("foo"), 0);
    QCOMPARE(builder.indexOfEnumerator("bar"), 1);
    QCOMPARE(builder.indexOfEnumerator("baz"), -1);
    QCOMPARE(builder.enumerator(1).name(), QByteArray("bar"));
    QCOMPARE(builder.enumerator(9).name(), QByteArray());

    // Modify the attributes on enum1.
    enum1.setIsFlag(true);
    QCOMPARE(enum1.addKey("ABC", 0), 0);
    QCOMPARE(enum1.addKey("DEF", 1), 1);
    QCOMPARE(enum1.addKey("GHI", -1), 2);

    // Check that enum1 is changed, but enum2 is not.
    QCOMPARE(enum1.name(), QByteArray("foo"));
    QVERIFY(enum1.isFlag());
    QCOMPARE(enum1.keyCount(), 3);
    QCOMPARE(enum1.index(), 0);
    QCOMPARE(enum1.key(0), QByteArray("ABC"));
    QCOMPARE(enum1.key(1), QByteArray("DEF"));
    QCOMPARE(enum1.key(2), QByteArray("GHI"));
    QCOMPARE(enum1.key(3), QByteArray());
    QCOMPARE(enum1.value(0), 0);
    QCOMPARE(enum1.value(1), 1);
    QCOMPARE(enum1.value(2), -1);
    QCOMPARE(enum2.name(), QByteArray("bar"));
    QVERIFY(!enum2.isFlag());
    QCOMPARE(enum2.keyCount(), 0);
    QCOMPARE(enum2.index(), 1);

    // Modify the attributes on enum2.
    enum2.setIsFlag(true);
    QCOMPARE(enum2.addKey("XYZ", 10), 0);
    QCOMPARE(enum2.addKey("UVW", 19), 1);

    // This time check that only method2 changed.
    QCOMPARE(enum1.name(), QByteArray("foo"));
    QVERIFY(enum1.isFlag());
    QCOMPARE(enum1.keyCount(), 3);
    QCOMPARE(enum1.index(), 0);
    QCOMPARE(enum1.key(0), QByteArray("ABC"));
    QCOMPARE(enum1.key(1), QByteArray("DEF"));
    QCOMPARE(enum1.key(2), QByteArray("GHI"));
    QCOMPARE(enum1.key(3), QByteArray());
    QCOMPARE(enum1.value(0), 0);
    QCOMPARE(enum1.value(1), 1);
    QCOMPARE(enum1.value(2), -1);
    QCOMPARE(enum2.name(), QByteArray("bar"));
    QVERIFY(enum2.isFlag());
    QCOMPARE(enum2.keyCount(), 2);
    QCOMPARE(enum2.index(), 1);
    QCOMPARE(enum2.key(0), QByteArray("XYZ"));
    QCOMPARE(enum2.key(1), QByteArray("UVW"));
    QCOMPARE(enum2.key(2), QByteArray());
    QCOMPARE(enum2.value(0), 10);
    QCOMPARE(enum2.value(1), 19);

    // Remove enum1 key
    enum1.removeKey(2);
    QCOMPARE(enum1.name(), QByteArray("foo"));
    QVERIFY(enum1.isFlag());
    QCOMPARE(enum1.keyCount(), 2);
    QCOMPARE(enum1.index(), 0);
    QCOMPARE(enum1.key(0), QByteArray("ABC"));
    QCOMPARE(enum1.key(1), QByteArray("DEF"));
    QCOMPARE(enum1.key(2), QByteArray());
    QCOMPARE(enum1.value(0), 0);
    QCOMPARE(enum1.value(1), 1);
    QCOMPARE(enum1.value(2), -1);
    QCOMPARE(enum2.name(), QByteArray("bar"));
    QVERIFY(enum2.isFlag());
    QCOMPARE(enum2.keyCount(), 2);
    QCOMPARE(enum2.index(), 1);
    QCOMPARE(enum2.key(0), QByteArray("XYZ"));
    QCOMPARE(enum2.key(1), QByteArray("UVW"));
    QCOMPARE(enum2.key(2), QByteArray());
    QCOMPARE(enum2.value(0), 10);
    QCOMPARE(enum2.value(1), 19);

    // Remove enum1 and check that enum2 becomes index 0.
    builder.removeEnumerator(0);
    QCOMPARE(builder.enumeratorCount(), 1);
    enum2 = builder.enumerator(0);
    QCOMPARE(enum2.name(), QByteArray("bar"));
    QVERIFY(enum2.isFlag());
    QCOMPARE(enum2.keyCount(), 2);
    QCOMPARE(enum2.index(), 0);
    QCOMPARE(enum2.key(0), QByteArray("XYZ"));
    QCOMPARE(enum2.key(1), QByteArray("UVW"));
    QCOMPARE(enum2.key(2), QByteArray());
    QCOMPARE(enum2.value(0), 10);
    QCOMPARE(enum2.value(1), 19);

    // Perform index-based lookup again.
    QCOMPARE(builder.indexOfEnumerator("foo"), -1);
    QCOMPARE(builder.indexOfEnumerator("bar"), 0);
    QCOMPARE(builder.indexOfEnumerator("baz"), -1);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::Enumerators));
}

void tst_QMetaObjectBuilder::classInfo()
{
    QMetaObjectBuilder builder;

    // Add two items of class information and check their attributes.
    QCOMPARE(builder.addClassInfo("foo", "value1"), 0);
    QCOMPARE(builder.addClassInfo("bar", "value2"), 1);
    QCOMPARE(builder.classInfoName(0), QByteArray("foo"));
    QCOMPARE(builder.classInfoValue(0), QByteArray("value1"));
    QCOMPARE(builder.classInfoName(1), QByteArray("bar"));
    QCOMPARE(builder.classInfoValue(1), QByteArray("value2"));
    QCOMPARE(builder.classInfoName(9), QByteArray());
    QCOMPARE(builder.classInfoValue(9), QByteArray());
    QCOMPARE(builder.classInfoCount(), 2);

    // Perform index-based lookup.
    QCOMPARE(builder.indexOfClassInfo("foo"), 0);
    QCOMPARE(builder.indexOfClassInfo("bar"), 1);
    QCOMPARE(builder.indexOfClassInfo("baz"), -1);

    // Remove the first one and check again.
    builder.removeClassInfo(0);
    QCOMPARE(builder.classInfoName(0), QByteArray("bar"));
    QCOMPARE(builder.classInfoValue(0), QByteArray("value2"));
    QCOMPARE(builder.classInfoCount(), 1);

    // Perform index-based lookup again.
    QCOMPARE(builder.indexOfClassInfo("foo"), -1);
    QCOMPARE(builder.indexOfClassInfo("bar"), 0);
    QCOMPARE(builder.indexOfClassInfo("baz"), -1);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::ClassInfos));
}

void tst_QMetaObjectBuilder::relatedMetaObject()
{
    QMetaObjectBuilder builder;

    // Add two related meta objects and check their attributes.
    QCOMPARE(builder.addRelatedMetaObject(&QObject::staticMetaObject), 0);
    QCOMPARE(builder.addRelatedMetaObject(&staticMetaObject), 1);
    QCOMPARE(builder.relatedMetaObject(0), &QObject::staticMetaObject);
    QCOMPARE(builder.relatedMetaObject(1), &staticMetaObject);
    QCOMPARE(builder.relatedMetaObjectCount(), 2);

    // Remove the first one and check again.
    builder.removeRelatedMetaObject(0);
    QCOMPARE(builder.relatedMetaObject(0), &staticMetaObject);
    QCOMPARE(builder.relatedMetaObjectCount(), 1);

    // Check that nothing else changed.
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::RelatedMetaObjects));
}

static void smetacall(QObject *, QMetaObject::Call, int, void **)
{
    return;
}

void tst_QMetaObjectBuilder::staticMetacall()
{
    QMetaObjectBuilder builder;
    QVERIFY(!builder.staticMetacallFunction());
    builder.setStaticMetacallFunction(smetacall);
    QVERIFY(builder.staticMetacallFunction() == smetacall);
    QVERIFY(checkForSideEffects(builder, QMetaObjectBuilder::StaticMetacall));
}

// Copy the entire contents of a static QMetaObject and then check
// that QMetaObjectBuilder will produce an exact copy as output.
void tst_QMetaObjectBuilder::copyMetaObject()
{
    QMetaObjectBuilder builder(&QObject::staticMetaObject);
    QMetaObject *meta = builder.toMetaObject();
    QVERIFY(sameMetaObject(meta, &QObject::staticMetaObject));
    free(meta);

    QMetaObjectBuilder builder2(&staticMetaObject);
    meta = builder2.toMetaObject();
    QVERIFY(sameMetaObject(meta, &staticMetaObject));
    free(meta);

    QMetaObjectBuilder builder3(&SomethingOfEverything::staticMetaObject);
    meta = builder3.toMetaObject();
    QVERIFY(sameMetaObject(meta, &SomethingOfEverything::staticMetaObject));
    free(meta);
}

// Serialize and deserialize a meta object and check that
// it round-trips to the exact same value.
void tst_QMetaObjectBuilder::serialize()
{
    // Full QMetaObjectBuilder
    {
    QMetaObjectBuilder builder(&SomethingOfEverything::staticMetaObject);
    QMetaObject *meta = builder.toMetaObject();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly | QIODevice::Append);
    builder.serialize(stream);

    QMetaObjectBuilder builder2;
    QDataStream stream2(data);
    QMap<QByteArray, const QMetaObject *> references;
    references.insert(QByteArray("QLocale"), &QLocale::staticMetaObject);
    builder2.deserialize(stream2, references);
    builder2.setStaticMetacallFunction(builder.staticMetacallFunction());
    QMetaObject *meta2 = builder2.toMetaObject();

    QVERIFY(sameMetaObject(meta, meta2));
    free(meta);
    free(meta2);
    }

    // Partial QMetaObjectBuilder
    {
    QMetaObjectBuilder builder;
    builder.setClassName("Test");
    builder.addProperty("foo", "int");

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly | QIODevice::Append);
    builder.serialize(stream);

    QMetaObjectBuilder builder2;
    QDataStream stream2(data);
    builder2.deserialize(stream2, QMap<QByteArray, const QMetaObject *>());

    QCOMPARE(builder.superClass(), builder2.superClass());
    QCOMPARE(builder.className(), builder2.className());
    QCOMPARE(builder.propertyCount(), builder2.propertyCount());
    QCOMPARE(builder.property(0).name(), builder2.property(0).name());
    QCOMPARE(builder.property(0).type(), builder2.property(0).type());
    }
}

void tst_QMetaObjectBuilder::relocatableData()
{
    QMetaObjectBuilder builder;
    builder.setClassName("TestObject");

    QMetaMethodBuilder intPropChanged = builder.addSignal("intPropChanged(int)");
    intPropChanged.setParameterNames(QList<QByteArray>() << "newIntPropValue");

    QMetaPropertyBuilder prop = builder.addProperty("intProp", "int");
    prop.setNotifySignal(intPropChanged);

    QMetaMethodBuilder voidSlotInt = builder.addSlot("voidSlotInt(int)");
    voidSlotInt.setParameterNames(QList<QByteArray>() << "slotIntArg");

    QMetaMethodBuilder listInvokableQRealQString = builder.addMethod("listInvokableQRealQString(qreal,QString)");
    listInvokableQRealQString.setReturnType("QVariantList");
    listInvokableQRealQString.setParameterNames(QList<QByteArray>() << "qrealArg" << "qstringArg");

    bool ok = false;
    QByteArray data = builder.toRelocatableData(&ok);
    QVERIFY(ok);

    QMetaObjectBuilder builder2;
    QMetaObject meta2;
    builder2.fromRelocatableData(&meta2, &QObject::staticMetaObject, data);

    QMetaObject *meta = builder.toMetaObject();

    QVERIFY(sameMetaObject(meta, &meta2));

    QVERIFY(!meta2.d.extradata);
    QVERIFY(!meta2.d.relatedMetaObjects);
    QVERIFY(!meta2.d.static_metacall);

    free(meta);
}


// Check that removing a method updates notify signals appropriately
void tst_QMetaObjectBuilder::removeNotifySignal()
{
    QMetaObjectBuilder builder;

    builder.addSignal("foo(const QString&, int)");
    QMetaMethodBuilder method = builder.addSignal("bar(QString)");

    // Setup property
    QMetaPropertyBuilder prop = builder.addProperty("prop", "const QString &");
    prop.setNotifySignal(method);
    QVERIFY(prop.hasNotifySignal());
    QCOMPARE(prop.notifySignal().index(), 1);

    // Remove non-notify signal
    builder.removeMethod(0);
    QVERIFY(prop.hasNotifySignal());
    QCOMPARE(prop.notifySignal().index(), 0);

    // Remove notify signal
    builder.removeMethod(0);
    QVERIFY(!prop.hasNotifySignal());
}

// Check that the only changes to a "builder" relative to the default
// state is specified by "members".
bool tst_QMetaObjectBuilder::checkForSideEffects
        (const QMetaObjectBuilder& builder,
         QMetaObjectBuilder::AddMembers members)
{
    if ((members & QMetaObjectBuilder::ClassName) == 0) {
        if (!builder.className().isEmpty())
            return false;
    }

    if ((members & QMetaObjectBuilder::SuperClass) == 0) {
        if (builder.superClass() != &QObject::staticMetaObject)
            return false;
    }

    if ((members & QMetaObjectBuilder::Methods) == 0) {
        if (builder.methodCount() != 0)
            return false;
    }

    if ((members & QMetaObjectBuilder::Constructors) == 0) {
        if (builder.constructorCount() != 0)
            return false;
    }

    if ((members & QMetaObjectBuilder::Properties) == 0) {
        if (builder.propertyCount() != 0)
            return false;
    }

    if ((members & QMetaObjectBuilder::Enumerators) == 0) {
        if (builder.enumeratorCount() != 0)
            return false;
    }

    if ((members & QMetaObjectBuilder::ClassInfos) == 0) {
        if (builder.classInfoCount() != 0)
            return false;
    }

    if ((members & QMetaObjectBuilder::RelatedMetaObjects) == 0) {
        if (builder.relatedMetaObjectCount() != 0)
            return false;
    }

    if ((members & QMetaObjectBuilder::StaticMetacall) == 0) {
        if (builder.staticMetacallFunction() != 0)
            return false;
    }

    return true;
}

static bool sameMethod(const QMetaMethod& method1, const QMetaMethod& method2)
{
    if (method1.methodSignature() != method2.methodSignature())
        return false;

    if (QByteArray(method1.typeName()) != QByteArray(method2.typeName()))
        return false;

    if (method1.parameterTypes() != method2.parameterTypes())
        return false;

    if (method1.parameterNames() != method2.parameterNames())
        return false;

    if (QByteArray(method1.tag()) != QByteArray(method2.tag()))
        return false;

    if (method1.access() != method2.access())
        return false;

    if (method1.methodType() != method2.methodType())
        return false;

    if (method1.attributes() != method2.attributes())
        return false;

    if (method1.revision() != method2.revision())
        return false;

    return true;
}

static bool sameProperty(const QMetaProperty& prop1, const QMetaProperty& prop2)
{
    if (QByteArray(prop1.name()) != QByteArray(prop2.name()))
        return false;

    if (QByteArray(prop1.typeName()) != QByteArray(prop2.typeName()))
        return false;

    if (prop1.isReadable() != prop2.isReadable() ||
        prop1.isWritable() != prop2.isWritable() ||
        prop1.isResettable() != prop2.isResettable() ||
        prop1.isDesignable() != prop2.isDesignable() ||
        prop1.isScriptable() != prop2.isScriptable() ||
        prop1.isStored() != prop2.isStored() ||
        prop1.isEditable() != prop2.isEditable() ||
        prop1.isUser() != prop2.isUser() ||
        prop1.isFlagType() != prop2.isFlagType() ||
        prop1.isEnumType() != prop2.isEnumType() ||
        prop1.hasNotifySignal() != prop2.hasNotifySignal() ||
        prop1.hasStdCppSet() != prop2.hasStdCppSet())
        return false;

    if (prop1.hasNotifySignal()) {
        if (prop1.notifySignalIndex() != prop2.notifySignalIndex())
            return false;
    }

    if (prop1.revision() != prop2.revision())
        return false;

    return true;
}

static bool sameEnumerator(const QMetaEnum& enum1, const QMetaEnum& enum2)
{
    if (QByteArray(enum1.name()) != QByteArray(enum2.name()))
        return false;

    if (enum1.isFlag() != enum2.isFlag())
        return false;

    if (enum1.keyCount() != enum2.keyCount())
        return false;

    for (int index = 0; index < enum1.keyCount(); ++index) {
        if (QByteArray(enum1.key(index)) != QByteArray(enum2.key(index)))
            return false;
        if (enum1.value(index) != enum2.value(index))
            return false;
    }

    if (QByteArray(enum1.scope()) != QByteArray(enum2.scope()))
        return false;

    return true;
}

// Determine if two meta objects are identical.
bool tst_QMetaObjectBuilder::sameMetaObject
        (const QMetaObject *meta1, const QMetaObject *meta2)
{
    int index;

    if (strcmp(meta1->className(), meta2->className()) != 0)
        return false;

    if (meta1->superClass() != meta2->superClass())
        return false;

    if (meta1->constructorCount() != meta2->constructorCount() ||
        meta1->methodCount() != meta2->methodCount() ||
        meta1->enumeratorCount() != meta2->enumeratorCount() ||
        meta1->propertyCount() != meta2->propertyCount() ||
        meta1->classInfoCount() != meta2->classInfoCount())
        return false;

    for (index = 0; index < meta1->constructorCount(); ++index) {
        if (!sameMethod(meta1->constructor(index), meta2->constructor(index)))
            return false;
    }

    for (index = 0; index < meta1->methodCount(); ++index) {
        if (!sameMethod(meta1->method(index), meta2->method(index)))
            return false;
    }

    for (index = 0; index < meta1->propertyCount(); ++index) {
        if (!sameProperty(meta1->property(index), meta2->property(index)))
            return false;
    }

    for (index = 0; index < meta1->enumeratorCount(); ++index) {
        if (!sameEnumerator(meta1->enumerator(index), meta2->enumerator(index)))
            return false;
    }

    for (index = 0; index < meta1->classInfoCount(); ++index) {
        if (QByteArray(meta1->classInfo(index).name()) !=
            QByteArray(meta2->classInfo(index).name()))
            return false;
        if (QByteArray(meta1->classInfo(index).value()) !=
            QByteArray(meta2->classInfo(index).value()))
            return false;
    }

    const QMetaObject * const *objects1 = meta1->d.relatedMetaObjects;
    const QMetaObject * const *objects2 = meta2->d.relatedMetaObjects;
    if (objects1 && !objects2)
        return false;
    if (objects2 && !objects1)
        return false;
    if (objects1 && objects2) {
        while (*objects1 != 0 && *objects2 != 0) {
            if (*objects1 != *objects2)
                return false;
            ++objects1;
            ++objects2;
        }
    }

    return true;
}


// This class is used to test that the meta-object generated by QMOB can be
// used by a real object.
// The class manually implements the functions normally generated by moc, and
// creates the corresponding meta-object using QMOB. The autotests check that
// this object can be used by QObject/QMetaObject functionality (property
// access, signals & slots, constructing instances, ...).

class TestObject : public QObject
{
    // Manually expanded from Q_OBJECT macro
public:
    Q_OBJECT_CHECK
    static QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **);
private:
    Q_DECL_HIDDEN static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **);

    //Q_PROPERTY(int intProp READ intProp WRITE setIntProp NOTIFY intPropChanged)
public:
    TestObject(QObject *parent = 0); // Q_INVOKABLE
    ~TestObject();

    // Property accessors
    int intProp() const;
    void setIntProp(int v);

    void emitIntPropChanged();

    int voidSlotIntArgument() const;

// Q_INVOKABLE
    QVariantList listInvokableQRealQString(qreal, const QString &);

//public Q_SLOTS:
    void voidSlotInt(int);

//Q_SIGNALS:
    void intPropChanged(int);

private:
    static QMetaObject *buildMetaObject();

    QMetaObject *m_metaObject;
    int m_intProp;
    int m_voidSlotIntArg;
};

QMetaObject TestObject::staticMetaObject = {
    { 0, 0, 0, 0, 0, 0 }
};

TestObject::TestObject(QObject *parent)
    : QObject(parent), m_metaObject(buildMetaObject()),
      m_intProp(-1), m_voidSlotIntArg(-1)
{
    staticMetaObject = *m_metaObject;
}

TestObject::~TestObject()
{
    free(m_metaObject);
}

QMetaObject *TestObject::buildMetaObject()
{
    QMetaObjectBuilder builder;
    // NOTE: If you change the meta-object, remember to adapt qt_metacall and
    // friends below accordingly.

    builder.setClassName("TestObject");

    builder.setStaticMetacallFunction(qt_static_metacall);

    QMetaMethodBuilder intPropChanged = builder.addSignal("intPropChanged(int)");
    intPropChanged.setParameterNames(QList<QByteArray>() << "newIntPropValue");

    QMetaPropertyBuilder prop = builder.addProperty("intProp", "int");
    prop.setNotifySignal(intPropChanged);

    QMetaMethodBuilder voidSlotInt = builder.addSlot("voidSlotInt(int)");
    voidSlotInt.setParameterNames(QList<QByteArray>() << "slotIntArg");

    QMetaMethodBuilder listInvokableQRealQString = builder.addMethod("listInvokableQRealQString(qreal,QString)");
    listInvokableQRealQString.setReturnType("QVariantList");
    listInvokableQRealQString.setParameterNames(QList<QByteArray>() << "qrealArg" << "qstringArg");

    builder.addConstructor("TestObject(QObject*)");
    builder.addConstructor("TestObject()");

    return builder.toMetaObject();
}

int TestObject::intProp() const
{
    return m_intProp;
}

void TestObject::setIntProp(int value)
{
    if (m_intProp != value) {
        m_intProp = value;
        emit intPropChanged(value);
    }
}

void TestObject::emitIntPropChanged()
{
    emit intPropChanged(m_intProp);
}

QVariantList TestObject::listInvokableQRealQString(qreal r, const QString &s)
{
    return QVariantList() << r << s;
}

void TestObject::voidSlotInt(int value)
{
    m_voidSlotIntArg = value;
}

int TestObject::voidSlotIntArgument() const
{
    return m_voidSlotIntArg;
}

void TestObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { TestObject *_r = new TestObject((*reinterpret_cast< QObject*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        case 1: { TestObject *_r = new TestObject();
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        default: {
            QMetaMethod ctor = _o->metaObject()->constructor(_id);
            qFatal("You forgot to add a case for CreateInstance %s", ctor.methodSignature().constData());
          }
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(_o->metaObject()->cast(_o));
        TestObject *_t = static_cast<TestObject *>(_o);
        switch (_id) {
        case 0: _t->intPropChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->voidSlotInt((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: *reinterpret_cast<QVariantList(*)>(_a[0]) = _t->listInvokableQRealQString(*reinterpret_cast<qreal(*)>(_a[1]), *reinterpret_cast<QString(*)>(_a[2])); break;
        default: {
            QMetaMethod method = _o->metaObject()->method(_o->metaObject()->methodOffset() + _id);
            qFatal("You forgot to add a case for InvokeMetaMethod %s", method.methodSignature().constData());
          }
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (TestObject::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TestObject::intPropChanged)) {
                *result = 0;
            }
        }
        {
            typedef void (TestObject::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TestObject::voidSlotInt)) {
                *result = 1;
            }
        }
        {
            typedef QVariantList (TestObject::*_t)(qreal, const QString &);
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TestObject::listInvokableQRealQString)) {
                *result = 2;
            }
        }
    }
}

const QMetaObject *TestObject::metaObject() const
{
    return m_metaObject;
}

void *TestObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, "TestObject"))
        return static_cast<void*>(const_cast< TestObject*>(this));
    return QObject::qt_metacast(_clname);
}

int TestObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    int ownMethodCount = m_metaObject->methodCount() - m_metaObject->methodOffset();
    int ownPropertyCount = m_metaObject->propertyCount() - m_metaObject->propertyOffset();
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < ownMethodCount)
            qt_static_metacall(this, _c, _id, _a);
        _id -= ownMethodCount;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = intProp(); break;
        default: if (_id < ownPropertyCount) {
            QMetaProperty prop = m_metaObject->property(m_metaObject->propertyOffset() + _id);
            qFatal("You forgot to add a case for ReadProperty %s", prop.name());
          }
        }
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setIntProp(*reinterpret_cast< int*>(_v)); break;
        default: if (_id < ownPropertyCount) {
            QMetaProperty prop = m_metaObject->property(m_metaObject->propertyOffset() + _id);
            qFatal("You forgot to add a case for WriteProperty %s", prop.name());
          }
        }
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= ownPropertyCount;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= ownPropertyCount;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void TestObject::intPropChanged(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, m_metaObject, 0, _a);
}


void tst_QMetaObjectBuilder::usage_signal()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    QSignalSpy propChangedSpy(testObject.data(), &TestObject::intPropChanged);
    testObject->emitIntPropChanged();
    QCOMPARE(propChangedSpy.count(), 1);
    QCOMPARE(propChangedSpy.at(0).count(), 1);
    QCOMPARE(propChangedSpy.at(0).at(0).toInt(), testObject->intProp());
}

void tst_QMetaObjectBuilder::usage_property()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    QVariant prop = testObject->property("intProp");
    QCOMPARE(prop.type(), QVariant::Int);
    QCOMPARE(prop.toInt(), testObject->intProp());

    QSignalSpy propChangedSpy(testObject.data(), &TestObject::intPropChanged);
    QVERIFY(testObject->intProp() != 123);
    testObject->setProperty("intProp", 123);
    QCOMPARE(propChangedSpy.count(), 1);
    prop = testObject->property("intProp");
    QCOMPARE(prop.type(), QVariant::Int);
    QCOMPARE(prop.toInt(), 123);
}

void tst_QMetaObjectBuilder::usage_slot()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    int index = testObject->metaObject()->indexOfMethod("voidSlotInt(int)");
    QVERIFY(index != -1);
    QMetaMethod voidSlotInt = testObject->metaObject()->method(index);

    QCOMPARE(testObject->voidSlotIntArgument(), -1);
    QVERIFY(voidSlotInt.invoke(testObject.data(), Q_ARG(int, 123)));
    QCOMPARE(testObject->voidSlotIntArgument(), 123);
}

void tst_QMetaObjectBuilder::usage_method()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    int index = testObject->metaObject()->indexOfMethod("listInvokableQRealQString(qreal,QString)");
    QVERIFY(index != -1);
    QMetaMethod listInvokableQRealQString = testObject->metaObject()->method(index);
    QVariantList list;
    QVERIFY(listInvokableQRealQString.invoke(testObject.data(), Q_RETURN_ARG(QVariantList, list),
                                             Q_ARG(qreal, 123.0), Q_ARG(QString, "ciao")));
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).type(), QVariant::Type(QMetaType::QReal));
    QCOMPARE(list.at(0).toDouble(), double(123));
    QCOMPARE(list.at(1).type(), QVariant::String);
    QCOMPARE(list.at(1).toString(), QString::fromLatin1("ciao"));
}

void tst_QMetaObjectBuilder::usage_constructor()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    QCOMPARE(testObject->metaObject()->constructorCount(), 2);
    QScopedPointer<QObject> testInstance(testObject->metaObject()->newInstance());
    QVERIFY(testInstance != 0);
    QScopedPointer<QObject> testInstance2(testObject->metaObject()->newInstance(Q_ARG(QObject*, testInstance.data())));
    QVERIFY(testInstance2 != 0);
    QCOMPARE(testInstance2->parent(), testInstance.data());
}

void tst_QMetaObjectBuilder::usage_connect()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    QVERIFY(QObject::connect(testObject.data(), SIGNAL(intPropChanged(int)),
                             testObject.data(), SLOT(voidSlotInt(int))));

    QCOMPARE(testObject->voidSlotIntArgument(), -1);
    testObject->setProperty("intProp", 123);
    QCOMPARE(testObject->voidSlotIntArgument(), 123);

    QVERIFY(QObject::disconnect(testObject.data(), SIGNAL(intPropChanged(int)),
                                testObject.data(), SLOT(voidSlotInt(int))));
}

void tst_QMetaObjectBuilder::usage_templateConnect()
{
    QScopedPointer<TestObject> testObject(new TestObject);

    QMetaObject::Connection con = QObject::connect(testObject.data(), &TestObject::intPropChanged,
                                                   testObject.data(), &TestObject::voidSlotInt);
    QVERIFY(con);

    QCOMPARE(testObject->voidSlotIntArgument(), -1);
    testObject->setProperty("intProp", 123);
    QCOMPARE(testObject->voidSlotIntArgument(), 123);

    QVERIFY(QObject::disconnect(testObject.data(), &TestObject::intPropChanged,
                                testObject.data(), &TestObject::voidSlotInt));

    // Something that isn't a signal
    QTest::ignoreMessage(QtWarningMsg, "QObject::connect: signal not found in TestObject");
    con = QObject::connect(testObject.data(), &TestObject::setIntProp,
                           testObject.data(), &TestObject::intPropChanged);
    QVERIFY(!con);
}

void tst_QMetaObjectBuilder::classNameFirstInStringData()
{
    QMetaObjectBuilder builder;
    builder.addMetaObject(&SomethingOfEverything::staticMetaObject);
    builder.setClassName(QByteArrayLiteral("TestClass"));
    QMetaObject *mo = builder.toMetaObject();

    QByteArrayDataPtr header;
    header.ptr = const_cast<QByteArrayData*>(mo->d.stringdata);
    QCOMPARE(QByteArray(header), QByteArrayLiteral("TestClass"));

    free(mo);
}

QTEST_MAIN(tst_QMetaObjectBuilder)

#include "tst_qmetaobjectbuilder.moc"
