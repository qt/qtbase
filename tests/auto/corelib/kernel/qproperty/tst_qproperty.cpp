/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QObject>
#include <qtest.h>
#include <qproperty.h>
#include <private/qproperty_p.h>

using namespace QtPrivate;

class tst_QProperty : public QObject
{
    Q_OBJECT
private slots:
    void functorBinding();
    void basicDependencies();
    void multipleDependencies();
    void bindingWithDeletedDependency();
    void recursiveDependency();
    void bindingAfterUse();
    void switchBinding();
    void avoidDependencyAllocationAfterFirstEval();
    void propertyArrays();
    void boolProperty();
    void takeBinding();
    void replaceBinding();
    void swap();
    void moveNotifies();
    void moveCtor();
    void changeHandler();
    void propertyChangeHandlerApi();
    void subscribe();
    void changeHandlerThroughBindings();
    void dontTriggerDependenciesIfUnchangedValue();
    void bindingSourceLocation();
    void bindingError();
    void bindingLoop();
    void changePropertyFromWithinChangeHandler();
    void changePropertyFromWithinChangeHandlerThroughDependency();
    void changePropertyFromWithinChangeHandler2();
    void settingPropertyValueDoesRemoveBinding();
    void genericPropertyBinding();
    void genericPropertyBindingBool();
    void setBindingFunctor();
    void multipleObservers();
    void propertyAlias();
    void notifiedProperty();
    void notifiedPropertyWithOldValueCallback();
    void notifiedPropertyWithGuard();
    void typeNoOperatorEqual();
    void bindingValueReplacement();
};

void tst_QProperty::functorBinding()
{
    QProperty<int> property([]() { return 42; });
    QCOMPARE(property.value(), int(42));
    property = Qt::makePropertyBinding([]() { return 100; });
    QCOMPARE(property.value(), int(100));
    property.setBinding([]() { return 50; });
    QCOMPARE(property.value(), int(50));
}

void tst_QProperty::basicDependencies()
{
    QProperty<int> right(100);

    QProperty<int> left = Qt::makePropertyBinding(right);

    QCOMPARE(left.value(), int(100));

    right = 42;

    QCOMPARE(left.value(), int(42));
}

void tst_QProperty::multipleDependencies()
{
    QProperty<int> firstDependency(1);
    QProperty<int> secondDependency(2);

    QProperty<int> sum;
    sum = Qt::makePropertyBinding([&]() { return firstDependency + secondDependency; });

    QCOMPARE(QPropertyBasePointer::get(firstDependency).observerCount(), 0);
    QCOMPARE(QPropertyBasePointer::get(secondDependency).observerCount(), 0);

    QCOMPARE(sum.value(), int(3));
    QCOMPARE(QPropertyBasePointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBasePointer::get(secondDependency).observerCount(), 1);

    firstDependency = 10;

    QCOMPARE(sum.value(), int(12));
    QCOMPARE(QPropertyBasePointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBasePointer::get(secondDependency).observerCount(), 1);

    secondDependency = 20;

    QCOMPARE(sum.value(), int(30));
    QCOMPARE(QPropertyBasePointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBasePointer::get(secondDependency).observerCount(), 1);

    firstDependency = 1;
    secondDependency = 1;
    QCOMPARE(sum.value(), int(2));
    QCOMPARE(QPropertyBasePointer::get(firstDependency).observerCount(), 1);
    QCOMPARE(QPropertyBasePointer::get(secondDependency).observerCount(), 1);
}

void tst_QProperty::bindingWithDeletedDependency()
{
    QScopedPointer<QProperty<int>> dynamicProperty(new QProperty<int>(100));

    QProperty<int> staticProperty(1000);

    QProperty<bool> bindingReturnsDynamicProperty(false);

    QProperty<int> propertySelector;
    propertySelector = Qt::makePropertyBinding([&]() {
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

void tst_QProperty::recursiveDependency()
{
    QProperty<int> first(1);

    QProperty<int> second;
    second = Qt::makePropertyBinding(first);

    QProperty<int> third;
    third = Qt::makePropertyBinding(second);

    QCOMPARE(third.value(), int(1));

    first = 2;

    QCOMPARE(third.value(), int(2));
}

void tst_QProperty::bindingAfterUse()
{
    QProperty<int> propWithBindingLater(1);

    QProperty<int> propThatUsesFirstProp;
    propThatUsesFirstProp = Qt::makePropertyBinding(propWithBindingLater);

    QCOMPARE(propThatUsesFirstProp.value(), int(1));
    QCOMPARE(QPropertyBasePointer::get(propWithBindingLater).observerCount(), 1);

    QProperty<int> injectedValue(42);
    propWithBindingLater = Qt::makePropertyBinding(injectedValue);

    QCOMPARE(propThatUsesFirstProp.value(), int(42));
    QCOMPARE(QPropertyBasePointer::get(propWithBindingLater).observerCount(), 1);
}

void tst_QProperty::switchBinding()
{
    QProperty<int> first(1);

    QProperty<int> propWithChangingBinding;
    propWithChangingBinding = Qt::makePropertyBinding(first);

    QCOMPARE(propWithChangingBinding.value(), 1);

    QProperty<int> output;
    output = Qt::makePropertyBinding(propWithChangingBinding);
    QCOMPARE(output.value(), 1);

    QProperty<int> second(2);
    propWithChangingBinding = Qt::makePropertyBinding(second);
    QCOMPARE(output.value(), 2);
}

void tst_QProperty::avoidDependencyAllocationAfterFirstEval()
{
    QProperty<int> firstDependency(1);
    QProperty<int> secondDependency(10);

    QProperty<int> propWithBinding;
    propWithBinding = Qt::makePropertyBinding([&]() { return firstDependency + secondDependency; });

    QCOMPARE(propWithBinding.value(), int(11));

    QVERIFY(QPropertyBasePointer::get(propWithBinding).bindingPtr());
    QCOMPARE(QPropertyBasePointer::get(propWithBinding).bindingPtr()->dependencyObserverCount, 2u);

    firstDependency = 100;
    QCOMPARE(propWithBinding.value(), int(110));
    QCOMPARE(QPropertyBasePointer::get(propWithBinding).bindingPtr()->dependencyObserverCount, 2u);
}

void tst_QProperty::propertyArrays()
{
    std::vector<QProperty<int>> properties;

    int expectedSum = 0;
    for (int i = 0; i < 10; ++i) {
        properties.emplace_back(i);
        expectedSum += i;
    }

    QProperty<int> sum;
    sum = Qt::makePropertyBinding([&]() {
        return std::accumulate(properties.begin(), properties.end(), 0);
    });

    QCOMPARE(sum.value(), expectedSum);

    properties[4] = properties[4] + 42;
    expectedSum += 42;
    QCOMPARE(sum.value(), expectedSum);
}

void tst_QProperty::boolProperty()
{
    static_assert(sizeof(QProperty<bool>) == sizeof(void*), "Size of QProperty<bool> specialization must not exceed size of pointer");

    QProperty<bool> first(true);
    QProperty<bool> second(false);
    QProperty<bool> all;
    all = Qt::makePropertyBinding([&]() { return first && second; });

    QCOMPARE(all.value(), false);

    second = true;

    QCOMPARE(all.value(), true);
}

void tst_QProperty::takeBinding()
{
    QPropertyBinding<int> existingBinding;
    QVERIFY(existingBinding.isNull());

    QProperty<int> first(100);
    QProperty<int> second = Qt::makePropertyBinding(first);

    QCOMPARE(second.value(), int(100));

    existingBinding = second.takeBinding();
    QVERIFY(!existingBinding.isNull());

    first = 10;
    QCOMPARE(second.value(), int(100));

    second = 25;
    QCOMPARE(second.value(), int(25));

    second = existingBinding;
    QCOMPARE(second.value(), int(10));
    QVERIFY(!existingBinding.isNull());
}

void tst_QProperty::replaceBinding()
{
    QProperty<int> first(100);
    QProperty<int> second = Qt::makePropertyBinding(first);

    QCOMPARE(second.value(), 100);

    auto constantBinding = Qt::makePropertyBinding([]() { return 42; });
    auto oldBinding = second.setBinding(constantBinding);
    QCOMPARE(second.value(), 42);

    second = oldBinding;
    QCOMPARE(second.value(), 100);
}

void tst_QProperty::swap()
{
    QProperty<int> firstDependency(1);
    QProperty<int> secondDependency(2);

    QProperty<int> first = Qt::makePropertyBinding(firstDependency);
    QProperty<int> second = Qt::makePropertyBinding(secondDependency);

    QCOMPARE(first.value(), 1);
    QCOMPARE(second.value(), 2);

    std::swap(first, second);

    QCOMPARE(first.value(), 2);
    QCOMPARE(second.value(), 1);

    secondDependency = 20;
    QCOMPARE(first.value(), 20);
    QCOMPARE(second.value(), 1);

    firstDependency = 100;
    QCOMPARE(first.value(), 20);
    QCOMPARE(second.value(), 100);
}

void tst_QProperty::moveNotifies()
{
    QProperty<int> first(1);
    QProperty<int> second(2);

    QProperty<int> propertyInTheMiddle = Qt::makePropertyBinding(first);

    QProperty<int> finalProp1 = Qt::makePropertyBinding(propertyInTheMiddle);
    QProperty<int> finalProp2 = Qt::makePropertyBinding(propertyInTheMiddle);

    QCOMPARE(finalProp1.value(), 1);
    QCOMPARE(finalProp2.value(), 1);

    QCOMPARE(QPropertyBasePointer::get(propertyInTheMiddle).observerCount(), 2);

    QProperty<int> other = Qt::makePropertyBinding(second);
    QCOMPARE(other.value(), 2);

    QProperty<int> otherDep = Qt::makePropertyBinding(other);
    QCOMPARE(otherDep.value(), 2);
    QCOMPARE(QPropertyBasePointer::get(other).observerCount(), 1);

    propertyInTheMiddle = std::move(other);

    QCOMPARE(QPropertyBasePointer::get(other).observerCount(), 0);

    QCOMPARE(finalProp1.value(), 2);
    QCOMPARE(finalProp2.value(), 2);
}

void tst_QProperty::moveCtor()
{
    QProperty<int> first(1);

    QProperty<int> intermediate = Qt::makePropertyBinding(first);
    QCOMPARE(intermediate.value(), 1);
    QCOMPARE(QPropertyBasePointer::get(first).observerCount(), 1);

    QProperty<int> targetProp(std::move(first));

    QCOMPARE(QPropertyBasePointer::get(targetProp).observerCount(), 0);
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

    QCOMPARE(recordedValues.count(), 2);
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

    QCOMPARE(recordedValues.count(), 3);
    QCOMPARE(recordedValues.at(0), 42);
    QCOMPARE(recordedValues.at(1), 1);
    QCOMPARE(recordedValues.at(2), 2);
}

void tst_QProperty::changeHandlerThroughBindings()
{
    QProperty<bool> trigger(false);
    QProperty<bool> blockTrigger(false);
    QProperty<bool> condition = Qt::makePropertyBinding([&]() {
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
    QProperty<int> observer = Qt::makePropertyBinding([&]() { triggered = true; return property.value(); });

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
    auto bindingLine = std::experimental::source_location::current().line() + 1;
    auto binding = Qt::makePropertyBinding([]() { return 42; });
    QCOMPARE(QPropertyBindingPrivate::get(binding)->sourceLocation().line, bindingLine);
#else
    QSKIP("Skipping this in the light of missing binding source location support");
#endif
}

void tst_QProperty::bindingError()
{
    QProperty<int> prop = Qt::makePropertyBinding([]() -> int {
        QPropertyBindingError error(QPropertyBindingError::UnknownError, QLatin1String("my error"));
        QPropertyBindingPrivate::currentlyEvaluatingBinding()->setError(std::move(error));
        return 0;
    });
    QCOMPARE(prop.value(), 0);
    QCOMPARE(prop.binding().error().description(), QString("my error"));
}

void tst_QProperty::bindingLoop()
{
    QScopedPointer<QProperty<int>> firstProp;

    QProperty<int> secondProp = Qt::makePropertyBinding([&]() -> int {
        return firstProp ? firstProp->value() : 0;
    });

    QProperty<int> thirdProp = Qt::makePropertyBinding([&]() -> int {
        return secondProp.value();
    });

    firstProp.reset(new QProperty<int>());
    *firstProp = Qt::makePropertyBinding([&]() -> int {
        return secondProp.value();
    });

    QCOMPARE(thirdProp.value(), 0);
    QCOMPARE(secondProp.binding().error().type(), QPropertyBindingError::BindingLoop);
}

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
    QProperty<int> property = Qt::makePropertyBinding(sourceProperty);
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
    QCOMPARE(property.value(), 100);
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
}

void tst_QProperty::settingPropertyValueDoesRemoveBinding()
{
    QProperty<int> source(42);

    QProperty<int> property = Qt::makePropertyBinding(source);

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
                                              [](const QMetaType &, void *) -> bool {
            Q_ASSERT(false);
            return true;
        }, QPropertyBindingSourceLocation());
        QVERIFY(!property.setBinding(doubleBinding));
    }

    QUntypedPropertyBinding intBinding(QMetaType::fromType<int>(),
                                    [](const QMetaType &metaType, void *dataPtr) -> bool {
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
            [](const QMetaType &, void *dataPtr) -> bool {
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

void tst_QProperty::propertyAlias()
{
    QScopedPointer<QProperty<int>> property(new QProperty<int>);
    property->setValue(5);
    QPropertyAlias alias(property.get());
    QVERIFY(alias.isValid());
    QCOMPARE(alias.value(), 5);

    int value1 = 1;
    auto changeHandler = alias.onValueChanged([&]() { value1 = alias.value(); });
    QCOMPARE(value1, 1);

    int value2 = 2;
    auto subscribeHandler = alias.subscribe([&]() { value2 = alias.value(); });
    QCOMPARE(value2, 5);

    alias.setValue(6);
    QVERIFY(alias.isValid());
    QCOMPARE(alias.value(), 6);
    QCOMPARE(value1, 6);
    QCOMPARE(value2, 6);

    alias.setBinding([]() { return 12; });
    QCOMPARE(value1, 12);
    QCOMPARE(value2, 12);
    QCOMPARE(alias.value(), 12);

    alias.setValue(22);
    QCOMPARE(value1, 22);
    QCOMPARE(value2, 22);
    QCOMPARE(alias.value(), 22);

    property.reset();

    QVERIFY(!alias.isValid());
    QCOMPARE(alias.value(), int());
    QCOMPARE(value1, 22);
    QCOMPARE(value2, 22);

    // Does not crash
    alias.setValue(25);
    QCOMPARE(alias.value(), int());
    QCOMPARE(value1, 22);
    QCOMPARE(value2, 22);
}

struct ClassWithNotifiedProperty
{
    QList<int> recordedValues;

    void callback() { recordedValues << property.value(); }

    QNotifiedProperty<int, &ClassWithNotifiedProperty::callback> property;
};

struct ClassWithNotifiedProperty2
{
    QList<int> recordedValues;

    void callback(int oldValue) { recordedValues << oldValue; }

    QNotifiedProperty<int, &ClassWithNotifiedProperty2::callback> property;
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

    instance.property.setValue(&instance, 42);
    QCOMPARE(instance.recordedValues.count(), 1);
    QCOMPARE(instance.recordedValues.at(0), 42);
    instance.recordedValues.clear();
    check();

    instance.property.setValue(&instance, 42);
    QVERIFY(instance.recordedValues.isEmpty());
    check();

    int subscribedCount = 0;
    QProperty<int> injectedValue(100);
    instance.property.setBinding(&instance, [&injectedValue]() { return injectedValue.value(); });
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
    QCOMPARE(instance.recordedValues.count(), 1);
    QCOMPARE(instance.recordedValues.at(0), 100);
    instance.recordedValues.clear();
    check();
    QCOMPARE(subscribedCount, 0);

    injectedValue = 200;
    QCOMPARE(instance.property.value(), 200);
    QCOMPARE(instance.recordedValues.count(), 1);
    QCOMPARE(instance.recordedValues.at(0), 200);
    instance.recordedValues.clear();
    check();
    QCOMPARE(subscribedCount, 10);
    subscribedCount = 0;

    injectedValue = 400;
    QCOMPARE(instance.property.value(), 400);
    QCOMPARE(instance.recordedValues.count(), 1);
    QCOMPARE(instance.recordedValues.at(0), 400);
    instance.recordedValues.clear();
    check();
    QCOMPARE(subscribedCount, 10);
}

void tst_QProperty::notifiedPropertyWithOldValueCallback()
{
    ClassWithNotifiedProperty2 instance;
    instance.property.setValue(&instance, 1);
    instance.property.setBinding(&instance, [](){return 2;});
    instance.property.setBinding(&instance, [](){return 3;});
    instance.property.setValue(&instance, 4);
    QList<int> expected {0, 1, 2, 3};
    QCOMPARE(instance.recordedValues, expected);
    QCOMPARE(instance.property.value(), 4);
}

struct ClassWithNotifiedPropertyWithGuard
{
    using This = ClassWithNotifiedPropertyWithGuard;
    QList<int> recordedValues;

    void callback() { recordedValues << property.value(); }
    void callback2() { recordedValues << property2.value(); }
    void callback3() {}
    bool trivialGuard(int ) {return true;}
    bool reject42(int newValue) {return newValue != 42;}
    bool bound(int &newValue) { newValue = qBound<int>(0, newValue, 100); return true; }

    QNotifiedProperty<int, &This::callback, &This::trivialGuard> property;
    QNotifiedProperty<int, &This::callback2, &This::reject42> property2;
    QNotifiedProperty<int, &This::callback3, &This::bound> property3;
};

void tst_QProperty::notifiedPropertyWithGuard()
{
    {
        // property with guard that returns always true is the same as using no guard
        ClassWithNotifiedPropertyWithGuard instance;

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

        instance.property.setValue(&instance, 42);
        QCOMPARE(instance.recordedValues.count(), 1);
        QCOMPARE(instance.recordedValues.at(0), 42);
        instance.recordedValues.clear();
        check();

        instance.property.setValue(&instance, 42);
        QVERIFY(instance.recordedValues.isEmpty());
        check();

        int subscribedCount = 0;
        QProperty<int> injectedValue(100);
        instance.property.setBinding(&instance, [&injectedValue]() { return injectedValue.value(); });
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
        QCOMPARE(instance.recordedValues.count(), 1);
        QCOMPARE(instance.recordedValues.at(0), 100);
        instance.recordedValues.clear();
        check();
        QCOMPARE(subscribedCount, 0);

        injectedValue = 200;
        QCOMPARE(instance.property.value(), 200);
        QCOMPARE(instance.recordedValues.count(), 1);
        QCOMPARE(instance.recordedValues.at(0), 200);
        instance.recordedValues.clear();
        check();
        QCOMPARE(subscribedCount, 10);
        subscribedCount = 0;

        injectedValue = 400;
        QCOMPARE(instance.property.value(), 400);
        QCOMPARE(instance.recordedValues.count(), 1);
        QCOMPARE(instance.recordedValues.at(0), 400);
        instance.recordedValues.clear();
        check();
        QCOMPARE(subscribedCount, 10);
    }

    {
        // Values can be rejected
        ClassWithNotifiedPropertyWithGuard instance2;

        instance2.property2.setValue(&instance2, 1);
        instance2.property2.setBinding(&instance2, [](){return 42;});
        instance2.property2.setValue(&instance2, 2);
        instance2.property2.setBinding(&instance2, [](){return 3;});
        instance2.property2.setValue(&instance2, 42);
        // Note that we get 1 twice
        // This is an unfortunate result of the lazyness used for bindings
        // When we call setBinding, the binding does not get evaluated, but we
        // call the callback in notify; the callback will in our case query the
        // properties' value. At that point we then evaluate the binding, and
        // notice that the value is in fact disallowed. Thus we return the old
        // value.
        QList<int> expected {1,  1, 2, 3};
        QCOMPARE(instance2.recordedValues, expected);
    }

    {
        // guard can modify incoming values
        ClassWithNotifiedPropertyWithGuard instance3;
        instance3.property3.setValue(&instance3, 5);
        int i1 = 5;
        QCOMPARE(instance3.property3.value(), i1);
        instance3.property3.setBinding(&instance3, [](){return 255;});
        QCOMPARE(instance3.property3.value(), 100);
        const int i2 = -1;
        instance3.property3.setValue(&instance3, i2);
        QCOMPARE(instance3.property3.value(), 0);
    }
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
    QProperty<Uncomparable> p2 = Qt::makePropertyBinding(p1);

    QCOMPARE(p1.value().data, p2.value().data);
    p1.setValue(u1);
    QCOMPARE(p1.value().data, u1.data);
    QCOMPARE(p1.value().data, p2.value().data);
    p2.setValue(u2);
    QCOMPARE(p1.value().data, u1.data);
    QCOMPARE(p2.value().data, u2.data);

    QProperty<Uncomparable> p3 = Qt::makePropertyBinding(p1);
    p1.setValue(u1);
    QCOMPARE(p1.value().data, p3.value().data);

    QNotifiedProperty<Uncomparable, &Uncomparable::changed> np;
    QVERIFY(np.value().data != u1.data);
    np.setValue(&u1, u1);
    QVERIFY(u1.changedCalled);
    u1.changedCalled = false;
    QCOMPARE(np.value().data, u1.data);
    np.setValue(&u1, u1);
    QVERIFY(u1.changedCalled);
}


struct Test {
    void notify() {};
    bool bindText(int);
    bool bindIconText(int);
    QProperty<int> text;
    QNotifiedProperty<int, &Test::notify, &Test::bindIconText> iconText;
};

bool Test::bindIconText(int) {
    Q_UNUSED(iconText.value()); // force read
    if (!iconText.hasBinding()) {
        iconText.setBinding(this, [=]() { return 0; });
    }
    return true;
}

void tst_QProperty::bindingValueReplacement()
{
    Test test;
    test.text = 0;
    test.bindIconText(0);
    test.iconText.setValue(&test, 42); // should not crash
    QCOMPARE(test.iconText.value(), 42);
    test.text = 1;
    QCOMPARE(test.iconText.value(), 42);
}

QTEST_MAIN(tst_QProperty);

#include "tst_qproperty.moc"
