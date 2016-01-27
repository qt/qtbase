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


#include <QtTest/QtTest>

#include <qvariant.h>

#include "tst_qvariant_common.h"


class tst_QWidgetsVariant : public QObject
{
    Q_OBJECT

private slots:

    void constructor_invalid_data();
    void constructor_invalid();

    void canConvert_data();
    void canConvert();

    void writeToReadFromDataStream_data();
    void writeToReadFromDataStream();

    void qvariant_cast_QObject_data();
    void qvariant_cast_QObject();
    void qvariant_cast_QObject_derived();

    void debugStream_data();
    void debugStream();

    void implicitConstruction();

    void widgetsVariantAtExit();
};

void tst_QWidgetsVariant::constructor_invalid_data()
{
    QTest::addColumn<uint>("typeId");

    QTest::newRow("LastGuiType + 1") << uint(QMetaType::LastGuiType + 1);
    QVERIFY(!QMetaType::isRegistered(QMetaType::LastGuiType + 1));
    QTest::newRow("LastWidgetsType + 1") << uint(QMetaType::LastWidgetsType + 1);
    QVERIFY(!QMetaType::isRegistered(QMetaType::LastWidgetsType + 1));
}

void tst_QWidgetsVariant::constructor_invalid()
{

    QFETCH(uint, typeId);
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type, type id:"));
        QVariant variant(static_cast<QVariant::Type>(typeId));
        QVERIFY(!variant.isValid());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type, type id:"));
        QVariant variant(typeId, /* copy */ 0);
        QVERIFY(!variant.isValid());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
}

void tst_QWidgetsVariant::canConvert_data()
{
    TST_QVARIANT_CANCONVERT_DATATABLE_HEADERS

#ifdef Y
#undef Y
#endif
#ifdef N
#undef N
#endif
#define Y true
#define N false

    QVariant var;

    //            bita bitm bool brsh byta col  curs date dt   dbl  font img  int  inv  kseq list ll   map  pal  pen  pix  pnt  rect reg  size sp   str  strl time uint ull


    var = QVariant::fromValue(QSizePolicy());
    QTest::newRow("SizePolicy")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N;

#undef N
#undef Y
}

void tst_QWidgetsVariant::canConvert()
{
    TST_QVARIANT_CANCONVERT_FETCH_DATA

    TST_QVARIANT_CANCONVERT_COMPARE_DATA
}


void tst_QWidgetsVariant::writeToReadFromDataStream_data()
{
    QTest::addColumn<QVariant>("writeVariant");
    QTest::addColumn<bool>("isNull");

    QTest::newRow( "sizepolicy_valid" ) << QVariant::fromValue( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) ) << false;
}

void tst_QWidgetsVariant::writeToReadFromDataStream()
{
    QFETCH( QVariant, writeVariant );
    QFETCH( bool, isNull );
    QByteArray data;

    QDataStream writeStream( &data, QIODevice::WriteOnly );
    writeStream << writeVariant;

    QVariant readVariant;
    QDataStream readStream( &data, QIODevice::ReadOnly );
    readStream >> readVariant;
    QVERIFY( readVariant.isNull() == isNull );
}

class CustomQWidget : public QWidget {
    Q_OBJECT
public:
    CustomQWidget(QWidget *parent = 0) : QWidget(parent) {}
};

void tst_QWidgetsVariant::qvariant_cast_QObject_data()
{
    QTest::addColumn<QVariant>("data");
    QTest::addColumn<bool>("success");

    QWidget *widget = new QWidget;
    widget->setObjectName(QString::fromLatin1("Hello"));
    QTest::newRow("from QWidget") << QVariant::fromValue(widget) << true;

    CustomQWidget *customWidget = new CustomQWidget;
    customWidget->setObjectName(QString::fromLatin1("Hello"));
    QTest::newRow("from Derived QWidget") << QVariant::fromValue(customWidget) << true;
}

void tst_QWidgetsVariant::qvariant_cast_QObject()
{
    QFETCH(QVariant, data);
    QFETCH(bool, success);

    QObject *o = qvariant_cast<QObject *>(data);
    QCOMPARE(o != 0, success);
    if (success) {
        QCOMPARE(o->objectName(), QString::fromLatin1("Hello"));
        QVERIFY(data.canConvert<QObject*>());
        QVERIFY(data.canConvert(QMetaType::QObjectStar));
        QVERIFY(data.canConvert(::qMetaTypeId<QObject*>()));
        QVERIFY(data.value<QObject*>());
        QVERIFY(data.convert(QMetaType::QObjectStar));
        QCOMPARE(data.userType(), int(QMetaType::QObjectStar));

        QVERIFY(data.canConvert<QWidget*>());
        QVERIFY(data.canConvert(::qMetaTypeId<QWidget*>()));
        QVERIFY(data.value<QWidget*>());
        QVERIFY(data.convert(qMetaTypeId<QWidget*>()));
        QCOMPARE(data.userType(), qMetaTypeId<QWidget*>());
    } else {
        QVERIFY(!data.canConvert<QObject*>());
        QVERIFY(!data.canConvert(QMetaType::QObjectStar));
        QVERIFY(!data.canConvert(::qMetaTypeId<QObject*>()));
        QVERIFY(!data.value<QObject*>());
        QVERIFY(!data.convert(QMetaType::QObjectStar));
        QVERIFY(data.userType() != QMetaType::QObjectStar);
    }
    delete o;
}

void tst_QWidgetsVariant::qvariant_cast_QObject_derived()
{
    CustomQWidget customWidget;
    QWidget *widget = &customWidget;
    QVariant data = QVariant::fromValue(widget);
    QCOMPARE(data.userType(), qMetaTypeId<QWidget*>());

    QCOMPARE(data.value<QObject*>(), widget);
    QCOMPARE(data.value<QWidget*>(), widget);
    QCOMPARE(data.value<CustomQWidget*>(), widget);
}

void tst_QWidgetsVariant::debugStream_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<int>("typeId");
    for (int id = QMetaType::LastGuiType + 1; id < QMetaType::User; ++id) {
        const char *tagName = QMetaType::typeName(id);
        if (!tagName)
            continue;
        QTest::newRow(tagName) << QVariant(static_cast<QVariant::Type>(id)) << id;
    }
}

void tst_QWidgetsVariant::debugStream()
{
    QFETCH(QVariant, variant);
    QFETCH(int, typeId);

    MessageHandler msgHandler(typeId);
    qDebug() << variant;
    QVERIFY(msgHandler.testPassed());
}

void tst_QWidgetsVariant::widgetsVariantAtExit()
{
    // crash test, it should not crash at QApplication exit
    static QVariant sizePolicy = QSizePolicy();
    Q_UNUSED(sizePolicy);
    QVERIFY(true);
}


void tst_QWidgetsVariant::implicitConstruction()
{
    // This is a compile-time test
    QVariant v;

#define FOR_EACH_WIDGETS_CLASS(F) \
    F(SizePolicy) \

#define CONSTRUCT(TYPE) \
    { \
        Q##TYPE t; \
        v = t; \
        QVERIFY(true); \
    }

    FOR_EACH_WIDGETS_CLASS(CONSTRUCT)

#undef CONSTRUCT
#undef FOR_EACH_WIDGETS_CLASS
}

QTEST_MAIN(tst_QWidgetsVariant)
#include "tst_qwidgetsvariant.moc"
