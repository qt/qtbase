/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
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


#include <QtTest/QtTest>
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
  CustomWidget(QWidget *parent = 0)
    : QWidget(parent)
  {

  }
};

Q_STATIC_ASSERT(( QMetaTypeId2<QSizePolicy>::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<QWidget*>::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<QList<QSizePolicy> >::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<QMap<QString,QSizePolicy> >::IsBuiltIn));


void tst_QWidgetMetaType::metaObject()
{
    QCOMPARE(QMetaType::metaObjectForType(qMetaTypeId<QWidget*>()), &QWidget::staticMetaObject);
    QCOMPARE(QMetaType::metaObjectForType(qMetaTypeId<QLabel*>()), &QLabel::staticMetaObject);
    QCOMPARE(QMetaType::metaObjectForType(qMetaTypeId<CustomWidget*>()), &CustomWidget::staticMetaObject);
    QCOMPARE(QMetaType::metaObjectForType(qMetaTypeId<QSizePolicy>()), &QSizePolicy::staticMetaObject);
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

    void *value = QMetaType::create(type);

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);
    QCOMPARE(QMetaType::save(stream, type, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable)
        QVERIFY(QMetaType::load(stream, type, value));

    stream.device()->seek(0);
    stream.resetStatus();
    QCOMPARE(QMetaType::load(stream, type, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable)
        QVERIFY(QMetaType::load(stream, type, value));

    QMetaType::destroy(type, value);
}


QTEST_MAIN(tst_QWidgetMetaType)
#include "tst_qwidgetmetatype.moc"
