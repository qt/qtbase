// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QLoggingCategory>

#ifdef Q_CC_GNU
#define NEVER_INLINE __attribute__((__noinline__))
#else
#define NEVER_INLINE
#endif

struct T {
    T() { qDebug("static constructor"); }
    ~T() { qDebug("static destructor"); }
} t;

class MyClass : public QObject
{
    Q_OBJECT
public slots:
    virtual NEVER_INLINE QString mySlot1();
private:
    virtual NEVER_INLINE void myFunction(int a);
};

QString MyClass::mySlot1()
{
    myFunction(34);
    return QString();
}

void MyClass::myFunction(int a)
{
    qDebug() << "from_a_function" << a;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("tst_qlogging");

    qSetMessagePattern("[%{type}] %{message}");

    qDebug("qDebug");
    qInfo("qInfo");
    qWarning("qWarning");
    qCritical("qCritical");

    QLoggingCategory cat("category");
    qCWarning(cat) << "qDebug with category";

    qSetMessagePattern(QString());

    qDebug("qDebug2");

    MyClass cl;
    QMetaObject::invokeMethod(&cl, "mySlot1");

    return 0;
}

#include "main.moc"
