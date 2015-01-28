/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtTest/QtTest>
#include <QtDBus>

typedef QMap<int,QString> IntStringMap;
Q_DECLARE_METATYPE(IntStringMap)

struct MyStruct
{
    int i;
    QString s;

    MyStruct() : i(1), s("String") { }
    bool operator==(const MyStruct &other) const
    { return i == other.i && s == other.s; }
};
Q_DECLARE_METATYPE(MyStruct)

QDBusArgument &operator<<(QDBusArgument &arg, const MyStruct &ms)
{
    arg.beginStructure();
    arg << ms.i << ms.s;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, MyStruct &ms)
{
    arg.beginStructure();
    arg >> ms.i >> ms.s;
    arg.endStructure();
    return arg;
}

class TypesInterface;
class tst_QDBusPendingReply: public QObject
{
    Q_OBJECT
    QDBusInterface *iface;
    TypesInterface *adaptor;
public:
    tst_QDBusPendingReply();

private slots:
    void initTestCase()
    {
        qDBusRegisterMetaType<IntStringMap>();
        qDBusRegisterMetaType<MyStruct>();
    }

    void init();
    void unconnected();
    void simpleTypes();
    void complexTypes();
    void wrongTypes();
    void multipleTypes();

    void synchronousSimpleTypes();

    void errors();
};

class TypesInterface: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.Qt.Autotests.TypesInterface")
public:
    TypesInterface(QObject *parent)
        : QDBusAbstractAdaptor(parent)
    { }

public slots:
    void retrieveVoid()
    { }

    bool retrieveBool()
    {
        return true;
    }

    uchar retrieveUChar()
    {
        return 'A';
    }

    short retrieveShort()
    {
        return -47;
    }

    ushort retrieveUShort()
    {
        return 42U;
    }

    int retrieveInt()
    {
        return -470000;
    }

    void retrieveIntInt(int &i1, int &i2)
    {
        i1 = -424242;
        i2 = 434343;
    }

    uint retrieveUInt()
    {
        return 42424242;
    }

    qlonglong retrieveLongLong()
    {
        return -(Q_INT64_C(1) << 32);
    }

    qulonglong retrieveULongLong()
    {
        return Q_INT64_C(1) << 32;
    }

    double retrieveDouble()
    {
        return 1.5;
    }

    QString retrieveString()
    {
        return "This string you should see";
    }

    QDBusObjectPath retrieveObjectPath()
    {
        return QDBusObjectPath("/");
    }

    QDBusSignature retrieveSignature()
    {
        return QDBusSignature("g");
    }

    QDBusVariant retrieveVariant()
    {
        return QDBusVariant(retrieveString());
    }

    QStringList retrieveStringList()
    {
        return QStringList() << "one" << "two";
    }

    QByteArray retrieveByteArray()
    {
        return "Hello, World";
    }

    QVariantList retrieveList()
    {
        return QVariantList() << retrieveInt() << retrieveString()
                              << retrieveByteArray();
    }

    QVariantMap retrieveMap()
    {
        QVariantMap map;
        map["one"] = 1;
        map["two"] = 2U;
        map["string"] = retrieveString();
        map["stringlist"] = retrieveStringList();
        return map;
    }

    IntStringMap retrieveIntStringMap()
    {
        IntStringMap map;
        map[1] = "1";
        map[2] = "2";
        map[-1231456] = "foo";
        return map;
    }

    MyStruct retrieveStruct()
    {
        return MyStruct();
    }

    void sendError(const QDBusMessage &msg)
    {
        msg.setDelayedReply(true);
        QDBusConnection::sessionBus()
            .send(msg.createErrorReply("local.AnErrorName", "You've got an error!"));
    }
};

tst_QDBusPendingReply::tst_QDBusPendingReply()
{
    adaptor = new TypesInterface(this);
    QDBusConnection::sessionBus().registerObject("/", this);

    iface = new QDBusInterface(QDBusConnection::sessionBus().baseService(), "/",
                               "org.qtproject.Qt.Autotests.TypesInterface",
                               QDBusConnection::sessionBus(),
                               this);
}

void tst_QDBusPendingReply::init()
{
    QVERIFY(iface);
    QVERIFY(iface->isValid());
}

void tst_QDBusPendingReply::unconnected()
{
    QDBusConnection con("invalid stored connection");
    QVERIFY(!con.isConnected());
    QDBusInterface iface("doesnt.matter", "/", "doesnt.matter", con);
    QVERIFY(!iface.isValid());

    QDBusPendingReply<> rvoid = iface.asyncCall("ReloadConfig");
    QVERIFY(rvoid.isFinished());
    QVERIFY(!rvoid.isValid());
    QVERIFY(rvoid.isError());
    rvoid.waitForFinished();
    QVERIFY(!rvoid.isValid());
    QVERIFY(rvoid.isError());

    QDBusPendingReply<QString> rstring = iface.asyncCall("GetId");
    QVERIFY(rstring.isFinished());
    QVERIFY(!rstring.isValid());
    QVERIFY(rstring.isError());
    rstring.waitForFinished();
    QVERIFY(!rstring.isValid());
    QVERIFY(rstring.isError());
}

void tst_QDBusPendingReply::simpleTypes()
{
    QDBusPendingReply<> rvoid = iface->asyncCall("retrieveVoid");
    rvoid.waitForFinished();
    QVERIFY(rvoid.isFinished());
    QVERIFY(!rvoid.isError());

    QDBusPendingReply<bool> rbool = iface->asyncCall("retrieveBool");
    rbool.waitForFinished();
    QVERIFY(rbool.isFinished());
    QCOMPARE(rbool.argumentAt<0>(), adaptor->retrieveBool());

    QDBusPendingReply<uchar> ruchar = iface->asyncCall("retrieveUChar");
    ruchar.waitForFinished();
    QVERIFY(ruchar.isFinished());
    QCOMPARE(ruchar.argumentAt<0>(), adaptor->retrieveUChar());

    QDBusPendingReply<short> rshort = iface->asyncCall("retrieveShort");
    rshort.waitForFinished();
    QVERIFY(rshort.isFinished());
    QCOMPARE(rshort.argumentAt<0>(), adaptor->retrieveShort());

    QDBusPendingReply<ushort> rushort = iface->asyncCall("retrieveUShort");
    rushort.waitForFinished();
    QVERIFY(rushort.isFinished());
    QCOMPARE(rushort.argumentAt<0>(), adaptor->retrieveUShort());

    QDBusPendingReply<int> rint = iface->asyncCall("retrieveInt");
    rint.waitForFinished();
    QVERIFY(rint.isFinished());
    QCOMPARE(rint.argumentAt<0>(), adaptor->retrieveInt());

    QDBusPendingReply<uint> ruint = iface->asyncCall("retrieveUInt");
    ruint.waitForFinished();
    QVERIFY(ruint.isFinished());
    QCOMPARE(ruint.argumentAt<0>(), adaptor->retrieveUInt());

    QDBusPendingReply<qlonglong> rqlonglong = iface->asyncCall("retrieveLongLong");
    rqlonglong.waitForFinished();
    QVERIFY(rqlonglong.isFinished());
    QCOMPARE(rqlonglong.argumentAt<0>(), adaptor->retrieveLongLong());

    QDBusPendingReply<qulonglong> rqulonglong = iface->asyncCall("retrieveULongLong");
    rqulonglong.waitForFinished();
    QVERIFY(rqulonglong.isFinished());
    QCOMPARE(rqulonglong.argumentAt<0>(), adaptor->retrieveULongLong());

    QDBusPendingReply<double> rdouble = iface->asyncCall("retrieveDouble");
    rdouble.waitForFinished();
    QVERIFY(rdouble.isFinished());
    QCOMPARE(rdouble.argumentAt<0>(), adaptor->retrieveDouble());

    QDBusPendingReply<QString> rstring = iface->asyncCall("retrieveString");
    rstring.waitForFinished();
    QVERIFY(rstring.isFinished());
    QCOMPARE(rstring.argumentAt<0>(), adaptor->retrieveString());

    QDBusPendingReply<QDBusObjectPath> robjectpath = iface->asyncCall("retrieveObjectPath");
    robjectpath.waitForFinished();
    QVERIFY(robjectpath.isFinished());
    QCOMPARE(robjectpath.argumentAt<0>().path(), adaptor->retrieveObjectPath().path());

    QDBusPendingReply<QDBusSignature> rsignature = iface->asyncCall("retrieveSignature");
    rsignature.waitForFinished();
    QVERIFY(rsignature.isFinished());
    QCOMPARE(rsignature.argumentAt<0>().signature(), adaptor->retrieveSignature().signature());

    QDBusPendingReply<QDBusVariant> rdbusvariant = iface->asyncCall("retrieveVariant");
    rdbusvariant.waitForFinished();
    QVERIFY(rdbusvariant.isFinished());
    QCOMPARE(rdbusvariant.argumentAt<0>().variant(), adaptor->retrieveVariant().variant());

    QDBusPendingReply<QVariant> rvariant = iface->asyncCall("retrieveVariant");
    rvariant.waitForFinished();
    QVERIFY(rvariant.isFinished());
    QCOMPARE(rvariant.argumentAt<0>(), adaptor->retrieveVariant().variant());

    QDBusPendingReply<QByteArray> rbytearray = iface->asyncCall("retrieveByteArray");
    rbytearray.waitForFinished();
    QVERIFY(rbytearray.isFinished());
    QCOMPARE(rbytearray.argumentAt<0>(), adaptor->retrieveByteArray());

    QDBusPendingReply<QStringList> rstringlist = iface->asyncCall("retrieveStringList");
    rstringlist.waitForFinished();
    QVERIFY(rstringlist.isFinished());
    QCOMPARE(rstringlist.argumentAt<0>(), adaptor->retrieveStringList());
}

void tst_QDBusPendingReply::complexTypes()
{
    QDBusPendingReply<QVariantList> rlist = iface->asyncCall("retrieveList");
    rlist.waitForFinished();
    QVERIFY(rlist.isFinished());
    QCOMPARE(rlist.argumentAt<0>(), adaptor->retrieveList());

    QDBusPendingReply<QVariantMap> rmap = iface->asyncCall("retrieveMap");
    rmap.waitForFinished();
    QVERIFY(rmap.isFinished());
    QCOMPARE(rmap.argumentAt<0>(), adaptor->retrieveMap());

    QDBusPendingReply<IntStringMap> rismap = iface->asyncCall("retrieveIntStringMap");
    rismap.waitForFinished();
    QVERIFY(rismap.isFinished());
    QCOMPARE(rismap.argumentAt<0>(), adaptor->retrieveIntStringMap());

    QDBusPendingReply<MyStruct> rstruct = iface->asyncCall("retrieveStruct");
    rstruct.waitForFinished();
    QVERIFY(rstruct.isFinished());
    QCOMPARE(rstruct.argumentAt<0>(), adaptor->retrieveStruct());
}

#define VERIFY_WRONG_TYPE(error)                                \
    QVERIFY(error.isValid());                                   \
    QCOMPARE(error.type(), QDBusError::InvalidSignature)

void tst_QDBusPendingReply::wrongTypes()
{
    QDBusError error;

    QDBusPendingReply<bool> rbool = iface->asyncCall("retrieveInt");
    rbool.waitForFinished();
    QVERIFY(rbool.isFinished());
    QVERIFY(rbool.isError());
    error = rbool.error();
    VERIFY_WRONG_TYPE(error);

    rbool = iface->asyncCall("retrieveShort");
    rbool.waitForFinished();
    QVERIFY(rbool.isFinished());
    QVERIFY(rbool.isError());
    error = rbool.error();
    VERIFY_WRONG_TYPE(error);

    rbool = iface->asyncCall("retrieveStruct");
    rbool.waitForFinished();
    QVERIFY(rbool.isFinished());
    QVERIFY(rbool.isError());
    error = rbool.error();
    VERIFY_WRONG_TYPE(error);

    QDBusPendingReply<short> rshort = iface->asyncCall("retrieveInt");
    rshort.waitForFinished();
    QVERIFY(rshort.isFinished());
    QVERIFY(rshort.isError());
    error = rshort.error();
    VERIFY_WRONG_TYPE(error);

    rshort = iface->asyncCall("retrieveBool");
    rshort.waitForFinished();
    QVERIFY(rshort.isFinished());
    QVERIFY(rshort.isError());
    error = rshort.error();
    VERIFY_WRONG_TYPE(error);

    rshort = iface->asyncCall("retrieveStruct");
    rshort.waitForFinished();
    QVERIFY(rshort.isFinished());
    QVERIFY(rshort.isError());
    error = rshort.error();
    VERIFY_WRONG_TYPE(error);

    QDBusPendingReply<MyStruct> rstruct = iface->asyncCall("retrieveInt");
    rstruct.waitForFinished();
    QVERIFY(rstruct.isFinished());
    QVERIFY(rstruct.isError());
    error = rstruct.error();
    VERIFY_WRONG_TYPE(error);

    rstruct = iface->asyncCall("retrieveShort");
    rstruct.waitForFinished();
    QVERIFY(rstruct.isFinished());
    QVERIFY(rstruct.isError());
    error = rstruct.error();
    VERIFY_WRONG_TYPE(error);

    rstruct = iface->asyncCall("retrieveIntStringMap");
    rstruct.waitForFinished();
    QVERIFY(rstruct.isFinished());
    QVERIFY(rstruct.isError());
    error = rstruct.error();
    VERIFY_WRONG_TYPE(error);
}

void tst_QDBusPendingReply::multipleTypes()
{
    QDBusPendingReply<int, int> rintint = iface->asyncCall("retrieveIntInt");
    rintint.waitForFinished();
    QVERIFY(rintint.isFinished());
    QVERIFY(!rintint.isError());

    int i1, i2;
    adaptor->retrieveIntInt(i1, i2);
    QCOMPARE(rintint.argumentAt<0>(), i1);
    QCOMPARE(rintint.argumentAt<1>(), i2);
}

void tst_QDBusPendingReply::synchronousSimpleTypes()
{
    QDBusPendingReply<bool> rbool(iface->call("retrieveBool"));
    rbool.waitForFinished();
    QVERIFY(rbool.isFinished());
    QCOMPARE(rbool.argumentAt<0>(), adaptor->retrieveBool());

    QDBusPendingReply<uchar> ruchar(iface->call("retrieveUChar"));
    ruchar.waitForFinished();
    QVERIFY(ruchar.isFinished());
    QCOMPARE(ruchar.argumentAt<0>(), adaptor->retrieveUChar());

    QDBusPendingReply<short> rshort(iface->call("retrieveShort"));
    rshort.waitForFinished();
    QVERIFY(rshort.isFinished());
    QCOMPARE(rshort.argumentAt<0>(), adaptor->retrieveShort());

    QDBusPendingReply<ushort> rushort(iface->call("retrieveUShort"));
    rushort.waitForFinished();
    QVERIFY(rushort.isFinished());
    QCOMPARE(rushort.argumentAt<0>(), adaptor->retrieveUShort());

    QDBusPendingReply<int> rint(iface->call("retrieveInt"));
    rint.waitForFinished();
    QVERIFY(rint.isFinished());
    QCOMPARE(rint.argumentAt<0>(), adaptor->retrieveInt());

    QDBusPendingReply<uint> ruint(iface->call("retrieveUInt"));
    ruint.waitForFinished();
    QVERIFY(ruint.isFinished());
    QCOMPARE(ruint.argumentAt<0>(), adaptor->retrieveUInt());

    QDBusPendingReply<qlonglong> rqlonglong(iface->call("retrieveLongLong"));
    rqlonglong.waitForFinished();
    QVERIFY(rqlonglong.isFinished());
    QCOMPARE(rqlonglong.argumentAt<0>(), adaptor->retrieveLongLong());

    QDBusPendingReply<qulonglong> rqulonglong(iface->call("retrieveULongLong"));
    rqulonglong.waitForFinished();
    QVERIFY(rqulonglong.isFinished());
    QCOMPARE(rqulonglong.argumentAt<0>(), adaptor->retrieveULongLong());

    QDBusPendingReply<double> rdouble(iface->call("retrieveDouble"));
    rdouble.waitForFinished();
    QVERIFY(rdouble.isFinished());
    QCOMPARE(rdouble.argumentAt<0>(), adaptor->retrieveDouble());

    QDBusPendingReply<QString> rstring(iface->call("retrieveString"));
    rstring.waitForFinished();
    QVERIFY(rstring.isFinished());
    QCOMPARE(rstring.argumentAt<0>(), adaptor->retrieveString());

    QDBusPendingReply<QDBusObjectPath> robjectpath(iface->call("retrieveObjectPath"));
    robjectpath.waitForFinished();
    QVERIFY(robjectpath.isFinished());
    QCOMPARE(robjectpath.argumentAt<0>().path(), adaptor->retrieveObjectPath().path());

    QDBusPendingReply<QDBusSignature> rsignature(iface->call("retrieveSignature"));
    rsignature.waitForFinished();
    QVERIFY(rsignature.isFinished());
    QCOMPARE(rsignature.argumentAt<0>().signature(), adaptor->retrieveSignature().signature());

    QDBusPendingReply<QDBusVariant> rdbusvariant(iface->call("retrieveVariant"));
    rdbusvariant.waitForFinished();
    QVERIFY(rdbusvariant.isFinished());
    QCOMPARE(rdbusvariant.argumentAt<0>().variant(), adaptor->retrieveVariant().variant());

    QDBusPendingReply<QByteArray> rbytearray(iface->call("retrieveByteArray"));
    rbytearray.waitForFinished();
    QVERIFY(rbytearray.isFinished());
    QCOMPARE(rbytearray.argumentAt<0>(), adaptor->retrieveByteArray());

    QDBusPendingReply<QStringList> rstringlist(iface->call("retrieveStringList"));
    rstringlist.waitForFinished();
    QVERIFY(rstringlist.isFinished());
    QCOMPARE(rstringlist.argumentAt<0>(), adaptor->retrieveStringList());
}

#define VERIFY_ERROR(error)                                     \
    QVERIFY(error.isValid());                                   \
    QCOMPARE(error.name(), QString("local.AnErrorName"));       \
    QCOMPARE(error.type(), QDBusError::Other)

void tst_QDBusPendingReply::errors()
{
    QDBusError error;

    QDBusPendingReply<> rvoid(iface->asyncCall("sendError"));
    rvoid.waitForFinished();
    QVERIFY(rvoid.isFinished());
    QVERIFY(rvoid.isError());
    error = rvoid.error();
    VERIFY_ERROR(error);

    QDBusPendingReply<int> rint(iface->asyncCall("sendError"));
    rint.waitForFinished();
    QVERIFY(rint.isFinished());
    QVERIFY(rint.isError());
    error = rint.error();
    VERIFY_ERROR(error);
    int dummyint = rint;
    QCOMPARE(dummyint, int());

    QDBusPendingReply<int,int> rintint(iface->asyncCall("sendError"));
    rintint.waitForFinished();
    QVERIFY(rintint.isFinished());
    QVERIFY(rintint.isError());
    error = rintint.error();
    VERIFY_ERROR(error);
    dummyint = rintint;
    QCOMPARE(dummyint, int());
    QCOMPARE(rintint.argumentAt<1>(), int());

    QDBusPendingReply<QString> rstring(iface->asyncCall("sendError"));
    rstring.waitForFinished();
    QVERIFY(rstring.isFinished());
    QVERIFY(rstring.isError());
    error = rstring.error();
    VERIFY_ERROR(error);
    QString dummystring = rstring;
    QCOMPARE(dummystring, QString());
}

QTEST_MAIN(tst_QDBusPendingReply)

#include "tst_qdbuspendingreply.moc"
