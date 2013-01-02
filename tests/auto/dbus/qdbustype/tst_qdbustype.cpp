/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the FOO module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>

#include <QtDBus/private/qdbusutil_p.h>

#include <dbus/dbus.h>

class tst_QDBusType : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void isValidFixedType_data();
    void isValidFixedType();
    void isValidBasicType_data();
    void isValidBasicType();
    void isValidSingleSignature_data();
    void isValidSingleSignature();
    void isValidArray_data();
    void isValidArray();
    void isValidSignature_data();
    void isValidSignature();
};

enum { Invalid = false, Valid = true };

static void addColumns()
{
    // All tests use these two columns only
    QTest::addColumn<QString>("data");
    QTest::addColumn<bool>("result");
    QTest::addColumn<bool>("isValid");
}

// ---- type adds ---
static void addFixedTypes()
{
    QTest::newRow("bool") << DBUS_TYPE_BOOLEAN_AS_STRING << true << true;
    QTest::newRow("byte") << DBUS_TYPE_BYTE_AS_STRING << true << true;
    QTest::newRow("int16") << DBUS_TYPE_INT16_AS_STRING << true << true;
    QTest::newRow("uint16") << DBUS_TYPE_UINT16_AS_STRING << true << true;
    QTest::newRow("int32") << DBUS_TYPE_INT32_AS_STRING << true << true;
    QTest::newRow("uint32") << DBUS_TYPE_UINT32_AS_STRING << true << true;
    QTest::newRow("int64") << DBUS_TYPE_INT64_AS_STRING << true << true;
    QTest::newRow("uint64") << DBUS_TYPE_UINT64_AS_STRING << true << true;
    QTest::newRow("double") << DBUS_TYPE_DOUBLE_AS_STRING << true << true;
#ifdef DBUS_TYPE_UNIX_FD_AS_STRING
    QTest::newRow("unixfd") << DBUS_TYPE_UNIX_FD_AS_STRING << true << true;
#endif
}

static void addInvalidSingleLetterTypes()
{
    QChar nulString[] = { 0 };
    QTest::newRow("nul") << QString(nulString, 1) << false << false;
    QTest::newRow("tilde") << "~" << false << false;
    QTest::newRow("struct-begin") << "(" << false << false;
    QTest::newRow("struct-end") << ")" << false << false;
    QTest::newRow("dict-entry-begin") << "{" << false << false;
    QTest::newRow("dict-entry-end") << "}" << false << false;
    QTest::newRow("array-no-element") << "a" << false << false;
}

static void addBasicTypes(bool basicsAreValid)
{
    addFixedTypes();
    QTest::newRow("string") << DBUS_TYPE_STRING_AS_STRING << basicsAreValid << true;
    QTest::newRow("object-path") << DBUS_TYPE_OBJECT_PATH_AS_STRING << basicsAreValid << true;
    QTest::newRow("signature") << DBUS_TYPE_SIGNATURE_AS_STRING << basicsAreValid << true;
}

static void addVariant(bool variantIsValid)
{
    QTest::newRow("variant") << "v" << variantIsValid << true;
}

static void addSingleSignatures()
{
    addBasicTypes(Valid);
    addVariant(Valid);
    QTest::newRow("struct-1") << "(y)" << true;
    QTest::newRow("struct-2") << "(yy)" << true;
    QTest::newRow("struct-3") << "(yyv)" << true;

    QTest::newRow("struct-nested-1") << "((y))" << true;
    QTest::newRow("struct-nested-2") << "((yy))" << true;
    QTest::newRow("struct-nested-3") << "(y(y))" << true;
    QTest::newRow("struct-nested-4") << "((y)y)" << true;
    QTest::newRow("struct-nested-5") << "(y(y)y)" << true;
    QTest::newRow("struct-nested-6") << "((y)(y))" << true;

    QTest::newRow("array-1") << "as" << true;
    QTest::newRow("struct-array-1") << "(as)" << true;
    QTest::newRow("struct-array-2") << "(yas)" << true;
    QTest::newRow("struct-array-3") << "(asy)" << true;
    QTest::newRow("struct-array-4") << "(yasy)" << true;

    QTest::newRow("dict-1") << "a{sy}" << true;
    QTest::newRow("dict-2") << "a{sv}" << true;
    QTest::newRow("dict-struct-1") << "a{s(y)}" << true;
    QTest::newRow("dict-struct-2") << "a{s(yyyy)}" << true;
    QTest::newRow("dict-struct-array") << "a{s(ay)}" << true;
    QTest::newRow("dict-array") << "a{sas}" << true;
    QTest::newRow("dict-array-struct") << "a{sa(y)}" << true;

    addInvalidSingleLetterTypes();
    QTest::newRow("naked-dict-empty") << "{}" << false;
    QTest::newRow("naked-dict-missing-value") << "{i}" << false;

    QTest::newRow("dict-empty") << "a{}" << false;
    QTest::newRow("dict-missing-value") << "a{i}" << false;
    QTest::newRow("dict-non-basic-key") << "a{vi}" << false;
    QTest::newRow("dict-struct-key") << "a{(y)y}" << false;
    QTest::newRow("dict-missing-close") << "a{sv" << false;
    QTest::newRow("dict-mismatched-close") << "a{sv)" << false;
    QTest::newRow("dict-missing-value-close") << "a{s" << false;

    QTest::newRow("empty-struct") << "()" << false;
    QTest::newRow("struct-missing-close") << "(s" << false;
    QTest::newRow("struct-nested-missing-close-1") << "((s)" << false;
    QTest::newRow("struct-nested-missing-close-2") << "((s" << false;

    QTest::newRow("struct-ending-array-no-element") << "(a)" << false;
}

static void addNakedDictEntry()
{
    QTest::newRow("naked-dict-entry") << "{sv}" << false;
}

// ---- tests ----

void tst_QDBusType::isValidFixedType_data()
{
    addColumns();
    addFixedTypes();
    addBasicTypes(Invalid);
    addVariant(Invalid);
    addInvalidSingleLetterTypes();
}

void tst_QDBusType::isValidFixedType()
{
    QFETCH(QString, data);
    QFETCH(bool, result);
    QFETCH(bool, isValid);
    QVERIFY2(data.length() == 1, "Test is malformed, this function must test only one-letter types");
    QVERIFY(isValid || (!isValid && !result));

    int type = data.at(0).unicode();
    if (isValid)
        QCOMPARE(bool(dbus_type_is_fixed(type)), result);
    QCOMPARE(QDBusUtil::isValidFixedType(type), result);
}

void tst_QDBusType::isValidBasicType_data()
{
    addColumns();
    addBasicTypes(Valid);
    addVariant(Invalid);
    addInvalidSingleLetterTypes();
}

void tst_QDBusType::isValidBasicType()
{
    QFETCH(QString, data);
    QFETCH(bool, result);
    QFETCH(bool, isValid);
    QVERIFY2(data.length() == 1, "Test is malformed, this function must test only one-letter types");
    QVERIFY(isValid || (!isValid && !result));

    int type = data.at(0).unicode();
    if (isValid)
        QCOMPARE(bool(dbus_type_is_basic(type)), result);
    QCOMPARE(QDBusUtil::isValidBasicType(type), result);
}

void tst_QDBusType::isValidSingleSignature_data()
{
    addColumns();
    addSingleSignatures();
    addNakedDictEntry();
}

void tst_QDBusType::isValidSingleSignature()
{
    QFETCH(QString, data);
    QFETCH(bool, result);

    QCOMPARE(bool(dbus_signature_validate_single(data.toLatin1(), 0)), result);
    QCOMPARE(QDBusUtil::isValidSingleSignature(data), result);
}

void tst_QDBusType::isValidArray_data()
{
    addColumns();
    addSingleSignatures();
}

void tst_QDBusType::isValidArray()
{
    QFETCH(QString, data);
    QFETCH(bool, result);

    data.prepend("a");
    QCOMPARE(bool(dbus_signature_validate_single(data.toLatin1(), 0)), result);
    QCOMPARE(QDBusUtil::isValidSingleSignature(data), result);

    data.prepend("a");
    QCOMPARE(bool(dbus_signature_validate_single(data.toLatin1(), 0)), result);
    QCOMPARE(QDBusUtil::isValidSingleSignature(data), result);
}

void tst_QDBusType::isValidSignature_data()
{
    isValidSingleSignature_data();
}

void tst_QDBusType::isValidSignature()
{
    QFETCH(QString, data);
    QFETCH(bool, result);

    data.append(data);
    if (data.at(0).unicode())
        QCOMPARE(bool(dbus_signature_validate(data.toLatin1(), 0)), result);
    QCOMPARE(QDBusUtil::isValidSignature(data), result);
}

QTEST_MAIN(tst_QDBusType)

#include "tst_qdbustype.moc"
