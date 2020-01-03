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
#include <private/qpropertybinding_p.h>

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
    QCOMPARE(QPropertyBasePointer::get(propWithBinding).bindingPtr()->dependencyObservers.size(), 2);
    QVERIFY(QPropertyBasePointer::get(propWithBinding).bindingPtr()->dependencyObservers.capacity() >= 2);

    firstDependency = 100;
    QCOMPARE(propWithBinding.value(), int(110));
    QCOMPARE(QPropertyBasePointer::get(propWithBinding).bindingPtr()->dependencyObservers.size(), 2);
    QVERIFY(QPropertyBasePointer::get(propWithBinding).bindingPtr()->dependencyObservers.capacity() >= 2);
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
    QVector<int> recordedValues;

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
    QVector<int> recordedValues;

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
    QCOMPARE(QPropertyBindingPrivate::get(binding)->location.line, bindingLine);
#else
    QSKIP("Skipping this in the light of missing binding source location support");
#endif
}

void tst_QProperty::bindingError()
{
    QProperty<int> prop = Qt::makePropertyBinding([]() -> std::variant<int, QPropertyBindingError> {
        QPropertyBindingError error(QPropertyBindingError::UnknownError);
        error.setDescription(QLatin1String("my error"));
        return error;
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
            return false;
        }, QPropertyBindingSourceLocation());
        QVERIFY(!property.setBinding(doubleBinding));
    }

    QUntypedPropertyBinding intBinding(QMetaType::fromType<int>(),
                                    [](const QMetaType &metaType, void *dataPtr) -> bool {
        Q_ASSERT(metaType.id() == qMetaTypeId<int>());

        int *intPtr = reinterpret_cast<int*>(dataPtr);
        if (*intPtr == 100)
            return false;
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
        if (*boolPtr)
            return false;
        *boolPtr = true;
        return true;
    }, QPropertyBindingSourceLocation());
    QVERIFY(property.setBinding(boolBinding));

    QVERIFY(property.value());
}

QTEST_MAIN(tst_QProperty);

#include "tst_qproperty.moc"
