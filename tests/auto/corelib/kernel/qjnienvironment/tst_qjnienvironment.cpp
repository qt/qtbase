/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtCore/QJniEnvironment>
#include <QtTest/QtTest>

class tst_QJniEnvironment : public QObject
{
    Q_OBJECT

private slots:
    void jniEnv();
    void javaVM();
};

void tst_QJniEnvironment::jniEnv()
{
    QJniEnvironment env;
    JavaVM *javaVM = env.javaVM();
    QVERIFY(javaVM);

    {
        // JNI environment should now be attached to the current thread
        JNIEnv *jni = 0;
        QCOMPARE(javaVM->GetEnv((void**)&jni, JNI_VERSION_1_6), JNI_OK);

        JNIEnv *e = env;
        QVERIFY(e);

        QCOMPARE(env->GetVersion(), JNI_VERSION_1_6);

        // try to find an existing class
        QVERIFY(env->FindClass("java/lang/Object"));
        QVERIFY(!env->ExceptionCheck());

        // try to find a nonexistent class
        QVERIFY(!env->FindClass("this/doesnt/Exist"));
        QVERIFY(env->ExceptionCheck());
        env->ExceptionClear();

        QVERIFY(env->FindClass("java/lang/Object"));
        QVERIFY(!QJniEnvironment::exceptionCheckAndClear(env));

        // try to find a nonexistent class
        QVERIFY(!env->FindClass("this/doesnt/Exist"));
        QVERIFY(QJniEnvironment::exceptionCheckAndClear(env));

        // try to find an existing class with QJniEnvironment
        QJniEnvironment env;
        QVERIFY(env.findClass("java/lang/Object"));

        // try to find a nonexistent class
        QVERIFY(!env.findClass("this/doesnt/Exist"));

        // clear exception with member function
        QVERIFY(!env->FindClass("this/doesnt/Exist"));
        QVERIFY(env.exceptionCheckAndClear());
    }

    // The env does not detach automatically, even if it goes out of scope. The only way it can
    // be detached is if it's done explicitly, or if the thread we attached to gets killed (TLS clean-up).
    JNIEnv *jni = nullptr;
    QCOMPARE(javaVM->GetEnv((void**)&jni, JNI_VERSION_1_6), JNI_OK);
}

void tst_QJniEnvironment::javaVM()
{
    QJniEnvironment env;
    JavaVM *javaVM = env.javaVM();
    QVERIFY(javaVM);

    QCOMPARE(env.javaVM(), javaVM);

    JavaVM *vm = 0;
    QCOMPARE(env->GetJavaVM(&vm), JNI_OK);
    QCOMPARE(env.javaVM(), vm);
}

QTEST_MAIN(tst_QJniEnvironment)

#include "tst_qjnienvironment.moc"
