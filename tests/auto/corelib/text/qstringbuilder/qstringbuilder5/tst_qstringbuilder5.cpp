// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// SCENARIO 5
// We disable QStringBuilder and compile with normal operator+ to verify
// that all QSB supported operations are still available when QSB is disabled.
#undef QT_USE_QSTRINGBUILDER
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringbuilder.h>
#include <QtTest/qtest.h>

#include <q20iterator.h> // For q20::ssize

namespace {
#define P +
#include "../qstringbuilder1/stringbuilder.cpp"
#undef P
}

class tst_QStringBuilder5 : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scenario() { runScenario(); }
};

#include "tst_qstringbuilder5.moc"

QTEST_APPLESS_MAIN(tst_QStringBuilder5)
