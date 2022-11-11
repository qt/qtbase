// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROPERTYTESTHELPER_P_H
#define QPROPERTYTESTHELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QProperty>
#include <QtTest/QSignalSpy>
#include <QTest>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

/*!
    \internal

    This helper macro is used as a wrapper around \l QVERIFY2() to provide a
    detailed error message in case of failure. It is intended to be used \e only
    in the helper functions below.

    The custom \a comparator method is used to check if the \a actual and
    \a expected values are equal or not.

    The macro uses a custom \a represent callback to generate the string
    representation of \a actual and \a expected.

    The error message is close to the one provided by the \l QCOMPARE() macro.
    Specifically the implementation is taken from the \c formatFailMessage()
    function, which is defined in the \c qtestresult.cpp file.
*/
#define QPROPERTY_TEST_COMPARISON_HELPER(actual, expected, comparator, represent)                  \
    do {                                                                                           \
        const size_t maxMsgLen = 1024;                                                             \
        char msg[maxMsgLen] = { '\0' };                                                            \
        auto actualStr = represent(actual);                                                        \
        auto expectedStr = represent(expected);                                                    \
        const size_t len1 = mbstowcs(nullptr, #actual, maxMsgLen);                                 \
        const size_t len2 = mbstowcs(nullptr, #expected, maxMsgLen);                               \
        qsnprintf(msg, maxMsgLen, "\n%s\n   Actual   (%s)%*s %s\n   Expected (%s)%*s %s\n",        \
                  "Comparison failed!", #actual, qMax(len1, len2) - len1 + 1, ":",                 \
                  actualStr ? actualStr : "<null>", #expected, qMax(len1, len2) - len2 + 1, ":",   \
                  expectedStr ? expectedStr : "<null>");                                           \
        delete[] actualStr;                                                                        \
        delete[] expectedStr;                                                                      \
        QVERIFY2(comparator(actual, expected), msg);                                               \
    } while (false)

/*!
    \internal
    Basic testing of a bindable property.

    This helper function tests the behavior of bindable read/write property
    \a propertyName, of type \c PropertyType, in class \c TestedClass.
    The caller must supply an \a instance of \c TestedClass and two distinct
    values, \a initial and \a changed, of \c PropertyType.

    Since the first part of the test sets the property to \a initial, it
    \e {must not} be the default value of the property, or the check that it
    was set will be vacuous.

    By default \c {operator==()} is used to compare values of the property and
    \c {QTest::toString()} is used to generate proper error messages.

    If such comparison is not supported for \c PropertyType, or the comparison
    it supports is not appropriate to this property, a custom \a comparator can
    be supplied.

    Apart from that, a custom \a represent callback can also be specified to
    generate a string representation of \c PropertyType. If supplied, it must
    allocate its returned string using \c {new char[]}, so that it can be used
    in place of \l {QTest::toString()}.

    \note Any test calling this method will need to call
    \code
    if (QTest::currentTestFailed())
        return;
    \endcode
    after doing so, if there is any later code in the test. If testing several
    properties in one test method, emitting a warning message saying which
    property failed, before returning, is a kindness to readers of the output.
*/
template<typename TestedClass, typename PropertyType>
void testReadWritePropertyBasics(
        TestedClass &instance, const PropertyType &initial, const PropertyType &changed,
        const char *propertyName,
        std::function<bool(const PropertyType &, const PropertyType &)> comparator =
                [](const PropertyType &lhs, const PropertyType &rhs) { return lhs == rhs; },
        std::function<char *(const PropertyType &)> represent =
                [](const PropertyType &val) { return QTest::toString(val); })
{
    // get the property
    const QMetaObject *metaObject = instance.metaObject();
    QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(propertyName));
    QVERIFY2(metaProperty.metaType() == QMetaType::fromType<PropertyType>(),
             QByteArray("Preconditions not met for ") +  propertyName  + "\n"
             "The type of initial and changed value does not match the type of the property.\n"
             "Please ensure that the types match exactly (convertability is not enough).\n"
             "You can provide the template types to the "
             "function explicitly to force a certain type.\n"
             "Expected was a " + metaProperty.metaType().name()
             + " but " + QMetaType::fromType<PropertyType>().name() + " was provided.");

    // in case the TestedClass has setProperty()/property() methods.
    QObject &testedObj = static_cast<QObject &>(instance);

    QVERIFY2(metaProperty.isBindable() && metaProperty.isWritable(),
             "Preconditions not met for " + QByteArray(propertyName));

    QScopedPointer<QSignalSpy> spy(nullptr);
    if (metaProperty.hasNotifySignal())
        spy.reset(new QSignalSpy(&instance, metaProperty.notifySignal()));

    testedObj.setProperty(propertyName, QVariant::fromValue(initial));
    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), initial, comparator,
            represent);
    if (spy)
        QCOMPARE(spy->size(), 1);

    QUntypedBindable bindable = metaProperty.bindable(&instance);

    // Bind to the object's property (using both lambda and
    // Qt:makePropertyBinding).
    QProperty<PropertyType> propObserver(changed);
    propObserver.setBinding(bindable.makeBinding());
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), initial, comparator, represent);

    QProperty<PropertyType> propObserverLambda(changed);
    propObserverLambda.setBinding(
            [&]() { return testedObj.property(propertyName).template value<PropertyType>(); });
    QPROPERTY_TEST_COMPARISON_HELPER(propObserverLambda.value(), initial, comparator, represent);

    testedObj.setProperty(propertyName, QVariant::fromValue(changed));
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), changed, comparator, represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserverLambda.value(), changed, comparator, represent);
    if (spy)
        QCOMPARE(spy->size(), 2);

    // Bind object's property to other property
    QProperty<PropertyType> propSetter(initial);
    QVERIFY(!bindable.hasBinding());
    bindable.setBinding(Qt::makePropertyBinding(propSetter));

    QVERIFY(bindable.hasBinding());
    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), initial, comparator,
            represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), initial, comparator, represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserverLambda.value(), initial, comparator, represent);
    if (spy)
        QCOMPARE(spy->size(), 3);

    // Count notifications triggered; should only happen on actual change.
    int updateCount = 0;
    auto handler = bindable.onValueChanged([&updateCount]() { ++updateCount; });
    Q_UNUSED(handler)

    propSetter.setValue(changed);
    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), changed, comparator,
            represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), changed, comparator, represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserverLambda.value(), changed, comparator, represent);
    QCOMPARE(updateCount, 1);
    if (spy)
        QCOMPARE(spy->size(), 4);

    // Test that manually setting the value (even the same one) breaks the
    // binding.
    testedObj.setProperty(propertyName, QVariant::fromValue(changed));
    QVERIFY(!bindable.hasBinding());
    // Setting the same value should have no impact on udpateCount.
    QCOMPARE(updateCount, 1);

    // value didn't change -> the signal should not be emitted
    if (spy)
        QCOMPARE(spy->size(), 4);
}

/*!
    \internal
    Basic testing of a bindable property that is writable only once.

    The write-once properties are writable properties which accept only
    one valid setting of the value ("write"), after which later attempts
    are ignored.

    This helper function tests the behavior of bindable write-once property
    \a propertyName, of type \c PropertyType, in class \c TestedClass.
    The caller must supply an \a instance of \c TestedClass and two distinct
    values, \a initial and \a changed, of \c PropertyType.

    The property of \a instance must not yet have been set when this function
    is called. The value it has before being set should be passed as \a prior
    and a distinct value, that this test can set it to, as \a changed.

    The \a bindingPreservedOnWrite parameter controls whether this function
    expects the binding set by this function to be preserved when setting a value
    directly. The default value is 'true'.

    By default \c {operator==()} is used to compare values of the property and
    \c {QTest::toString()} is used to generate proper error messages.

    If such comparison is not supported for \c PropertyType, or the comparison
    it supports is not appropriate to this property, a custom \a comparator can
    be supplied.

    Apart from that, a custom \a represent callback can also be specified to
    generate a string representation of \c PropertyType. If supplied, it must
    allocate its returned string using \c {new char[]}, so that it can be used
    in place of \l {QTest::toString()}.

    \note Any test calling this method will need to call
    \code
    if (QTest::currentTestFailed())
        return;
    \endcode
    after doing so, if there is any later code in the test. If testing several
    properties in one test method, emitting a warning message saying which
    property failed, before returning, is a kindness to readers of the output.
*/

template<typename TestedClass, typename PropertyType>
void testWriteOncePropertyBasics(
        TestedClass &instance, const PropertyType &prior, const PropertyType &changed,
        const char *propertyName,
        bool bindingPreservedOnWrite = true,
        std::function<bool(const PropertyType &, const PropertyType &)> comparator =
                [](const PropertyType &lhs, const PropertyType &rhs) { return lhs == rhs; },
        std::function<char *(const PropertyType &)> represent =
                [](const PropertyType &val) { return QTest::toString(val); })
{
    // get the property
    const QMetaObject *metaObject = instance.metaObject();
    QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(propertyName));

    // in case the TestedClass has setProperty()/property() methods.
    QObject &testedObj = static_cast<QObject &>(instance);

    QVERIFY2(metaProperty.metaType() == QMetaType::fromType<PropertyType>(),
             QByteArray("Preconditions not met for ") +  propertyName + "\n"
             "The type of prior and changed value does not match the type of the property.\n"
             "Please ensure that the types match exactly (convertability is not enough).\n"
             "You can provide the template types to the "
             "function explicitly to force a certain type.\n"
             "Property is " + metaProperty.metaType().name()
             + " but parameters are " + QMetaType::fromType<PropertyType>().name() + ".\n");

    QVERIFY2(metaProperty.isBindable(), "Preconditions not met for " + QByteArray(propertyName));

    QUntypedBindable bindable = metaProperty.bindable(&instance);

    QScopedPointer<QSignalSpy> spy(nullptr);
    if (metaProperty.hasNotifySignal())
        spy.reset(new QSignalSpy(&instance, metaProperty.notifySignal()));

    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), prior, comparator,
            represent);

    QProperty<PropertyType> propObserver;
    propObserver.setBinding(bindable.makeBinding());
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), prior, comparator, represent);

    // Create a binding that sets the 'changed' value to the property
    QProperty<PropertyType> propSetter(changed);
    QVERIFY(!bindable.hasBinding());
    bindable.setBinding(Qt::makePropertyBinding(propSetter));
    QVERIFY(bindable.hasBinding());

    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), changed, comparator,
            represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), changed, comparator, represent);
    if (spy)
        QCOMPARE(spy->size(), 1);

    // Attempt to set back the 'prior' value and verify that it has no effect
    testedObj.setProperty(propertyName, QVariant::fromValue(prior));
    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), changed, comparator,
            represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), changed, comparator, represent);
    if (spy)
        QCOMPARE(spy->size(), 1);
    if (bindingPreservedOnWrite)
        QVERIFY(bindable.hasBinding());
    else
        QVERIFY(!bindable.hasBinding());
}


/*!
    \internal
    Basic testing of a read-only bindable property.

    This helper function tests the behavior of bindable read-only property
    \a propertyName, of type \c PropertyType, in class \c TestedClass.
    The caller must supply an \a instance of \c TestedClass and two distinct
    values, \a initial and \a changed, of \c PropertyType.

    When this function is called, the property's value must be \a initial.
    The \a mutator must, when called, cause the property's value to be revised
    to \a changed.

    By default \c {operator==()} is used to compare values of the property and
    \c {QTest::toString()} is used to generate proper error messages.

    If such comparison is not supported for \c PropertyType, or the comparison
    it supports is not appropriate to this property, a custom \a comparator can
    be supplied.

    Apart from that, a custom \a represent callback can also be specified to
    generate a string representation of \c PropertyType. If supplied, it must
    allocate its returned string using \c {new char[]}, so that it can be used
    in place of \l {QTest::toString()}.

    \note Any test calling this method will need to call
    \code
    if (QTest::currentTestFailed())
        return;
    \endcode
    after doing so, if there is any later code in the test. If testing several
    properties in one test method, emitting a warning message saying which
    property failed, before returning, is a kindness to readers of the output.
*/
template<typename TestedClass, typename PropertyType>
void testReadOnlyPropertyBasics(
        TestedClass &instance, const PropertyType &initial, const PropertyType &changed,
        const char *propertyName,
        std::function<void()> mutator = []() { QFAIL("Data modifier function must be provided"); },
        std::function<bool(const PropertyType &, const PropertyType &)> comparator =
                [](const PropertyType &lhs, const PropertyType &rhs) { return lhs == rhs; },
        std::function<char *(const PropertyType &)> represent =
                [](const PropertyType &val) { return QTest::toString(val); })
{
    // get the property
    const QMetaObject *metaObject = instance.metaObject();
    QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(propertyName));

    // in case the TestedClass has setProperty()/property() methods.
    QObject &testedObj = static_cast<QObject &>(instance);

    QVERIFY2(metaProperty.metaType() == QMetaType::fromType<PropertyType>(),
             QByteArray("Preconditions not met for ") +  propertyName  + "\n"
             "The type of initial and changed value does not match the type of the property.\n"
             "Please ensure that the types match exactly (convertability is not enough).\n"
             "You can provide the template types to the "
             "function explicitly to force a certain type.\n"
             "Expected was a " + metaProperty.metaType().name()
             + " but " + QMetaType::fromType<PropertyType>().name() + " was provided.");

    QVERIFY2(metaProperty.isBindable(), "Preconditions not met for " + QByteArray(propertyName));

    QUntypedBindable bindable = metaProperty.bindable(&instance);

    QScopedPointer<QSignalSpy> spy(nullptr);
    if (metaProperty.hasNotifySignal())
        spy.reset(new QSignalSpy(&instance, metaProperty.notifySignal()));

    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), initial, comparator,
            represent);

    // Check that attempting to bind this read-only property to another property has no effect:
    QProperty<PropertyType> propSetter(initial);
    QVERIFY(!bindable.hasBinding());
    bindable.setBinding(Qt::makePropertyBinding(propSetter));
    QVERIFY(!bindable.hasBinding());
    propSetter.setValue(changed);
    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), initial, comparator,
            represent);
    if (spy)
        QCOMPARE(spy->size(), 0);

    QProperty<PropertyType> propObserver;
    propObserver.setBinding(bindable.makeBinding());

    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), initial, comparator, represent);

    // Invoke mutator function. Now property value should be changed.
    mutator();

    QPROPERTY_TEST_COMPARISON_HELPER(
            testedObj.property(propertyName).template value<PropertyType>(), changed, comparator,
            represent);
    QPROPERTY_TEST_COMPARISON_HELPER(propObserver.value(), changed, comparator, represent);

    if (spy)
        QCOMPARE(spy->size(), 1);
}

} // namespace QTestPrivate

QT_END_NAMESPACE

#endif // QPROPERTYTESTHELPER_P_H
