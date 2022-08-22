// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


void wrapInFunction()
{

//! [0]
class MyClass : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("author", "Sabrina Schweinsteiger")
    Q_CLASSINFO("url", "http://doc.moosesoft.co.uk/1.0/")

public:
    ...
};
//! [0]


//! [1]
QByteArray normType = QMetaObject::normalizedType(" int    const  *");
// normType is now "const int*"
//! [1]


//! [2]
QMetaObject::invokeMethod(thread, "quit",
                          Qt::QueuedConnection);
//! [2]


//! [3]
QMetaObject::invokeMethod: Unable to handle unregistered datatype 'MyType'
//! [3]

//! [invokemethod-no-macro]
QString retVal;
QMetaObject::invokeMethod(obj, "compute", Qt::DirectConnection,
                         qReturnArg(retVal),
                         QString("sqrt"), 42, 9.7);
//! [invokemethod-no-macro]


//! [invokemethod-no-macro-other-types]
QString retVal;
QMetaObject::invokeMethod(obj, "compute", Qt::DirectConnection,
                         qReturnArg(retVal),
                         QStringView("sqrt"), qsizetype(42), 9.7f);
//! [invokemethod-no-macro-other-types]


//! [4]
QString retVal;
QMetaObject::invokeMethod(obj, "compute", Qt::DirectConnection,
                          Q_RETURN_ARG(QString, retVal),
                          Q_ARG(QString, "sqrt"),
                          Q_ARG(int, 42),
                          Q_ARG(double, 9.7));
//! [4]


//! [5]
class MyClass
{
    Q_OBJECT
    Q_CLASSINFO("author", "Sabrina Schweinsteiger")
    Q_CLASSINFO("url", "http://doc.moosesoft.co.uk/1.0/")

public:
    ...
};
//! [5]


//! [propertyCount]
const QMetaObject* metaObject = obj->metaObject();
QStringList properties;
for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i)
    properties << QString::fromLatin1(metaObject->property(i).name());
//! [propertyCount]


//! [methodCount]
const QMetaObject* metaObject = obj->metaObject();
QStringList methods;
for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
    methods << QString::fromLatin1(metaObject->method(i).methodSignature());
//! [methodCount]

//! [6]
int methodIndex = pushButton->metaObject()->indexOfMethod("animateClick()");
QMetaMethod method = metaObject->method(methodIndex);
method.invoke(pushButton, Qt::QueuedConnection);
//! [6]

//! [7]
QMetaMethod::invoke: Unable to handle unregistered datatype 'MyType'
//! [7]

//! [invoke-no-macro]
QString retVal;
QByteArray normalizedSignature = QMetaObject::normalizedSignature("compute(QString, int, double)");
int methodIndex = obj->metaObject()->indexOfMethod(normalizedSignature);
QMetaMethod method = obj->metaObject()->method(methodIndex);
method.invoke(obj, Qt::DirectConnection, qReturnArg(retVal),
              QString("sqrt"), 42, 9.7);
//! [invoke-no-macro]

//! [invoke-no-macro-other-types]
QString retVal;
QByteArray normalizedSignature = QMetaObject::normalizedSignature("compute(QByteArray, qint64, long double)");
int methodIndex = obj->metaObject()->indexOfMethod(normalizedSignature);
QMetaMethod method = obj->metaObject()->method(methodIndex);
method.invoke(obj, Qt::DirectConnection, qReturnArg(retVal),
              QByteArray("sqrt"), qint64(42), 9.7L);
//! [invoke-no-macro-other-types]

//! [8]
QString retVal;
QByteArray normalizedSignature = QMetaObject::normalizedSignature("compute(QString, int, double)");
int methodIndex = obj->metaObject()->indexOfMethod(normalizedSignature);
QMetaMethod method = obj->metaObject()->method(methodIndex);
method.invoke(obj,
              Qt::DirectConnection,
              Q_RETURN_ARG(QString, retVal),
              Q_ARG(QString, "sqrt"),
              Q_ARG(int, 42),
              Q_ARG(double, 9.7));
//! [8]

//! [9]
QMetaMethod destroyedSignal = QMetaMethod::fromSignal(&QObject::destroyed);
//! [9]

//! [10]
    // In the class MainWindow declaration
    #ifndef Q_MOC_RUN
    // define the tag text as empty, so the compiler doesn't see it
    #  define MY_CUSTOM_TAG
    #endif
    ...
    private slots:
        MY_CUSTOM_TAG void testFunc();
//! [10]

//! [11]
    MainWindow win;
    win.show();

    int functionIndex = win.metaObject()->indexOfSlot("testFunc()");
    QMetaMethod mm = win.metaObject()->method(functionIndex);
    qDebug() << mm.tag(); // prints MY_CUSTOM_TAG
//! [11]
}
