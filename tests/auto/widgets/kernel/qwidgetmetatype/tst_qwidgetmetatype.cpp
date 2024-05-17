// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <qwidget.h>
#include <qlabel.h>

class tst_QWidgetMetaType : public QObject
{
    Q_OBJECT

public:
    tst_QWidgetMetaType() {}
    virtual ~tst_QWidgetMetaType() {}

private slots:
    void metaObject();
    void saveAndLoadBuiltin_data();
    void saveAndLoadBuiltin();
};

class CustomWidget : public QWidget
{
  Q_OBJECT
public:
  CustomWidget(QWidget *parent = nullptr)
    : QWidget(parent)
  {

  }
};

static_assert(( QMetaTypeId2<QSizePolicy>::IsBuiltIn));
static_assert((!QMetaTypeId2<QWidget*>::IsBuiltIn));
static_assert((!QMetaTypeId2<QList<QSizePolicy> >::IsBuiltIn));
static_assert((!QMetaTypeId2<QMap<QString,QSizePolicy> >::IsBuiltIn));


void tst_QWidgetMetaType::metaObject()
{
    QCOMPARE(QMetaType::fromType<QWidget*>().metaObject(), &QWidget::staticMetaObject);
    QCOMPARE(QMetaType::fromType<QLabel*>().metaObject(), &QLabel::staticMetaObject);
    QCOMPARE(QMetaType::fromType<CustomWidget*>().metaObject(), &CustomWidget::staticMetaObject);
    QCOMPARE(QMetaType::fromType<QSizePolicy>().metaObject(), &QSizePolicy::staticMetaObject);
}

template <typename T>
struct StreamingTraits
{
    // Streamable by default, as currently all widgets built-in types are streamable
    enum { isStreamable = 1 };
};

void tst_QWidgetMetaType::saveAndLoadBuiltin_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isStreamable");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId << bool(StreamingTraits<RealType>::isStreamable);
    QT_FOR_EACH_STATIC_WIDGETS_CLASS(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QWidgetMetaType::saveAndLoadBuiltin()
{
    QFETCH(int, type);
    QFETCH(bool, isStreamable);

    void *value = QMetaType(type).create();

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);
    QCOMPARE(QMetaType(type).save(stream, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable)
        QVERIFY(QMetaType(type).load(stream, value));

    stream.device()->seek(0);
    stream.resetStatus();
    QCOMPARE(QMetaType(type).load(stream, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable)
        QVERIFY(QMetaType(type).load(stream, value));

    QMetaType(type).destroy(value);
}


QTEST_MAIN(tst_QWidgetMetaType)
#include "tst_qwidgetmetatype.moc"
