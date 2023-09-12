// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qglobal.h>
#include <QtCore/qtversion.h>
#include <QtCore/qtypes.h>

#ifdef Q_COMPILER_THREAD_LOCAL
#  include <threads.h>
#endif

/*
 * Certain features of qglobal.h must work in C mode too. We test that
 * everything works.
 */

/* Types and Q_UNUSED */
void tst_GlobalTypes()
{
    qint8 s8;
    qint16 s16;
    qint32 s32;
    qint64 s64;
    qlonglong sll;
    Q_UNUSED(s8); Q_UNUSED(s16); Q_UNUSED(s32); Q_UNUSED(s64); Q_UNUSED(sll);

    quint8 u8;
    quint16 u16;
    quint32 u32;
    quint64 u64;
    qulonglong ull;
    Q_UNUSED(u8); Q_UNUSED(u16); Q_UNUSED(u32); Q_UNUSED(u64); Q_UNUSED(ull);

    uchar uc;
    ushort us;
    uint ui;
    ulong ul;
    Q_UNUSED(uc); Q_UNUSED(us); Q_UNUSED(ui); Q_UNUSED(ul);

    qreal qr;
    Q_UNUSED(qr);

    qsizetype qs;
    qptrdiff qp;
    qintptr qip;
    quintptr qup;
    Q_UNUSED(qs); Q_UNUSED(qp); Q_UNUSED(qip); Q_UNUSED(qup);

#ifdef QT_SUPPORTS_INT128
    qint128 s128;
    quint128 u128;
    Q_UNUSED(s128); Q_UNUSED(u128);
#endif /* QT_SUPPORTS_INT128 */
}

#if QT_SUPPORTS_INT128
qint128 tst_qint128_min() { return Q_INT128_MIN + 0; }
qint128 tst_qint128_max() { return 0 + Q_INT128_MAX; }
quint128 tst_quint128_max() { return Q_UINT128_MAX - 1 + 1; }
#endif

/* Qt version */
int tst_QtVersion()
{
    return QT_VERSION;
}

const char *tst_qVersion() Q_DECL_NOEXCEPT;
const char *tst_qVersion()
{
#if !defined(QT_NAMESPACE)
    return qVersion();
#else
    return NULL;
#endif
}

/* Static assertion */
Q_STATIC_ASSERT(true);
Q_STATIC_ASSERT(1);
Q_STATIC_ASSERT_X(true, "Message");
Q_STATIC_ASSERT_X(1, "Message");

Q_STATIC_ASSERT(!false);
Q_STATIC_ASSERT(!0);

Q_STATIC_ASSERT(!!true);
Q_STATIC_ASSERT(!!1);

#ifdef __COUNTER__
// if the compiler supports __COUNTER__, multiple
// Q_STATIC_ASSERT's on a single line should compile:
Q_STATIC_ASSERT(true); Q_STATIC_ASSERT_X(!false, "");
#endif // __COUNTER__

#ifdef Q_COMPILER_THREAD_LOCAL
static thread_local int gt_var;
void thread_local_test()
{
    static thread_local int t_var;
    t_var = gt_var;
    Q_UNUSED(t_var);
}
#endif

