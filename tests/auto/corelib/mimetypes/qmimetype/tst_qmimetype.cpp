// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qmimetype_p.h>

#include <qmimetype.h>
#include <qmimedatabase.h>
#include <QVariantMap>

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>

class tst_qmimetype : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void compareCompiles();
    void isValid();
    void compareQMimetypes();
    void name();
    void genericIconName();
    void iconName();
    void gadget();
};

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::initTestCase()
{
    qputenv("XDG_DATA_DIRS", "doesnotexist");
}

// ------------------------------------------------------------------------------------------------

static QString qMimeTypeName()
{
    static const QString result("group/fake-mime");
    return result;
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QMimeType>();
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::compareQMimetypes()
{
    QMimeType instantiatedQMimeType{ QMimeTypePrivate(qMimeTypeName()) };
    QMimeType otherQMimeType (instantiatedQMimeType);
    QMimeType defaultQMimeType;

    QVERIFY(!defaultQMimeType.isValid());
    QT_TEST_EQUALITY_OPS(defaultQMimeType, QMimeType(), true);
    QT_TEST_EQUALITY_OPS(QMimeType(), QMimeType(), true);
    QT_TEST_EQUALITY_OPS(instantiatedQMimeType, QMimeType(), false);
    QT_TEST_EQUALITY_OPS(otherQMimeType, defaultQMimeType, false);
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::isValid()
{
    QMimeType instantiatedQMimeType{ QMimeTypePrivate(qMimeTypeName()) };
    QVERIFY(instantiatedQMimeType.isValid());

    QMimeType otherQMimeType (instantiatedQMimeType);

    QVERIFY(otherQMimeType.isValid());
    QT_TEST_EQUALITY_OPS(instantiatedQMimeType, otherQMimeType, true);

    QMimeType defaultQMimeType;

    QVERIFY(!defaultQMimeType.isValid());
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::name()
{
    QMimeType instantiatedQMimeType{ QMimeTypePrivate(qMimeTypeName()) };
    QMimeType otherQMimeType{ QMimeTypePrivate(QString()) };

    // Verify that the Name is part of the equality test:
    QCOMPARE(instantiatedQMimeType.name(), qMimeTypeName());

    QT_TEST_EQUALITY_OPS(instantiatedQMimeType, otherQMimeType, false);
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::genericIconName()
{
    const QMimeType instantiatedQMimeType{ QMimeTypePrivate(qMimeTypeName()) };
    QCOMPARE(instantiatedQMimeType.genericIconName(), "group-x-generic");
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::iconName()
{
    const QMimeType instantiatedQMimeType{ QMimeTypePrivate(qMimeTypeName()) };
    QCOMPARE(instantiatedQMimeType.iconName(), "group-fake-mime");
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::gadget()
{
    QMimeType instantiatedQMimeType = QMimeDatabase().mimeTypeForName("text/plain");

    const QMetaObject *metaObject = &instantiatedQMimeType.staticMetaObject;

    QCOMPARE(metaObject->className(), "QMimeType");
    QVariantMap properties;
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        properties[property.name()] = property.readOnGadget(&instantiatedQMimeType);
    }

    QCOMPARE(properties["valid"].toBool(), instantiatedQMimeType.isValid());
    QCOMPARE(properties["isDefault"].toBool(), instantiatedQMimeType.isDefault());
    QCOMPARE(properties["name"].toString(), instantiatedQMimeType.name());
    QCOMPARE(properties["comment"].toString(), instantiatedQMimeType.comment());
    QCOMPARE(properties["genericIconName"].toString(), instantiatedQMimeType.genericIconName());
    QCOMPARE(properties["iconName"].toString(), instantiatedQMimeType.iconName());
    QCOMPARE(properties["globPatterns"].toStringList(), instantiatedQMimeType.globPatterns());
    QCOMPARE(properties["parentMimeTypes"].toStringList(), instantiatedQMimeType.parentMimeTypes());
    QCOMPARE(properties["allAncestors"].toStringList(), instantiatedQMimeType.allAncestors());
    QCOMPARE(properties["aliases"].toStringList(), instantiatedQMimeType.aliases());
    QCOMPARE(properties["suffixes"].toStringList(), instantiatedQMimeType.suffixes());
    QCOMPARE(properties["preferredSuffix"].toString(), instantiatedQMimeType.preferredSuffix());
    QCOMPARE(properties["filterString"].toString(), instantiatedQMimeType.filterString());

    QVERIFY(metaObject->indexOfMethod("inherits(QString)") >= 0);
}

// ------------------------------------------------------------------------------------------------

QTEST_GUILESS_MAIN(tst_qmimetype)
#include "tst_qmimetype.moc"
