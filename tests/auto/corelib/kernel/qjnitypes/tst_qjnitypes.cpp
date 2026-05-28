// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qjnitypes.h>
#include <QtCore/qjniarray.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class tst_QJniTypes : public QObject
{
    Q_OBJECT

public:
    tst_QJniTypes() = default;

    static void nativeClassMethod(JNIEnv *, jclass, int);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(nativeClassMethod);

private slots:
    void initTestCase();
    void nativeMethod();
    void construct();
    void stringTypeCantBeArgument();
};

struct QtJavaWrapper {};
template<>
struct QtJniTypes::Traits<QtJavaWrapper>
{
    static constexpr auto signature()
    {
        return QtJniTypes::CTString("Lorg/qtproject/qt/android/QtJavaWrapper;");
    }
};

struct QtCustomJniObject : QJniObject {};

template<>
struct QtJniTypes::Traits<QtCustomJniObject>
{
    static constexpr auto signature()
    {
        return QtJniTypes::CTString("Lorg/qtproject/qt/android/QtCustomJniObject;");
    }
};

static_assert(QtJniTypes::Traits<QtJavaWrapper>::signature() == "Lorg/qtproject/qt/android/QtJavaWrapper;");
static_assert(QtJniTypes::Traits<QtJavaWrapper>::signature() != "Ljava/lang/Object;");
static_assert(!(QtJniTypes::Traits<QtJavaWrapper>::signature() == "X"));

Q_DECLARE_JNI_CLASS(JavaType, "org/qtproject/qt/JavaType");
static_assert(QtJniTypes::Traits<QtJniTypes::JavaType>::signature() == "Lorg/qtproject/qt/JavaType;");
static_assert(QtJniTypes::Traits<QtJniTypes::JavaType[]>::signature() == "[Lorg/qtproject/qt/JavaType;");

static_assert(QtJniTypes::Traits<jstring>::className() == "java/lang/String");
static_assert(QtJniTypes::Traits<QtJniTypes::String>::className() == "java/lang/String");
static_assert(QtJniTypes::Traits<QtJniTypes::String>::signature() == "Ljava/lang/String;");
static_assert(QtJniTypes::Traits<QtJniTypes::String[]>::signature() == "[Ljava/lang/String;");

Q_DECLARE_JNI_CLASS(QtTextToSpeech, "org/qtproject/qt/android/speech/QtTextToSpeech")
static_assert(QtJniTypes::Traits<QtJniTypes::QtTextToSpeech>::className() == "org/qtproject/qt/android/speech/QtTextToSpeech");

// declaring two types Size in different packages
Q_DECLARE_JNI_CLASS(android, util, Size)
Q_DECLARE_JNI_CLASS(org, qtproject, android, Size)

static_assert(QtJniTypes::Traits<QtJniTypes::android::util::Size>::className() == "android/util/Size");
static_assert(QtJniTypes::Traits<QtJniTypes::org::qtproject::android::Size>::className() == "org/qtproject/android/Size");

using namespace QtJniTypes::org::qtproject;
static_assert(QtJniTypes::Traits<android::Size>::className() == "org/qtproject/android/Size");

static_assert(QtJniTypes::fieldSignature<jint>() == "I");
static_assert(QtJniTypes::fieldSignature<jint[]>() == "[I");
static_assert(QtJniTypes::fieldSignature<jint>() != "X");
static_assert(QtJniTypes::fieldSignature<jint>() != "Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<jlong>() == "J");
static_assert(QtJniTypes::fieldSignature<jstring>() == "Ljava/lang/String;");
static_assert(QtJniTypes::fieldSignature<jobject>() == "Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<jobject[]>() == "[Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<jobjectArray>() == "[Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<QJniObject>() == "Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<QtJavaWrapper>() == "Lorg/qtproject/qt/android/QtJavaWrapper;");
static_assert(QtJniTypes::fieldSignature<QtJavaWrapper[]>() == "[Lorg/qtproject/qt/android/QtJavaWrapper;");
static_assert(QtJniTypes::fieldSignature<QtCustomJniObject>() == "Lorg/qtproject/qt/android/QtCustomJniObject;");

static_assert(QtJniTypes::methodSignature<void>() == "()V");
static_assert(QtJniTypes::methodSignature<void>() != "()X");
static_assert(QtJniTypes::methodSignature<void, jint>() == "(I)V");
static_assert(QtJniTypes::methodSignature<void, jint, jstring>() == "(ILjava/lang/String;)V");
static_assert(QtJniTypes::methodSignature<jlong, jint, jclass>() == "(ILjava/lang/Class;)J");
static_assert(QtJniTypes::methodSignature<jobject, jint, jstring>() == "(ILjava/lang/String;)Ljava/lang/Object;");
static_assert(QtJniTypes::methodSignature<QtJniTypes::JavaType, jint, jstring>()
                                      == "(ILjava/lang/String;)Lorg/qtproject/qt/JavaType;");

static_assert(QtJniTypes::isPrimitiveType<jint>());
static_assert(QtJniTypes::isPrimitiveType<void>());
static_assert(!QtJniTypes::isPrimitiveType<jobject>());
static_assert(!QtJniTypes::isPrimitiveType<QtCustomJniObject>());

static_assert(!QtJniTypes::isObjectType<jint>());
static_assert(!QtJniTypes::isObjectType<void>());
static_assert(QtJniTypes::isObjectType<jobject>());
static_assert(QtJniTypes::isObjectType<jobjectArray>());
static_assert(QtJniTypes::isObjectType<QtCustomJniObject>());

static_assert(!QtJniTypes::isArrayType<jint>());
static_assert(QtJniTypes::isArrayType<jint[]>());
static_assert(QtJniTypes::isArrayType<jobject[]>());
static_assert(QtJniTypes::isArrayType<jobjectArray>());
static_assert(QtJniTypes::isArrayType<QtJavaWrapper[]>());

static_assert(QtJniTypes::CTString("ABCDE").startsWith("ABC"));
static_assert(QtJniTypes::CTString("ABCDE").startsWith("A"));
static_assert(QtJniTypes::CTString("ABCDE").startsWith("ABCDE"));
static_assert(!QtJniTypes::CTString("ABCDE").startsWith("ABCDEF"));
static_assert(!QtJniTypes::CTString("ABCDE").startsWith("9AB"));
static_assert(QtJniTypes::CTString("ABCDE").startsWith('A'));
static_assert(!QtJniTypes::CTString("ABCDE").startsWith('B'));

static_assert(QtJniTypes::Traits<QJniArray<jobject>>::signature() == "[Ljava/lang/Object;");
static_assert(QtJniTypes::Traits<QJniArray<jbyte>>::signature() == "[B");
static_assert(QtJniTypes::isObjectType<QJniArray<jbyte>>());

static_assert(QtJniTypes::CTString("ABCDE").endsWith("CDE"));
static_assert(QtJniTypes::CTString("ABCDE").endsWith("E"));
static_assert(QtJniTypes::CTString("ABCDE").endsWith("ABCDE"));
static_assert(!QtJniTypes::CTString("ABCDE").endsWith("DEF"));
static_assert(!QtJniTypes::CTString("ABCDE").endsWith("ABCDEF"));
static_assert(QtJniTypes::CTString("ABCDE").endsWith('E'));
static_assert(!QtJniTypes::CTString("ABCDE").endsWith('F'));

enum UnscopedEnum {};
enum class ScopedEnum {};
enum class IntEnum : int {};
enum class UnsignedEnum : unsigned {};
enum class Int8Enum : int8_t {};
enum class ShortEnum : short {};
enum class LongEnum : quint64 {};
enum class JIntEnum : jint {};

static_assert(QtJniTypes::Traits<UnscopedEnum>::signature() == "I");
static_assert(QtJniTypes::Traits<ScopedEnum>::signature() == "I");
static_assert(QtJniTypes::Traits<IntEnum>::signature() == "I");
static_assert(QtJniTypes::Traits<UnsignedEnum>::signature() == "I");
static_assert(QtJniTypes::Traits<Int8Enum>::signature() == "B");
static_assert(QtJniTypes::Traits<LongEnum>::signature() == "J");
static_assert(QtJniTypes::Traits<JIntEnum>::signature() == "I");

void tst_QJniTypes::initTestCase()
{

}

static bool nativeFunction(JNIEnv *, jclass, int, jstring, quint64)
{
    return true;
}
Q_DECLARE_JNI_NATIVE_METHOD(nativeFunction)

static_assert(QtJniTypes::nativeMethodSignature(nativeFunction) == "(ILjava/lang/String;J)Z");

static QString nativeFunctionStrings(JNIEnv *, jclass, const QString &, const QtJniTypes::String &)
{
    return QString();
}
Q_DECLARE_JNI_NATIVE_METHOD(nativeFunctionStrings)

static_assert(QtJniTypes::nativeMethodSignature(nativeFunctionStrings)
                == "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");

static int forwardDeclaredNativeFunction(JNIEnv *, jobject, bool);
Q_DECLARE_JNI_NATIVE_METHOD(forwardDeclaredNativeFunction)
static int forwardDeclaredNativeFunction(JNIEnv *, jobject, bool) { return 0; }
static_assert(QtJniTypes::nativeMethodSignature(forwardDeclaredNativeFunction) == "(Z)I");

static_assert(QtJniTypes::nativeMethodSignature(tst_QJniTypes::nativeClassMethod) == "(I)V");
void tst_QJniTypes::nativeClassMethod(JNIEnv *, jclass, int) {}

void tst_QJniTypes::nativeMethod()
{
    using namespace QtJniMethods;
    {
        const auto method = Q_JNI_NATIVE_METHOD(nativeFunction);
        QVERIFY(method.fnPtr == &(nativeFunction_Helper::call<bool, jclass, int, jstring, quint64>));
        QCOMPARE(method.name, "nativeFunction");
        QCOMPARE(method.signature, "(ILjava/lang/String;J)Z");
    }

    {
        const auto method = Q_JNI_NATIVE_METHOD(forwardDeclaredNativeFunction);
        QVERIFY(method.fnPtr == &(forwardDeclaredNativeFunction_Helper::call<int, jobject, bool>));
    }

    {
        const auto method = Q_JNI_NATIVE_SCOPED_METHOD(nativeClassMethod, tst_QJniTypes);
        QVERIFY(method.fnPtr == &(nativeClassMethod_QtJniMethod::call<void, jclass, int>));
    }
}

void tst_QJniTypes::construct()
{
    using namespace QtJniTypes;

    const QString text = u"Java String"_s;
    String str(text);
    QVERIFY(str.isValid());
    QCOMPARE(str.toString(), text);

    jobject jref = nullptr; // must be jobject, not jstring
    {
        // if jref would be a jstring, then this would call the
        // Java String copy constructor!
        String jstr(jref);
        QVERIFY(!jstr.isValid());
    }
    jref = str.object<jstring>();
    {
        String jstr(jref);
        QVERIFY(jstr.isValid());
        QCOMPARE(jstr.toString(), text);
    }

    String str2 = str;
    QCOMPARE(str.toString(), text);
    String str3 = std::move(str2);
    QCOMPARE(str3.toString(), text);
}

template <typename ...Arg>
static constexpr bool isValidArgument(Arg &&...) noexcept
{
    return QtJniTypes::ValidSignatureTypesDetail<q20::remove_cvref_t<Arg>...>;
}

enum class Overload
{
    ClassNameAndMethod,
    OnlyMethod,
};

template <typename Ret, typename ...Args
#ifndef Q_QDOC
    , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
>
static constexpr auto callStaticMethod(const char *className, const char *methodName, Args &&...)
{
    Q_UNUSED(className);
    Q_UNUSED(methodName);
    return Overload::ClassNameAndMethod;
}

template <typename Klass, typename Ret, typename ...Args
#ifndef Q_QDOC
    , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
>
static constexpr auto callStaticMethod(const char *methodName, Args &&...)
{
    Q_UNUSED(methodName);
    return Overload::OnlyMethod;
}

void tst_QJniTypes::stringTypeCantBeArgument()
{
    const char *methodName = "staticEchoMethod";

    static_assert(!isValidArgument(QtJniTypes::Traits<QtJniTypes::JavaType>::className()));
    static_assert(!isValidArgument("someFunctionName"));
    static_assert(!isValidArgument(methodName));
    static_assert(!isValidArgument(QtJniTypes::Traits<QtJniTypes::JavaType>::className(),
                                   "someFunctionName", methodName, 42));

    static_assert(callStaticMethod<jstring, jint>("class name", "method name", 42)
                  == Overload::ClassNameAndMethod);
    static_assert(callStaticMethod<QtJniTypes::JavaType, jint>("method name", 42)
                  == Overload::OnlyMethod);
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QJniTypes)

#include "tst_qjnitypes.moc"
