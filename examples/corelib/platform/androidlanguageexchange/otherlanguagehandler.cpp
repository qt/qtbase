// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "otherlanguagehandler.h"

OtherLanguageHandler::OtherLanguageHandler(DataExchanger* dataExchanger)
    : mDataExchanger(dataExchanger)
{
    registerNatives();
    // These can't be done in the ctor-initializer, because registerNatives() needs
    // to be called first.
    mJavaExchanger = QtJniTypes::JavaExchanger(reinterpret_cast<jlong>(mDataExchanger));
    mKotlinExchanger = QtJniTypes::KotlinExchanger(reinterpret_cast<jlong>(mDataExchanger));
}

//! [Forwarding the Java/Kotlin call to Qt]
// The Java/Kotlin code calls this. The C++ code doesn't decide where and when
// it is called.
// Note that the function must be registered with the Qt native method
// registration facilities because it relies on the Qt wrapping that knows how to
// convert a jstring to a QString.
void fromOther(JNIEnv* /*env*/, jobject /*thiz*/,
                       jlong cppExchanger, const QString &str)
{
    DataExchanger* exchanger = reinterpret_cast<DataExchanger*>(cppExchanger);
    exchanger->fromOther(str);
}
//! [Forwarding the Java/Kotlin call to Qt]

Q_DECLARE_JNI_NATIVE_METHOD(fromOther)

//! [Registering the Java/Kotlin class as an observer for the C++ signal]
// We provide this function so that the Java/Kotlin objects can register themselves
// as signal observers. The C++ code doesn't decide where and when that happens.
void connectToCppExchanger(JNIEnv* /*env*/, jobject thiz, jlong cppExchanger)
{
    DataExchanger* exchanger = reinterpret_cast<DataExchanger*>(cppExchanger);
    QJniObject androidExchanger(thiz);
    QObject::connect(exchanger, &DataExchanger::fromCpp, exchanger,
                     // Intentional, and important capture by value of the QJniObject.
                     // This keeps the underlying Java/Kotlin object alive until the call
                     // to it is performed.
                     [androidExchanger](DataExchanger::OtherLanguageType type, const QString& str) {
                         androidExchanger.callMethod("fromCpp",
                                                     static_cast<int>(type), str);
                     });
}
//! [Registering the Java/Kotlin class as an observer for the C++ signal]

Q_DECLARE_JNI_NATIVE_METHOD(connectToCppExchanger)

void OtherLanguageHandler::registerNatives()
{
    QtJniTypes::JavaExchanger::registerNativeMethods({
        Q_JNI_NATIVE_METHOD(fromOther), Q_JNI_NATIVE_METHOD(connectToCppExchanger)
    });
    QtJniTypes::KotlinExchanger::registerNativeMethods({
        Q_JNI_NATIVE_METHOD(fromOther), Q_JNI_NATIVE_METHOD(connectToCppExchanger)
    });
}
