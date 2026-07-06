// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QMetaProperty>
#include <QtAssert>
#include <QBrush>
#include <QFile>


//! [1]
class MyClass_1
{
public:
    enum Option {
        NoOptions = 0x0,
        ShowTabs = 0x1,
        ShowAll = 0x2,
        SqueezeBlank = 0x4
    };
    Q_DECLARE_FLAGS(Options, Option)
    //...
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MyClass_1::Options)
//! [1]

//! [meta-object flags]
class MyClass_2
{
    Q_OBJECT
public:
    enum Option {
        NoOptions = 0x0,
        ShowTabs = 0x1,
        ShowAll = 0x2,
        SqueezeBlank = 0x4
    };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)
    //...
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MyClass_2::Options)
//! [meta-object flags]

struct DummyDriver {
    bool isOpen() const { return true; }
    bool isOpenError() const { return false; }
};

DummyDriver* driver() {
    static DummyDriver d;
    return &d;
}

bool examples()
{
    enum Enum {};

    //! [2]
    typedef QFlags<Enum> Flags;
    //! [2]

    //! [4]
    if (!driver()->isOpen() || driver()->isOpenError()) {
        qWarning("QSqlQuery::exec: database not open");
        return false;
    }
    //! [4]

    {
        //! [5]
        qint64 value = Q_INT64_C(932838457459459);
        //! [5]
    }

    {
        //! [6]
        quint64 value = Q_UINT64_C(932838457459459);
        //! [6]
    }

    {
        //! [8]
        qint64 value = Q_INT64_C(932838457459459);
        //! [8]
    }

    {
        //! [9]
        quint64 value = Q_UINT64_C(932838457459459);
        //! [9]
    }

    {
    //! [10]
    int absoluteValue;
    int myValue = -4;

    absoluteValue = qAbs(myValue);
    // absoluteValue == 4
    //! [10]
    }

    {
        //! [11A]
        double valueA = 2.3;
        double valueB = 2.7;

        int roundedValueA = qRound(valueA);
        // roundedValueA = 2
        int roundedValueB = qRound(valueB);
        // roundedValueB = 3
        //! [11A]
    }

    {
        //! [11B]
        float valueA = 2.3f;
        float valueB = 2.7f;

        int roundedValueA = qRound(valueA);
        // roundedValueA = 2
        int roundedValueB = qRound(valueB);
        // roundedValueB = 3
        //! [11B]
    }

    {
        //! [12A]
        double valueA = 42949672960.3;
        double valueB = 42949672960.7;

        qint64 roundedValueA = qRound64(valueA);
        // roundedValueA = 42949672960
        qint64 roundedValueB = qRound64(valueB);
        // roundedValueB = 42949672961
        //! [12A]
    }

    {
        //! [12B]
        float valueA = 42949672960.3f;
        float valueB = 42949672960.7f;

        qint64 roundedValueA = qRound64(valueA);
        // roundedValueA = 42949672960
        qint64 roundedValueB = qRound64(valueB);
        // roundedValueB = 42949672961
        //! [12B]
    }

    {
        //! [13]
        int myValue = 6;
        int yourValue = 4;

        int minValue = qMin(myValue, yourValue);
        // minValue == yourValue
        //! [13]
    }

    {
        //! [14]
        int myValue = 6;
        int yourValue = 4;

        int maxValue = qMax(myValue, yourValue);
        // maxValue == myValue
        //! [14]
    }

    {
        //! [15]
        int myValue = 10;
        int minValue = 2;
        int maxValue = 6;

        int boundedValue = qBound(minValue, myValue, maxValue);
        // boundedValue == 6
        //! [15]
    }

    return true;
}


//! [17&19_include_open]
// File: div.cpp

#include <QtGlobal>

int divide(int a, int b)
{
//! [17&19_include_open]

    //! [17assert]
        Q_ASSERT(b != 0);
    //! [17assert]

    //! [19assert]
        Q_ASSERT_X(b != 0, "divide", "division by zero");
    //! [19assert]

    //! [17&19_return_close]
        return a / b;
}
//! [17&19_return_close]

#if 0
//! [18]
ASSERT: "b != 0" in file div.cpp, line 7
//! [18]

//! [20]
ASSERT failure in divide: "division by zero", file div.cpp, line 7
//! [20]
#endif


void pointer_example()
{
    //! [21]
    int *a;

    Q_CHECK_PTR(a = new int[80]);   // WRONG!

    a = new (std::nothrow) int[80];      // Right
    Q_CHECK_PTR(a);
    //! [21]
}

//! [22]
template<typename TInputType>
const TInputType &myMin(const TInputType &value1, const TInputType &value2)
{
    qDebug() << Q_FUNC_INFO << "was called with value1:" << value1 << "value2:" << value2;

    if(value1 < value2)
        return value1;
    else
        return value2;
}
//! [22]


void debug_info_example()
{
    QList<int> myList;
    QBrush myQBrush(Qt::red);
    int i = 0;
    //! [24]
    qDebug("Items in list: %d", myList.size());
    //! [24]


    //! [25]
    qDebug() << "Brush:" << myQBrush << "Other value:" << i;
    //! [25]


    //! [qInfo_printf]
    qInfo("Items in list: %d", myList.size());
    //! [qInfo_printf]

    //! [qInfo_stream]
    qInfo() << "Brush:" << myQBrush << "Other value:" << i;
    //! [qInfo_stream]
}

//! [26]
void f(int c)
{
    if (c > 200)
        qWarning("f: bad argument, c == %d", c);
}
//! [26]

void warning_example()
{
    QBrush myQBrush(Qt::red);
    int i = 0;
    //! [27]
    qWarning() << "Brush:" << myQBrush << "Other value:" << i;
    //! [27]
}

//! [28]
void load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.exists())
        qCritical("File '%s' does not exist!", qUtf8Printable(fileName));
}
//! [28]

void critical_example()
{
    QBrush myQBrush(Qt::red);
    int i = 0;
    //! [29]
    qCritical() << "Brush:" << myQBrush << "Other value:" << i;
    //! [29]
}

//! [30]
int divide_by_zero(int a, int b)
{
    if (b == 0)                                // program error
        qFatal("divide: cannot divide by zero");
    return a / b;
}
//! [30]

void forever_example()
{
    //! [31]
    forever {
        // ...
    }
    //! [31]
}

# if 0
//! [32]
CONFIG += no_keywords
//! [32]
#endif

namespace snippet_34
{
    class FriendlyConversation
    {
    public:
        QString greeting(int type);
    };

    QString tr(const char *)
    {
        return "";
    }

    //! [34]
    QString FriendlyConversation::greeting(int type)
    {
        static const char *greeting_strings[] = {
            QT_TR_NOOP("Hello"),
            QT_TR_NOOP("Goodbye")
        };
        return tr(greeting_strings[type]);
    }
    //! [34]
}


namespace snippet_qttrnnoop
{
    class StatusClass
    {
    public:
        static QString status(int type, int count);
        static const char * const status_strings[];
    };

    QString tr(const char *, const char *, int)
    {
        return "";
    }
    //! [qttrnnoop]
    const char * const StatusClass::status_strings[] = {
        QT_TR_N_NOOP("There are %n new message(s)"),
        QT_TR_N_NOOP("There are %n total message(s)")
    };

    QString StatusClass::status(int type, int count)
    {
        return tr(status_strings[type], nullptr, count);
    }
    //! [qttrnnoop]
}

QString translate(const char *, const char *, const char *, int)
{
    return "";
}

//! [qttranslatennoop]
static const char * const greeting_strings[] = {
    QT_TRANSLATE_N_NOOP("Welcome Msg", "Hello, you have %n message(s)"),
    QT_TRANSLATE_N_NOOP("Welcome Msg", "Hi, you have %n message(s)")
};

QString global_greeting(int type, int msgcnt)
{
    return translate("Welcome Msg", greeting_strings[type], nullptr, msgcnt);
}
//! [qttranslatennoop]


void qttrid_example()
{
    int n = 0;
    //! [qttrid]
    //% "%n fooish bar(s) found.\n"
    //% "Do you want to continue?"
    QString text = qtTrId("qtn_foo_bar", n);
    //! [qttrid]
}


namespace qttrid_n_noop
{
    //! [qttrid_n_noop]
    static const char * const ids[] = {
        //% "%n foo(s) found."
        QT_TRID_N_NOOP("qtn_foo"),
        //% "%n bar(s) found."
        QT_TRID_N_NOOP("qtn_bar"),
        0
    };

    QString result(int type, int n)
    {
        return qtTrId(ids[type], n);
    }
    //! [qttrid_n_noop]
}

QT_BEGIN_NAMESPACE

//! [38]
struct Point3D
{
    int x;
    int y;
    int z;
};

Q_DECLARE_TYPEINFO(Point3D, Q_PRIMITIVE_TYPE);
//! [38]

//! [39]
class Point2D
{
public:
    Point2D() { data = new int[2]; }
    Point2D(const Point2D &other) { /*...*/ }
    ~Point2D() { delete[] data; }

    Point2D &operator=(const Point2D &other) { /*...*/ }

    int x() const { return data[0]; }
    int y() const { return data[1]; }

private:
    int *data;
};

Q_DECLARE_TYPEINFO(Point2D, Q_RELOCATABLE_TYPE);
//! [39]


//! [40]
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
//...
#endif

//or

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
//...
#endif

//! [40]


//! [41]

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
//...
#endif

//! [41]


//! [42]
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
//...
#endif

//! [42]

//! [begin namespace macro]
namespace QT_NAMESPACE {
//! [begin namespace macro]

//! [end namespace macro]
}
//! [end namespace macro]

namespace snippet_43
{
    //! [43]
    class MyClass : public QObject
    {
    private:
        Q_DISABLE_COPY(MyClass)
    };

    //! [43]
}

//! [44]
class MyClass : public QObject
{
private:
    MyClass(const MyClass &) = delete;
    MyClass &operator=(const MyClass &) = delete;
};
//! [44]


void qfuzzycompare_example()
{
    //! [46]
    // Instead of comparing with 0.0
    qFuzzyCompare(0.0, 1.0e-200); // This will return false
    // Compare adding 1 to both values will fix the problem
    qFuzzyCompare(1 + 0.0, 1 + 1.0e-200); // This will return true
    //! [46]
}

//! [49]
void myMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);
//! [49]

//! [50]
class B {/*...*/};
class C {/*...*/};
class D {/*...*/};
struct A : public B {
    C c;
    D d;
};
//! [50]

//! [51]
template<> class QTypeInfo<A> : public QTypeInfoMerger<A, B, C, D> {};
//! [51]

QT_END_NAMESPACE

namespace snippet_52
{
    //! [52]
    struct Foo {
        void overloadedFunction();
        void overloadedFunction(int, const QString &);
    };
    auto ptr_1 = qOverload<>(&Foo::overloadedFunction);
    auto ptr_2 = qOverload<int, const QString &>(&Foo::overloadedFunction);
    //! [52]
}


//! [54]
    struct Foo {
        void overloadedFunction(int, const QString &);
        void overloadedFunction(int, const QString &) const;
    };
    auto ptr_1 = qConstOverload<int, const QString &>(&Foo::overloadedFunction);
    auto ptr_2 = qNonConstOverload<int, const QString &>(&Foo::overloadedFunction);
//! [54]

bool isWorkingDay(int day)
{
    return false;
}

void qlikely_example()
{
    //! [qlikely]
        // the condition inside the "if" will be successful most of the times
        for (int i = 1; i <= 365; i++) {
            if (Q_LIKELY(isWorkingDay(i))) {
                //...
            }
            //...
        }
    //! [qlikely]
}

//! [qunlikely]
bool readConfiguration(const QFile &file)
{
    // We expect to be asked to read an existing file
    if (Q_UNLIKELY(!file.exists())) {
        qWarning() << "File not found";
        return false;
    }

    //...
    return true;
}
//! [qunlikely]

//! [qunreachable-enum]
   enum Shapes {
       Rectangle,
       Triangle,
       Circle,
       NumShapes
   };
//! [qunreachable-enum]

int rectangle() { return 0; }
int triangle() { return 1; }
int circle() { return 2; }

int qunreachable_example(Shapes shape)
{
    //! [qunreachable-switch]
        switch (shape) {
            case Rectangle:
                return rectangle();
            case Triangle:
                return triangle();
            case Circle:
                return circle();
            case NumShapes:
                Q_UNREACHABLE();
                break;
        }
    //! [qunreachable-switch]
    return -1;
}


void qgetenv_examples()
{
    const char *varName = "MY_ENV_VAR";
    bool *ok = nullptr;

    //! [is-empty]
       bool is_empty = qgetenv(varName).isEmpty();
    //! [is-empty]

    //! [to-int]
       int to_int = qgetenv(varName).toInt(ok, 0);
    //! [to-int]

    //! [int-value_or]
       auto value = qEnvironmentVariableIntegerValue(varName).value_or(0);
    //! [int-value_or]

    //! [int-eq0]
       bool equals_zero = qEnvironmentVariableIntegerValue(varName) == 0;
    //! [int-eq0]

    //! [is-null]
       bool is_not_null = !qgetenv(varName).isNull();
    //! [is-null]
}

QString funcReturningQString()
{
    return "Hello, World!";
}

void process(const QChar &ch) { }

void qchar_examples()
{
#if QT_DEPRECATED_SINCE(6, 6)
    {
    //! [as-const-0]
        QString s = "...";
        for (QChar ch : s) // detaches 's' (performs a deep-copy if 's' was shared)
            process(ch);
        for (QChar ch : qAsConst(s)) // ok, no detach attempt
            process(ch);
    //! [as-const-0]
    }
#endif // QT_DEPRECATED_SINCE(6, 6)

    //! [as-const-1]
        const QString s = "...";
        for (QChar ch : s) // ok, no detach attempt on const objects
            process(ch);
    //! [as-const-1]

    //! [as-const-2]
        for (QChar ch : funcReturningQString())
            process(ch); // OK, the returned object is kept alive for the loop's duration
    //! [as-const-2]

#if MAKE_ERRORS
    //! [as-const-3]
        for (QChar ch : qAsConst(funcReturningQString()))
            process(ch); // ERROR: ch is copied from deleted memory
    //! [as-const-3]

    //! [as-const-4]
        for (QChar ch : qAsConst(funcReturningQString()))
            process(ch); // ERROR: ch is copied from deleted memory
    //! [as-const-4]
#endif
}

class ExampleClass
{
protected:
#if MAKE_ERRORS
    //! [qdecloverride]
        // generate error if this doesn't actually override anything:
        virtual void override_func() override;
    //! [qdecloverride]
#endif

    //! [qdeclfinal-1]
        // more-derived classes no longer permitted to override this:
        virtual void final_func() final;
    //! [qdeclfinal-1]
};
//! [qdeclfinal-2]
    class SomeClass final { // cannot be derived from
        // ...
    };
//! [qdeclfinal-2]
