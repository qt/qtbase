// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qglobal.h>

// SCENARIO 2
// this is the "full" version. Operator+ is replaced by a QStringBuilder
// based version
// with NO_CAST * defined
#define QT_USE_QSTRINGBUILDER
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>
#include <QtTest/QTest>

#include <QtCore/q20iterator.h>

#define LITERAL "some literal"

namespace {
#define P +
#include "../qstringbuilder1/stringbuilder.cpp"
#undef P
} // namespace

class tst_QStringBuilder2 : public QObject
{
    Q_OBJECT

private slots:
    void scenario() { runScenario(); }
};

#include "tst_qstringbuilder2.moc"

QTEST_APPLESS_MAIN(tst_QStringBuilder2)
