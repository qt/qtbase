// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>
#include <QSignalSpy>
#include <qtest.h>
#include <qproperty.h>
#include <private/qproperty_p.h>
#include <private/qobject_p.h>

#if __has_include(<source_location>) && __cplusplus >= 202002L && !defined(Q_QDOC)
#include <source_location>
#define QT_SOURCE_LOCATION_NAMESPACE std
#elif __has_include(<experimental/source_location>) && !defined(Q_QDOC)
#include <experimental/source_location>
#define QT_SOURCE_LOCATION_NAMESPACE std::experimental
#endif

using namespace QtPrivate;
using namespace Qt::StringLiterals;

struct DtorCounter {
    static inline int counter = 0;
    bool shouldIncrement = false;
    ~DtorCounter() {if (shouldIncrement) ++counter;}
};

class tst_QProperty : public QObject
{
    Q_OBJECT
private slots:
    void functorBinding();
    void basicDependencies();
    void multipleDependencies();
    void bindingWithDeletedDependency();
    void dependencyChangeDuringDestruction();
    void recursiveDependency();
    void bindingAfterUse();
    void bindingFunctionDtorCalled();
    void switchBinding();
    void avoidDependencyAllocationAfterFirstEval();
    void boolProperty();
    void takeBinding();
    void stickyBinding();
    void replaceBinding();
    void changeHandler();
    void propertyChangeHandlerApi();
    void subscribe();
    void changeHandlerThroughBindings();
    void dontTriggerDependenciesIfUnchangedValue();
    void bindingSourceLocation();
    void bindingError();
    void bindingLoop();
    void realloc();
    void changePropertyFromWithinChangeHandler();
    void changePropertyFromWithinChangeHandlerThroughDependency();
    void changePropertyFromWithinChangeHandler2();
    void settingPropertyValueDoesRemoveBinding();
    void genericPropertyBinding();
    void genericPropertyBindingBool();
    void setBindingFunctor();
    void multipleObservers();
    void arrowAndStarOperator();
    void notifiedProperty();
    void typeNoOperatorEqual();
    void bindingValueReplacement();
    void quntypedBindableApi();
    void readonlyConstQBindable();
    void qobjectBindableManualNotify();
    void qobjectBindableReallocatedBindingStorage();
    void qobjectBindableSignalTakingNewValue();

    void testNewStuff();
    void qobjectObservers();
    void compatBindings();
    void metaProperty();

    void modifyObserverListWhileIterating();
    void noDoubleCapture();
    void compatPropertyNoDobuleNotification();
    void compatPropertySignals();

    void noFakeDependencies();
#if QT_CONFIG(thread)
    void threadSafety();
    void threadSafety2();
#endif // QT_CONFIG(thread)

    void bindablePropertyWithInitialization();
    void noDoubleNotification();
    void groupedNotifications();
    void groupedNotificationConsistency();
    void bindingGroupMovingBindingData();
    void bindingGroupBindingDeleted();
    void uninstalledBindingDoesNotEvaluate();

    void notify();

    void bindableInterfaceOfCompatPropertyUsesSetter();

    void selfBindingShouldNotCrash();

    void qpropertyAlias();
    void scheduleNotify();

    void notifyAfterAllDepsGone();

    void propertyAdaptorBinding();
    void propertyUpdateViaSignaledProperty();
};

void tst_QProperty::functorBinding()
{
    QProperty<int> property([]() { return 42; });
    QCOMPARE(property.value(), int(42));
    property.setBinding([]() { return 100; });
    QCOMPARE(property.value(), int(100));
    property.setBinding([]() { return 50; });
    QCOMPARE(property.value(), int(50));
}

void tst_QProperty::basicDependencies()
{
    QProperty<int> right(100);

    QProperty<int> left(Qt::makePropertyBinding(right));

    QCOMPARE(left.value(), int(100));

    right = 42;

    QCOMPARE(left.value(), int(42));
}

void tst_QProperty::multipleDependencies()
{
    QProperty<int> firstDependency(1);
    QProperty<int> secondDependency(2);

    QProperty<int> sum;
    sum.setBinding([&]() { return firstDependency + secondDependency; });

    QCOMPARE(QPropertyBindingDataPointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBindingDataPointer::get(secondDependency).observerCount(), 1);

    QCOMPARE(sum.value(), int(3));
    QCOMPARE(QPropertyBindingDataPointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBindingDataPointer::get(secondDependency).observerCount(), 1);

    firstDependency = 10;

    QCOMPARE(sum.value(), int(12));
    QCOMPARE(QPropertyBindingDataPointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBindingDataPointer::get(secondDependency).observerCount(), 1);

    secondDependency = 20;

    QCOMPARE(sum.value(), int(30));
    QCOMPARE(QPropertyBindingDataPointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBindingDataPointer::get(secondDependency).observerCount(), 1);

    firstDependency = 1;
    secondDependency = 1;
    QCOMPARE(sum.value(), int(2));
    QCOMPARE(QPropertyBindingDataPointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBindingDataPointer::get(secondDependency).observerCount(), 1);
}

void tst_QProperty::bindingWithDeletedDependency()
{
    QScopedPointer<QProperty<int>> dynamicProperty(new QProperty<int>(100));

    QProperty<int> staticProperty(1000);

    QProperty<bool> bindingReturnsDynamicProperty(false);

    QProperty<int> propertySelector([&]() {
        if (bindingReturnsDynamicProperty && !dynamicProperty.isNull())
            return dynamicProperty->value();
        else
            return staticProperty.value();
    });

    QCOMPARE(propertySelector.value(), staticProperty.value());

    bindingReturnsDynamicProperty = true;

    QCOMPARE(propertySelector.value(), dynamicProperty->value());

    dynamicProperty.reset();

    QCOMPARE(propertySelector.value(), 100);

    bindingReturnsDynamicProperty = false;

    QCOMPARE(propertySelector.value(), staticProperty.value());
}

class ChangeDuringDtorTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop READ prop WRITE setProp BINDABLE bindableProp)

public:
    void setProp(int i) { m_prop = i;}
    int prop() const { return m_prop; }
    QBindable<int> bindableProp() { return &m_prop; }
private:
    Q_OBJECT_COMPAT_PROPERTY(ChangeDuringDtorTester, int, m_prop, &ChangeDuringDtorTester::setProp)
};

void tst_QProperty::dependencyChangeDuringDestruction()
{
    auto tester = std::make_unique<ChangeDuringDtorTester>();
    QProperty<int> iprop {42};
    tester->bindableProp().setBinding(Qt::makePropertyBinding(iprop));
    QObject::connect(tester.get(), &QObject::destroyed, [&](){
        iprop = 12;
    });
    bool failed = false;
    auto handler = tester->bindableProp().onValueChanged([&](){
        failed = true;
    });
    tester.reset();
    QVERIFY(!failed);
}

void tst_QProperty::recursiveDependency()
{
    QProperty<int> first(1);

    QProperty<int> second;
    second.setBinding(Qt::makePropertyBinding(first));

    QProperty<int> third;
    third.setBinding(Qt::makePropertyBinding(second));

    QCOMPARE(third.value(), int(1));

    first = 2;

    QCOMPARE(third.value(), int(2));
}

void tst_QProperty::bindingAfterUse()
{
    QProperty<int> propWithBindingLater(1);

    QProperty<int> propThatUsesFirstProp;
    propThatUsesFirstProp.setBinding(Qt::makePropertyBinding(propWithBindingLater));

    QCOMPARE(propThatUsesFirstProp.value(), int(1));
    QCOMPARE(QPropertyBindingDataPointer::get(propWithBindingLater).observerCount(), 1);

    QProperty<int> injectedValue(42);
    propWithBindingLater.setBinding(Qt::makePropertyBinding(injectedValue));

    QCOMPARE(propThatUsesFirstProp.value(), int(42));
    QCOMPARE(QPropertyBindingDataPointer::get(propWithBindingLater).observerCount(), 1);
}

void tst_QProperty::bindingFunctionDtorCalled()
{
    DtorCounter dc;
    {
        QProperty<int> prop;
        prop.setBinding([dc]() mutable {
                dc.shouldIncrement = true;
                return 42;
        });
        QCOMPARE(prop.value(), 42);
    }
    QCOMPARE(DtorCounter::counter, 1);
}

void tst_QProperty::switchBinding()
{
    QProperty<int> first(1);

    QProperty<int> propWithChangingBinding;
    propWithChangingBinding.setBinding(Qt::makePropertyBinding(first));

    QCOMPARE(propWithChangingBinding.value(), 1);

    QProperty<int> output;
    output.setBinding(Qt::makePropertyBinding(propWithChangingBinding));
    QCOMPARE(output.value(), 1);

    QProperty<int> second(2);
    propWithChangingBinding.setBinding(Qt::makePropertyBinding(second));
    QCOMPARE(output.value(), 2);
}

void tst_QProperty::avoidDependencyAllocationAfterFirstEval()
{
    QProperty<int> firstDependency(1);
    QProperty<int> secondDependency(10);

    QProperty<int> propWithBinding([&]() { return firstDependency + secondDependency; });

    QCOMPARE(propWithBinding.value(), int(11));

    QVERIFY(QPropertyBindingDataPointer::get(propWithBinding).binding());
    QCOMPARE(QPropertyBindingDataPointer::get(propWithBinding).binding()->dependencyObserverCount, 2u);

    firstDependency = 100;
    QCOMPARE(propWithBinding.value(), int(110));
    QCOMPARE(QPropertyBindingDataPointer::get(propWithBinding).binding()->dependencyObserverCount, 2u);
}

void tst_QProperty::boolProperty()
{
    QProperty<bool> first(true);
    QProperty<bool> second(false);
    QProperty<bool> all([&]() { return first && second; });

    QCOMPARE(all.value(), false);

    second = true;

    QCOMPARE(all.value(), true);
}

void tst_QProperty::takeBinding()
{
    QPropertyBinding<int> existingBinding;
    QVERIFY(existingBinding.isNull());

    QProperty<int> first(100);
    QProperty<int> second(Qt::makePropertyBinding(first));

    QCOMPARE(second.value(), int(100));

    existingBinding = second.takeBinding();
    QVERIFY(!existingBinding.isNull());

    first = 10;
    QCOMPARE(second.value(), int(100));

    second = 25;
    QCOMPARE(second.value(), int(25));

    second.setBinding(existingBinding);
    QCOMPARE(second.value(), int(10));
    QVERIFY(!existingBinding.isNull());
}

void tst_QProperty::stickyBinding()
{
    QProperty<int> prop;
    QProperty<int> prop2 {2};
    prop.setBinding([&](){ return prop2.value(); });
    QCOMPARE(prop.value(), 2);
    auto privBinding = QPropertyBindingPrivate::get(prop.binding());
    // If we make a binding sticky,
    privBinding->setSticky();
    // then writing to the property does not remove it
    prop = 1;
    QVERIFY(prop.hasBinding());
    // but the value still changes.
    QCOMPARE(prop.value(), 1);
    // The binding continues to work normally.
    prop2 = 3;
    QCOMPARE(prop.value(), 3);
    // If we remove the stickiness
    privBinding->setSticky(false);
    // the binding goes away on the next write
    prop = 42;
    QVERIFY(!prop.hasBinding());
}

void tst_QProperty::replaceBinding()
{
    QProperty<int> first(100);
    QProperty<int> second(Qt::makePropertyBinding(first));

    QCOMPARE(second.value(), 100);

    auto constantBinding = Qt::makePropertyBinding([]() { return 42; });
    auto oldBinding = second.setBinding(constantBinding);
    QCOMPARE(second.value(), 42);

    second.setBinding(oldBinding);
    QCOMPARE(second.value(), 100);
}

void tst_QProperty::changeHandler()
{
    QProperty<int> testProperty(0);
    QList<int> recordedValues;

    {
        auto handler = testProperty.onValueChanged([&]() {
            recordedValues << testProperty;
        });

        testProperty = 1;
        testProperty = 2;
    }
    testProperty = 3;

    QCOMPARE(recordedValues.size(), 2);
    QCOMPARE(recordedValues.at(0), 1);
    QCOMPARE(recordedValues.at(1), 2);
}

void tst_QProperty::propertyChangeHandlerApi()
{
    int changeHandlerCallCount = 0;
    QPropertyChangeHandler handler([&changeHandlerCallCount]() {
        ++changeHandlerCallCount;
    });

    QProperty<int> source1;
    QProperty<int> source2;

    handler.setSource(source1);

    source1 = 100;
    QCOMPARE(changeHandlerCallCount, 1);

    handler.setSource(source2);
    source1 = 101;
    QCOMPARE(changeHandlerCallCount, 1);

    source2 = 200;
    QCOMPARE(changeHandlerCallCount, 2);
}

void tst_QProperty::subscribe()
{
    QProperty<int> testProperty(42);
    QList<int> recordedValues;

    {
        auto handler = testProperty.subscribe([&]() {
            recordedValues << testProperty;
        });

        testProperty = 1;
        testProperty = 2;
    }
    testProperty = 3;

    QCOMPARE(recordedValues.size(), 3);
    QCOMPARE(recordedValues.at(0), 42);
    QCOMPARE(recordedValues.at(1), 1);
    QCOMPARE(recordedValues.at(2), 2);
}

void tst_QProperty::changeHandlerThroughBindings()
{
    QProperty<bool> trigger(false);
    QProperty<bool> blockTrigger(false);
    QProperty<bool> condition([&]() {
        bool triggerValue = trigger;
        bool blockTriggerValue = blockTrigger;
        return triggerValue && !blockTriggerValue;
    });
    bool changeHandlerCalled = false;
    auto handler = condition.onValueChanged([&]() {
        changeHandlerCalled = true;
    });

    QVERIFY(!condition);
    QVERIFY(!changeHandlerCalled);

    trigger = true;

    QVERIFY(condition);
    QVERIFY(changeHandlerCalled);
    changeHandlerCalled = false;

    trigger = false;

    QVERIFY(!condition);
    QVERIFY(changeHandlerCalled);
    changeHandlerCalled = false;

    blockTrigger = true;

    QVERIFY(!condition);
    QVERIFY(!changeHandlerCalled);
}

void tst_QProperty::dontTriggerDependenciesIfUnchangedValue()
{
    QProperty<int> property(42);

    bool triggered = false;
    QProperty<int> observer([&]() { triggered = true; return property.value(); });

    QCOMPARE(observer.value(), 42);
    QVERIFY(triggered);
    triggered = false;
    property = 42;
    QCOMPARE(observer.value(), 42);
    QVERIFY(!triggered);
}

void tst_QProperty::bindingSourceLocation()
{
#if defined(QT_PROPERTY_COLLECT_BINDING_LOCATION)
    auto bindingLine = QT_SOURCE_LOCATION_NAMESPACE::source_location::current().line() + 1;
    auto binding = Qt::makePropertyBinding([]() { return 42; });
    QCOMPARE(QPropertyBindingPrivate::get(binding)->sourceLocation().line, bindingLine);
#else
    QSKIP("Skipping this in the light of missing binding source location support");
#endif
}

void tst_QProperty::bindingError()
{
    QProperty<int> prop([]() -> int {
        QPropertyBindingError error(QPropertyBindingError::UnknownError, QLatin1String("my error"));
        QPropertyBindingPrivate::currentlyEvaluatingBinding()->setError(std::move(error));
        return 0;
    });
    QCOMPARE(prop.value(), 0);
    QCOMPARE(prop.binding().error().description(), QString("my error"));
}



class BindingLoopTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int eagerProp READ eagerProp WRITE setEagerProp BINDABLE bindableEagerProp)
    Q_PROPERTY(int eagerProp2 READ eagerProp2 WRITE setEagerProp2 BINDABLE bindableEagerProp2)
    public:
    BindingLoopTester(QProperty<int> *i, QObject *parent = nullptr) : QObject(parent) {
        eagerData.setBinding(Qt::makePropertyBinding([&](){ return eagerData2.value() + i->value(); }  ) );
        eagerData2.setBinding(Qt::makePropertyBinding([&](){ return eagerData.value() + 1; }  ) );
        i->setValue(42);
    }
    BindingLoopTester() {}

    int eagerProp() {return eagerData.value();}
    void setEagerProp(int i) { eagerData.setValue(i); eagerData.notify(); }
    QBindable<int> bindableEagerProp() {return QBindable<int>(&eagerData);}
    Q_OBJECT_COMPAT_PROPERTY(BindingLoopTester, int, eagerData, &BindingLoopTester::setEagerProp)

    int eagerProp2() {return eagerData2.value();}
    void setEagerProp2(int i) { eagerData2.setValue(i); eagerData2.notify(); }
    QBindable<int> bindableEagerProp2() {return QBindable<int>(&eagerData2);}
    Q_OBJECT_COMPAT_PROPERTY(BindingLoopTester, int, eagerData2, &BindingLoopTester::setEagerProp2)
};

void tst_QProperty::bindingLoop()
{
    QProperty<int> firstProp;

    QProperty<int> secondProp([&]() -> int {
        return firstProp.value();
    });

    QProperty<int> thirdProp([&]() -> int {
        return secondProp.value();
    });

    firstProp.setBinding([&]() -> int {
        return secondProp.value() + thirdProp.value();
    });

    thirdProp.setValue(10);
    QCOMPARE(firstProp.binding().error().type(), QPropertyBindingError::BindingLoop);


    {
        QProperty<int> i;
        BindingLoopTester tester(&i);
        QCOMPARE(tester.bindableEagerProp().binding().error().type(), QPropertyBindingError::BindingLoop);
        QCOMPARE(tester.bindableEagerProp2().binding().error().type(), QPropertyBindingError::BindingLoop);
    }
    {
        BindingLoopTester tester;
        auto handler = tester.bindableEagerProp().onValueChanged([&]() {
            tester.bindableEagerProp().setBinding([](){return 42;});
        });
        tester.bindableEagerProp().setBinding([]() {return 42;});
        QCOMPARE(tester.bindableEagerProp().binding().error().type(), QPropertyBindingError::BindingLoop);
        QCOMPARE(tester.bindableEagerProp().binding().error().description(), "Binding set during binding evaluation!");
    }
}

class ReallocTester : public QObject
{
    /*
     * This class and the realloc test rely on the fact that the internal property hashmap has an
     * initial capacity of 8 and a load factor of 0.5. Thus, it is necessary to cause actions which
     * allocate 5 different QPropertyBindingData
     * */
    Q_OBJECT
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 BINDABLE bindableProp1)
    Q_PROPERTY(int prop2 READ prop2 WRITE setProp2 BINDABLE bindableProp2)
    Q_PROPERTY(int prop3 READ prop3 WRITE setProp3 BINDABLE bindableProp3)
    Q_PROPERTY(int prop4 READ prop4 WRITE setProp4 BINDABLE bindableProp4)
    Q_PROPERTY(int prop5 READ prop5 WRITE setProp5 BINDABLE bindableProp5)
public:
    ReallocTester(QObject *parent = nullptr) : QObject(parent) {}


#define GEN(N) \
    int prop##N() {return propData##N.value();} \
    void setProp##N(int i) { if (i == propData##N) return; propData##N.setValue(i); propData##N.notify(); } \
    QBindable<int> bindableProp##N() {return QBindable<int>(&propData##N);} \
    Q_OBJECT_COMPAT_PROPERTY(ReallocTester, int, propData##N, &ReallocTester::setProp##N)
    GEN(1)
    GEN(2)
    GEN(3)
    GEN(4)
    GEN(5)
#undef GEN
};

void tst_QProperty::realloc()
{
    {
        // Triggering a reallocation does not crash
        ReallocTester tester;
        tester.bindableProp1().setBinding([&](){return tester.prop5();});
        tester.bindableProp2().setBinding([&](){return tester.prop5();});
        tester.bindableProp3().setBinding([&](){return tester.prop5();});
        tester.bindableProp4().setBinding([&](){return tester.prop5();});
        tester.bindableProp5().setBinding([&]() -> int{return 42;});
        QCOMPARE(tester.prop1(), 42);
    }
    {
        // After a reallocation, property observers still work
        ReallocTester tester;
        int modificationCount = 0;
        QPropertyChangeHandler observer {[&](){ ++modificationCount; }};
        tester.bindableProp1().observe(&observer);
        tester.setProp1(12);
        QCOMPARE(modificationCount, 1);
        QCOMPARE(tester.prop1(), 12);

        tester.bindableProp1().setBinding([&](){return tester.prop5();});
        QCOMPARE(modificationCount, 2);
        tester.bindableProp2().setBinding([&](){return tester.prop5();});
        tester.bindableProp3().setBinding([&](){return tester.prop5();});
        tester.bindableProp4().setBinding([&](){return tester.prop5();});
        tester.bindableProp5().setBinding([&]() -> int{return 42;});
        QCOMPARE(modificationCount, 3);
    }
};

void tst_QProperty::changePropertyFromWithinChangeHandler()
{
    QProperty<int> property(100);
    bool resetPropertyOnChange = false;
    int changeHandlerCallCount = 0;

    auto handler = property.onValueChanged([&]() {
        ++changeHandlerCallCount;
        if (resetPropertyOnChange)
            property = 100;
    });

    QCOMPARE(property.value(), 100);

    resetPropertyOnChange = true;
    property = 42;
    QCOMPARE(property.value(), 100);
    // changing the property value inside the change handler won't result in the change
    // handler being called again.
    QCOMPARE(changeHandlerCallCount, 1);
    changeHandlerCallCount = 0;
}

void tst_QProperty::changePropertyFromWithinChangeHandlerThroughDependency()
{
    QProperty<int> sourceProperty(100);
    QProperty<int> property(Qt::makePropertyBinding(sourceProperty));
    bool resetPropertyOnChange = false;
    int changeHandlerCallCount = 0;

    auto handler = property.onValueChanged([&]() {
        ++changeHandlerCallCount;
        if (resetPropertyOnChange)
            sourceProperty = 100;
    });

    QCOMPARE(property.value(), 100);

    resetPropertyOnChange = true;
    sourceProperty = 42;
    QVERIFY(property.value() == 100 || property.value() == 42);
    QVERIFY(property.binding().error().type() == QPropertyBindingError::BindingLoop);
    // changing the property value inside the change handler won't result in the change
    // handler being called again.
    QCOMPARE(changeHandlerCallCount, 1);
    changeHandlerCallCount = 0;
}

void tst_QProperty::changePropertyFromWithinChangeHandler2()
{
    QProperty<int> property(100);
    int changeHandlerCallCount = 0;

    auto handler = property.onValueChanged([&]() {
        ++changeHandlerCallCount;
        property = property.value() + 1;
    });

    QCOMPARE(property.value(), 100);

    property = 42;
    QCOMPARE(property.value(), 43);
    QVERIFY(!property.hasBinding()); // setting the value in the change handler removed the binding
}

void tst_QProperty::settingPropertyValueDoesRemoveBinding()
{
    QProperty<int> source(42);

    QProperty<int> property(Qt::makePropertyBinding(source));

    QCOMPARE(property.value(), 42);
    QVERIFY(!property.binding().isNull());

    property = 100;
    QCOMPARE(property.value(), 100);
    QVERIFY(property.binding().isNull());

    source = 1;
    QCOMPARE(property.value(), 100);
    QVERIFY(property.binding().isNull());
}

void tst_QProperty::genericPropertyBinding()
{
    QProperty<int> property;

    {
        QUntypedPropertyBinding doubleBinding(QMetaType::fromType<double>(),
                                              [](QMetaType , void *) -> bool {
            Q_ASSERT(false);
            return true;
        }, QPropertyBindingSourceLocation());
        QVERIFY(!property.setBinding(doubleBinding));
    }

    QUntypedPropertyBinding intBinding(QMetaType::fromType<int>(),
                                    [](QMetaType metaType, void *dataPtr) -> bool {
        Q_ASSERT(metaType.id() == qMetaTypeId<int>());

        int *intPtr = reinterpret_cast<int*>(dataPtr);
        *intPtr = 100;
        return true;
    }, QPropertyBindingSourceLocation());

    QVERIFY(property.setBinding(intBinding));

    QCOMPARE(property.value(), 100);
}

void tst_QProperty::genericPropertyBindingBool()
{
    QProperty<bool> property;

    QVERIFY(!property.value());

    QUntypedPropertyBinding boolBinding(QMetaType::fromType<bool>(),
            [](QMetaType, void *dataPtr) -> bool {
        auto boolPtr = reinterpret_cast<bool *>(dataPtr);
        *boolPtr = true;
        return true;
    }, QPropertyBindingSourceLocation());
    QVERIFY(property.setBinding(boolBinding));

    QVERIFY(property.value());
}

void tst_QProperty::setBindingFunctor()
{
    QProperty<int> property;
    QProperty<int> injectedValue(100);
    // Make sure that this picks the setBinding overload that takes a functor and
    // moves it correctly.
    property.setBinding([&injectedValue]() { return injectedValue.value(); });
    injectedValue = 200;
    QCOMPARE(property.value(), 200);
}

void tst_QProperty::multipleObservers()
{
    QProperty<int> property;
    property.setValue(5);
    QCOMPARE(property.value(), 5);

    int value1 = 1;
    auto changeHandler = property.onValueChanged([&]() { value1 = property.value(); });
    QCOMPARE(value1, 1);

    int value2 = 2;
    auto subscribeHandler = property.subscribe([&]() { value2 = property.value(); });
    QCOMPARE(value2, 5);

    property.setValue(6);
    QCOMPARE(property.value(), 6);
    QCOMPARE(value1, 6);
    QCOMPARE(value2, 6);

    property.setBinding([]() { return 12; });
    QCOMPARE(value1, 12);
    QCOMPARE(value2, 12);
    QCOMPARE(property.value(), 12);

    property.setBinding(QPropertyBinding<int>());
    QCOMPARE(value1, 12);
    QCOMPARE(value2, 12);
    QCOMPARE(property.value(), 12);

    property.setValue(22);
    QCOMPARE(value1, 22);
    QCOMPARE(value2, 22);
    QCOMPARE(property.value(), 22);
}

void tst_QProperty::arrowAndStarOperator()
{
    QString str("Hello");
    QProperty<QString *> prop(&str);

    QCOMPARE(prop->size(), str.size());
    QCOMPARE(**prop, str);

    struct Dereferenceable {
        QString x;
        QString *operator->() { return &x; }
        const QString *operator->() const { return &x; }
    };
    static_assert(QTypeTraits::is_dereferenceable_v<Dereferenceable>);

    QProperty<Dereferenceable> prop2(Dereferenceable{str});
    QCOMPARE(prop2->size(), str.size());
    QCOMPARE(**prop, str);

    QObject *object = new QObject;
    object->setObjectName("Hello");
    QProperty<QSharedPointer<QObject>> prop3(QSharedPointer<QObject>{object});

    QCOMPARE(prop3->objectName(), str);
    QCOMPARE(*prop3, object);

}

struct ClassWithNotifiedProperty : public QObject
{
    QList<int> recordedValues;

    void callback() { recordedValues << property.value(); }
    int getProp() { return 0; }

    Q_OBJECT_BINDABLE_PROPERTY(ClassWithNotifiedProperty, int, property, &ClassWithNotifiedProperty::callback);
};

void tst_QProperty::notifiedProperty()
{
    ClassWithNotifiedProperty instance;
    std::array<QProperty<int>, 5> otherProperties = {
        QProperty<int>([&]() { return instance.property + 1; }),
        QProperty<int>([&]() { return instance.property + 2; }),
        QProperty<int>([&]() { return instance.property + 3; }),
        QProperty<int>([&]() { return instance.property + 4; }),
        QProperty<int>([&]() { return instance.property + 5; }),
    };

    auto check = [&] {
        const int val = instance.property.value();
        for (int i = 0; i < int(otherProperties.size()); ++i)
            QCOMPARE(otherProperties[i].value(), val + i + 1);
    };

    QVERIFY(instance.recordedValues.isEmpty());
    check();

    instance.property.setValue(42);
    QCOMPARE(instance.recordedValues.size(), 1);
    QCOMPARE(instance.recordedValues.at(0), 42);
    instance.recordedValues.clear();
    check();

    instance.property.setValue(42);
    QVERIFY(instance.recordedValues.isEmpty());
    check();

    int subscribedCount = 0;
    QProperty<int> injectedValue(100);
    instance.property.setBinding([&injectedValue]() { return injectedValue.value(); });
    auto subscriber = [&] { ++subscribedCount; };
    std::array<QPropertyChangeHandler<decltype (subscriber)>, 10> subscribers = {
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber),
            instance.property.subscribe(subscriber)
    };

    QCOMPARE(subscribedCount, 10);
    subscribedCount = 0;

    QCOMPARE(instance.property.value(), 100);
    QCOMPARE(instance.recordedValues.size(), 1);
    QCOMPARE(instance.recordedValues.at(0), 100);
    instance.recordedValues.clear();
    check();
    QCOMPARE(subscribedCount, 0);

    injectedValue = 200;
    QCOMPARE(instance.property.value(), 200);
    QCOMPARE(instance.recordedValues.size(), 1);
    QCOMPARE(instance.recordedValues.at(0), 200);
    instance.recordedValues.clear();
    check();
    QCOMPARE(subscribedCount, 10);
    subscribedCount = 0;

    injectedValue = 400;
    QCOMPARE(instance.property.value(), 400);
    QCOMPARE(instance.recordedValues.size(), 1);
    QCOMPARE(instance.recordedValues.at(0), 400);
    instance.recordedValues.clear();
    check();
    QCOMPARE(subscribedCount, 10);
}

void tst_QProperty::typeNoOperatorEqual()
{
    struct Uncomparable
    {
        int data = -1;
        bool changedCalled = false;

        Uncomparable(int value = 0)
        : data(value)
        {}
        Uncomparable(const Uncomparable &other)
        {
            data = other.data;
            changedCalled = false;
        }
        Uncomparable(Uncomparable &&other)
        {
            data = other.data;
            changedCalled = false;
            other.data = -1;
            other.changedCalled = false;
        }
        Uncomparable &operator=(const Uncomparable &other)
        {
            data = other.data;
            return *this;
        }
        Uncomparable &operator=(Uncomparable &&other)
        {
            data = other.data;
            changedCalled = false;
            other.data = -1;
            other.changedCalled = false;
            return *this;
        }
        bool operator==(const Uncomparable&) = delete;
        bool operator!=(const Uncomparable&) = delete;

        void changed()
        {
            changedCalled = true;
        }
    };

    Uncomparable u1 = { 13 };
    Uncomparable u2 = { 27 };

    QProperty<Uncomparable> p1;
    QProperty<Uncomparable> p2(Qt::makePropertyBinding(p1));

    QCOMPARE(p1.value().data, p2.value().data);
    p1.setValue(u1);
    QCOMPARE(p1.value().data, u1.data);
    QCOMPARE(p1.value().data, p2.value().data);
    p2.setValue(u2);
    QCOMPARE(p1.value().data, u1.data);
    QCOMPARE(p2.value().data, u2.data);

    QProperty<Uncomparable> p3(Qt::makePropertyBinding(p1));
    p1.setValue(u1);
    QCOMPARE(p1.value().data, p3.value().data);

//    QNotifiedProperty<Uncomparable, &Uncomparable::changed> np;
//    QVERIFY(np.value().data != u1.data);
//    np.setValue(&u1, u1);
//    QVERIFY(u1.changedCalled);
//    u1.changedCalled = false;
//    QCOMPARE(np.value().data, u1.data);
//    np.setValue(&u1, u1);
//    QVERIFY(u1.changedCalled);
}


//struct Test {
//    void notify() {};
//    bool bindText(int);
//    bool bindIconText(int);
//    QProperty<int> text;
//    QNotifiedProperty<int, &Test::notify, &Test::bindIconText> iconText;
//};

//bool Test::bindIconText(int) {
//    Q_UNUSED(iconText.value()); // force read
//    if (!iconText.hasBinding()) {
//        iconText.setBinding(this, [=]() { return 0; });
//    }
//    return true;
//}

void tst_QProperty::bindingValueReplacement()
{
//    Test test;
//    test.text = 0;
//    test.bindIconText(0);
//    test.iconText.setValue(&test, 42); // should not crash
//    QCOMPARE(test.iconText.value(), 42);
//    test.text = 1;
//    QCOMPARE(test.iconText.value(), 42);
}

void tst_QProperty::quntypedBindableApi()
{
    QProperty<int> iprop;
    QUntypedBindable bindable(&iprop);
    QVERIFY(!bindable.hasBinding());
    QVERIFY(bindable.binding().isNull());
    bindable.setBinding(Qt::makePropertyBinding([]() -> int {return 42;}));
    QVERIFY(bindable.hasBinding());
    QVERIFY(!bindable.binding().isNull());
    QUntypedPropertyBinding binding = bindable.takeBinding();
    QVERIFY(!bindable.hasBinding());
    bindable.setBinding(binding);
    QCOMPARE(iprop.value(), 42);
    QUntypedBindable propLess;
    QVERIFY(propLess.takeBinding().isNull());

    QUntypedBindable invalidBindable;
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "setBinding: Could not set binding via bindable interface. The QBindable is invalid.");
#endif
    invalidBindable.setBinding(Qt::makePropertyBinding(iprop));

    QUntypedBindable readOnlyBindable(static_cast<const QProperty<int> *>(&iprop) );
    QVERIFY(readOnlyBindable.isReadOnly());
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "setBinding: Could not set binding via bindable interface. The QBindable is read-only.");
#endif
    readOnlyBindable.setBinding(Qt::makePropertyBinding(iprop));

    QProperty<float> fprop;
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "setBinding: Could not set binding as the property expects it to be of type int but got float instead.");
#endif
    bindable.setBinding(Qt::makePropertyBinding(fprop));
}

void tst_QProperty::readonlyConstQBindable()
{
    QProperty<int> i {42};
    const QBindable<int> bindableI(const_cast<const QProperty<int> *>(&i));

    // check that read-only operations work with a const QBindable
    QVERIFY(bindableI.isReadOnly());
    QVERIFY(!bindableI.hasBinding());
    // we can still create a binding to a read only bindable through the interface
    QProperty<int> j;
    j.setBinding(bindableI.makeBinding());
    QCOMPARE(j.value(), bindableI.value());
    int counter = 0;
    auto observer = bindableI.subscribe([&](){++counter;});
    QCOMPARE(counter, 1);
    auto observer2 = bindableI.onValueChanged([&](){++counter;});
    i = 0;
    QCOMPARE(counter, 3);
}

class MyQObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo BINDABLE bindableFoo NOTIFY fooChanged)
    Q_PROPERTY(int bar READ bar WRITE setBar BINDABLE bindableBar NOTIFY barChanged)
    Q_PROPERTY(int read READ read)
    Q_PROPERTY(int computed READ computed STORED false)
    Q_PROPERTY(int compat READ compat WRITE setCompat NOTIFY compatChanged)

signals:
    void fooChanged(int newFoo);
    void barChanged();
    void compatChanged();

public slots:
    void fooHasChanged() { fooChangedCount++; }
    void barHasChanged() { barChangedCount++; }
    void compatHasChanged() { compatChangedCount++; }

public:
    int foo() const { return fooData.value(); }
    void setFoo(int i) { fooData.setValue(i); }
    int bar() const { return barData.value(); }
    void setBar(int i) { barData.setValue(i); }
    int read() const { return readData.value(); }
    int computed() const { return readData.value(); }
    int compat() const { return compatData; }
    void setCompat(int i)
    {
        if (compatData == i)
            return;
        // implement some side effect and clamping
        ++setCompatCalled;
        if (i < 0)
            i = 0;
        compatData.setValue(i);
        compatData.notify();
        emit compatChanged();
    }

    QBindable<int> bindableFoo() { return QBindable<int>(&fooData); }
    const QBindable<int> bindableFoo() const { return QBindable<int>(&fooData); }
    QBindable<int> bindableBar() { return QBindable<int>(&barData); }
    QBindable<int> bindableRead() { return QBindable<int>(&readData); }
    QBindable<int> bindableComputed() { return QBindable<int>(&computedData); }
    QBindable<int> bindableCompat() { return QBindable<int>(&compatData); }

public:
    int fooChangedCount = 0;
    int barChangedCount = 0;
    int compatChangedCount = 0;
    int setCompatCalled = 0;

    Q_OBJECT_BINDABLE_PROPERTY(MyQObject, int, fooData, &MyQObject::fooChanged);
    Q_OBJECT_BINDABLE_PROPERTY(MyQObject, int, barData, &MyQObject::barChanged);
    Q_OBJECT_BINDABLE_PROPERTY(MyQObject, int, readData);
    Q_OBJECT_COMPUTED_PROPERTY(MyQObject, int, computedData, &MyQObject::computed);
    Q_OBJECT_COMPAT_PROPERTY(MyQObject, int, compatData, &MyQObject::setCompat)
};


void tst_QProperty::qobjectBindableManualNotify()
{
    // Given an object of type MyQObject,
    MyQObject object;
    // track its foo property's change count
    auto bindable = object.bindableFoo();
    int fooChangeCount = 0;
    auto changeHandler = bindable.onValueChanged([&](){++fooChangeCount;});
    // and how many changed signals it emits.
    QSignalSpy fooChangedSpy(&object, &MyQObject::fooChanged);

    // If we bypass the bindings system,
    object.fooData.setValueBypassingBindings(42);
    // there is no change.
    QCOMPARE(fooChangeCount, 0);
    QCOMPARE(fooChangedSpy.size(), 0);
    // Once we notify manually
    object.fooData.notify();
    // observers are notified and the signal arrives.
    QCOMPARE(fooChangeCount, 1);
    QCOMPARE(fooChangedSpy.size(), 1);

    // If we set a binding
    int i = 1;
    object.fooData.setBinding([&](){return i;});
    // then the value changes
    QCOMPARE(object.foo(), 1);
    // and the change and signal count are incremented.
    QCOMPARE(fooChangeCount, 2);
    QCOMPARE(fooChangedSpy.size(), 2);
    // Changing a non-property won't trigger any notification.
    i = 2;
    QCOMPARE(fooChangeCount, 2);
    QCOMPARE(fooChangedSpy.size(), 2);
    // Manually triggering the notification
    object.fooData.notify();
    // increments the change count
    QCOMPARE(fooChangeCount, 3);
    QCOMPARE(fooChangedSpy.size(), 3);
    // but doesn't actually cause a binding reevaluation.
    QCOMPARE(object.foo(), 1);
}


struct ReallocObject : QObject {
    ReallocObject()
    { v.setBinding([this] { return x.value() + y.value() + z.value(); }); }
    Q_OBJECT_BINDABLE_PROPERTY(ReallocObject, int, v)
    Q_OBJECT_BINDABLE_PROPERTY(ReallocObject, int, x)
    Q_OBJECT_BINDABLE_PROPERTY(ReallocObject, int, y)
    Q_OBJECT_BINDABLE_PROPERTY(ReallocObject, int, z)
};

void tst_QProperty::qobjectBindableReallocatedBindingStorage()
{
    ReallocObject object;
    object.x = 1;
    QCOMPARE(object.v.value(), 1);
}

void tst_QProperty::qobjectBindableSignalTakingNewValue()
{
    // Given an object of type MyQObject,
    MyQObject object;
    // and tracking the values emitted via its fooChanged signal,
    int newValue = -1;
    QObject::connect(&object, &MyQObject::fooChanged, [&](int i){ newValue = i; } );

    // when we change the property's value via the bindable interface
    object.bindableFoo().setValue(1);
    // we obtain the newly set value.
    QCOMPARE(newValue, 1);

    // The same holds true when we set a binding
    QProperty<int> i {2};
    object.bindableFoo().setBinding(Qt::makePropertyBinding(i));
    QCOMPARE(newValue, 2);
    // and when the binding gets reevaluated to a new value
    i = 3;
    QCOMPARE(newValue, 3);
}

void tst_QProperty::testNewStuff()
{
    MyQObject testReadOnly;
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "setBinding: Could not set binding via bindable interface. The QBindable is read-only.");
#endif
    testReadOnly.bindableFoo().setBinding([](){return 42;});
    auto bindable = const_cast<const MyQObject&>(testReadOnly).bindableFoo();
    QVERIFY(bindable.hasBinding());
    QVERIFY(bindable.isReadOnly());

    MyQObject object;
    QVERIFY(!object.bindableFoo().isReadOnly());
    QObject::connect(&object, &MyQObject::fooChanged, &object, &MyQObject::fooHasChanged);
    QObject::connect(&object, &MyQObject::barChanged, &object, &MyQObject::barHasChanged);

    QCOMPARE(object.fooChangedCount, 0);
    object.setFoo(10);
    QCOMPARE(object.fooChangedCount, 1);
    QCOMPARE(object.foo(), 10);

    auto f = [&object]() -> int {
            return object.barData;
    };
    QCOMPARE(object.barChangedCount, 0);
    object.setBar(42);
    QCOMPARE(object.barChangedCount, 1);
    QCOMPARE(object.fooChangedCount, 1);
    object.fooData.setBinding(f);
    QCOMPARE(object.fooChangedCount, 2);
    QCOMPARE(object.fooData.value(), 42);
    object.setBar(666);
    QCOMPARE(object.fooChangedCount, 3);
    QCOMPARE(object.barChangedCount, 2);
    QCOMPARE(object.fooData.value(), 666);
    QCOMPARE(object.fooChangedCount, 3);

    auto f2 = [&object]() -> int {
            return object.barData / 2;
    };

    object.bindableFoo().setBinding(Qt::makePropertyBinding(f2));
    QVERIFY(object.bindableFoo().hasBinding());
    QCOMPARE(object.foo(), 333);
    auto oldBinding = object.bindableFoo().setBinding(QPropertyBinding<int>());
    QVERIFY(!object.bindableFoo().hasBinding());
    QVERIFY(!oldBinding.isNull());
    QCOMPARE(object.foo(), 333);
    object.setBar(222);
    QCOMPARE(object.foo(), 333);
    object.bindableFoo().setBinding(oldBinding);
    QCOMPARE(object.foo(), 111);

    auto b = object.bindableRead().makeBinding();
    object.bindableFoo().setBinding(b);
    QCOMPARE(object.foo(), 0);
    object.readData.setValue(10);
    QCOMPARE(object.foo(), 10);

    QCOMPARE(object.computed(), 10);
    object.readData.setValue(42);
    QCOMPARE(object.computed(), 42);

    object.bindableBar().setBinding(object.bindableComputed().makeBinding());
    QCOMPARE(object.computed(), 42);
    object.readData.setValue(111);
    QCOMPARE(object.computed(), 111);

    object.bindableComputed().setBinding(object.bindableBar().makeBinding());
    QCOMPARE(object.computed(), 111);
    object.bindableComputed().setValue(10);
    QCOMPARE(object.computed(), 111);

    QCOMPARE(object.bindableFoo().value(), 111);
    object.bindableFoo().setValue(24);
    QCOMPARE(object.foo(), 24);

    auto isCurrentlyEvaluatingBinding = []() {
        return QtPrivate::isAnyBindingEvaluating();
    };
    QVERIFY(!isCurrentlyEvaluatingBinding());
    QProperty<bool> evaluationDetector {false};
    evaluationDetector.setBinding(isCurrentlyEvaluatingBinding);
    QVERIFY(evaluationDetector.value());
    QVERIFY(!isCurrentlyEvaluatingBinding());
}

void tst_QProperty::qobjectObservers()
{
    MyQObject object;
    int onValueChangedCalled = 0;
    {
        auto handler = object.bindableFoo().onValueChanged([&onValueChangedCalled]() {
            ++onValueChangedCalled;
        });
        QCOMPARE(onValueChangedCalled, 0);

        object.setFoo(10);
        QCOMPARE(onValueChangedCalled, 1);

        object.bindableFoo().setBinding(object.bindableBar().makeBinding());
        QCOMPARE(onValueChangedCalled, 2);

        object.setBar(42);
        QCOMPARE(onValueChangedCalled, 3);
    }
    object.setBar(0);
    QCOMPARE(onValueChangedCalled, 3);
}

void tst_QProperty::compatBindings()
{
    MyQObject object;
    QObject::connect(&object, &MyQObject::fooChanged, &object, &MyQObject::fooHasChanged);
    QObject::connect(&object, &MyQObject::barChanged, &object, &MyQObject::barHasChanged);
    QObject::connect(&object, &MyQObject::compatChanged, &object, &MyQObject::compatHasChanged);

    QCOMPARE(object.compatData, 0);
    // setting data through the private interface should not call the changed signal or the public setter
    object.compatData.setValue(10);
    QCOMPARE(object.compatChangedCount, 0);
    QCOMPARE(object.setCompatCalled, 0);
    // going through the public API should emit the signal
    object.setCompat(42);
    QCOMPARE(object.compatChangedCount, 1);
    QCOMPARE(object.setCompatCalled, 1);

    // setting the same value again does nothing
    object.setCompat(42);
    QCOMPARE(object.compatChangedCount, 1);
    QCOMPARE(object.setCompatCalled, 1);

    object.setFoo(111);
    // just setting the binding. For a compat property, this should already trigger evaluation
    object.compatData.setBinding(object.bindableFoo().makeBinding());
    QCOMPARE(object.compatData.valueBypassingBindings(), 111);
    QCOMPARE(object.compatChangedCount, 2);
    QCOMPARE(object.setCompatCalled, 2);

    QCOMPARE(object.compat(), 111);
    QCOMPARE(object.compatChangedCount, 2);
    QCOMPARE(object.setCompatCalled, 2);

    object.setFoo(666);
    QCOMPARE(object.compatData.valueBypassingBindings(), 666);
    QCOMPARE(object.compatChangedCount, 3);
    QCOMPARE(object.setCompatCalled, 3);

    QCOMPARE(object.compat(), 666);
    QCOMPARE(object.compatChangedCount, 3);
    QCOMPARE(object.setCompatCalled, 3);

    object.setFoo(-42);
    QCOMPARE(object.compatChangedCount, 4);
    QCOMPARE(object.setCompatCalled, 4);

    QCOMPARE(object.compat(), 0);
    QCOMPARE(object.compatChangedCount, 4);
    QCOMPARE(object.setCompatCalled, 4);

    object.setCompat(0);
    QCOMPARE(object.compat(), 0);
    QCOMPARE(object.compatChangedCount, 4);
    QCOMPARE(object.setCompatCalled, 4);
}

void tst_QProperty::metaProperty()
{
    MyQObject object;
    QObject::connect(&object, &MyQObject::fooChanged, &object, &MyQObject::fooHasChanged);
    QObject::connect(&object, &MyQObject::barChanged, &object, &MyQObject::barHasChanged);
    QObject::connect(&object, &MyQObject::compatChanged, &object, &MyQObject::compatHasChanged);

    QCOMPARE(object.fooChangedCount, 0);
    object.setFoo(10);
    QCOMPARE(object.fooChangedCount, 1);
    QCOMPARE(object.foo(), 10);

    auto f = [&object]() -> int {
            return object.barData;
    };
    QCOMPARE(object.barChangedCount, 0);
    object.setBar(42);
    QCOMPARE(object.barChangedCount, 1);
    QCOMPARE(object.fooChangedCount, 1);
    int fooIndex = object.metaObject()->indexOfProperty("foo");
    QVERIFY(fooIndex >= 0);
    QMetaProperty fooProp = object.metaObject()->property(fooIndex);
    QVERIFY(fooProp.isValid());
    auto fooBindable = fooProp.bindable(&object);
    QVERIFY(fooBindable.isValid());
    QVERIFY(fooBindable.isBindable());
    QVERIFY(!fooBindable.hasBinding());
    fooBindable.setBinding(Qt::makePropertyBinding(f));
    QVERIFY(fooBindable.hasBinding());
    QCOMPARE(object.fooChangedCount, 2);
    QCOMPARE(object.fooData.value(), 42);
    object.setBar(666);
    QCOMPARE(object.fooChangedCount, 3);
    QCOMPARE(object.barChangedCount, 2);
    QCOMPARE(object.fooData.value(), 666);
    QCOMPARE(object.fooChangedCount, 3);

    fooBindable.setBinding(QUntypedPropertyBinding());
    QVERIFY(!fooBindable.hasBinding());
    QCOMPARE(object.fooData.value(), 666);

    object.setBar(0);
    QCOMPARE(object.fooData.value(), 666);
    object.setFoo(1);
    QCOMPARE(object.fooData.value(), 1);
}

void tst_QProperty::modifyObserverListWhileIterating()
{
        struct DestructingObserver : QPropertyObserver {
            DestructingObserver() :  QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
                auto This = static_cast<DestructingObserver *>(self);
                (*This)();
            }), m_target(this){}
            void operator()() {
                (*counter)++;
                std::destroy_at(m_target);
            }
            DestructingObserver *m_target;
            int *counter = nullptr;
        };
        union ObserverOrUninit {
            DestructingObserver observer = {};
            char* memory;
            ~ObserverOrUninit() {}
            ObserverOrUninit() {}
        };
    {
        // observer deletes itself while running the notification
        // while explicitly calling the destructor is rather unusual
        // it is completely plausible for this to happen because the object to which a
        // propertyobserver belongs has been destroyed
        ObserverOrUninit data;
        int counter = 0;
        data.observer.counter = &counter;
        QProperty<int> prop;
        QUntypedBindable bindableProp(&prop);
        bindableProp.observe(&data.observer);
        prop = 42; // should not crash
        QCOMPARE(counter, 1);
    }
    {
        // observer deletes the next observer in the list
        ObserverOrUninit data1;
        ObserverOrUninit data2;
        QProperty<int> prop;
        QUntypedBindable bindableProp(&prop);
        bindableProp.observe(&data1.observer);
        bindableProp.observe(&data2.observer);
        int counter = 0;
        data1.observer.m_target = &data2.observer;
        data1.observer.counter = &counter;
        data2.observer.m_target = &data1.observer;
        data2.observer.counter = &counter;
        prop = 42; // should not crash
        QCOMPARE(counter, 1); // only one trigger should run as the other has been deleted
    }
}

void tst_QProperty::noDoubleCapture()
{
    QProperty<long long> size;
    size = 3;
    QProperty<int> max;
    max.setBinding([&size]() -> int {
        // each loop run attempts to capture size
        for (int i = 0; i < size; ++i) {}
        return size.value();
    });
    auto bindingPriv = QPropertyBindingPrivate::get(max.binding());
    QCOMPARE(bindingPriv->dependencyObserverCount, 1U);
    size = 4; // should not crash
    QCOMPARE(max.value(), 4);
}

class CompatPropertyTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 BINDABLE bindableProp1)
    Q_PROPERTY(int prop2 READ prop2 WRITE setProp2 NOTIFY prop2Changed BINDABLE bindableProp2)
    Q_PROPERTY(int prop3 READ prop3 WRITE setProp3 NOTIFY prop3Changed BINDABLE bindableProp3)
    Q_PROPERTY(int prop4 READ prop4 WRITE setProp4 NOTIFY prop4Changed BINDABLE bindableProp4)
public:
    CompatPropertyTester(QObject *parent = nullptr) : QObject(parent) { }

    int prop1() {return prop1Data.value();}
    void setProp1(int i) { if (i == prop1Data) return; prop1Data.setValue(i); prop1Data.notify(); }
    QBindable<int> bindableProp1() {return QBindable<int>(&prop1Data);}

    int prop2() { return prop2Data.value(); }
    void setProp2(int i)
    {
        if (i == prop2Data)
            return;
        prop2Data.setValue(i);
        prop2Data.notify();
    }
    QBindable<int> bindableProp2() { return QBindable<int>(&prop2Data); }

    int prop3() { return prop3Data.value(); }
    void setProp3(int i)
    {
        if (i == prop3Data)
            return;
        prop3Data.setValue(i);
        prop3Data.notify();
    }
    QBindable<int> bindableProp3() { return QBindable<int>(&prop3Data); }

    int prop4() const
    {
        auto val = prop4Data.value();
        return val == 0 ? 42 : val;
    }

    void setProp4(int i)
    {
        if (i == prop4Data)
            return;
        prop4Data.setValue(i);
        prop4Data.notify();
    }
    QBindable<int> bindableProp4() { return QBindable<int>(&prop4Data); }

signals:
    void prop2Changed(int value);
    void prop3Changed();
    void prop4Changed(int value);

private:
    Q_OBJECT_COMPAT_PROPERTY(CompatPropertyTester, int, prop1Data, &CompatPropertyTester::setProp1)
    Q_OBJECT_COMPAT_PROPERTY(CompatPropertyTester, int, prop2Data, &CompatPropertyTester::setProp2,
                             &CompatPropertyTester::prop2Changed)
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(CompatPropertyTester, int, prop3Data,
                                       &CompatPropertyTester::setProp3,
                                       &CompatPropertyTester::prop3Changed, 1)
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(CompatPropertyTester, int, prop4Data,
                                       &CompatPropertyTester::setProp4,
                                       &CompatPropertyTester::prop4Changed,
                                       &CompatPropertyTester::prop4, 0)
};

void tst_QProperty::compatPropertyNoDobuleNotification()
{
    CompatPropertyTester tester;
    int counter = 0;
    QProperty<int> iprop {1};
    tester.bindableProp1().setBinding([&]() -> int {return iprop;});
    auto observer = tester.bindableProp1().onValueChanged([&](){++counter;});
    iprop.setValue(2);
    QCOMPARE(counter, 1);
}

void tst_QProperty::compatPropertySignals()
{
    CompatPropertyTester tester;

    // Compat property with signal. Signal has parameter.
    QProperty<int> prop2Observer;
    prop2Observer.setBinding(tester.bindableProp2().makeBinding());

    QSignalSpy prop2Spy(&tester, &CompatPropertyTester::prop2Changed);

    tester.setProp2(10);

    QCOMPARE(prop2Observer.value(), 10);
    QCOMPARE(prop2Spy.size(), 1);
    QList<QVariant> arguments = prop2Spy.takeFirst();
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(arguments.at(0).metaType().id(), QMetaType::Int);
    QCOMPARE(arguments.at(0).toInt(), 10);

    // Compat property with signal and default value. Signal has no parameter.
    QProperty<int> prop3Observer;
    prop3Observer.setBinding(tester.bindableProp3().makeBinding());
    QCOMPARE(prop3Observer.value(), 1);

    QSignalSpy prop3Spy(&tester, &CompatPropertyTester::prop3Changed);

    tester.setProp3(5);

    QCOMPARE(prop3Observer.value(), 5);
    QCOMPARE(prop3Spy.size(), 1);

    // Compat property with signal, default value, and custom setter. Signal has parameter.
    QProperty<int> prop4Observer;
    prop4Observer.setBinding(tester.bindableProp4().makeBinding());
    QCOMPARE(prop4Observer.value(), 42);

    QSignalSpy prop4Spy(&tester, &CompatPropertyTester::prop4Changed);

    tester.setProp4(10);

    QCOMPARE(prop4Observer.value(), 10);
    QCOMPARE(prop4Spy.size(), 1);
    arguments = prop4Spy.takeFirst();
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(arguments.at(0).metaType().id(), QMetaType::Int);
    QCOMPARE(arguments.at(0).toInt(), 10);

    tester.setProp4(42);

    QCOMPARE(prop4Observer.value(), 42);
    QCOMPARE(prop4Spy.size(), 1);
    arguments = prop4Spy.takeFirst();
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(arguments.at(0).metaType().id(), QMetaType::Int);
    QCOMPARE(arguments.at(0).toInt(), 42);

    tester.setProp4(0);

    QCOMPARE(prop4Observer.value(), 42);
    QCOMPARE(prop4Spy.size(), 1);
    arguments = prop4Spy.takeFirst();
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(arguments.at(0).metaType().id(), QMetaType::Int);
    QCOMPARE(arguments.at(0).toInt(), 42);
}

class FakeDependencyCreator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 NOTIFY prop1Changed BINDABLE bindableProp1)
    Q_PROPERTY(int prop2 READ prop2 WRITE setProp2 NOTIFY prop2Changed BINDABLE bindableProp2)
    Q_PROPERTY(int prop3 READ prop3 WRITE setProp3 NOTIFY prop3Changed BINDABLE bindableProp3)

signals:
    void prop1Changed();
    void prop2Changed();
    void prop3Changed();

public:
    void setProp1(int val) { prop1Data.setValue(val); prop1Data.notify();}
    void setProp2(int val) { prop2Data.setValue(val); prop2Data.notify();}
    void setProp3(int val) { prop3Data.setValue(val); prop3Data.notify();}

    int prop1() { return prop1Data; }
    int prop2() { return prop2Data; }
    int prop3() { return prop3Data; }

    QBindable<int> bindableProp1() { return QBindable<int>(&prop1Data); }
    QBindable<int> bindableProp2() { return QBindable<int>(&prop2Data); }
    QBindable<int> bindableProp3() { return QBindable<int>(&prop3Data); }

private:
    Q_OBJECT_COMPAT_PROPERTY(FakeDependencyCreator, int, prop1Data, &FakeDependencyCreator::setProp1, &FakeDependencyCreator::prop1Changed);
    Q_OBJECT_COMPAT_PROPERTY(FakeDependencyCreator, int, prop2Data, &FakeDependencyCreator::setProp2, &FakeDependencyCreator::prop2Changed);
    Q_OBJECT_COMPAT_PROPERTY(FakeDependencyCreator, int, prop3Data, &FakeDependencyCreator::setProp3, &FakeDependencyCreator::prop3Changed);
};

void tst_QProperty::noFakeDependencies()
{
    FakeDependencyCreator fdc;
    int bindingFunctionCalled = 0;
    fdc.bindableProp1().setBinding([&]() -> int {++bindingFunctionCalled; return fdc.prop2();});
    fdc.setProp2(42);
    QCOMPARE(fdc.prop1(), 42); // basic binding works

    int slotCounter = 0;
    QObject::connect(&fdc, &FakeDependencyCreator::prop1Changed, &fdc, [&](){ (void) fdc.prop3(); ++slotCounter;});
    fdc.setProp2(13);
    QCOMPARE(slotCounter, 1); // sanity check
    int old = bindingFunctionCalled;
    fdc.setProp3(100);
    QCOMPARE(old, bindingFunctionCalled);
}

class PropertyAdaptorTester : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int foo READ foo WRITE setFoo NOTIFY fooChanged)
    Q_PROPERTY(int foo1 READ foo WRITE setFoo)

signals:
    void dummySignal1();
    void dummySignal2();
    void dummySignal3();
    void dummySignal4();
    void dummySignal5();
    void dummySignal6();
    void dummySignal7();
    void dummySignal8();
    void fooChanged(int newFoo);

public slots:
    void fooHasChanged() { fooChangedCount++; }

public:
    int foo() const { return fooData; }
    void setFoo(int i)
    {
        if (i != fooData) {
            fooData = i;
            fooChanged(fooData);
        }
    }

public:
    int fooData = 0;
    int fooChangedCount = 0;
};

void tst_QProperty::propertyAdaptorBinding()
{
    QProperty<int> source { 5 };
    QProperty<int> dest1 { 99 };
    QProperty<int> dest2 { 98 };

    // Check binding of non BINDABLE property
    PropertyAdaptorTester object;
    // set up a dummy connection (needed to verify that the QBindable avoids an out-of-bounds read)
    QObject::connect(&object, &PropertyAdaptorTester::dummySignal1, [](){});
    QBindable<int> binding(&object, "foo");
    QObject::connect(&object, &PropertyAdaptorTester::fooChanged, &object,
                     &PropertyAdaptorTester::fooHasChanged);
    binding.setBinding([&]() { return source + 1; });
    QCOMPARE(object.foo(), 6);
    QCOMPARE(object.fooChangedCount, 1);

    struct MyBindable : QBindable<int> {
        using QBindable<int>::QBindable;
        QtPrivate::QPropertyAdaptorSlotObject* data() {
            return static_cast<QtPrivate::QPropertyAdaptorSlotObject*>(QUntypedBindable::data);
        }
    } dataBinding(&object, "foo");
    QPropertyBindingDataPointer data{&dataBinding.data()->bindingData()};

    QCOMPARE(data.observerCount(), 0);
    dest1.setBinding(binding.makeBinding());
    QCOMPARE(data.observerCount(), 1);
    dest2.setBinding([=]() { return binding.value() + 1; });
    binding = {};
    QCOMPARE(data.observerCount(), 2);

    // Check addNotifer
    {
        int local_foo = 0;
        auto notifier = QBindable<int>(&object, "foo").addNotifier([&]() { local_foo++; });
        QCOMPARE(data.observerCount(), 3);
        QCOMPARE(object.foo(), 6);
        QCOMPARE(dest1.value(), 6);
        QCOMPARE(dest2.value(), 7);
        QCOMPARE(local_foo, 0);
        QCOMPARE(object.fooChangedCount, 1);

        source = 7;
        QCOMPARE(object.foo(), 8);
        QCOMPARE(dest1.value(), 8);
        QCOMPARE(dest2.value(), 9);
        QCOMPARE(local_foo, 1);
        QCOMPARE(object.fooChangedCount, 2);
    }

    QCOMPARE(data.observerCount(), 2);

    // Check a new QBindable object can override the existing binding
    QBindable<int>(&object, "foo").setValue(10);
    QCOMPARE(object.foo(), 10);
    QCOMPARE(dest1.value(), 10);
    QCOMPARE(dest2.value(), 11);
    QCOMPARE(object.fooChangedCount, 3);
    source.setValue(99);
    QCOMPARE(object.foo(), 10);
    QCOMPARE(dest1.value(), 10);
    QCOMPARE(dest2.value(), 11);
    QCOMPARE(object.fooChangedCount, 3);
    object.setFoo(12);
    QCOMPARE(object.foo(), 12);
    QCOMPARE(dest1.value(), 12);
    QCOMPARE(dest2.value(), 13);
    QCOMPARE(object.fooChangedCount, 4);

    // Check binding multiple notifiers
    QProperty<int> source2 { 20 };
    source.setValue(21);
    binding = QBindable<int>(&object, "foo");
    binding.setBinding([&]() { return source + source2; });
    QCOMPARE(object.foo(), 41);
    QCOMPARE(dest1.value(), 41);
    QCOMPARE(object.fooChangedCount, 5);
    source.setValue(22);
    QCOMPARE(object.foo(), 42);
    QCOMPARE(dest1.value(), 42);
    QCOMPARE(object.fooChangedCount, 6);
    source2.setValue(21);
    QCOMPARE(object.foo(), 43);
    QCOMPARE(dest1.value(), 43);
    QCOMPARE(object.fooChangedCount, 7);

    // Check update group
    {
        const QScopedPropertyUpdateGroup guard;
        source.setValue(23);
        source2.setValue(22);
        QCOMPARE(object.foo(), 43);
        QCOMPARE(dest1.value(), 43);
        QCOMPARE(object.fooChangedCount, 7);
    }
    QCOMPARE(object.foo(), 45);
    QCOMPARE(dest1.value(), 45);
    QCOMPARE(object.fooChangedCount, 8);

    PropertyAdaptorTester object2;
    PropertyAdaptorTester object3;

    // Check multiple observers
    QBindable<int> binding2(&object2, "foo");
    QBindable<int> binding3(&object3, "foo");
    binding.setBinding([=]() { return binding2.value(); });
    binding3.setBinding([=]() { return binding.value(); });
    QCOMPARE(object.foo(), 0);
    QCOMPARE(object2.foo(), 0);
    QCOMPARE(object3.foo(), 0);
    QCOMPARE(dest1.value(), 0);
    object2.setFoo(1);
    QCOMPARE(object.foo(), 1);
    QCOMPARE(object2.foo(), 1);
    QCOMPARE(object3.foo(), 1);
    QCOMPARE(dest1.value(), 1);

    // Check interoperation with BINDABLE properties
    MyQObject bindableObject;
    bindableObject.fooData.setBinding([]() { return 5; });
    QVERIFY(bindableObject.fooData.hasBinding());
    QVERIFY(!bindableObject.barData.hasBinding());
    QVERIFY(QBindable<int>(&bindableObject, "foo").hasBinding());
    QBindable<int> bindableBar(&bindableObject, "bar");
    QVERIFY(!bindableBar.hasBinding());
    bindableBar.setBinding([]() { return 6; });
    QVERIFY(bindableBar.hasBinding());
    QVERIFY(bindableObject.barData.hasBinding());

    // Check bad arguments
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QUntypedBindable: Property is not valid");
#endif
    QVERIFY(!QBindable<int>(&object, QMetaProperty{}).isValid());
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QUntypedBindable: Property foo1 has no notify signal");
#endif
    QVERIFY(!QBindable<int>(&object, "foo1").isValid());
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QUntypedBindable: Property foo of type int does not match requested type bool");
#endif
    QVERIFY(!QBindable<bool>(&object, "foo").isValid());
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         "QUntypedBindable: Property foo does not belong to this object");
#endif
    QObject qobj;
    QVERIFY(!QBindable<int>(
                     &qobj,
                     object.metaObject()->property(object.metaObject()->indexOfProperty("foo")))
                     .isValid());
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QUntypedBindable: No property named fizz");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QUntypedBindable: Property is not valid");
#endif
    QVERIFY(!QBindable<int>(&object, "fizz").isValid());
}

#if QT_CONFIG(thread)
struct ThreadSafetyTester : public QObject
{
    Q_OBJECT

public:
    ThreadSafetyTester(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE bool hasCorrectStatus() const
    {
        return qGetBindingStorage(this)->status({}) == QtPrivate::getBindingStatus({});
    }

    Q_INVOKABLE bool bindingTest()
    {
        QProperty<QString> name(u"inThread"_s);
        bindableObjectName().setBinding([&]() -> QString { return name; });
        name = u"inThreadChanged"_s;
        const bool nameChangedCorrectly = objectName() == name;
        bindableObjectName().takeBinding();
        return nameChangedCorrectly;
    }
};


void tst_QProperty::threadSafety()
{
    QThread workerThread;
    auto cleanup = qScopeGuard([&](){
        QMetaObject::invokeMethod(&workerThread, "quit");
        workerThread.wait();
    });
    QScopedPointer<ThreadSafetyTester> scopedObj1(new ThreadSafetyTester);
    auto obj1 = scopedObj1.data();
    auto child1 = new ThreadSafetyTester(obj1);
    obj1->moveToThread(&workerThread);
    const auto mainThreadBindingStatus = QtPrivate::getBindingStatus({});
    QCOMPARE(qGetBindingStorage(child1)->status({}), nullptr);
    workerThread.start();

    bool correctStatus = false;
    bool ok = QMetaObject::invokeMethod(obj1, "hasCorrectStatus", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, correctStatus));
    QVERIFY(ok);
    QVERIFY(correctStatus);

    bool bindingWorks = false;
    ok = QMetaObject::invokeMethod(obj1, "bindingTest", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, bindingWorks));
    QVERIFY(ok);
    QVERIFY(bindingWorks);

    correctStatus = false;
    ok = QMetaObject::invokeMethod(child1, "hasCorrectStatus", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, correctStatus));
    QVERIFY(ok);
    QVERIFY(correctStatus);

    QScopedPointer scopedObj2(new ThreadSafetyTester);
    auto obj2 = scopedObj2.data();
    QCOMPARE(qGetBindingStorage(obj2)->status({}), mainThreadBindingStatus);

    obj2->setObjectName("moved");
    QCOMPARE(obj2->objectName(), "moved");

    obj2->moveToThread(&workerThread);
    correctStatus = false;
    ok = QMetaObject::invokeMethod(obj2, "hasCorrectStatus", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, correctStatus));

    QVERIFY(ok);
    QVERIFY(correctStatus);
    // potentially unsafe, but should still work (no writes in owning thread)
    QCOMPARE(obj2->objectName(), "moved");


    QScopedPointer scopedObj3(new ThreadSafetyTester);
    auto obj3 = scopedObj3.data();
    obj3->setObjectName("moved");
    QCOMPARE(obj3->objectName(), "moved");
    obj3->moveToThread(nullptr);
    QCOMPARE(obj2->objectName(), "moved");
    obj3->setObjectName("moved again");
    QCOMPARE(obj3->objectName(), "moved again");
}

class QPropertyUsingThread : public QThread
{
public:
    QPropertyUsingThread(QObject **dest, QThread *destThread) : dest(dest), destThread(destThread) {}
    void run() override
    {
        scopedObj1.reset(new ThreadSafetyTester());
        scopedObj1->setObjectName("test");
        QObject *child = new ThreadSafetyTester(scopedObj1.get());
        child->setObjectName("child");
        exec();
        scopedObj1->moveToThread(destThread);
        *dest = scopedObj1.release();
    }
    std::unique_ptr<ThreadSafetyTester> scopedObj1;
    QObject **dest;
    QThread *destThread;
};

void tst_QProperty::threadSafety2()
{
    std::unique_ptr<QObject> movedObj;
    {
        QObject *tmp = nullptr;
        QPropertyUsingThread workerThread(&tmp, QThread::currentThread());
        workerThread.start();
        workerThread.quit();
        workerThread.wait();
        movedObj.reset(tmp);
    }

    QCOMPARE(movedObj->objectName(), "test");
    QCOMPARE(movedObj->children().first()->objectName(), "child");
}
#endif // QT_CONFIG(thread)

struct CustomType
{
    CustomType() = default;
    CustomType(int val) : value(val) { }
    CustomType(int val, int otherVal) : value(val), anotherValue(otherVal) { }
    CustomType(const CustomType &) = default;
    CustomType(CustomType &&) = default;
    ~CustomType() = default;
    CustomType &operator=(const CustomType &) = default;
    CustomType &operator=(CustomType &&) = default;
    bool operator==(const CustomType &other) const
    {
        return (value == other.value) && (anotherValue == other.anotherValue);
    }

    int value = 0;
    int anotherValue = 0;
};

class PropertyWithInitializationTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 NOTIFY prop1Changed BINDABLE bindableProp1)
    Q_PROPERTY(CustomType prop2 READ prop2 WRITE setProp2 BINDABLE bindableProp2)
    Q_PROPERTY(CustomType prop3 READ prop3 WRITE setProp3 BINDABLE bindableProp3)
signals:
    void prop1Changed();

public:
    PropertyWithInitializationTester(QObject *parent = nullptr) : QObject(parent) { }

    int prop1() { return prop1Data.value(); }
    void setProp1(int i) { prop1Data = i; }
    QBindable<int> bindableProp1() { return QBindable<int>(&prop1Data); }

    CustomType prop2() { return prop2Data.value(); }
    void setProp2(CustomType val) { prop2Data = val; }
    QBindable<CustomType> bindableProp2() { return QBindable<CustomType>(&prop2Data); }

    CustomType prop3() { return prop3Data.value(); }
    void setProp3(CustomType val) { prop3Data.setValue(val); }
    QBindable<CustomType> bindableProp3() { return QBindable<CustomType>(&prop3Data); }

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PropertyWithInitializationTester, int, prop1Data, 5,
                                         &PropertyWithInitializationTester::prop1Changed)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PropertyWithInitializationTester, CustomType, prop2Data,
                                         CustomType(5))
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(PropertyWithInitializationTester, CustomType, prop3Data,
                                       &PropertyWithInitializationTester::setProp3,
                                       CustomType(10, 20))
};

void tst_QProperty::bindablePropertyWithInitialization()
{
    PropertyWithInitializationTester tester;

    QCOMPARE(tester.prop1(), 5);
    QCOMPARE(tester.prop2().value, 5);
    QCOMPARE(tester.prop3().value, 10);
    QCOMPARE(tester.prop3().anotherValue, 20);
}

void tst_QProperty::noDoubleNotification()
{
    /* dependency graph for this test
       x --> y means y depends on x
      a-->b-->d
      \       ^
       \->c--/
    */
    QProperty<int> a(0);
    QProperty<int> b;
    b.setBinding([&](){ return a.value(); });
    QProperty<int> c;
    c.setBinding([&](){ return a.value(); });
    QProperty<int> d;
    d.setBinding([&](){ return b.value() + c.value(); });
    int nNotifications = 0;
    int expected = 0;
    auto connection = d.subscribe([&](){
        ++nNotifications;
        QCOMPARE(d.value(), expected);
    });
    QCOMPARE(nNotifications, 1);
    expected = 2;
    a = 1;
    QCOMPARE(nNotifications, 2);
    expected = 4;
    a = 2;
    QCOMPARE(nNotifications, 3);
}

void tst_QProperty::groupedNotifications()
{
    QProperty<int> a(0);
    QProperty<int> b;
    b.setBinding([&](){ return a.value(); });
    QProperty<int> c;
    c.setBinding([&](){ return a.value(); });
    QProperty<int> d;
    QProperty<int> e;
    e.setBinding([&](){ return b.value() + c.value() + d.value(); });
    int nNotifications = 0;
    int expected = 0;
    auto connection = e.subscribe([&](){
        ++nNotifications;
        QCOMPARE(e.value(), expected);
    });
    QCOMPARE(nNotifications, 1);

    expected = 2;
    {
        const QScopedPropertyUpdateGroup guard;
        a = 1;
        QCOMPARE(b.value(), 0);
        QCOMPARE(c.value(), 0);
        QCOMPARE(d.value(), 0);
        QCOMPARE(nNotifications, 1);
    }
    QCOMPARE(b.value(), 1);
    QCOMPARE(c.value(), 1);
    QCOMPARE(e.value(), 2);
    QCOMPARE(nNotifications, 2);

    expected = 7;
    {
        const QScopedPropertyUpdateGroup guard;
        a = 2;
        d = 3;
        QCOMPARE(b.value(), 1);
        QCOMPARE(c.value(), 1);
        QCOMPARE(d.value(), 3);
        QCOMPARE(nNotifications, 2);
    }
    QCOMPARE(b.value(), 2);
    QCOMPARE(c.value(), 2);
    QCOMPARE(e.value(), 7);
    QCOMPARE(nNotifications, 3);


}

void tst_QProperty::groupedNotificationConsistency()
{
    QProperty<int> i(0);
    QProperty<int> j(0);
    bool areEqual = true;

    auto observer = i.onValueChanged([&](){
        areEqual = i == j;
    });

    i = 1;
    j = 1;
    QVERIFY(!areEqual); // value changed runs before j = 1

    {
        const QScopedPropertyUpdateGroup guard;
        i = 2;
        j = 2;
    }
    QVERIFY(areEqual); // value changed runs after everything has been evaluated
}

void tst_QProperty::bindingGroupMovingBindingData()
{
    auto tester = std::make_unique<ClassWithNotifiedProperty>();
    auto testerPriv = QObjectPrivate::get(tester.get());

    auto dummyNotifier = tester->property.addNotifier([](){});
    auto bindingData = testerPriv->bindingStorage.bindingData(&tester->property);
    QVERIFY(bindingData); // we have a notifier, so there should be binding data

    Qt::beginPropertyUpdateGroup();
    auto cleanup = qScopeGuard([](){ Qt::endPropertyUpdateGroup(); });
    tester->property = 42;
    QCOMPARE(testerPriv->bindingStorage.bindingData(&tester->property), bindingData);
    auto proxyData = QPropertyBindingDataPointer::proxyData(bindingData);
    // as we've modified the property, we now should have a proxy for the delayed notification
    QVERIFY(proxyData);
    // trigger binding data reallocation
    std::array<QUntypedPropertyData, 10> propertyDataArray;
    for (auto&& data: propertyDataArray)
        testerPriv->bindingStorage.bindingData(&data, true);
    // binding data has moved
    QVERIFY(testerPriv->bindingStorage.bindingData(&tester->property) != bindingData);
    bindingData = testerPriv->bindingStorage.bindingData(&tester->property);
    // the proxy data has been updated
    QCOMPARE(proxyData->originalBindingData, bindingData);

    tester.reset();
    // the property data is gone, proxyData should have been informed
    QCOMPARE(proxyData->originalBindingData, nullptr);
    QVERIFY(proxyData);
}

void tst_QProperty::bindingGroupBindingDeleted()
{
    auto deleter = std::make_unique<ClassWithNotifiedProperty>();
    auto toBeDeleted = std::make_unique<ClassWithNotifiedProperty>();

    bool calledHandler = false;
    deleter->property.setBinding([&](){
        int newValue = toBeDeleted->property;
        if (newValue == 42)
            toBeDeleted.reset();
        return newValue;
    });
    auto handler = toBeDeleted->property.onValueChanged([&]() { calledHandler = true; } );
    {
        Qt::beginPropertyUpdateGroup();
        auto cleanup = qScopeGuard([](){ Qt::endPropertyUpdateGroup(); });
        QVERIFY(toBeDeleted);
        toBeDeleted->property = 42;
        // ASAN should not complain here
    }
    QVERIFY(!toBeDeleted);
    // the change notification is sent, even if the binding is deleted during evaluation
    QVERIFY(calledHandler);
}

void tst_QProperty::uninstalledBindingDoesNotEvaluate()
{
    QProperty<int> i;
    QProperty<int> j;
    int bindingEvaluationCounter = 0;
    i.setBinding([&](){
        bindingEvaluationCounter++;
        return j.value();
    });
    QCOMPARE(bindingEvaluationCounter, 1);
    // Sanity check: if we force a binding reevaluation,
    j = 42;
    // the binding function will be called again.
    QCOMPARE(bindingEvaluationCounter, 2);
    // If we keep referencing the binding
    auto keptBinding = i.binding();
    // but have it not installed on a property
    i = 10;
    QVERIFY(!i.hasBinding());
    QVERIFY(!keptBinding.isNull());
    // then changing a dependency
    j = 12;
    // does not lead to the binding being reevaluated.
    QCOMPARE(bindingEvaluationCounter, 2);
}

void tst_QProperty::notify()
{
    QProperty<int> testProperty(0);
    QList<int> recordedValues;
    int value = 0;
    QPropertyNotifier notifier;

    {
        QPropertyNotifier handler = testProperty.addNotifier([&]() {
            recordedValues << testProperty;
        });
        notifier = testProperty.addNotifier([&]() {
            value = testProperty;
        });

        testProperty = 1;
        testProperty = 2;
    }
    QCOMPARE(value, 2);
    testProperty = 3;
    QCOMPARE(value, 3);
    notifier = {};
    testProperty = 4;
    QCOMPARE(value, 3);

    QCOMPARE(recordedValues.size(), 2);
    QCOMPARE(recordedValues.at(0), 1);
    QCOMPARE(recordedValues.at(1), 2);
}

void tst_QProperty::bindableInterfaceOfCompatPropertyUsesSetter()
{
    MyQObject obj;
    QBindable<int> bindable = obj.bindableCompat();
    QCOMPARE(obj.setCompatCalled, 0);
    bindable.setValue(42);
    QCOMPARE(obj.setCompatCalled, 1);
}

void tst_QProperty::selfBindingShouldNotCrash()
{
    QProperty<int> i;
    i.setBinding([&](){ return i+1; });
    QVERIFY(i.binding().error().hasError());
}

void tst_QProperty::qpropertyAlias()
{
#if QT_DEPRECATED_SINCE(6, 6)
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    std::unique_ptr<QProperty<int>> i {new QProperty<int>};
    QPropertyAlias<int> alias(i.get());
    QVERIFY(alias.isValid());
    alias.setValue(42);
    QCOMPARE(i->value(), 42);
    QProperty<int> j;
    bool notifierCalled = false;
    auto myNotifier = alias.addNotifier([&](){notifierCalled = true;});
    i->setBinding([&]() -> int { return j; });
    QVERIFY(notifierCalled);
    j.setValue(42);
    QCOMPARE(alias.value(), 42);
    i.reset();
    QVERIFY(!alias.isValid());
    QT_WARNING_POP
#endif
}

void tst_QProperty::scheduleNotify()
{
    int notifications = 0;
    QProperty<int> p;
    QCOMPARE(p.value(), 0);
    const auto handler = p.addNotifier([&](){ ++notifications; });
    QCOMPARE(notifications, 0);
    QPropertyBinding<int> b([]() { return 0; }, QPropertyBindingSourceLocation());
    QPropertyBindingPrivate::get(b)->scheduleNotify();
    QCOMPARE(notifications, 0);
    p.setBinding(b);
    QCOMPARE(notifications, 1);
    QCOMPARE(p.value(), 0);
}

void tst_QProperty::notifyAfterAllDepsGone()
{
    bool b = true;
    QProperty<int> iprop;
    QProperty<int> jprop(42);
    iprop.setBinding([&](){
        if (b)
            return jprop.value();
        return 13;
    });
    int changeCounter = 0;
    auto keepAlive = iprop.onValueChanged([&](){ changeCounter++; });
    QCOMPARE(iprop.value(), 42);
    jprop = 44;
    QCOMPARE(iprop.value(), 44);
    QCOMPARE(changeCounter, 1);
    b = false;
    jprop = 43;
    QCOMPARE(iprop.value(), 13);
    QCOMPARE(changeCounter, 2);
}

class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int signaled READ signaled WRITE setSignaled NOTIFY signaledChanged FINAL)
    Q_PROPERTY(int bindable1 READ bindable1 WRITE setBindable1 BINDABLE bindable1Bindable NOTIFY bindable1Changed FINAL)
    Q_PROPERTY(int bindable2 READ bindable2 WRITE setBindable2 BINDABLE bindable2Bindable NOTIFY bindable2Changed FINAL)

public:
    int signaled() const
    {
        return m_signaled;
    }

    void setSignaled(int newSignaled)
    {
        if (m_signaled == newSignaled)
            return;
        m_signaled = newSignaled;
        emit signaledChanged();
    }

    int bindable1() const
    {
        return m_bindable1;
    }

    void setBindable1(int newBindable1)
    {
        if (m_bindable1 == newBindable1)
            return;
        m_bindable1 = newBindable1;
        emit bindable1Changed();
    }

    QBindable<int> bindable1Bindable()
    {
        return QBindable<int>(&m_bindable1);
    }

    int bindable2() const
    {
        return m_bindable2;
    }

    void setBindable2(int newBindable2)
    {
        if (m_bindable2 == newBindable2)
            return;
        m_bindable2 = newBindable2;
        emit bindable2Changed();
    }

    QBindable<int> bindable2Bindable()
    {
        return QBindable<int>(&m_bindable2);
    }

signals:
    void signaledChanged();
    void bindable1Changed();
    void bindable2Changed();

private:
    int m_signaled = 0;
    Q_OBJECT_COMPAT_PROPERTY(TestObject, int, m_bindable1, &TestObject::setBindable1, &TestObject::bindable1Changed);
    Q_OBJECT_COMPAT_PROPERTY(TestObject, int, m_bindable2, &TestObject::setBindable2, &TestObject::bindable2Changed);
};

void tst_QProperty::propertyUpdateViaSignaledProperty()
{
    TestObject o;
    QProperty<int> rootTrigger;
    QProperty<int> signalTrigger;

    o.bindable1Bindable().setBinding([&]() {
        return rootTrigger.value();
    });

    QObject::connect(&o, &TestObject::bindable1Changed, &o, [&]() {
        // Signaled changes only once, doesn't actually depend on bindable1.
        // In reality, there could be some complicated calculation behind this that changes
        // on certain checkpoints, but not on every iteration.
        o.setSignaled(40);
    });

    o.bindable2Bindable().setBinding([&]() {
        return signalTrigger.value() - o.bindable1();
    });

    QObject::connect(&o, &TestObject::signaledChanged, &o, [&]() {
        signalTrigger.setValue(o.signaled());
    });

    rootTrigger.setValue(2);
    QCOMPARE(o.bindable1(), 2);
    QCOMPARE(o.bindable2(), 38);
    rootTrigger.setValue(3);
    QCOMPARE(o.bindable1(), 3);
    QCOMPARE(o.bindable2(), 37);
    rootTrigger.setValue(4);
    QCOMPARE(o.bindable1(), 4);
    QCOMPARE(o.bindable2(), 36);
}

QTEST_MAIN(tst_QProperty);

#undef QT_SOURCE_LOCATION_NAMESPACE

#include "tst_qproperty.moc"
