// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef OTHERLANGUAGEHANDLER_H
#define OTHERLANGUAGEHANDLER_H

#include <QJniObject>
#include <QtJniTypes>

#include "dataexchanger.h"

Q_DECLARE_JNI_CLASS(JavaExchanger, "org/qtproject/example/androidlanguageexchange/JavaExchanger")
Q_DECLARE_JNI_CLASS(KotlinExchanger, "org/qtproject/example/androidlanguageexchange/KotlinExchanger")

// A class that handles the ownership of the Java/Kotlin objects, and sets
// up the signaling for them. We pass the DataExchanger to the Java/Kotlin
// objects and allow them to observe the signals and call the fromOther()
// signal of DataExchanger, but we don't decide on the C++ side where and
// when the Java/Kotlin code does those things.
class OtherLanguageHandler
{
public:
    explicit OtherLanguageHandler(DataExchanger* dataExchanger);
private:
    static void registerNatives();
    DataExchanger *mDataExchanger;
    QtJniTypes::JavaExchanger mJavaExchanger;
    QtJniTypes::KotlinExchanger mKotlinExchanger;
};

#endif // OTHERLANGUAGEHANDLER_H
