// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qglobal.h>

// SCENARIO 6
// We disable QStringBuilder and compile with normal operator+ to verify
// that all QSB supported operations are still available when QSB is disabled.
// We also allow casts to/from ASCII
#undef QT_USE_QSTRINGBUILDER
#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII

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

class tst_QStringBuilder6 : public QObject
{
    Q_OBJECT

private slots:
    void scenario() { runScenario(); }
};

#include "tst_qstringbuilder6.moc"

QTEST_APPLESS_MAIN(tst_QStringBuilder6)
