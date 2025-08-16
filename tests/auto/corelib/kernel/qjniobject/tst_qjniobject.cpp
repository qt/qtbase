// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <jni.h>

#include <QString>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QTest>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr const char testClassName[] = "org/qtproject/qt/android/testdatapackage/QtJniObjectTestClass";
Q_DECLARE_JNI_CLASS(QtJniObjectTestClass, testClassName)
using TestClass = QtJniTypes::QtJniObjectTestClass;

static const jbyte A_BYTE_VALUE = 127;
static const jshort A_SHORT_VALUE = 32767;
static const jint A_INT_VALUE = 060701;
static const jlong A_LONG_VALUE = 060701;
static const jfloat A_FLOAT_VALUE = 1.0;
static const jdouble A_DOUBLE_VALUE = 1.0;
static const jchar A_CHAR_VALUE = 'Q';

static QString A_STRING_OBJECT()
{
    return QStringLiteral("TEST_DATA_STRING");
}

class tst_QJniObject : public QObject
{
    Q_OBJECT

public:
    tst_QJniObject();

private slots:
    void initTestCase();
    void init();

    void ctor();
    void callMethodTest();
    void callMethodThrowsException();
    void callObjectMethodTest();
    void stringConvertionTest();
    void compareOperatorTests();
    void className();
    void callStaticMethodThrowsException();
    void callStaticObjectMethodClassName();
    void callStaticObjectMethod();
    void callStaticObjectMethodById();
    void callStaticBooleanMethodClassName();
    void callStaticBooleanMethod();
    void callStaticBooleanMethodById();
    void callStaticCharMethodClassName();
    void callStaticCharMethod();
    void callStaticCharMethodById();
    void callStaticIntMethodClassName();
    void callStaticIntMethod();
    void callStaticIntMethodById();
    void callStaticByteMethodClassName();
    void callStaticByteMethod();
    void callStaticByteMethodById();
    void callStaticDoubleMethodClassName();
    void callStaticDoubleMethod();
    void callStaticDoubleMethodById();
    void callStaticFloatMethodClassName();
    void callStaticFloatMethod();
    void callStaticFloatMethodById();
    void callStaticLongMethodClassName();
    void callStaticLongMethod();
    void callStaticLongMethodById();
    void callStaticShortMethodClassName();
    void callStaticShortMethod();
    void callStaticShortMethodById();
    void getStaticObjectFieldClassName();
    void getStaticObjectField();
    void getStaticIntFieldClassName();
    void getStaticIntField();
    void getStaticByteFieldClassName();
    void getStaticByteField();
    void getStaticBooleanField();
    void getStaticLongFieldClassName();
    void getStaticLongField();
    void getStaticDoubleFieldClassName();
    void getStaticDoubleField();
    void getStaticFloatFieldClassName();
    void getStaticFloatField();
    void getStaticShortFieldClassName();
    void getStaticShortField();
    void getStaticCharFieldClassName();
    void getStaticCharField();
    void getBooleanField();
    void getIntField();

    void setIntField();
    void setByteField();
    void setLongField();
    void setDoubleField();
    void setFloatField();
    void setShortField();
    void setCharField();
    void setBooleanField();
    void setObjectField();
    void setStaticIntField();
    void setStaticByteField();
    void setStaticLongField();
    void setStaticDoubleField();
    void setStaticFloatField();
    void setStaticShortField();
    void setStaticCharField();
    void setStaticBooleanField();
    void setStaticObjectField();

    void templateApiCheck();
    void defaultTemplateApiCheck();
    void isClassAvailable();
    void fromLocalRef();
    void largeObjectArray();
    void arrayLifetime();

    void callback_data();
    void callback();
    void callStaticOverloadResolution();

    void cleanupTestCase();
};

tst_QJniObject::tst_QJniObject()
{
}

void tst_QJniObject::initTestCase()
{
}

void tst_QJniObject::init()
{
    // Unless explicitly ignored to test error handling, warning messages
    // in this test about a failure to look up a field, method, or class
    // make the test fail.
    QTest::failOnWarning(QRegularExpression("java.lang.NoSuch.*Error"));
}

void tst_QJniObject::cleanupTestCase()
{
}

void tst_QJniObject::ctor()
{
    {
        QJniObject object;
        QVERIFY(!object.isValid());
    }

    {
        QJniObject object("java/lang/String");
        QVERIFY(object.isValid());
    }

    {
        QJniObject object = QJniObject::construct<jstring>();
        QVERIFY(object.isValid());
    }

    {
        // from Qt 6.7 on we can construct declared classes through the helper type
        QJniObject object = TestClass::construct();
        QVERIFY(object.isValid());

        // or even directly
        TestClass testObject;
        QVERIFY(testObject.isValid());
    }

    {
        QJniObject string = QJniObject::fromString(QLatin1String("Hello, Java"));
        QJniObject object("java/lang/String", "(Ljava/lang/String;)V", string.object<jstring>());
        QVERIFY(object.isValid());
        QCOMPARE(string.toString(), object.toString());
    }

    {
        QJniObject string = QJniObject::fromString(QLatin1String("Hello, Java"));
        QJniObject object = QJniObject::construct<jstring>(string.object<jstring>());
        QVERIFY(object.isValid());
        QCOMPARE(string.toString(), object.toString());
    }

    {
        QJniEnvironment env;
        jclass javaStringClass = env->FindClass("java/lang/String");
        QJniObject string(javaStringClass);
        QVERIFY(string.isValid());
    }

    {
        QJniEnvironment env;
        const QString qString = QLatin1String("Hello, Java");
        jclass javaStringClass = env->FindClass("java/lang/String");
        QJniObject string = QJniObject::fromString(qString);
        QJniObject stringCpy(javaStringClass, "(Ljava/lang/String;)V", string.object<jstring>());
        QVERIFY(stringCpy.isValid());
        QCOMPARE(qString, stringCpy.toString());
    }

    {
        QJniEnvironment env;
        const QString qString = QLatin1String("Hello, Java");
        jclass javaStringClass = env->FindClass("java/lang/String");
        QJniObject string = QJniObject::fromString(qString);
        QJniObject stringCpy(javaStringClass, string.object<jstring>());
        QVERIFY(stringCpy.isValid());
        QCOMPARE(qString, stringCpy.toString());
    }
}

void tst_QJniObject::callMethodTest()
{
    {
        const QString qString1 = u"Hello, Java"_s;
        const QString qString2 = u"hELLO, jAVA"_s;
        QJniObject jString1 = QJniObject::fromString(qString1);
        QJniObject jString2 = QJniObject::fromString(qString2);
        QVERIFY(jString1 != jString2);

        const jboolean isEmpty = jString1.callMethod<jboolean>("isEmpty");
        QVERIFY(!isEmpty);

        jint ret = jString1.callMethod<jint>("compareToIgnoreCase",
                                             "(Ljava/lang/String;)I",
                                             jString2.object<jstring>());
        QVERIFY(0 == ret);

        ret = jString1.callMethod<jint>("compareToIgnoreCase", jString2.object<jstring>());
        QVERIFY(0 == ret);

        // as of Qt 6.7, we can pass QString directly
        ret = jString1.callMethod<jint>("compareToIgnoreCase", qString2);
        QVERIFY(0 == ret);
    }

    {
        jlong jLong = 100;
        QJniObject longObject("java/lang/Long", "(J)V", jLong);
        jlong ret = longObject.callMethod<jlong>("longValue");
        QCOMPARE(ret, jLong);
    }

    // as of Qt 6.4, callMethod works with an object type as well!
    {
        const QString qString = QLatin1String("Hello, Java");
        QJniObject jString = QJniObject::fromString(qString);
        const QString qStringRet = jString.callMethod<jstring>("toUpperCase").toString();
        QCOMPARE(qString.toUpper(), qStringRet);

        QJniObject subString = jString.callMethod<jstring>("substring", 0, 4);
        QCOMPARE(subString.toString(), qString.mid(0, 4));

        // and as of Qt 6.7, we can return and take QString directly
        QCOMPARE(jString.callMethod<QString>("substring", 0, 4), qString.mid(0, 4));

        QCOMPARE(jString.callMethod<jstring>("substring", 0, 7)
                        .callMethod<jstring>("toUpperCase")
                        .callMethod<QString>("concat", u"C++"_s), u"HELLO, C++"_s);
    }
}

void tst_QJniObject::callMethodThrowsException()
{
    QtJniTypes::QtJniObjectTestClass instance;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("java.lang.Exception"));
    auto res = instance.callMethod<jobject>("callMethodThrowsException");
    QVERIFY(!res.isValid());
    QVERIFY(!QJniEnvironment().checkAndClearExceptions());
}

void tst_QJniObject::callObjectMethodTest()
{
    const QString qString = QLatin1String("Hello, Java");
    QJniObject jString = QJniObject::fromString(qString);
    const QString qStringRet = jString.callObjectMethod<jstring>("toUpperCase").toString();
    QCOMPARE(qString.toUpper(), qStringRet);

    QJniObject subString = jString.callObjectMethod("substring",
                                                    "(II)Ljava/lang/String;",
                                                    0, 4);
    QCOMPARE(subString.toString(), qString.mid(0, 4));

    subString = jString.callObjectMethod<jstring>("substring", 0, 4);
    QCOMPARE(subString.toString(), qString.mid(0, 4));

}

void tst_QJniObject::stringConvertionTest()
{
    const QString qString(QLatin1String("Hello, Java"));
    QJniObject jString = QJniObject::fromString(qString);
    QVERIFY(jString.isValid());
    QString qStringRet = jString.toString();
    QCOMPARE(qString, qStringRet);
}

void tst_QJniObject::compareOperatorTests()
{
    QString str("hello!");
    QJniObject stringObject = QJniObject::fromString(str);

    jobject obj = stringObject.object();
    jobject jobj = stringObject.object<jobject>();
    jstring jsobj = stringObject.object<jstring>();

    QVERIFY(obj == stringObject);
    QVERIFY(jobj == stringObject);
    QVERIFY(stringObject == jobj);
    QVERIFY(jsobj == stringObject);
    QVERIFY(stringObject == jsobj);

    QJniObject stringObject3 = stringObject.object<jstring>();
    QVERIFY(stringObject3 == stringObject);

    QJniObject stringObject2 = QJniObject::fromString(str);
    QVERIFY(stringObject != stringObject2);

    jstring jstrobj = nullptr;
    QJniObject invalidStringObject;
    QVERIFY(invalidStringObject == jstrobj);

    QVERIFY(jstrobj != stringObject);
    QVERIFY(stringObject != jstrobj);
    QVERIFY(!invalidStringObject.isValid());

    QJniObject movedTo(std::move(stringObject3));
    QVERIFY(!stringObject3.isValid());
    QCOMPARE_NE(movedTo, stringObject3);
    QCOMPARE(invalidStringObject, stringObject3);
}

void tst_QJniObject::className()
{
    const QString str("Hello!");
    QJniObject jString = QJniObject::fromString(str);
    {
        QCOMPARE(jString.className(), "java/lang/String");
        QCOMPARE(jString.toString(), str);
    }

    {
        QJniObject strObject = QJniObject("java/lang/String", str);
        QCOMPARE(strObject.className(), "java/lang/String");
        QCOMPARE(strObject.toString(), str);
    }

    {
        TestClass test;
        QCOMPARE(test.className(), testClassName);
    }
}

void tst_QJniObject::callStaticMethodThrowsException()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("java.lang.Exception"));
    auto res = QtJniTypes::QtJniObjectTestClass::callStaticMethod<jobject>(
            "callStaticMethodThrowsException");
    QVERIFY(!res.isValid());
    QVERIFY(!QJniEnvironment().checkAndClearExceptions());
}

void tst_QJniObject::callStaticObjectMethodClassName()
{
    QJniObject formatString = QJniObject::fromString(QLatin1String("test format"));
    QVERIFY(formatString.isValid());

    QVERIFY(QJniObject::isClassAvailable("java/lang/String"));
    QJniObject returnValue = QJniObject::callStaticObjectMethod("java/lang/String",
                                                                "format",
                                                                "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;",
                                                                formatString.object<jstring>(),
                                                                jobjectArray(0));
    QVERIFY(returnValue.isValid());

    QString returnedString = returnValue.toString();

    QCOMPARE(returnedString, QString::fromLatin1("test format"));

    returnValue = QJniObject::callStaticObjectMethod<jstring>("java/lang/String",
                                                              "format",
                                                              formatString.object<jstring>(),
                                                              jobjectArray(0));
    QVERIFY(returnValue.isValid());

    returnedString = returnValue.toString();

    QCOMPARE(returnedString, QString::fromLatin1("test format"));
}

void tst_QJniObject::callStaticObjectMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/String");
    QVERIFY(cls != 0);

    const QString string = u"test format"_s;
    QJniObject formatString = QJniObject::fromString(string);
    QVERIFY(formatString.isValid());

    QJniObject returnValue = QJniObject::callStaticObjectMethod(cls,
                                                                "format",
                                                                "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;",
                                                                formatString.object<jstring>(),
                                                                jobjectArray(0));
    QVERIFY(returnValue.isValid());
    QCOMPARE(returnValue.toString(), string);

    returnValue = QJniObject::callStaticObjectMethod<jstring>(cls,
                                                              "format",
                                                              formatString.object<jstring>(),
                                                              jobjectArray(0));
    QVERIFY(returnValue.isValid());
    QCOMPARE(returnValue.toString(), string);

    // from 6.4 on we can use callStaticMethod
    returnValue = QJniObject::callStaticMethod<jstring>(cls,
                                                        "format",
                                                        formatString.object<jstring>(),
                                                        jobjectArray(0));
    QVERIFY(returnValue.isValid());
    QCOMPARE(returnValue.toString(), string);

    // from 6.7 we can use callStaticMethod without specifying the class string
    returnValue = QJniObject::callStaticMethod<jstring, jstring>("format",
                                                                 formatString.object<jstring>(),
                                                                 jobjectArray(0));
    QVERIFY(returnValue.isValid());
    QCOMPARE(returnValue.toString(), string);

    // from 6.7 we can pass QString directly, both as parameters and return type
    QString result = QJniObject::callStaticMethod<jstring, QString>("format",
                                                                    string,
                                                                    jobjectArray(0));
    QCOMPARE(result, string);
}

void tst_QJniObject::callStaticObjectMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/String");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(
            cls, "format", "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
    QVERIFY(id != 0);

    QJniObject formatString = QJniObject::fromString(QLatin1String("test format"));
    QVERIFY(formatString.isValid());

    QJniObject returnValue = QJniObject::callStaticObjectMethod(
            cls, id, formatString.object<jstring>(), jobjectArray(0));
    QVERIFY(returnValue.isValid());

    QString returnedString = returnValue.toString();

    QCOMPARE(returnedString, QString::fromLatin1("test format"));

    // from Qt 6.4 on we can use callStaticMethod as well
    returnValue = QJniObject::callStaticMethod<jstring>(
            cls, id, formatString.object<jstring>(), jobjectArray(0));
    QVERIFY(returnValue.isValid());

    returnedString = returnValue.toString();

    QCOMPARE(returnedString, QString::fromLatin1("test format"));
}

void tst_QJniObject::callStaticBooleanMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Boolean");
    QVERIFY(cls != 0);

    {
        QJniObject parameter = QJniObject::fromString("true");
        QVERIFY(parameter.isValid());

        jboolean b = QJniObject::callStaticMethod<jboolean>(cls,
                                                            "parseBoolean",
                                                            "(Ljava/lang/String;)Z",
                                                            parameter.object<jstring>());
        QVERIFY(b);

        b = QJniObject::callStaticMethod<jboolean>(cls, "parseBoolean", parameter.object<jstring>());
        QVERIFY(b);
    }

    {
        QJniObject parameter = QJniObject::fromString("false");
        QVERIFY(parameter.isValid());

        jboolean b = QJniObject::callStaticMethod<jboolean>(cls,
                                                            "parseBoolean",
                                                            "(Ljava/lang/String;)Z",
                                                            parameter.object<jstring>());
        QVERIFY(!b);

        b = QJniObject::callStaticMethod<jboolean>(cls, "parseBoolean", parameter.object<jstring>());
        QVERIFY(!b);
    }
}

void tst_QJniObject::callStaticBooleanMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Boolean");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseBoolean", "(Ljava/lang/String;)Z");
    QVERIFY(id != 0);

    {
        QJniObject parameter = QJniObject::fromString("true");
        QVERIFY(parameter.isValid());

        jboolean b = QJniObject::callStaticMethod<jboolean>(cls, id, parameter.object<jstring>());
        QVERIFY(b);
    }

    {
        QJniObject parameter = QJniObject::fromString("false");
        QVERIFY(parameter.isValid());

        jboolean b = QJniObject::callStaticMethod<jboolean>(cls, id, parameter.object<jstring>());
        QVERIFY(!b);
    }
}

void tst_QJniObject::callStaticBooleanMethodClassName()
{
    {
        QJniObject parameter = QJniObject::fromString("true");
        QVERIFY(parameter.isValid());

        jboolean b = QJniObject::callStaticMethod<jboolean>("java/lang/Boolean",
                                                            "parseBoolean",
                                                            "(Ljava/lang/String;)Z",
                                                            parameter.object<jstring>());
        QVERIFY(b);
        b = QJniObject::callStaticMethod<jboolean>("java/lang/Boolean",
                                                   "parseBoolean",
                                                   parameter.object<jstring>());
        QVERIFY(b);
    }

    {
        QJniObject parameter = QJniObject::fromString("false");
        QVERIFY(parameter.isValid());

        jboolean b = QJniObject::callStaticMethod<jboolean>("java/lang/Boolean",
                                                            "parseBoolean",
                                                            "(Ljava/lang/String;)Z",
                                                            parameter.object<jstring>());
        QVERIFY(!b);
        b = QJniObject::callStaticMethod<jboolean>("java/lang/Boolean",
                                                   "parseBoolean",
                                                   parameter.object<jstring>());
        QVERIFY(!b);
    }
}

void tst_QJniObject::callStaticByteMethodClassName()
{
    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jbyte returnValue = QJniObject::callStaticMethod<jbyte>("java/lang/Byte",
                                                            "parseByte",
                                                            parameter.object<jstring>());
    QCOMPARE(returnValue, jbyte(number.toInt()));
}

void tst_QJniObject::callStaticByteMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Byte");
    QVERIFY(cls != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jbyte returnValue = QJniObject::callStaticMethod<jbyte>(cls,
                                                            "parseByte",
                                                            parameter.object<jstring>());
    QCOMPARE(returnValue, jbyte(number.toInt()));
}

void tst_QJniObject::callStaticByteMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Byte");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseByte", "(Ljava/lang/String;)B");
    QVERIFY(id != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jbyte returnValue = QJniObject::callStaticMethod<jbyte>(cls, id, parameter.object<jstring>());
    QCOMPARE(returnValue, jbyte(number.toInt()));
}

void tst_QJniObject::callStaticIntMethodClassName()
{
    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jint returnValue = QJniObject::callStaticMethod<jint>("java/lang/Integer",
                                                          "parseInt",
                                                          parameter.object<jstring>());
    QCOMPARE(returnValue, number.toInt());
}


void tst_QJniObject::callStaticIntMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Integer");
    QVERIFY(cls != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jint returnValue = QJniObject::callStaticMethod<jint>(cls,
                                                          "parseInt",
                                                          parameter.object<jstring>());
    QCOMPARE(returnValue, number.toInt());
}

void tst_QJniObject::callStaticIntMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Integer");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseInt", "(Ljava/lang/String;)I");
    QVERIFY(id != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jint returnValue = QJniObject::callStaticMethod<jint>(cls, id, parameter.object<jstring>());
    QCOMPARE(returnValue, number.toInt());
}

void tst_QJniObject::callStaticCharMethodClassName()
{
    jchar returnValue = QJniObject::callStaticMethod<jchar>("java/lang/Character",
                                                            "toUpperCase",
                                                            jchar('a'));
    QCOMPARE(returnValue, jchar('A'));
}


void tst_QJniObject::callStaticCharMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Character");
    QVERIFY(cls != 0);

    jchar returnValue = QJniObject::callStaticMethod<jchar>(cls,
                                                            "toUpperCase",
                                                            jchar('a'));
    QCOMPARE(returnValue, jchar('A'));
}

void tst_QJniObject::callStaticCharMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Character");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "toUpperCase", "(C)C");
    QVERIFY(id != 0);

    jchar returnValue = QJniObject::callStaticMethod<jchar>(cls, id, jchar('a'));
    QCOMPARE(returnValue, jchar('A'));
}

void tst_QJniObject::callStaticDoubleMethodClassName    ()
{
    QString number = QString::number(123.45);
    QJniObject parameter = QJniObject::fromString(number);

    jdouble returnValue = QJniObject::callStaticMethod<jdouble>("java/lang/Double",
                                                                "parseDouble",
                                                                parameter.object<jstring>());
    QCOMPARE(returnValue, number.toDouble());
}


void tst_QJniObject::callStaticDoubleMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Double");
    QVERIFY(cls != 0);

    QString number = QString::number(123.45);
    QJniObject parameter = QJniObject::fromString(number);

    jdouble returnValue = QJniObject::callStaticMethod<jdouble>(cls,
                                                                "parseDouble",
                                                                parameter.object<jstring>());
    QCOMPARE(returnValue, number.toDouble());
}

void tst_QJniObject::callStaticDoubleMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Double");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseDouble", "(Ljava/lang/String;)D");
    QVERIFY(id != 0);

    QString number = QString::number(123.45);
    QJniObject parameter = QJniObject::fromString(number);

    jdouble returnValue =
            QJniObject::callStaticMethod<jdouble>(cls, id, parameter.object<jstring>());
    QCOMPARE(returnValue, number.toDouble());
}

void tst_QJniObject::callStaticFloatMethodClassName()
{
    QString number = QString::number(123.45);
    QJniObject parameter = QJniObject::fromString(number);

    jfloat returnValue = QJniObject::callStaticMethod<jfloat>("java/lang/Float",
                                                              "parseFloat",
                                                              parameter.object<jstring>());
    QCOMPARE(returnValue, number.toFloat());
}


void tst_QJniObject::callStaticFloatMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Float");
    QVERIFY(cls != 0);

    QString number = QString::number(123.45);
    QJniObject parameter = QJniObject::fromString(number);

    jfloat returnValue = QJniObject::callStaticMethod<jfloat>(cls,
                                                              "parseFloat",
                                                              parameter.object<jstring>());
    QCOMPARE(returnValue, number.toFloat());
}

void tst_QJniObject::callStaticFloatMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Float");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseFloat", "(Ljava/lang/String;)F");
    QVERIFY(id != 0);

    QString number = QString::number(123.45);
    QJniObject parameter = QJniObject::fromString(number);

    jfloat returnValue = QJniObject::callStaticMethod<jfloat>(cls, id, parameter.object<jstring>());
    QCOMPARE(returnValue, number.toFloat());
}

void tst_QJniObject::callStaticShortMethodClassName()
{
    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jshort returnValue = QJniObject::callStaticMethod<jshort>("java/lang/Short",
                                                              "parseShort",
                                                              parameter.object<jstring>());
    QCOMPARE(returnValue, number.toShort());
}


void tst_QJniObject::callStaticShortMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Short");
    QVERIFY(cls != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jshort returnValue = QJniObject::callStaticMethod<jshort>(cls,
                                                              "parseShort",
                                                              parameter.object<jstring>());
    QCOMPARE(returnValue, number.toShort());
}

void tst_QJniObject::callStaticShortMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Short");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseShort", "(Ljava/lang/String;)S");
    QVERIFY(id != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jshort returnValue = QJniObject::callStaticMethod<jshort>(cls, id, parameter.object<jstring>());
    QCOMPARE(returnValue, number.toShort());
}

void tst_QJniObject::callStaticLongMethodClassName()
{
    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jlong returnValue = QJniObject::callStaticMethod<jlong>("java/lang/Long",
                                                            "parseLong",
                                                            parameter.object<jstring>());
    QCOMPARE(returnValue, jlong(number.toLong()));
}

void tst_QJniObject::callStaticLongMethod()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Long");
    QVERIFY(cls != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jlong returnValue = QJniObject::callStaticMethod<jlong>(cls,
                                                            "parseLong",
                                                            parameter.object<jstring>());
    QCOMPARE(returnValue, jlong(number.toLong()));
}

void tst_QJniObject::callStaticLongMethodById()
{
    QJniEnvironment env;
    jclass cls = env.findClass("java/lang/Long");
    QVERIFY(cls != 0);

    jmethodID id = env.findStaticMethod(cls, "parseLong", "(Ljava/lang/String;)J");
    QVERIFY(id != 0);

    QString number = QString::number(123);
    QJniObject parameter = QJniObject::fromString(number);

    jlong returnValue = QJniObject::callStaticMethod<jlong>(cls, id, parameter.object<jstring>());
    QCOMPARE(returnValue, jlong(number.toLong()));
}

void tst_QJniObject::getStaticObjectFieldClassName()
{
    {
        QJniObject boolObject = QJniObject::getStaticObjectField("java/lang/Boolean",
                                                                 "FALSE",
                                                                 "Ljava/lang/Boolean;");
        QVERIFY(boolObject.isValid());

        jboolean booleanValue = boolObject.callMethod<jboolean>("booleanValue");
        QVERIFY(!booleanValue);
    }

    {
        QJniObject boolObject = QJniObject::getStaticObjectField("java/lang/Boolean",
                                                                 "TRUE",
                                                                 "Ljava/lang/Boolean;");
        QVERIFY(boolObject.isValid());

        jboolean booleanValue = boolObject.callMethod<jboolean>("booleanValue");
        QVERIFY(booleanValue);
    }

    {
        QJniObject boolObject = QJniObject::getStaticObjectField("java/lang/Boolean",
                                                                 "FALSE",
                                                                 "Ljava/lang/Boolean;");
        QVERIFY(boolObject.isValid());
        jboolean booleanValue = boolObject.callMethod<jboolean>("booleanValue");
        QVERIFY(!booleanValue);
    }
}

void tst_QJniObject::getStaticObjectField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Boolean");
    QVERIFY(cls != 0);

    {
        QJniObject boolObject = QJniObject::getStaticObjectField(cls,
                                                                 "FALSE",
                                                                 "Ljava/lang/Boolean;");
        QVERIFY(boolObject.isValid());

        jboolean booleanValue = boolObject.callMethod<jboolean>("booleanValue");
        QVERIFY(!booleanValue);
    }

    {
        QJniObject boolObject = QJniObject::getStaticObjectField(cls,
                                                                 "TRUE",
                                                                 "Ljava/lang/Boolean;");
        QVERIFY(boolObject.isValid());

        jboolean booleanValue = boolObject.callMethod<jboolean>("booleanValue");
        QVERIFY(booleanValue);
    }

    {
        QJniObject boolObject = QJniObject::getStaticObjectField(cls,
                                                                 "FALSE",
                                                                 "Ljava/lang/Boolean;");
        QVERIFY(boolObject.isValid());

        jboolean booleanValue = boolObject.callMethod<jboolean>("booleanValue");
        QVERIFY(!booleanValue);
    }
}

void tst_QJniObject::getStaticIntFieldClassName()
{
    jint i = QJniObject::getStaticField<jint>("java/lang/Double", "SIZE");
    QCOMPARE(i, 64);
}

void tst_QJniObject::getStaticIntField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Double");
    QVERIFY(cls != 0);

    jint i = QJniObject::getStaticField<jint>(cls, "SIZE");
    QCOMPARE(i, 64);

    enum class Enum { SIZE = 64 };
    Enum e = QJniObject::getStaticField<Enum>(cls, "SIZE");
    QCOMPARE(e, Enum::SIZE);
}

void tst_QJniObject::getStaticByteFieldClassName()
{
    jbyte i = QJniObject::getStaticField<jbyte>("java/lang/Byte", "MAX_VALUE");
    QCOMPARE(i, jbyte(127));
}

void tst_QJniObject::getStaticByteField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Byte");
    QVERIFY(cls != 0);

    jbyte i = QJniObject::getStaticField<jbyte>(cls, "MAX_VALUE");
    QCOMPARE(i, jbyte(127));

    enum class Enum : jbyte { MAX_VALUE = 127 };
    Enum e = QJniObject::getStaticField<Enum>(cls, "MAX_VALUE");
    QCOMPARE(e, Enum::MAX_VALUE);
}

void tst_QJniObject::getStaticBooleanField()
{
    QCOMPARE(TestClass::getStaticField<jboolean>("S_BOOLEAN_VAR"),
             TestClass::getStaticField<bool>("S_BOOLEAN_VAR"));
}

void tst_QJniObject::getStaticLongFieldClassName()
{
    jlong i = QJniObject::getStaticField<jlong>("java/lang/Long", "MAX_VALUE");
    QCOMPARE(i, jlong(9223372036854775807L));
}

void tst_QJniObject::getStaticLongField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Long");
    QVERIFY(cls != 0);

    jlong i = QJniObject::getStaticField<jlong>(cls, "MAX_VALUE");
    QCOMPARE(i, jlong(9223372036854775807L));

    enum class Enum : jlong { MAX_VALUE = 9223372036854775807L };
    Enum e = QJniObject::getStaticField<Enum>(cls, "MAX_VALUE");
    QCOMPARE(e, Enum::MAX_VALUE);
}

void tst_QJniObject::getStaticDoubleFieldClassName()
{
    jdouble i = QJniObject::getStaticField<jdouble>("java/lang/Double", "NaN");
    jlong *k = reinterpret_cast<jlong*>(&i);
    QCOMPARE(*k, jlong(0x7ff8000000000000L));
}

void tst_QJniObject::getStaticDoubleField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Double");
    QVERIFY(cls != 0);

    jdouble i = QJniObject::getStaticField<jdouble>(cls, "NaN");
    jlong *k = reinterpret_cast<jlong*>(&i);
    QCOMPARE(*k, jlong(0x7ff8000000000000L));
}

void tst_QJniObject::getStaticFloatFieldClassName()
{
    jfloat i = QJniObject::getStaticField<jfloat>("java/lang/Float", "NaN");
    unsigned *k = reinterpret_cast<unsigned*>(&i);
    QCOMPARE(*k, unsigned(0x7fc00000));
}

void tst_QJniObject::getStaticFloatField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Float");
    QVERIFY(cls != 0);

    jfloat i = QJniObject::getStaticField<jfloat>(cls, "NaN");
    unsigned *k = reinterpret_cast<unsigned*>(&i);
    QCOMPARE(*k, unsigned(0x7fc00000));
}

void tst_QJniObject::getStaticShortFieldClassName()
{
    jshort i = QJniObject::getStaticField<jshort>("java/lang/Short", "MAX_VALUE");
    QCOMPARE(i, jshort(32767));
}

void tst_QJniObject::getStaticShortField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Short");
    QVERIFY(cls != 0);

    jshort i = QJniObject::getStaticField<jshort>(cls, "MAX_VALUE");
    QCOMPARE(i, jshort(32767));
    enum class Enum : jshort { MAX_VALUE = 32767 };
    Enum e = QJniObject::getStaticField<Enum>(cls, "MAX_VALUE");
    QCOMPARE(e, Enum::MAX_VALUE);
}

void tst_QJniObject::getStaticCharFieldClassName()
{
    jchar i = QJniObject::getStaticField<jchar>("java/lang/Character", "MAX_VALUE");
    QCOMPARE(i, jchar(0xffff));
}

void tst_QJniObject::getStaticCharField()
{
    QJniEnvironment env;
    jclass cls = env->FindClass("java/lang/Character");
    QVERIFY(cls != 0);

    jchar i = QJniObject::getStaticField<jchar>(cls, "MAX_VALUE");
    QCOMPARE(i, jchar(0xffff));

    enum class Enum : jchar { MAX_VALUE = 0xffff };
    Enum e = QJniObject::getStaticField<Enum>(cls, "MAX_VALUE");
    QCOMPARE(e, Enum::MAX_VALUE);
}


void tst_QJniObject::getBooleanField()
{
    QJniObject obj(testClassName);

    QVERIFY(obj.isValid());
    QVERIFY(obj.getField<jboolean>("BOOL_FIELD"));
    QVERIFY(obj.getField<bool>("BOOL_FIELD"));
}

void tst_QJniObject::getIntField()
{
    QJniObject obj(testClassName);

    QVERIFY(obj.isValid());
    jint res = obj.getField<jint>("INT_FIELD");
    QCOMPARE(res, 123);
}

template <typename T>
void setField(const char *fieldName, T testValue)
{
    QJniObject obj(testClassName);
    QVERIFY(obj.isValid());

    obj.setField(fieldName, testValue);

    T res = obj.getField<T>(fieldName);
    QCOMPARE(res, testValue);
}

void tst_QJniObject::setIntField()
{
    setField("INT_VAR", 555);
    enum class Enum : jint { VALUE = 555 };
    setField("INT_VAR", Enum::VALUE);
}

void tst_QJniObject::setByteField()
{
    setField("BYTE_VAR", jbyte(123));
    enum class Enum : jbyte { VALUE = 123 };
    setField("BYTE_VAR", Enum::VALUE);
}

void tst_QJniObject::setLongField()
{
    setField("LONG_VAR", jlong(9223372036847758232L));
    enum class Enum : jlong { VALUE = 9223372036847758232L };
    setField("LONG_VAR", Enum::VALUE);
}

void tst_QJniObject::setDoubleField()
{
    setField("DOUBLE_VAR", jdouble(1.2));
}

void tst_QJniObject::setFloatField()
{
    setField("FLOAT_VAR", jfloat(1.2));
}

void tst_QJniObject::setShortField()
{
    setField("SHORT_VAR", jshort(555));
    enum class Enum : jshort { VALUE = 555 };
    setField("SHORT_VAR", Enum::VALUE);
}

void tst_QJniObject::setCharField()
{
    setField("CHAR_VAR", jchar('A'));
    enum class Enum : jchar { VALUE = 'A' };
    setField("CHAR_VAR", Enum::VALUE);
}

void tst_QJniObject::setBooleanField()
{
    setField("BOOLEAN_VAR", jboolean(true));
    setField("BOOLEAN_VAR", true);
}

void tst_QJniObject::setObjectField()
{
    QJniObject obj(testClassName);
    QVERIFY(obj.isValid());

    const QString qString = u"Hello"_s;
    QJniObject testValue = QJniObject::fromString(qString);
    obj.setField("STRING_OBJECT_VAR", testValue.object<jstring>());

    QJniObject res = obj.getObjectField<jstring>("STRING_OBJECT_VAR");
    QCOMPARE(res.toString(), testValue.toString());

    // as of Qt 6.7, we can set and get strings directly
    obj.setField("STRING_OBJECT_VAR", qString);
    QCOMPARE(obj.getField<QString>("STRING_OBJECT_VAR"), qString);
}

template <typename T>
void setStaticField(const char *fieldName, T testValue)
{
    QJniObject::setStaticField(testClassName, fieldName, testValue);

    T res = QJniObject::getStaticField<T>(testClassName, fieldName);
    QCOMPARE(res, testValue);

    // use template overload to reset to default
    T defaultValue = {};
    TestClass::setStaticField(fieldName, defaultValue);
    res = TestClass::getStaticField<T>(fieldName);
    QCOMPARE(res, defaultValue);
}

void tst_QJniObject::setStaticIntField()
{
    setStaticField("S_INT_VAR", 555);
    enum class Enum : jint { VALUE = 555 };
    setStaticField("S_INT_VAR", Enum::VALUE);
}

void tst_QJniObject::setStaticByteField()
{
    setStaticField("S_BYTE_VAR", jbyte(123));
    enum class Enum : jbyte { VALUE = 123 };
    setStaticField("S_BYTE_VAR", Enum::VALUE);
}

void tst_QJniObject::setStaticLongField()
{
    setStaticField("S_LONG_VAR", jlong(9223372036847758232L));
    enum class Enum : jlong { VALUE = 9223372036847758232L };
    setStaticField("S_LONG_VAR", Enum::VALUE);
}

void tst_QJniObject::setStaticDoubleField()
{
    setStaticField("S_DOUBLE_VAR", jdouble(1.2));
}

void tst_QJniObject::setStaticFloatField()
{
    setStaticField("S_FLOAT_VAR", jfloat(1.2));
}

void tst_QJniObject::setStaticShortField()
{
    setStaticField("S_SHORT_VAR", jshort(555));
    enum class Enum : jshort { VALUE = 555 };
    setStaticField("S_SHORT_VAR", Enum::VALUE);
}

void tst_QJniObject::setStaticCharField()
{
    setStaticField("S_CHAR_VAR", jchar('A'));
    enum class Enum : jchar { VALUE = 'A' };
    setStaticField("S_CHAR_VAR", Enum::VALUE);
}

void tst_QJniObject::setStaticBooleanField()
{
    setStaticField("S_BOOLEAN_VAR", jboolean(true));
    setStaticField("S_BOOLEAN_VAR", true);
}

void tst_QJniObject::setStaticObjectField()
{
    const QString qString = u"Hello"_s;
    QJniObject testValue = QJniObject::fromString(qString);
    QJniObject::setStaticField(testClassName, "S_STRING_OBJECT_VAR", testValue.object<jstring>());

    QJniObject res = QJniObject::getStaticObjectField<jstring>(testClassName, "S_STRING_OBJECT_VAR");
    QCOMPARE(res.toString(), testValue.toString());

    // as of Qt 6.7, we can set and get strings directly
    using namespace QtJniTypes;
    QtJniObjectTestClass::setStaticField("S_STRING_OBJECT_VAR", qString);
    QCOMPARE(QtJniObjectTestClass::getStaticField<QString>("S_STRING_OBJECT_VAR"), qString);
}

void tst_QJniObject::templateApiCheck()
{
    QJniObject testClass(testClassName);
    QVERIFY(testClass.isValid());

    // void ---------------------------------------------------------------------------------------
    QJniObject::callStaticMethod<void>(testClassName, "staticVoidMethod");
    QJniObject::callStaticMethod<void>(testClassName,
                                       "staticVoidMethodWithArgs",
                                       "(IZC)V",
                                       1,
                                       true,
                                       'c');
    QJniObject::callStaticMethod<void>(testClassName,
                                       "staticVoidMethodWithArgs",
                                       1,
                                       true,
                                       'c');

    testClass.callMethod<void>("voidMethod");
    testClass.callMethod<void>("voidMethodWithArgs", "(IZC)V", 1, true, 'c');
    testClass.callMethod<void>("voidMethodWithArgs", 1, true, 'c');

    // jboolean -----------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jboolean>(testClassName, "staticBooleanMethod"));
    QVERIFY(QJniObject::callStaticMethod<jboolean>(testClassName,
                                                   "staticBooleanMethodWithArgs",
                                                   "(ZZZ)Z",
                                                   true,
                                                   true,
                                                   true));
    QVERIFY(QJniObject::callStaticMethod<jboolean>(testClassName,
                                                   "staticBooleanMethodWithArgs",
                                                   true,
                                                   true,
                                                   true));

    QVERIFY(testClass.callMethod<jboolean>("booleanMethod"));
    QVERIFY(testClass.callMethod<jboolean>("booleanMethodWithArgs",
                                           "(ZZZ)Z",
                                           true,
                                           true,
                                           true));
    QVERIFY(testClass.callMethod<jboolean>("booleanMethodWithArgs",
                                           true,
                                           true,
                                           true));

    // jbyte --------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jbyte>(testClassName,
                                                "staticByteMethod") == A_BYTE_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jbyte>(testClassName,
                                                "staticByteMethodWithArgs",
                                                "(BBB)B",
                                                1,
                                                1,
                                                1) == A_BYTE_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jbyte>(testClassName,
                                                "staticByteMethodWithArgs",
                                                jbyte(1),
                                                jbyte(1),
                                                jbyte(1)) == A_BYTE_VALUE);

    QVERIFY(testClass.callMethod<jbyte>("byteMethod") == A_BYTE_VALUE);
    QVERIFY(testClass.callMethod<jbyte>("byteMethodWithArgs", "(BBB)B", 1, 1, 1) == A_BYTE_VALUE);
    QVERIFY(testClass.callMethod<jbyte>("byteMethodWithArgs", jbyte(1), jbyte(1), jbyte(1)) == A_BYTE_VALUE);

    // jchar --------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jchar>(testClassName,
                                                       "staticCharMethod") == A_CHAR_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jchar>(testClassName,
                                                "staticCharMethodWithArgs",
                                                "(CCC)C",
                                                jchar(1),
                                                jchar(1),
                                                jchar(1)) == A_CHAR_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jchar>(testClassName,
                                                "staticCharMethodWithArgs",
                                                jchar(1),
                                                jchar(1),
                                                jchar(1)) == A_CHAR_VALUE);

    QVERIFY(testClass.callMethod<jchar>("charMethod") == A_CHAR_VALUE);
    QVERIFY(testClass.callMethod<jchar>("charMethodWithArgs",
                                        "(CCC)C",
                                        jchar(1),
                                        jchar(1),
                                        jchar(1)) == A_CHAR_VALUE);
    QVERIFY(testClass.callMethod<jchar>("charMethodWithArgs",
                                        jchar(1),
                                        jchar(1),
                                        jchar(1)) == A_CHAR_VALUE);

    // jshort -------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jshort>(testClassName,
                                                        "staticShortMethod") == A_SHORT_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jshort>(testClassName,
                                                 "staticShortMethodWithArgs",
                                                 "(SSS)S",
                                                 jshort(1),
                                                 jshort(1),
                                                 jshort(1)) == A_SHORT_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jshort>(testClassName,
                                                 "staticShortMethodWithArgs",
                                                 jshort(1),
                                                 jshort(1),
                                                 jshort(1)) == A_SHORT_VALUE);

    QVERIFY(testClass.callMethod<jshort>("shortMethod") == A_SHORT_VALUE);
    QVERIFY(testClass.callMethod<jshort>("shortMethodWithArgs",
                                         "(SSS)S",
                                         jshort(1),
                                         jshort(1),
                                         jshort(1)) == A_SHORT_VALUE);
    QVERIFY(testClass.callMethod<jshort>("shortMethodWithArgs",
                                         jshort(1),
                                         jshort(1),
                                         jshort(1)) == A_SHORT_VALUE);

    // jint ---------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jint>(testClassName,
                                                      "staticIntMethod") == A_INT_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jint>(testClassName,
                                               "staticIntMethodWithArgs",
                                               "(III)I",
                                               jint(1),
                                               jint(1),
                                               jint(1)) == A_INT_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jint>(testClassName,
                                               "staticIntMethodWithArgs",
                                               jint(1),
                                               jint(1),
                                               jint(1)) == A_INT_VALUE);

    QVERIFY(testClass.callMethod<jint>("intMethod") == A_INT_VALUE);
    QVERIFY(testClass.callMethod<jint>("intMethodWithArgs",
                                       "(III)I",
                                       jint(1),
                                       jint(1),
                                       jint(1)) == A_INT_VALUE);
    QVERIFY(testClass.callMethod<jint>("intMethodWithArgs",
                                       jint(1),
                                       jint(1),
                                       jint(1)) == A_INT_VALUE);

    // jlong --------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jlong>(testClassName,
                                                       "staticLongMethod") == A_LONG_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jlong>(testClassName,
                                                "staticLongMethodWithArgs",
                                                "(JJJ)J",
                                                jlong(1),
                                                jlong(1),
                                                jlong(1)) == A_LONG_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jlong>(testClassName,
                                                "staticLongMethodWithArgs",
                                                jlong(1),
                                                jlong(1),
                                                jlong(1)) == A_LONG_VALUE);

    QVERIFY(testClass.callMethod<jlong>("longMethod") == A_LONG_VALUE);
    QVERIFY(testClass.callMethod<jlong>("longMethodWithArgs",
                                        "(JJJ)J",
                                        jlong(1),
                                        jlong(1),
                                        jlong(1)) == A_LONG_VALUE);
    QVERIFY(testClass.callMethod<jlong>("longMethodWithArgs",
                                        jlong(1),
                                        jlong(1),
                                        jlong(1)) == A_LONG_VALUE);

    // jfloat -------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jfloat>(testClassName,
                                                        "staticFloatMethod") == A_FLOAT_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jfloat>(testClassName,
                                                 "staticFloatMethodWithArgs",
                                                 "(FFF)F",
                                                 jfloat(1.1),
                                                 jfloat(1.1),
                                                 jfloat(1.1)) == A_FLOAT_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jfloat>(testClassName,
                                                 "staticFloatMethodWithArgs",
                                                 jfloat(1.1),
                                                 jfloat(1.1),
                                                 jfloat(1.1)) == A_FLOAT_VALUE);

    QVERIFY(testClass.callMethod<jfloat>("floatMethod") == A_FLOAT_VALUE);
    QVERIFY(testClass.callMethod<jfloat>("floatMethodWithArgs",
                                         "(FFF)F",
                                         jfloat(1.1),
                                         jfloat(1.1),
                                         jfloat(1.1)) == A_FLOAT_VALUE);
    QVERIFY(testClass.callMethod<jfloat>("floatMethodWithArgs",
                                         jfloat(1.1),
                                         jfloat(1.1),
                                         jfloat(1.1)) == A_FLOAT_VALUE);

    // jdouble ------------------------------------------------------------------------------------
    QVERIFY(QJniObject::callStaticMethod<jdouble>(testClassName,
                                                         "staticDoubleMethod") == A_DOUBLE_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jdouble>(testClassName,
                                                  "staticDoubleMethodWithArgs",
                                                  "(DDD)D",
                                                  jdouble(1.1),
                                                  jdouble(1.1),
                                                  jdouble(1.1)) == A_DOUBLE_VALUE);
    QVERIFY(QJniObject::callStaticMethod<jdouble>(testClassName,
                                                  "staticDoubleMethodWithArgs",
                                                  jdouble(1.1),
                                                  jdouble(1.1),
                                                  jdouble(1.1)) == A_DOUBLE_VALUE);

    QVERIFY(testClass.callMethod<jdouble>("doubleMethod") == A_DOUBLE_VALUE);
    QVERIFY(testClass.callMethod<jdouble>("doubleMethodWithArgs",
                                          "(DDD)D",
                                          jdouble(1.1),
                                          jdouble(1.1),
                                          jdouble(1.1)) == A_DOUBLE_VALUE);
    QVERIFY(testClass.callMethod<jdouble>("doubleMethodWithArgs",
                                          jdouble(1.1),
                                          jdouble(1.1),
                                          jdouble(1.1)) == A_DOUBLE_VALUE);

    // jobject ------------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jobject>(testClassName,
                                                                     "staticObjectMethod");
        QVERIFY(res.isValid());
    }

    {
        QJniObject res = testClass.callObjectMethod<jobject>("objectMethod");
        QVERIFY(res.isValid());
    }

    // jclass -------------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jclass>(testClassName,
                                                                    "staticClassMethod");
        QVERIFY(res.isValid());
        QJniEnvironment env;
        QVERIFY(env->IsInstanceOf(testClass.object(), res.object<jclass>()));
    }

    {
        QJniObject res = testClass.callObjectMethod<jclass>("classMethod");
        QVERIFY(res.isValid());
        QJniEnvironment env;
        QVERIFY(env->IsInstanceOf(testClass.object(), res.object<jclass>()));
    }
    // jstring ------------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jstring>(testClassName,
                                                                     "staticStringMethod");
        QVERIFY(res.isValid());
        QVERIFY(res.toString() == A_STRING_OBJECT());
    }

    {
        QJniObject res = testClass.callObjectMethod<jstring>("stringMethod");
        QVERIFY(res.isValid());
        QVERIFY(res.toString() == A_STRING_OBJECT());

    }
    // jthrowable ---------------------------------------------------------------------------------
    {
        // The Throwable object the same message (see: "getMessage()") as A_STRING_OBJECT
        QJniObject res = QJniObject::callStaticObjectMethod<jthrowable>(testClassName,
                                                                        "staticThrowableMethod");
        QVERIFY(res.isValid());
        QVERIFY(res.callObjectMethod<jstring>("getMessage").toString() == A_STRING_OBJECT());
    }

    {
        QJniObject res = testClass.callObjectMethod<jthrowable>("throwableMethod");
        QVERIFY(res.isValid());
        QVERIFY(res.callObjectMethod<jstring>("getMessage").toString() == A_STRING_OBJECT());
    }

    // jobjectArray -------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jobjectArray>(testClassName,
                                                                          "staticObjectArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jobject[]>("staticObjectArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);

        QJniArray<jobject> newArray(QList<QJniObject>{QJniObject::fromString(u"one"_s),
                                                      QJniObject::fromString(u"two"_s),
                                                      QJniObject::fromString(u"three"_s)});
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jobject[]>("staticReverseObjectArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.size(), 3);
        QCOMPARE(QJniObject(reverse.at(0)).toString(), u"three"_s);
        QCOMPARE(QJniObject(reverse.at(1)).toString(), u"two"_s);
        QCOMPARE(QJniObject(reverse.at(2)).toString(), u"one"_s);
    }

    {
        QJniObject res = testClass.callObjectMethod<jobjectArray>("objectArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jobject[]>("objectArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);

        QJniArray<jobject> newArray(QList<QJniObject>{QJniObject::fromString(u"one"_s),
                                                      QJniObject::fromString(u"two"_s),
                                                      QJniObject::fromString(u"three"_s)});
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jobject[]>("reverseObjectArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.size(), 3);
        // QJniArray::at returns a jobject that's a local reference; make sure we don't free it twice
        QCOMPARE(QJniObject::fromLocalRef(reverse.at(0)).toString(), u"three"_s);
        QCOMPARE(QJniObject::fromLocalRef(reverse.at(1)).toString(), u"two"_s);
        QCOMPARE(QJniObject::fromLocalRef(reverse.at(2)).toString(), u"one"_s);
    }

    // jstringArray ------------------------------------------------------------------------------
    {
        const QStringList strings{"First", "Second", "Third"};
        const auto array = TestClass::callStaticMethod<QJniArray<QtJniTypes::String>>("staticStringArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.at(0).toString(), strings.first());
        QCOMPARE(array.toContainer<QStringList>(), strings);
    }

    // jstringArray via implicit QString support -------------------------------------------------
    {
        const QStringList strings{"First", "Second", "Third"};
        const auto array = TestClass::callStaticMethod<QJniArray<QString>>("staticStringArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.at(0), strings.first());
        QCOMPARE(array.toContainer(), strings);
    }

    // jbooleanArray ------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jbooleanArray>(testClassName,
                                                                           "staticBooleanArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jboolean[]>("staticBooleanArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jboolean>{true, true, true}));

        QJniArray<jboolean> newArray(QList<jboolean>{true, false, false});
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jboolean[]>("staticReverseBooleanArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jboolean>{false, false, true}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jbooleanArray>("booleanArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jboolean[]>("booleanArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jboolean>{true, true, true}));

        QJniArray<jboolean> newArray(QList<jboolean>{true, false, false});
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jboolean[]>("reverseBooleanArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jboolean>{false, false, true}));
    }

    // jbyteArray ---------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jbyteArray>(testClassName,
                                                                        "staticByteArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jbyte[]>("staticByteArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), "abc");

        QJniArray<jbyte> newArray(QByteArray{"cba"});
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jbyte[]>("staticReverseByteArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), "abc");

        const QByteArray reverse2 = TestClass::callStaticMethod<QByteArray>("staticReverseByteArray",
                                                                            QByteArray("abc"));
        QCOMPARE(reverse2, "cba");
    }

    {
        QJniObject res = testClass.callObjectMethod<jbyteArray>("byteArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jbyte[]>("byteArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), "abc");

        QJniArray newArray = QJniArray(QByteArray{"cba"});
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jbyte[]>("reverseByteArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), "abc");
    }

    // jcharArray ---------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jcharArray>(testClassName,
                                                                        "staticCharArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jchar[]>("staticCharArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jchar>{u'a', u'b', u'c'}));

        QJniArray<jchar> newArray = {u'c', u'b', u'a'};
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jchar[]>("staticReverseCharArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jchar>{u'a', u'b', u'c'}));

        const QList<jchar> reverse2 = TestClass::callStaticMethod<QList<jchar>>("staticReverseCharArray",
                                                                        (QList<jchar>{u'c', u'b', u'a'}));
        QCOMPARE(reverse2, (QList<jchar>{u'a', u'b', u'c'}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jcharArray>("charArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jchar[]>("charArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jchar>{u'a', u'b', u'c'}));

        QJniArray<jchar> newArray = {u'c', u'b', u'a'};
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jchar[]>("reverseCharArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jchar>{u'a', u'b', u'c'}));
    }

    // jshortArray --------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jshortArray>(testClassName,
                                                                         "staticShortArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jshort[]>("staticShortArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jshort>{3, 2, 1}));

        QJniArray<jshort> newArray = {3, 2, 1};
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jshort[]>("staticReverseShortArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jshort>{1, 2, 3}));

        const QList<jshort> reverse2 = TestClass::callStaticMethod<QList<jshort>>("staticReverseShortArray",
                                                                                  (QList<jshort>{1, 2, 3}));
        QCOMPARE(reverse2, (QList<jshort>{3, 2, 1}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jshortArray>("shortArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jshort[]>("shortArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jshort>{3, 2, 1}));

        QJniArray<jshort> newArray = {3, 2, 1};
        static_assert(std::is_same_v<decltype(newArray)::Type, jshort>);
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jshort[]>("reverseShortArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jshort>{1, 2, 3}));
    }

    // jintArray ----------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jintArray>(testClassName,
                                                                       "staticIntArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jint[]>("staticIntArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jint>{3, 2, 1}));

        QJniArray<jint> newArray = {3, 2, 1};
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jint[]>("staticReverseIntArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jint>{1, 2, 3}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jintArray>("intArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jint[]>("intArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jint>{3, 2, 1}));

        QJniArray<jint> newArray = {3, 2, 1};
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jint[]>("reverseIntArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jint>{1, 2, 3}));
    }

    // jlongArray ---------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jlongArray>(testClassName,
                                                                        "staticLongArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jlong[]>("staticLongArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jlong>{3, 2, 1}));

        QJniArray<jlong> newArray = {3, 2, 1};
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jlong[]>("staticReverseLongArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jlong>{1, 2, 3}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jlongArray>("longArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jlong[]>("longArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jlong>{3, 2, 1}));

        QJniArray<jlong> newArray = {3, 2, 1};
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jlong[]>("reverseLongArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jlong>{1, 2, 3}));
    }

    // jfloatArray --------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jfloatArray>(testClassName,
                                                                         "staticFloatArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jfloat[]>("staticFloatArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jfloat>{1.0f, 2.0f, 3.0f}));

        QJniArray<jfloat> newArray = {3.0f, 2.0f, 1.0f};
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jfloat[]>("staticReverseFloatArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jfloat>{1.0f, 2.0f, 3.0f}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jfloatArray>("floatArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jfloat[]>("floatArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jfloat>{1.0f, 2.0f, 3.0f}));

        QJniArray<jfloat> newArray = {3.0f, 2.0f, 1.0f};
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jfloat[]>("reverseFloatArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jfloat>{1.0f, 2.0f, 3.0f}));
    }

    // jdoubleArray -------------------------------------------------------------------------------
    {
        QJniObject res = QJniObject::callStaticObjectMethod<jdoubleArray>(testClassName,
                                                                          "staticDoubleArrayMethod");
        QVERIFY(res.isValid());

        const auto array = TestClass::callStaticMethod<jdouble[]>("staticDoubleArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jdouble>{3.0, 2.0, 1.0}));

        QJniArray<jdouble> newArray = {3.0, 2.0, 1.0};
        QVERIFY(newArray.isValid());
        const auto reverse = TestClass::callStaticMethod<jdouble[]>("staticReverseDoubleArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jdouble>{1.0, 2.0, 3.0}));
    }

    {
        QJniObject res = testClass.callObjectMethod<jdoubleArray>("doubleArrayMethod");
        QVERIFY(res.isValid());

        const auto array = testClass.callMethod<jdouble[]>("doubleArrayMethod");
        QVERIFY(array.isValid());
        QCOMPARE(array.size(), 3);
        QCOMPARE(array.toContainer(), (QList<jdouble>{3.0, 2.0, 1.0}));

        QJniArray<jdouble> newArray = {3.0, 2.0, 1.0};
        QVERIFY(newArray.isValid());
        const auto reverse = testClass.callMethod<jdouble[]>("reverseDoubleArray", newArray);
        QVERIFY(reverse.isValid());
        QCOMPARE(reverse.toContainer(), (QList<jdouble>{1.0, 2.0, 3.0}));
    }

}

void tst_QJniObject::defaultTemplateApiCheck()
{
    // static QJniObject calls --------------------------------------------------------------------
    QJniObject::callStaticMethod(testClassName, "staticVoidMethod");
    QJniObject::callStaticMethod(testClassName, "staticVoidMethodWithArgs", "(IZC)V", 1, true, 'c');
    QJniObject::callStaticMethod(testClassName, "staticVoidMethodWithArgs", 1, true, 'c');

    // instance QJniObject calls ------------------------------------------------------------------
    QJniObject testClass(testClassName);
    QVERIFY(testClass.isValid());

    testClass.callMethod("voidMethod");
    testClass.callMethod("voidMethodWithArgs", "(IZC)V", 1, true, 'c');
    testClass.callMethod("voidMethodWithArgs", 1, true, 'c');

    // static QtJniType calls ---------------------------------------------------------------------
    TestClass::callStaticMethod("staticVoidMethod");
    TestClass::callStaticMethod("staticVoidMethodWithArgs", 1, true, 'c');

    // instance QtJniType calls -------------------------------------------------------------------
    TestClass instance;
    instance.callMethod("voidMethod");
    instance.callMethod("voidMethodWithArgs", 1, true, 'c');
}

void tst_QJniObject::isClassAvailable()
{
    QVERIFY(QJniObject::isClassAvailable("java/lang/String"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("java.lang.ClassNotFoundException"));
    QVERIFY(!QJniObject::isClassAvailable("class/not/Available"));
    QVERIFY(QJniObject::isClassAvailable("org/qtproject/qt/android/QtActivityDelegate"));
}

void tst_QJniObject::fromLocalRef()
{
    const int limit = 512 + 1;
    QJniEnvironment env;
    for (int i = 0; i != limit; ++i)
        QJniObject o = QJniObject::fromLocalRef(env->FindClass("java/lang/String"));
}

void tst_QJniObject::largeObjectArray()
{
    QJniArray<jobject> newArray(QList<QJniObject>{QJniObject::fromString(u"one"_s),
                                                    QJniObject::fromString(u"two"_s),
                                                    QJniObject::fromString(u"three"_s)});
    QVERIFY(newArray.isValid());
    const QJniArray<QJniObject> reverse = TestClass::callStaticMethod<jobject[]>(
                                                            "staticReverseObjectArray", newArray);
    QVERIFY(reverse.isValid());
    QCOMPARE(reverse.size(), 3);

    // make sure we don't leak local references
    for (int i = 0; i < 10000; ++i) {
        QVERIFY(reverse.at(0).isValid());
        QVERIFY(reverse.at(1).isValid());
        QVERIFY(reverse.at(2).isValid());
    }
}

void tst_QJniObject::arrayLifetime()
{
    const auto stringData = A_STRING_OBJECT();

    QJniArray oldChars = TestClass::callStaticMethod<jchar[]>("getStaticCharArray");
    QVERIFY(oldChars.isValid());
    QCOMPARE(oldChars.size(), stringData.size());
    QCOMPARE(QChar(oldChars.toContainer().at(0)), stringData.at(0));

    QJniArray<jchar> newChars{'a', 'b', 'c'};
    // replace the first three characters in the array
    TestClass::callStaticMethod<void>("mutateStaticCharArray", newChars);
    // the old jcharArray is still valid and the size is unchanged
    QVERIFY(oldChars.isValid());
    QCOMPARE(oldChars.size(), A_STRING_OBJECT().size());
    QCOMPARE(oldChars.toContainer().at(0), jchar('a'));

    // get a second reference to the Java array
    QJniArray updatedChars = TestClass::getStaticField<jchar[]>("S_CHAR_ARRAY");
    // the two QJniArrays reference the same jobject
    QCOMPARE(updatedChars.size(), oldChars.size());
    QCOMPARE(updatedChars, oldChars);

    // replace the Java array; the old jcharArray is still valid and unchanged
    TestClass::callStaticMethod<void>("replaceStaticCharArray", newChars);
    // the old jcharArray is still valid and unchanged
    QVERIFY(oldChars.isValid());
    QCOMPARE(oldChars.size(), stringData.size());
    QCOMPARE(oldChars, updatedChars);

    // we get the same object that we set
    updatedChars = TestClass::getStaticField<jchar[]>("S_CHAR_ARRAY");
    QCOMPARE(updatedChars, newChars);
    QCOMPARE_NE(updatedChars, oldChars);
}

enum class CallbackParameterType
{
    Object,
    ObjectRef,
    String,
    Byte,
    Boolean,
    Int,
    Double,
    Float,
    JniArray,
    RawArray,
    QList,
    QStringList,
    Null,
    Many,
};

static std::optional<TestClass> calledWithObject;
static int callbackWithObject(JNIEnv *, jobject, TestClass that)
{
    calledWithObject.emplace(that);
    return int(CallbackParameterType::Object);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithObject)
static int callbackWithObjectRef(JNIEnv *, jobject, const TestClass &that)
{
    calledWithObject.emplace(that);
    return int(CallbackParameterType::ObjectRef);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithObjectRef)

static std::optional<QString> calledWithString;
static int callbackWithString(JNIEnv *, jobject, const QString &string)
{
    calledWithString.emplace(string);
    return int(CallbackParameterType::String);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithString)

static std::optional<jbyte> calledWithByte;
static int callbackWithByte(JNIEnv *, jobject, jbyte value)
{
    calledWithByte.emplace(value);
    return int(CallbackParameterType::Byte);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithByte)

static std::optional<jbyte> calledWithBoolean;
static int callbackWithBoolean(JNIEnv *, jobject, bool value)
{
    calledWithBoolean.emplace(value);
    return int(CallbackParameterType::Boolean);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithBoolean)

static std::optional<int> calledWithInt;
static int callbackWithInt(JNIEnv *, jobject, int value)
{
    calledWithInt.emplace(value);
    return int(CallbackParameterType::Int);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithInt)

static std::optional<double> calledWithDouble;
static int callbackWithDouble(JNIEnv *, jobject, double value)
{
    calledWithDouble.emplace(value);
    return int(CallbackParameterType::Double);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithDouble)

static std::optional<float> calledWithFloat;
static int callbackWithFloat(JNIEnv *, jobject, float value)
{
    calledWithFloat.emplace(value);
    return int(CallbackParameterType::Float);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithFloat)

static std::optional<float> calledWithFloatStatic;
static int callbackWithFloatStatic(JNIEnv *, jclass, float value)
{
    calledWithFloatStatic.emplace(value);
    return int(CallbackParameterType::Float);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithFloatStatic)

static std::optional<QJniArray<jdouble>> calledWithJniArray;
static int callbackWithJniArray(JNIEnv *, jobject, const QJniArray<jdouble> &value)
{
    calledWithJniArray.emplace(value);
    return int(CallbackParameterType::JniArray);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithJniArray)

static std::optional<QJniObject> calledWithRawArray;
static int callbackWithRawArray(JNIEnv *, jobject, jobjectArray value)
{
    calledWithRawArray.emplace(QJniObject(value));
    return int(CallbackParameterType::RawArray);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithRawArray)

static std::optional<QList<double>> calledWithQList;
static int callbackWithQList(JNIEnv *, jobject, const QList<double> &value)
{
    calledWithQList.emplace(value);
    return int(CallbackParameterType::QList);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithQList)

static std::optional<QStringList> calledWithStringList;
static int callbackWithStringList(JNIEnv *, jobject, const QStringList &value)
{
    calledWithStringList.emplace(value);
    return int(CallbackParameterType::QStringList);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithStringList)

static std::optional<QString> calledWithNull;
static int callbackWithNull(JNIEnv *, jobject, const QString &str)
{
    calledWithNull.emplace(str);
    return int(CallbackParameterType::Null);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithNull)

static std::optional<bool> calledWithMany;
static int callbackWithMany(JNIEnv *, jobject, jbyte byte_val, short short_val, int int_val,
                            jlong long_val, float float_val, double double_val, bool bool_val,
                            jchar char_val, QString string_val)
{
    auto diagnose = qScopeGuard([=]{
        qCritical() << "Received values: "
                    << byte_val << short_val << int_val << long_val
                    << float_val << double_val << bool_val
                    << char_val << string_val;
    });
    calledWithMany.emplace(TestClass::getStaticField<jbyte>("A_BYTE_VALUE") == byte_val
                        && TestClass::getStaticField<short>("A_SHORT_VALUE") == short_val
                        && TestClass::getStaticField<int>("A_INT_VALUE") == int_val
                        && TestClass::getStaticField<jlong>("A_LONG_VALUE") == long_val
                        && TestClass::getStaticField<float>("A_FLOAT_VALUE") == float_val
                        && TestClass::getStaticField<double>("A_DOUBLE_VALUE") == double_val
                        && TestClass::getStaticField<bool>("A_BOOLEAN_VALUE") == bool_val
                        && TestClass::getStaticField<jchar>("A_CHAR_VALUE") == char_val
                        && TestClass::getStaticField<QString>("A_STRING_OBJECT") == string_val
    );
    if (*calledWithMany)
        diagnose.dismiss();
    return int(CallbackParameterType::Many);
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackWithMany)

void tst_QJniObject::callback_data()
{
    QTest::addColumn<CallbackParameterType>("parameterType");

    QTest::addRow("Object")     << CallbackParameterType::Object;
    QTest::addRow("ObjectRef")  << CallbackParameterType::ObjectRef;
    QTest::addRow("String")     << CallbackParameterType::String;
    QTest::addRow("Byte")       << CallbackParameterType::Byte;
    QTest::addRow("Boolean")    << CallbackParameterType::Boolean;
    QTest::addRow("Int")        << CallbackParameterType::Int;
    QTest::addRow("Double")     << CallbackParameterType::Double;
    QTest::addRow("Float")      << CallbackParameterType::Float;
    QTest::addRow("JniArray")   << CallbackParameterType::JniArray;
    QTest::addRow("RawArray")   << CallbackParameterType::RawArray;
    QTest::addRow("QList")      << CallbackParameterType::QList;
    QTest::addRow("QStringList") << CallbackParameterType::QStringList;
    QTest::addRow("Null")       << CallbackParameterType::Null;
    QTest::addRow("More than 8") << CallbackParameterType::Many;
}

void tst_QJniObject::callback()
{
    QFETCH(const CallbackParameterType, parameterType);

    TestClass testObject;
    int result = -1;

    switch (parameterType) {
    case CallbackParameterType::Object:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithObject)
        }));
        result = testObject.callMethod<int>("callMeBackWithObject", testObject);
        QVERIFY(calledWithObject);
        QCOMPARE(calledWithObject.value(), testObject);
        break;
    case CallbackParameterType::ObjectRef:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithObjectRef)
        }));
        result = testObject.callMethod<int>("callMeBackWithObjectRef", testObject);
        QVERIFY(calledWithObject);
        QCOMPARE(calledWithObject.value(), testObject);
        break;
    case CallbackParameterType::String:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithString)
        }));
        result = testObject.callMethod<int>("callMeBackWithString", QString::number(123));
        QVERIFY(calledWithString);
        QCOMPARE(calledWithString.value(), "123");
        break;
    case CallbackParameterType::Byte:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithByte)
        }));
        result = testObject.callMethod<int>("callMeBackWithByte", jbyte(123));
        QVERIFY(calledWithByte);
        QCOMPARE(calledWithByte.value(), 123);
        break;
    case CallbackParameterType::Boolean:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithBoolean)
        }));
        result = testObject.callMethod<int>("callMeBackWithBoolean", true);
        QVERIFY(calledWithBoolean);
        QCOMPARE(calledWithBoolean.value(), true);
        break;
    case CallbackParameterType::Int:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithInt)
        }));
        result = testObject.callMethod<int>("callMeBackWithInt", 12345);
        QVERIFY(calledWithInt);
        QCOMPARE(calledWithInt.value(), 12345);
        break;
    case CallbackParameterType::Double:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithDouble)
        }));
        result = testObject.callMethod<int>("callMeBackWithDouble", 1.2345);
        QVERIFY(calledWithDouble);
        QCOMPARE(calledWithDouble.value(), 1.2345);
        break;
    case CallbackParameterType::Float:
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithFloat),
            Q_JNI_NATIVE_METHOD(callbackWithFloatStatic),
        }));
        result = testObject.callMethod<int>("callMeBackWithFloat", 1.2345f);
        QVERIFY(calledWithFloat);
        QCOMPARE(calledWithFloat.value(), 1.2345f);
        result = testObject.callMethod<int>("callMeBackWithFloatStatic", 1.2345f);
        QVERIFY(calledWithFloatStatic);
        QCOMPARE(calledWithFloatStatic.value(), 1.2345f);
        break;
    case CallbackParameterType::JniArray: {
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithJniArray)
        }));
        const QJniArray<double> doubles = { 1.2, 3.4, 5.6 };
        result = testObject.callMethod<int>("callMeBackWithJniArray", doubles);
        QVERIFY(calledWithJniArray);
        QCOMPARE(calledWithJniArray, doubles);
        break;
    }
    case CallbackParameterType::RawArray: {
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithRawArray)
        }));
        const QStringList strings{"a", "b", "c"};
        const QJniArray stringArray(strings);
        result = testObject.callMethod<int>("callMeBackWithRawArray",
                                            stringArray.object<jobjectArray>());
        QVERIFY(calledWithRawArray);
        const auto stringsReceived = QJniArray<QString>(*calledWithRawArray).toContainer();
        QCOMPARE(stringsReceived, strings);
        break;
    }
    case CallbackParameterType::QList: {
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithQList)
        }));
        const QList<double> doubles = { 1.2, 3.4, 5.6 };
        result = testObject.callMethod<int>("callMeBackWithQList", doubles);
        QVERIFY(calledWithQList);
        QCOMPARE(calledWithQList.value(), doubles);
        break;
    }
    case CallbackParameterType::QStringList: {
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithStringList)
        }));
        const QStringList strings = { "one", "two" };
        result = testObject.callMethod<int>("callMeBackWithStringList", strings);
        QVERIFY(calledWithStringList);
        QCOMPARE(calledWithStringList.value(), strings);
        break;
    }
    case CallbackParameterType::Null: {
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithNull)
        }));
        result = testObject.callMethod<int>("callMeBackWithNull");
        QVERIFY(calledWithNull);
        QCOMPARE(calledWithNull.value(), QString());
        break;
    }
    case CallbackParameterType::Many: {
        QVERIFY(TestClass::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(callbackWithMany)
        }));
        result = testObject.callMethod<int>("callMeBackWithMany");
        QVERIFY(calledWithMany);
        QVERIFY(calledWithMany.value());
        break;
    }
    }
    QCOMPARE(result, int(parameterType));
}

// Make sure the new callStaticMethod overload taking a class, return type,
// and argument as template parameters, doesn't break overload resolution
// and that the class name doesn't get interpreted as the function name.
void tst_QJniObject::callStaticOverloadResolution()
{
    const QString value = u"Hello World"_s;
    QJniObject str = QJniObject::fromString(value);
    const auto result = QJniObject::callStaticMethod<jstring, jstring>(
            QtJniTypes::Traits<TestClass>::className(),
            "staticEchoMethod", str.object<jstring>()).toString();
    QCOMPARE(result, value);
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QJniObject)

#include "tst_qjniobject.moc"
