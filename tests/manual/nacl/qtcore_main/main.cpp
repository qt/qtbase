#include <QtCore>
#include <iostream>

// Defining main and using Qt Core is possble,
// but there is no way to use QtGui since PPAPI
// is not started.

#if 0
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    puts("hello from C");
    std::cout << "hello from C++" << std::endl;
    qDebug() << "hello from Qt";
}

#else

#include <QtTest/QtTest>

class TestQString: public QObject
{
    Q_OBJECT
private slots:
    void toUpper();
};

void TestQString::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}

QTEST_MAIN(TestQString)
#include "main.moc"

#endif
