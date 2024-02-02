// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/QCoreApplication>

#include <QtDBus/private/qdbusutil_p.h>
#include <QtDBus/private/qdbus_symbols_p.h>

DEFINEFUNC(dbus_bool_t, dbus_signature_validate, (const char       *signature,
                                                DBusError        *error),
           (signature, error), return)

class tst_QDBusType: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void benchmarkSignature_data();
    void benchmarkSignature();
};

static inline void benchmarkAddRow(const char *name, const char *data)
{
    if (qdbus_loadLibDBus())
        QTest::newRow(QByteArray(QByteArray("native-") + name)) << data << true;
    QTest::newRow(name) << data << false;
}

void tst_QDBusType::benchmarkSignature_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<bool>("useNative");

    benchmarkAddRow("single-invalid", "~");
    benchmarkAddRow("single-invalid-array", "a~");
    benchmarkAddRow("single-invalid-struct", "(.)");

    benchmarkAddRow("single-char", "b");
    benchmarkAddRow("single-array", "as");
    benchmarkAddRow("single-simplestruct", "(y)");
    benchmarkAddRow("single-simpledict", "a{sv}");
    benchmarkAddRow("single-complexdict", "a{s(aya{io})}");

    benchmarkAddRow("multiple-char", "ssg");
    benchmarkAddRow("multiple-arrays", "asasay");

    benchmarkAddRow("struct-missingclose", "(ayyyy");
    benchmarkAddRow("longstruct", "(yyyyyyayasy)");
    benchmarkAddRow("invalid-longstruct", "(yyyyyyayas.y)");
    benchmarkAddRow("complexstruct", "(y(aasay)oga{sv})");
    benchmarkAddRow("multiple-simple-structs", "(y)(y)(y)");
}

void tst_QDBusType::benchmarkSignature()
{
    QFETCH(QString, data);
    QFETCH(bool, useNative);

    bool result;
    if (useNative) {
        q_dbus_signature_validate(data.toLatin1(), 0);
        QBENCHMARK {
            result = q_dbus_signature_validate(data.toLatin1(), 0);
        }
    } else {
        QDBusUtil::isValidSignature(data);
        QBENCHMARK {
            result = QDBusUtil::isValidSignature(data);
        }
    }
    Q_UNUSED(result);
}

QTEST_MAIN(tst_QDBusType)

#include "main.moc"
