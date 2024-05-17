// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#ifndef QOPENSSL_SYMBOLS_H
#define QOPENSSL_SYMBOLS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// This file is what was known as qsslsocket_openssl_symbols_p.h,
// reduced to the needs of our auto-tests, that have to mess with
// OpenSSL calls directly.

#include <QtCore/qset.h>
#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_REQUIRE_CONFIG(openssl);

#ifdef Q_OS_WIN
#include <QtCore/private/qsystemlibrary_p.h>
#elif QT_CONFIG(library)
#include <QtCore/qlibrary.h>
#endif // Q_OS_WIN

#include <QtCore/qloggingcategory.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>

#if defined(Q_OS_UNIX)
#include <QtCore/qdir.h>
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <link.h>
#endif

#ifdef Q_OS_DARWIN
#include <QtCore/private/qcore_mac_p.h>
#endif

#include <algorithm>
#include <memory>

#ifdef Q_OS_WIN
#include <qt_windows.h>
// wincrypt has those as macros, which may conflict with
// typedef names in OpenSSL.
#if defined(OCSP_RESPONSE)
#undef OCSP_RESPONSE
#endif
#if defined(X509_NAME)
#undef X509_NAME
#endif
#endif // Q_OS_WIN

#include <openssl/stack.h>
#include <openssl/x509.h>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/dh.h>
#if QT_CONFIG(ocsp)
#include <openssl/ocsp.h>
#endif // ocsp

QT_BEGIN_NAMESPACE

namespace {

// BIO-related functions, that auto-tests use:
BIO *q_BIO_new(const BIO_METHOD *a);
int q_BIO_free(BIO *a);
int q_BIO_write(BIO *a, const void *b, int c);
const BIO_METHOD *q_BIO_s_mem();

// EVP_PKEY-related functions, that auto-tests use:
EVP_PKEY *q_EVP_PKEY_new();
void q_EVP_PKEY_free(EVP_PKEY *a);
int q_EVP_PKEY_up_ref(EVP_PKEY *a);
EVP_PKEY *q_PEM_read_bio_PrivateKey(BIO *a, EVP_PKEY **b, pem_password_cb *c, void *d);
EVP_PKEY *q_PEM_read_bio_PUBKEY(BIO *a, EVP_PKEY **b, pem_password_cb *c, void *d);
const EVP_MD *q_EVP_sha1();
int q_EVP_PKEY_set1_RSA(EVP_PKEY *a, RSA *b);
int q_EVP_PKEY_set1_DSA(EVP_PKEY *a, DSA *b);
int q_EVP_PKEY_set1_DH(EVP_PKEY *a, DH *b);
#ifndef OPENSSL_NO_EC
int q_EVP_PKEY_set1_EC_KEY(EVP_PKEY *a, EC_KEY *b);
#endif // !OPENSSL_NO_EC
int q_EVP_PKEY_cmp(const EVP_PKEY *a, const EVP_PKEY *b);

// Stack-management functions, that auto-tests use:
int q_OPENSSL_sk_num(OPENSSL_STACK *a);
void q_OPENSSL_sk_pop_free(OPENSSL_STACK *a, void (*b)(void *));
OPENSSL_STACK *q_OPENSSL_sk_new_null();
void q_OPENSSL_sk_push(OPENSSL_STACK *st, void *data);
void q_OPENSSL_sk_free(OPENSSL_STACK *a);
void *q_OPENSSL_sk_value(OPENSSL_STACK *a, int b);

// X509-related functions:
void q_X509_up_ref(X509 *a);
void q_X509_free(X509 *a);

// ASN1_TIME-related functions:
ASN1_TIME *q_X509_gmtime_adj(ASN1_TIME *s, long adj);
void q_ASN1_TIME_free(ASN1_TIME *t);

#if QT_CONFIG(ocsp)
// OCSP auto-test:
int q_i2d_OCSP_RESPONSE(OCSP_RESPONSE *r, unsigned char **ppout);
OCSP_RESPONSE *q_OCSP_response_create(int status, OCSP_BASICRESP *bs);
void q_OCSP_RESPONSE_free(OCSP_RESPONSE *rs);
OCSP_SINGLERESP *q_OCSP_basic_add1_status(OCSP_BASICRESP *rsp, OCSP_CERTID *cid,
                                          int status, int reason, ASN1_TIME *revtime,
                                          ASN1_TIME *thisupd, ASN1_TIME *nextupd);
int q_OCSP_basic_sign(OCSP_BASICRESP *brsp, X509 *signer, EVP_PKEY *key, const EVP_MD *dgst,
                      STACK_OF(X509) *certs, unsigned long flags);
OCSP_BASICRESP *q_OCSP_BASICRESP_new();
void q_OCSP_BASICRESP_free(OCSP_BASICRESP *bs);
OCSP_CERTID *q_OCSP_cert_to_id(const EVP_MD *dgst, X509 *subject, X509 *issuer);
void q_OCSP_CERTID_free(OCSP_CERTID *cid);

#endif // QT_CONFIG(ocsp)

#ifndef QT_LINKED_OPENSSL

Q_LOGGING_CATEGORY(lcOsslSymbols, "qt.openssl.symbols");

void qsslSocketUnresolvedSymbolWarning(const char *functionName)
{
    qCWarning(lcOsslSymbols, "QSslSocket: cannot call unresolved function %s", functionName);
}

#if QT_CONFIG(library)
void qsslSocketCannotResolveSymbolWarning(const char *functionName)
{
    qCWarning(lcOsslSymbols, "QSslSocket: cannot resolve %s", functionName);
}
#endif // QT_CONFIG(library)

#endif // QT_LINKED_OPENSS

#define DUMMYARG

#if defined(QT_LINKED_OPENSSL)
// **************** Static declarations ******************

// ret func(arg)
#  define DEFINEFUNC(ret, func, arg, a, err, funcret) \
    [[maybe_unused]] ret q_##func(arg) { funcret func(a); }

// ret func(arg1, arg2)
#  define DEFINEFUNC2(ret, func, arg1, a, arg2, b, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2) { funcret func(a, b); }

// ret func(arg1, arg2, arg3)
#  define DEFINEFUNC3(ret, func, arg1, a, arg2, b, arg3, c, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3) { funcret func(a, b, c); }

// ret func(arg1, arg2, arg3, arg4)
#  define DEFINEFUNC4(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4) { funcret func(a, b, c, d); }

// ret func(arg1, arg2, arg3, arg4, arg5)
#  define DEFINEFUNC5(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5) { funcret func(a, b, c, d, e); }

// ret func(arg1, arg2, arg3, arg4, arg6)
#  define DEFINEFUNC6(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6) { funcret func(a, b, c, d, e, f); }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7)
#  define DEFINEFUNC7(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7) { funcret func(a, b, c, d, e, f, g); }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7, arg8, arg9)
#  define DEFINEFUNC9(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, arg8, h, arg9, i, err, funcret) \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { funcret func(a, b, c, d, e, f, g, h, i); }

// **************** Static declarations ******************

#else

// **************** Shared declarations ******************
// ret func(arg)

#  define DEFINEFUNC(ret, func, arg, a, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg); \
    static _q_PTR_##func _q_##func = 0; \
    [[maybe_unused]] ret q_##func(arg) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func); \
            err; \
        } \
        funcret _q_##func(a); \
    }

// ret func(arg1, arg2)
#  define DEFINEFUNC2(ret, func, arg1, a, arg2, b, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2);         \
    static _q_PTR_##func _q_##func = 0;               \
    [[maybe_unused]] ret q_##func(arg1, arg2) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func);\
            err; \
        } \
        funcret _q_##func(a, b); \
    }

// ret func(arg1, arg2, arg3)
#  define DEFINEFUNC3(ret, func, arg1, a, arg2, b, arg3, c, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3);            \
    static _q_PTR_##func _q_##func = 0;                        \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func); \
            err; \
        } \
        funcret _q_##func(a, b, c); \
    }

// ret func(arg1, arg2, arg3, arg4)
#  define DEFINEFUNC4(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4);               \
    static _q_PTR_##func _q_##func = 0;                                 \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4) { \
         if (Q_UNLIKELY(!_q_##func)) { \
             qsslSocketUnresolvedSymbolWarning(#func); \
             err; \
         } \
         funcret _q_##func(a, b, c, d); \
    }

// ret func(arg1, arg2, arg3, arg4, arg5)
#  define DEFINEFUNC5(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5);         \
    static _q_PTR_##func _q_##func = 0;                                 \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func); \
            err; \
        } \
        funcret _q_##func(a, b, c, d, e); \
    }

// ret func(arg1, arg2, arg3, arg4, arg6)
#  define DEFINEFUNC6(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6);   \
    static _q_PTR_##func _q_##func = 0;                                 \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func); \
            err; \
        } \
        funcret _q_##func(a, b, c, d, e, f); \
    }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7)
#  define DEFINEFUNC7(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);   \
    static _q_PTR_##func _q_##func = 0;                                       \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func); \
            err; \
        } \
        funcret _q_##func(a, b, c, d, e, f, g); \
    }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7, arg8, arg9)
#  define DEFINEFUNC9(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, arg8, h, arg9, i, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);   \
    static _q_PTR_##func _q_##func = 0;                                                   \
    [[maybe_unused]] ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { \
        if (Q_UNLIKELY(!_q_##func)) { \
            qsslSocketUnresolvedSymbolWarning(#func); \
            err; \
        }   \
        funcret _q_##func(a, b, c, d, e, f, g, h, i); \
    }
// **************** Shared declarations ******************

#endif // QT_LINKED_OPENSSL

// BIO:
DEFINEFUNC(BIO *, BIO_new, const BIO_METHOD *a, a, return nullptr, return)
DEFINEFUNC(int, BIO_free, BIO *a, a, return 0, return)
DEFINEFUNC3(int, BIO_write, BIO *a, a, const void *b, b, int c, c, return -1, return)
DEFINEFUNC(const BIO_METHOD *, BIO_s_mem, void, DUMMYARG, return nullptr, return)

// EVP:
DEFINEFUNC(EVP_PKEY *, EVP_PKEY_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, EVP_PKEY_free, EVP_PKEY *a, a, return, DUMMYARG)
DEFINEFUNC(int, EVP_PKEY_up_ref, EVP_PKEY *a, a, return 0, return)
DEFINEFUNC4(EVP_PKEY *, PEM_read_bio_PrivateKey, BIO *a, a, EVP_PKEY **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC4(EVP_PKEY *, PEM_read_bio_PUBKEY, BIO *a, a, EVP_PKEY **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC(const EVP_MD *, EVP_sha1, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC2(int, EVP_PKEY_set1_RSA, EVP_PKEY *a, a, RSA *b, b, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_DSA, EVP_PKEY *a, a, DSA *b, b, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_DH, EVP_PKEY *a, a, DH *b, b, return -1, return)
#ifndef OPENSSL_NO_EC
DEFINEFUNC2(int, EVP_PKEY_set1_EC_KEY, EVP_PKEY *a, a, EC_KEY *b, b, return -1, return)
#endif
DEFINEFUNC2(int, EVP_PKEY_cmp, const EVP_PKEY *a, a, const EVP_PKEY *b, b, return -1, return)

// Stack:
DEFINEFUNC(int, OPENSSL_sk_num, OPENSSL_STACK *a, a, return -1, return)
DEFINEFUNC2(void, OPENSSL_sk_pop_free, OPENSSL_STACK *a, a, void (*b)(void*), b, return, DUMMYARG)
DEFINEFUNC(OPENSSL_STACK *, OPENSSL_sk_new_null, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC2(void, OPENSSL_sk_push, OPENSSL_STACK *a, a, void *b, b, return, DUMMYARG)
DEFINEFUNC(void, OPENSSL_sk_free, OPENSSL_STACK *a, a, return, DUMMYARG)
DEFINEFUNC2(void *, OPENSSL_sk_value, OPENSSL_STACK *a, a, int b, b, return nullptr, return)

// X509:
DEFINEFUNC(void, X509_up_ref, X509 *a, a, return, DUMMYARG)
DEFINEFUNC(void, X509_free, X509 *a, a, return, DUMMYARG)

// ASN1_TIME:
DEFINEFUNC2(ASN1_TIME *, X509_gmtime_adj, ASN1_TIME *s, s, long adj, adj, return nullptr, return)
DEFINEFUNC(void, ASN1_TIME_free, ASN1_TIME *t, t, return, DUMMYARG)

#if QT_CONFIG(ocsp)

DEFINEFUNC2(int, i2d_OCSP_RESPONSE, OCSP_RESPONSE *r, r, unsigned char **ppout, ppout, return 0, return)
DEFINEFUNC2(OCSP_RESPONSE *, OCSP_response_create, int status, status, OCSP_BASICRESP *bs, bs, return nullptr, return)
DEFINEFUNC(void, OCSP_RESPONSE_free, OCSP_RESPONSE *rs, rs, return, DUMMYARG)
DEFINEFUNC7(OCSP_SINGLERESP *, OCSP_basic_add1_status, OCSP_BASICRESP *r, r, OCSP_CERTID *c, c, int s, s,
            int re, re, ASN1_TIME *rt, rt, ASN1_TIME *t, t, ASN1_TIME *n, n, return nullptr, return)
DEFINEFUNC6(int, OCSP_basic_sign, OCSP_BASICRESP *br, br, X509 *signer, signer, EVP_PKEY *key, key,
            const EVP_MD *dg, dg, STACK_OF(X509) *cs, cs, unsigned long flags, flags, return 0, return)
DEFINEFUNC(OCSP_BASICRESP *, OCSP_BASICRESP_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, OCSP_BASICRESP_free, OCSP_BASICRESP *bs, bs, return, DUMMYARG)
DEFINEFUNC3(OCSP_CERTID *, OCSP_cert_to_id, const EVP_MD *dgst, dgst, X509 *subject, subject, X509 *issuer, issuer, return nullptr, return)
DEFINEFUNC(void, OCSP_CERTID_free, OCSP_CERTID *cid, cid, return, DUMMYARG)

#endif // QT_CONFIG(ocsp)

#ifndef QT_LINKED_OPENSSL

#if !QT_CONFIG(library)
bool qt_auto_test_resolve_OpenSSL_symbols()
{
    qCWarning(lcOsslSymbols, "QSslSocket: unable to resolve symbols. Qt is configured without the "
                             "'library' feature, which means runtime resolving of libraries won't work.");
    qCWarning(lcOsslSymbols, "Either compile Qt statically or with support for runtime resolving "
                             "of libraries.");
    return false;
}

#else

#ifdef Q_OS_UNIX

struct NumericallyLess
{
    bool operator()(QStringView lhs, QStringView rhs) const
    {
        bool ok = false;
        int b = 0;
        int a = lhs.toInt(&ok);
        if (ok)
            b = rhs.toInt(&ok);
        if (ok) {
            // both toInt succeeded
            return a < b;
        } else {
            // compare as strings;
            return lhs < rhs;
        }
    }
};

struct LibGreaterThan
{
    bool operator()(QStringView lhs, QStringView rhs) const
    {
        const auto lhsparts = lhs.split(QLatin1Char('.'));
        const auto rhsparts = rhs.split(QLatin1Char('.'));
        Q_ASSERT(lhsparts.size() > 1 && rhsparts.size() > 1);

        // note: checking rhs < lhs, the same as lhs > rhs
        return std::lexicographical_compare(rhsparts.begin() + 1, rhsparts.end(),
                                            lhsparts.begin() + 1, lhsparts.end(),
                                            NumericallyLess());
    }
};

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

int dlIterateCallback(struct dl_phdr_info *info, size_t size, void *data)
{
    if (size < sizeof (info->dlpi_addr) + sizeof (info->dlpi_name))
        return 1;
    QSet<QString> *paths = (QSet<QString> *)data;
    QString path = QString::fromLocal8Bit(info->dlpi_name);
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        path = fi.absolutePath();
        if (!path.isEmpty())
            paths->insert(path);
    }
    return 0;
}

#endif // defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

QStringList libraryPathList()
{
    QStringList paths;

#ifdef Q_OS_DARWIN
    paths = QString::fromLatin1(qgetenv("DYLD_LIBRARY_PATH"))
            .split(QLatin1Char(':'), Qt::SkipEmptyParts);

    // search in .app/Contents/Frameworks
    UInt32 packageType;
    CFBundleGetPackageInfo(CFBundleGetMainBundle(), &packageType, nullptr);
    if (packageType == FOUR_CHAR_CODE('APPL')) {
        QUrl bundleUrl = QUrl::fromCFURL(QCFType<CFURLRef>(CFBundleCopyBundleURL(CFBundleGetMainBundle())));
        QUrl frameworksUrl = QUrl::fromCFURL(QCFType<CFURLRef>(CFBundleCopyPrivateFrameworksURL(CFBundleGetMainBundle())));
        paths << bundleUrl.resolved(frameworksUrl).path();
    }
#else // Q_OS_DARWIN
    paths = QString::fromLatin1(qgetenv("LD_LIBRARY_PATH")).split(QLatin1Char(':'), Qt::SkipEmptyParts);
#endif // Q_OS_DARWIN

    paths << QLatin1String("/lib") << QLatin1String("/usr/lib") << QLatin1String("/usr/local/lib");
    paths << QLatin1String("/lib64") << QLatin1String("/usr/lib64") << QLatin1String("/usr/local/lib64");
    paths << QLatin1String("/lib32") << QLatin1String("/usr/lib32") << QLatin1String("/usr/local/lib32");

#if defined(Q_OS_ANDROID)
    paths << QLatin1String("/system/lib");
#elif defined(Q_OS_LINUX)
    // discover paths of already loaded libraries
    QSet<QString> loadedPaths;
    dl_iterate_phdr(dlIterateCallback, &loadedPaths);
    paths.append(loadedPaths.values());
#endif // Q_OS_ANDROID

    return paths;
}

Q_NEVER_INLINE
QStringList findAllLibs(QLatin1String filter)
{
    const QStringList paths = libraryPathList();
    QStringList found;
    const QStringList filters((QString(filter)));

    for (const QString &path : paths) {
        QDir dir(path);
        QStringList entryList = dir.entryList(filters, QDir::Files);

        std::sort(entryList.begin(), entryList.end(), LibGreaterThan());
        for (const QString &entry : std::as_const(entryList))
            found << path + QLatin1Char('/') + entry;
    }

    return found;
}

QStringList findAllLibSsl()
{
    return findAllLibs(QLatin1String("libssl.*"));
}

QStringList findAllLibCrypto()
{
    return findAllLibs(QLatin1String("libcrypto.*"));
}

#endif // Q_OS_UNIX

#ifdef Q_OS_WIN

#if (OPENSSL_VERSION_NUMBER >> 28) < 3
#define QT_OPENSSL_VERSION "1_1"
#elif OPENSSL_VERSION_MAJOR == 3 // Starting with 3.0 this define is available
#define QT_OPENSSL_VERSION "3"
#endif // > 3 intentionally left undefined

struct LoadedOpenSsl {
    std::unique_ptr<QSystemLibrary> ssl, crypto;
};

bool tryToLoadOpenSslWin32Library(QLatin1String ssleay32LibName, QLatin1String libeay32LibName, LoadedOpenSsl &result)
{
    auto ssleay32 = std::make_unique<QSystemLibrary>(ssleay32LibName);
    if (!ssleay32->load(false)) {
        return FALSE;
    }

    auto libeay32 = std::make_unique<QSystemLibrary>(libeay32LibName);
    if (!libeay32->load(false)) {
        return FALSE;
    }

    result.ssl = std::move(ssleay32);
    result.crypto = std::move(libeay32);
    return TRUE;
}

static LoadedOpenSsl loadOpenSsl()
{
    LoadedOpenSsl result;

    // With OpenSSL 1.1 the names have changed to libssl-1_1 and libcrypto-1_1 for builds using
    // MSVC and GCC, with architecture suffixes for non-x86 builds.

#if defined(Q_PROCESSOR_X86_64)
#define QT_SSL_SUFFIX "-x64"
#elif defined(Q_PROCESSOR_ARM_64)
#define QT_SSL_SUFFIX "-arm64"
#elif defined(Q_PROCESSOR_ARM_32)
#define QT_SSL_SUFFIX "-arm"
#else
#define QT_SSL_SUFFIX
#endif

    tryToLoadOpenSslWin32Library(QLatin1String("libssl-" QT_OPENSSL_VERSION QT_SSL_SUFFIX),
                                 QLatin1String("libcrypto-" QT_OPENSSL_VERSION QT_SSL_SUFFIX),
                                 result);

#undef QT_SSL_SUFFIX
    return result;
}

#else // Q_OS_WIN

struct LoadedOpenSsl {
    std::unique_ptr<QLibrary> ssl, crypto;
};

LoadedOpenSsl loadOpenSsl()
{
    LoadedOpenSsl result = { std::make_unique<QLibrary>(), std::make_unique<QLibrary>() };

# if defined(Q_OS_UNIX)
    QLibrary * const libssl = result.ssl.get();
    QLibrary * const libcrypto = result.crypto.get();

    // Try to find the libssl library on the system.
    //
    // Up until Qt 4.3, this only searched for the "ssl" library at version -1, that
    // is, libssl.so on most Unix systems.  However, the .so file isn't present in
    // user installations because it's considered a development file.
    //
    // The right thing to do is to load the library at the major version we know how
    // to work with: the SHLIB_VERSION_NUMBER version (macro defined in opensslv.h)
    //
    // However, OpenSSL is a well-known case of binary-compatibility breakage. To
    // avoid such problems, many system integrators and Linux distributions change
    // the soname of the binary, letting the full version number be the soname. So
    // we'll find libssl.so.0.9.7, libssl.so.0.9.8, etc. in the system. For that
    // reason, we will search a few common paths (see findAllLibSsl() above) in hopes
    // we find one that works.
    //
    // If that fails, for OpenSSL 1.0 we also try some fallbacks -- look up
    // libssl.so with a hardcoded soname. The reason is QTBUG-68156: the binary
    // builds of Qt happen (at the time of this writing) on RHEL machines,
    // which change SHLIB_VERSION_NUMBER to a non-portable string. When running
    // those binaries on the target systems, this code won't pick up
    // libssl.so.MODIFIED_SHLIB_VERSION_NUMBER because it doesn't exist there.
    // Given that the only 1.0 supported release (at the time of this writing)
    // is 1.0.2, with soname "1.0.0", give that a try too. Note that we mandate
    // OpenSSL >= 1.0.0 with a configure-time check, and OpenSSL has kept binary
    // compatibility between 1.0.0 and 1.0.2.
    //
    // It is important, however, to try the canonical name and the unversioned name
    // without going through the loop. By not specifying a path, we let the system
    // dlopen(3) function determine it for us. This will include any DT_RUNPATH or
    // DT_RPATH tags on our library header as well as other system-specific search
    // paths. See the man page for dlopen(3) on your system for more information.

#ifdef Q_OS_OPENBSD
    libcrypto->setLoadHints(QLibrary::ExportExternalSymbolsHint);
#endif

#if defined(SHLIB_VERSION_NUMBER) && !defined(Q_OS_QNX) // on QNX, the libs are always libssl.so and libcrypto.so
    // first attempt: the canonical name is libssl.so.<SHLIB_VERSION_NUMBER>
    libssl->setFileNameAndVersion(QLatin1String("ssl"), QLatin1String(SHLIB_VERSION_NUMBER));
    libcrypto->setFileNameAndVersion(QLatin1String("crypto"), QLatin1String(SHLIB_VERSION_NUMBER));
    if (libcrypto->load() && libssl->load()) {
        // libssl.so.<SHLIB_VERSION_NUMBER> and libcrypto.so.<SHLIB_VERSION_NUMBER> found
        return result;
    } else {
        libssl->unload();
        libcrypto->unload();
    }
#endif // defined(SHLIB_VERSION_NUMBER) && !defined(Q_OS_QNX)

#ifndef Q_OS_DARWIN
    // second attempt: find the development files libssl.so and libcrypto.so
    //
    // disabled on macOS/iOS:
    //  macOS's /usr/lib/libssl.dylib, /usr/lib/libcrypto.dylib will be picked up in the third
    //    attempt, _after_ <bundle>/Contents/Frameworks has been searched.
    //  iOS does not ship a system libssl.dylib, libcrypto.dylib in the first place.
#if defined(Q_OS_ANDROID)
    // OpenSSL 1.1.x must be suffixed otherwise it will use the system libcrypto.so libssl.so which on API-21 are OpenSSL 1.0 not 1.1
    auto openSSLSuffix = [](const QByteArray &defaultSuffix = {}) {
        auto suffix = qgetenv("ANDROID_OPENSSL_SUFFIX");
        if (suffix.isEmpty())
            return defaultSuffix;
        return suffix;
    };

    static QString suffix = QString::fromLatin1(openSSLSuffix("_1_1"));

    libssl->setFileNameAndVersion(QLatin1String("ssl") + suffix, -1);
    libcrypto->setFileNameAndVersion(QLatin1String("crypto") + suffix, -1);
#else // Q_OS_ANDROID
    libssl->setFileNameAndVersion(QLatin1String("ssl"), -1);
    libcrypto->setFileNameAndVersion(QLatin1String("crypto"), -1);
#endif // Q_OS_ANDROID

    if (libcrypto->load() && libssl->load()) {
        // libssl.so.0 and libcrypto.so.0 found
        return result;
    } else {
        libssl->unload();
        libcrypto->unload();
    }
#endif // !Q_OS_DARWIN

    // third attempt: loop on the most common library paths and find libssl
    const QStringList sslList = findAllLibSsl();
    const QStringList cryptoList = findAllLibCrypto();

    for (const QString &crypto : cryptoList) {
#ifdef Q_OS_DARWIN
        // Clients should not load the unversioned libcrypto dylib as it does not have a stable ABI
        if (crypto.endsWith("libcrypto.dylib"))
            continue;
#endif
        libcrypto->setFileNameAndVersion(crypto, -1);
        if (libcrypto->load()) {
            QFileInfo fi(crypto);
            QString version = fi.completeSuffix();

            for (const QString &ssl : sslList) {
                if (!ssl.endsWith(version))
                    continue;

                libssl->setFileNameAndVersion(ssl, -1);

                if (libssl->load()) {
                    // libssl.so.x and libcrypto.so.x found
                    return result;
                } else {
                    libssl->unload();
                }
            }
        }
        libcrypto->unload();
    }

    // failed to load anything
    result = {};
    return result;

# else
    // not implemented for this platform yet
    return result;
# endif // Q_OS_UNIX
}

#endif // Q_OS_WIN

bool qt_auto_test_resolve_OpenSSL_symbols()
{
#define RESOLVEFUNC(func) \
    if (!(_q_##func = _q_PTR_##func(libs.ssl->resolve(#func)))     \
        && !(_q_##func = _q_PTR_##func(libs.crypto->resolve(#func)))) {\
        qsslSocketCannotResolveSymbolWarning(#func); \
        return false; \
    }

    LoadedOpenSsl libs = loadOpenSsl();
    if (!libs.ssl || !libs.crypto) {
        qCWarning(lcOsslSymbols, "Failed to load libcrypto/libssl");
        return false;
    }

    // BIO:
    RESOLVEFUNC(BIO_new)
    RESOLVEFUNC(BIO_free)
    RESOLVEFUNC(BIO_write)
    RESOLVEFUNC(BIO_s_mem)

    // EVP:
    RESOLVEFUNC(EVP_PKEY_new)
    RESOLVEFUNC(EVP_PKEY_free)
    RESOLVEFUNC(EVP_PKEY_up_ref)
    RESOLVEFUNC(PEM_read_bio_PrivateKey)
    RESOLVEFUNC(PEM_read_bio_PUBKEY)
    RESOLVEFUNC(EVP_sha1)
    RESOLVEFUNC(EVP_PKEY_set1_RSA)
    RESOLVEFUNC(EVP_PKEY_set1_DSA)
    RESOLVEFUNC(EVP_PKEY_set1_DH)
#ifndef OPENSSL_NO_EC
    RESOLVEFUNC(EVP_PKEY_set1_EC_KEY)
#endif
    RESOLVEFUNC(EVP_PKEY_cmp)

    // Stack:
    RESOLVEFUNC(OPENSSL_sk_num)
    RESOLVEFUNC(OPENSSL_sk_pop_free)
    RESOLVEFUNC(OPENSSL_sk_new_null)
    RESOLVEFUNC(OPENSSL_sk_push)
    RESOLVEFUNC(OPENSSL_sk_free)
    RESOLVEFUNC(OPENSSL_sk_value)

    // X509:
    RESOLVEFUNC(X509_up_ref)
    RESOLVEFUNC(X509_free)

    // ASN1_TIME:
    RESOLVEFUNC(X509_gmtime_adj)
    RESOLVEFUNC(ASN1_TIME_free)

#if QT_CONFIG(ocsp)

    RESOLVEFUNC(i2d_OCSP_RESPONSE)
    RESOLVEFUNC(OCSP_response_create)
    RESOLVEFUNC(OCSP_RESPONSE_free)
    RESOLVEFUNC(OCSP_basic_add1_status)
    RESOLVEFUNC(OCSP_basic_sign)
    RESOLVEFUNC(OCSP_BASICRESP_new)
    RESOLVEFUNC(OCSP_BASICRESP_free)
    RESOLVEFUNC(OCSP_cert_to_id)
    RESOLVEFUNC(OCSP_CERTID_free)

#endif // QT_CONFIG(ocsp)

    return true;
}

#endif // QT_CONFIG(library)

#else // !defined QT_LINKED_OPENSSL

bool qt_auto_test_resolve_OpenSSL_symbols()
{
#ifdef QT_NO_OPENSSL
    return false;
#endif
    return true;
}

#endif // !defined QT_LINKED_OPENSSL

} // Unnamed namespace

QT_END_NAMESPACE

#endif // QOPENSSL_SYMBOLS_H

