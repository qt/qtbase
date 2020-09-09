/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Copyright (C) 2016 Richard J. Moore <rich@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include "qssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"

#ifdef Q_OS_WIN
# include <private/qsystemlibrary_p.h>
#elif QT_CONFIG(library)
# include <QtCore/qlibrary.h>
#endif
#include <QtCore/qmutex.h>
#include <QtCore/qdatetime.h>
#if defined(Q_OS_UNIX)
#include <QtCore/qdir.h>
#endif
#include <QtCore/private/qmemory_p.h>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <link.h>
#endif
#ifdef Q_OS_DARWIN
#include "private/qcore_mac_p.h"
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

/*
    Note to maintainer:
    -------------------

    We load OpenSSL symbols dynamically. Because symbols are known to
    disappear, and signatures sometimes change, between releases, we need to
    be careful about how this is done. To ensure we don't end up dereferencing
    null function pointers, and continue running even if certain functions are
    missing, we define helper functions for each of the symbols we load from
    OpenSSL, all prefixed with "q_" (declared in
    qsslsocket_openssl_symbols_p.h). So instead of calling SSL_connect
    directly, we call q_SSL_connect, which is a function that checks if the
    actual SSL_connect fptr is null, and returns a failure if it is, or calls
    SSL_connect if it isn't.

    This requires a somewhat tedious process of declaring each function we
    want to call in OpenSSL thrice: once with the q_, in _p.h, once using the
    DEFINEFUNC macros below, and once in the function that actually resolves
    the symbols, below the DEFINEFUNC declarations below.

    There's one DEFINEFUNC macro declared for every number of arguments
    exposed by OpenSSL (feel free to extend when needed). The easiest thing to
    do is to find an existing entry that matches the arg count of the function
    you want to import, and do the same.

    The first macro arg is the function return type. The second is the
    verbatim name of the function/symbol. Then follows a list of N pairs of
    argument types with a variable name, and just the variable name (char *a,
    a, char *b, b, etc). Finally there's two arguments - a suitable return
    statement for the error case (for an int function, return 0 or return -1
    is usually right). Then either just "return" or DUMMYARG, the latter being
    for void functions.

    Note: Take into account that these macros and declarations are processed
    at compile-time, and the result depends on the OpenSSL headers the
    compiling host has installed, but the symbols are resolved at run-time,
    possibly with a different version of OpenSSL.
*/

#ifndef QT_LINKED_OPENSSL

namespace {
void qsslSocketUnresolvedSymbolWarning(const char *functionName)
{
    qCWarning(lcSsl, "QSslSocket: cannot call unresolved function %s", functionName);
}

#if QT_CONFIG(library)
void qsslSocketCannotResolveSymbolWarning(const char *functionName)
{
    qCWarning(lcSsl, "QSslSocket: cannot resolve %s", functionName);
}
#endif

}

#endif // QT_LINKED_OPENSSL

DEFINEFUNC(const unsigned char *, ASN1_STRING_get0_data, const ASN1_STRING *a, a, return nullptr, return)
DEFINEFUNC2(int, OPENSSL_init_ssl, uint64_t opts, opts, const OPENSSL_INIT_SETTINGS *settings, settings, return 0, return)
DEFINEFUNC2(int, OPENSSL_init_crypto, uint64_t opts, opts, const OPENSSL_INIT_SETTINGS *settings, settings, return 0, return)
DEFINEFUNC(BIO *, BIO_new, const BIO_METHOD *a, a, return nullptr, return)
DEFINEFUNC(const BIO_METHOD *, BIO_s_mem, void, DUMMYARG, return nullptr, return)
DEFINEFUNC2(int, BN_is_word, BIGNUM *a, a, BN_ULONG w, w, return 0, return)
DEFINEFUNC(int, EVP_CIPHER_CTX_reset, EVP_CIPHER_CTX *c, c, return 0, return)
DEFINEFUNC(int, EVP_PKEY_up_ref, EVP_PKEY *a, a, return 0, return)
DEFINEFUNC2(EVP_PKEY_CTX *, EVP_PKEY_CTX_new, EVP_PKEY *pkey, pkey, ENGINE *e, e, return nullptr, return)
DEFINEFUNC(int, EVP_PKEY_param_check, EVP_PKEY_CTX *ctx, ctx, return 0, return)
DEFINEFUNC(void, EVP_PKEY_CTX_free, EVP_PKEY_CTX *ctx, ctx, return, return)
DEFINEFUNC(int, EVP_PKEY_base_id, EVP_PKEY *a, a, return NID_undef, return)
DEFINEFUNC(int, RSA_bits, RSA *a, a, return 0, return)
DEFINEFUNC(int, DSA_bits, DSA *a, a, return 0, return)
DEFINEFUNC(int, OPENSSL_sk_num, OPENSSL_STACK *a, a, return -1, return)
DEFINEFUNC2(void, OPENSSL_sk_pop_free, OPENSSL_STACK *a, a, void (*b)(void*), b, return, DUMMYARG)
DEFINEFUNC(OPENSSL_STACK *, OPENSSL_sk_new_null, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC2(void, OPENSSL_sk_push, OPENSSL_STACK *a, a, void *b, b, return, DUMMYARG)
DEFINEFUNC(void, OPENSSL_sk_free, OPENSSL_STACK *a, a, return, DUMMYARG)
DEFINEFUNC2(void *, OPENSSL_sk_value, OPENSSL_STACK *a, a, int b, b, return nullptr, return)
DEFINEFUNC(int, SSL_session_reused, SSL *a, a, return 0, return)
DEFINEFUNC2(unsigned long, SSL_CTX_set_options, SSL_CTX *ctx, ctx, unsigned long op, op, return 0, return)
DEFINEFUNC(int, SSL_CTX_get_security_level, const SSL_CTX *ctx, ctx, return -1, return)
DEFINEFUNC2(void, SSL_CTX_set_security_level, SSL_CTX *ctx, ctx, int level, level, return, return)
#ifdef TLS1_3_VERSION
DEFINEFUNC2(int, SSL_CTX_set_ciphersuites, SSL_CTX *ctx, ctx, const char *str, str, return 0, return)
DEFINEFUNC2(void, SSL_set_psk_use_session_callback, SSL *ssl, ssl, q_SSL_psk_use_session_cb_func_t callback, callback, return, DUMMYARG)
DEFINEFUNC2(void, SSL_CTX_sess_set_new_cb, SSL_CTX *ctx, ctx, NewSessionCallback cb, cb, return, return)
DEFINEFUNC(int, SSL_SESSION_is_resumable, const SSL_SESSION *s, s, return 0, return)
#endif
DEFINEFUNC3(size_t, SSL_get_client_random, SSL *a, a, unsigned char *out, out, size_t outlen, outlen, return 0, return)
DEFINEFUNC3(size_t, SSL_SESSION_get_master_key, const SSL_SESSION *ses, ses, unsigned char *out, out, size_t outlen, outlen, return 0, return)
DEFINEFUNC6(int, CRYPTO_get_ex_new_index, int class_index, class_index, long argl, argl, void *argp, argp, CRYPTO_EX_new *new_func, new_func, CRYPTO_EX_dup *dup_func, dup_func, CRYPTO_EX_free *free_func, free_func, return -1, return)
DEFINEFUNC2(unsigned long, SSL_set_options, SSL *ssl, ssl, unsigned long op, op, return 0, return)

DEFINEFUNC(const SSL_METHOD *, TLS_method, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(const SSL_METHOD *, TLS_client_method, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(const SSL_METHOD *, TLS_server_method, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, X509_up_ref, X509 *a, a, return, DUMMYARG)
DEFINEFUNC(ASN1_TIME *, X509_getm_notBefore, X509 *a, a, return nullptr, return)
DEFINEFUNC(ASN1_TIME *, X509_getm_notAfter, X509 *a, a, return nullptr, return)
DEFINEFUNC(long, X509_get_version, X509 *a, a, return -1, return)
DEFINEFUNC(EVP_PKEY *, X509_get_pubkey, X509 *a, a, return nullptr, return)
DEFINEFUNC2(void, X509_STORE_set_verify_cb, X509_STORE *a, a, X509_STORE_CTX_verify_cb verify_cb, verify_cb, return, DUMMYARG)
DEFINEFUNC3(int, X509_STORE_set_ex_data, X509_STORE *a, a, int idx, idx, void *data, data, return 0, return)
DEFINEFUNC2(void *, X509_STORE_get_ex_data, X509_STORE *r, r, int idx, idx, return nullptr, return)
DEFINEFUNC(STACK_OF(X509) *, X509_STORE_CTX_get0_chain, X509_STORE_CTX *a, a, return nullptr, return)
DEFINEFUNC3(void, CRYPTO_free, void *str, str, const char *file, file, int line, line, return, DUMMYARG)
DEFINEFUNC(long, OpenSSL_version_num, void, DUMMYARG, return 0, return)
DEFINEFUNC(const char *, OpenSSL_version, int a, a, return nullptr, return)
DEFINEFUNC(unsigned long, SSL_SESSION_get_ticket_lifetime_hint, const SSL_SESSION *session, session, return 0, return)
DEFINEFUNC4(void, DH_get0_pqg, const DH *dh, dh, const BIGNUM **p, p, const BIGNUM **q, q, const BIGNUM **g, g, return, DUMMYARG)
DEFINEFUNC(int, DH_bits, DH *dh, dh, return 0, return)

#if QT_CONFIG(dtls)
DEFINEFUNC2(int, DTLSv1_listen, SSL *s, s, BIO_ADDR *c, c, return -1, return)
DEFINEFUNC(BIO_ADDR *, BIO_ADDR_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, BIO_ADDR_free, BIO_ADDR *ap, ap, return, DUMMYARG)
DEFINEFUNC2(BIO_METHOD *, BIO_meth_new, int type, type, const char *name, name, return nullptr, return)
DEFINEFUNC(void, BIO_meth_free, BIO_METHOD *biom, biom, return, DUMMYARG)
DEFINEFUNC2(int, BIO_meth_set_write, BIO_METHOD *biom, biom, DgramWriteCallback write, write, return 0, return)
DEFINEFUNC2(int, BIO_meth_set_read, BIO_METHOD *biom, biom, DgramReadCallback read, read, return 0, return)
DEFINEFUNC2(int, BIO_meth_set_puts, BIO_METHOD *biom, biom, DgramPutsCallback puts, puts, return 0, return)
DEFINEFUNC2(int, BIO_meth_set_ctrl, BIO_METHOD *biom, biom, DgramCtrlCallback ctrl, ctrl, return 0, return)
DEFINEFUNC2(int, BIO_meth_set_create, BIO_METHOD *biom, biom, DgramCreateCallback crt, crt, return 0, return)
DEFINEFUNC2(int, BIO_meth_set_destroy, BIO_METHOD *biom, biom, DgramDestroyCallback dtr, dtr, return 0, return)
#endif // dtls

#if QT_CONFIG(ocsp)
DEFINEFUNC(const OCSP_CERTID *, OCSP_SINGLERESP_get0_id, const OCSP_SINGLERESP *x, x, return nullptr, return)
DEFINEFUNC3(OCSP_RESPONSE *, d2i_OCSP_RESPONSE, OCSP_RESPONSE **a, a, const unsigned char **in, in, long len, len, return nullptr, return)
DEFINEFUNC(void, OCSP_RESPONSE_free, OCSP_RESPONSE *rs, rs, return, DUMMYARG)
DEFINEFUNC(OCSP_BASICRESP *, OCSP_response_get1_basic, OCSP_RESPONSE *resp, resp, return nullptr, return)
DEFINEFUNC(void, OCSP_BASICRESP_free, OCSP_BASICRESP *bs, bs, return, DUMMYARG)
DEFINEFUNC(int, OCSP_response_status, OCSP_RESPONSE *resp, resp, return OCSP_RESPONSE_STATUS_INTERNALERROR, return)
DEFINEFUNC4(int, OCSP_basic_verify, OCSP_BASICRESP *bs, bs, STACK_OF(X509) *certs, certs, X509_STORE *st, st, unsigned long flags, flags, return -1, return)
DEFINEFUNC(int, OCSP_resp_count, OCSP_BASICRESP *bs, bs, return 0, return)
DEFINEFUNC2(OCSP_SINGLERESP *, OCSP_resp_get0, OCSP_BASICRESP *bs, bs, int idx, idx, return nullptr, return)
DEFINEFUNC5(int, OCSP_single_get0_status, OCSP_SINGLERESP *single, single, int *reason, reason, ASN1_GENERALIZEDTIME **revtime, revtime,
            ASN1_GENERALIZEDTIME **thisupd, thisupd, ASN1_GENERALIZEDTIME **nextupd, nextupd, return -1, return)
DEFINEFUNC4(int, OCSP_check_validity, ASN1_GENERALIZEDTIME *thisupd, thisupd, ASN1_GENERALIZEDTIME *nextupd, nextupd, long nsec, nsec, long maxsec, maxsec, return 0, return)
DEFINEFUNC3(OCSP_CERTID *, OCSP_cert_to_id, const EVP_MD *dgst, dgst, X509 *subject, subject, X509 *issuer, issuer, return nullptr, return)
DEFINEFUNC(void, OCSP_CERTID_free, OCSP_CERTID *cid, cid, return, DUMMYARG)
DEFINEFUNC5(int, OCSP_id_get0_info, ASN1_OCTET_STRING **piNameHash, piNameHash, ASN1_OBJECT **pmd, pmd,
            ASN1_OCTET_STRING **piKeyHash, piKeyHash, ASN1_INTEGER **pserial, pserial, OCSP_CERTID *cid, cid,
            return 0, return)
DEFINEFUNC2(OCSP_RESPONSE *, OCSP_response_create, int status, status, OCSP_BASICRESP *bs, bs, return nullptr, return)
DEFINEFUNC(const STACK_OF(X509) *, OCSP_resp_get0_certs, const OCSP_BASICRESP *bs, bs, return nullptr, return)
DEFINEFUNC2(int, OCSP_id_cmp, OCSP_CERTID *a, a, OCSP_CERTID *b, b, return -1, return)
DEFINEFUNC7(OCSP_SINGLERESP *, OCSP_basic_add1_status, OCSP_BASICRESP *r, r, OCSP_CERTID *c, c, int s, s,
            int re, re, ASN1_TIME *rt, rt, ASN1_TIME *t, t, ASN1_TIME *n, n, return nullptr, return)
DEFINEFUNC(OCSP_BASICRESP *, OCSP_BASICRESP_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC2(int, i2d_OCSP_RESPONSE, OCSP_RESPONSE *r, r, unsigned char **ppout, ppout, return 0, return)
DEFINEFUNC6(int, OCSP_basic_sign, OCSP_BASICRESP *br, br, X509 *signer, signer, EVP_PKEY *key, key,
            const EVP_MD *dg, dg, STACK_OF(X509) *cs, cs, unsigned long flags, flags, return 0, return)
#endif // ocsp

DEFINEFUNC2(void, BIO_set_data, BIO *a, a, void *ptr, ptr, return, DUMMYARG)
DEFINEFUNC(void *, BIO_get_data, BIO *a, a, return nullptr, return)
DEFINEFUNC2(void, BIO_set_init, BIO *a, a, int init, init, return, DUMMYARG)
DEFINEFUNC(int, BIO_get_shutdown, BIO *a, a, return -1, return)
DEFINEFUNC2(void, BIO_set_shutdown, BIO *a, a, int shut, shut, return, DUMMYARG)

DEFINEFUNC(long, ASN1_INTEGER_get, ASN1_INTEGER *a, a, return 0, return)
DEFINEFUNC2(int, ASN1_INTEGER_cmp, const ASN1_INTEGER *a, a, const ASN1_INTEGER *b, b, return 1, return)
DEFINEFUNC(int, ASN1_STRING_length, ASN1_STRING *a, a, return 0, return)
DEFINEFUNC2(int, ASN1_STRING_to_UTF8, unsigned char **a, a, ASN1_STRING *b, b, return 0, return)
DEFINEFUNC4(long, BIO_ctrl, BIO *a, a, int b, b, long c, c, void *d, d, return -1, return)
DEFINEFUNC(int, BIO_free, BIO *a, a, return 0, return)
DEFINEFUNC2(BIO *, BIO_new_mem_buf, void *a, a, int b, b, return nullptr, return)
DEFINEFUNC3(int, BIO_read, BIO *a, a, void *b, b, int c, c, return -1, return)

DEFINEFUNC3(int, BIO_write, BIO *a, a, const void *b, b, int c, c, return -1, return)
DEFINEFUNC(int, BN_num_bits, const BIGNUM *a, a, return 0, return)
DEFINEFUNC2(BN_ULONG, BN_mod_word, const BIGNUM *a, a, BN_ULONG w, w, return static_cast<BN_ULONG>(-1), return)
#ifndef OPENSSL_NO_EC
DEFINEFUNC(const EC_GROUP*, EC_KEY_get0_group, const EC_KEY* k, k, return nullptr, return)
DEFINEFUNC(int, EC_GROUP_get_degree, const EC_GROUP* g, g, return 0, return)
#endif
DEFINEFUNC(DSA *, DSA_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, DSA_free, DSA *a, a, return, DUMMYARG)
DEFINEFUNC3(X509 *, d2i_X509, X509 **a, a, const unsigned char **b, b, long c, c, return nullptr, return)
DEFINEFUNC2(char *, ERR_error_string, unsigned long a, a, char *b, b, return nullptr, return)
DEFINEFUNC3(void, ERR_error_string_n, unsigned long e, e, char *b, b, size_t len, len, return, DUMMYARG)
DEFINEFUNC(unsigned long, ERR_get_error, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(EVP_CIPHER_CTX *, EVP_CIPHER_CTX_new, void, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, EVP_CIPHER_CTX_free, EVP_CIPHER_CTX *a, a, return, DUMMYARG)
DEFINEFUNC4(int, EVP_CIPHER_CTX_ctrl, EVP_CIPHER_CTX *ctx, ctx, int type, type, int arg, arg, void *ptr, ptr, return 0, return)
DEFINEFUNC2(int, EVP_CIPHER_CTX_set_key_length, EVP_CIPHER_CTX *ctx, ctx, int keylen, keylen, return 0, return)
DEFINEFUNC5(int, EVP_CipherInit, EVP_CIPHER_CTX *ctx, ctx, const EVP_CIPHER *type, type, const unsigned char *key, key, const unsigned char *iv, iv, int enc, enc, return 0, return)
DEFINEFUNC6(int, EVP_CipherInit_ex, EVP_CIPHER_CTX *ctx, ctx, const EVP_CIPHER *cipher, cipher, ENGINE *impl, impl, const unsigned char *key, key, const unsigned char *iv, iv, int enc, enc, return 0, return)
DEFINEFUNC5(int, EVP_CipherUpdate, EVP_CIPHER_CTX *ctx, ctx, unsigned char *out, out, int *outl, outl, const unsigned char *in, in, int inl, inl, return 0, return)
DEFINEFUNC3(int, EVP_CipherFinal, EVP_CIPHER_CTX *ctx, ctx, unsigned char *out, out, int *outl, outl, return 0, return)
DEFINEFUNC(const EVP_MD *, EVP_get_digestbyname, const char *name, name, return nullptr, return)
#ifndef OPENSSL_NO_DES
DEFINEFUNC(const EVP_CIPHER *, EVP_des_cbc, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(const EVP_CIPHER *, EVP_des_ede3_cbc, DUMMYARG, DUMMYARG, return nullptr, return)
#endif
#ifndef OPENSSL_NO_RC2
DEFINEFUNC(const EVP_CIPHER *, EVP_rc2_cbc, DUMMYARG, DUMMYARG, return nullptr, return)
#endif
#ifndef OPENSSL_NO_AES
DEFINEFUNC(const EVP_CIPHER *, EVP_aes_128_cbc, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(const EVP_CIPHER *, EVP_aes_192_cbc, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(const EVP_CIPHER *, EVP_aes_256_cbc, DUMMYARG, DUMMYARG, return nullptr, return)
#endif
DEFINEFUNC(const EVP_MD *, EVP_sha1, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC3(int, EVP_PKEY_assign, EVP_PKEY *a, a, int b, b, char *c, c, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_RSA, EVP_PKEY *a, a, RSA *b, b, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_DSA, EVP_PKEY *a, a, DSA *b, b, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_DH, EVP_PKEY *a, a, DH *b, b, return -1, return)
#ifndef OPENSSL_NO_EC
DEFINEFUNC2(int, EVP_PKEY_set1_EC_KEY, EVP_PKEY *a, a, EC_KEY *b, b, return -1, return)
#endif
DEFINEFUNC2(int, EVP_PKEY_cmp, const EVP_PKEY *a, a, const EVP_PKEY *b, b, return -1, return)
DEFINEFUNC(void, EVP_PKEY_free, EVP_PKEY *a, a, return, DUMMYARG)
DEFINEFUNC(DSA *, EVP_PKEY_get1_DSA, EVP_PKEY *a, a, return nullptr, return)
DEFINEFUNC(RSA *, EVP_PKEY_get1_RSA, EVP_PKEY *a, a, return nullptr, return)
DEFINEFUNC(DH *, EVP_PKEY_get1_DH, EVP_PKEY *a, a, return nullptr, return)
#ifndef OPENSSL_NO_EC
DEFINEFUNC(EC_KEY *, EVP_PKEY_get1_EC_KEY, EVP_PKEY *a, a, return nullptr, return)
#endif
DEFINEFUNC(EVP_PKEY *, EVP_PKEY_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(int, EVP_PKEY_type, int a, a, return NID_undef, return)
DEFINEFUNC2(int, i2d_X509, X509 *a, a, unsigned char **b, b, return -1, return)
DEFINEFUNC(const char *, OBJ_nid2sn, int a, a, return nullptr, return)
DEFINEFUNC(const char *, OBJ_nid2ln, int a, a, return nullptr, return)
DEFINEFUNC(int, OBJ_sn2nid, const char *s, s, return 0, return)
DEFINEFUNC(int, OBJ_ln2nid, const char *s, s, return 0, return)
DEFINEFUNC3(int, i2t_ASN1_OBJECT, char *a, a, int b, b, ASN1_OBJECT *c, c, return -1, return)
DEFINEFUNC4(int, OBJ_obj2txt, char *a, a, int b, b, ASN1_OBJECT *c, c, int d, d, return -1, return)
DEFINEFUNC(int, OBJ_obj2nid, const ASN1_OBJECT *a, a, return NID_undef, return)
DEFINEFUNC4(EVP_PKEY *, PEM_read_bio_PrivateKey, BIO *a, a, EVP_PKEY **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC4(DSA *, PEM_read_bio_DSAPrivateKey, BIO *a, a, DSA **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC4(RSA *, PEM_read_bio_RSAPrivateKey, BIO *a, a, RSA **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)

#ifndef OPENSSL_NO_EC
DEFINEFUNC4(EC_KEY *, PEM_read_bio_ECPrivateKey, BIO *a, a, EC_KEY **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC7(int, PEM_write_bio_ECPrivateKey, BIO *a, a, EC_KEY *b, b, const EVP_CIPHER *c, c, unsigned char *d, d, int e, e, pem_password_cb *f, f, void *g, g, return 0, return)
DEFINEFUNC4(EC_KEY *, PEM_read_bio_EC_PUBKEY, BIO *a, a, EC_KEY **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC2(int, PEM_write_bio_EC_PUBKEY, BIO *a, a, EC_KEY *b, b, return 0, return)
#endif // OPENSSL_NO_EC

DEFINEFUNC4(DH *, PEM_read_bio_DHparams, BIO *a, a, DH **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC7(int, PEM_write_bio_DSAPrivateKey, BIO *a, a, DSA *b, b, const EVP_CIPHER *c, c, unsigned char *d, d, int e, e, pem_password_cb *f, f, void *g, g, return 0, return)
DEFINEFUNC7(int, PEM_write_bio_RSAPrivateKey, BIO *a, a, RSA *b, b, const EVP_CIPHER *c, c, unsigned char *d, d, int e, e, pem_password_cb *f, f, void *g, g, return 0, return)
DEFINEFUNC7(int, PEM_write_bio_PrivateKey, BIO *a, a, EVP_PKEY *b, b, const EVP_CIPHER *c, c, unsigned char *d, d, int e, e, pem_password_cb *f, f, void *g, g, return 0, return)
DEFINEFUNC4(EVP_PKEY *, PEM_read_bio_PUBKEY, BIO *a, a, EVP_PKEY **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC4(DSA *, PEM_read_bio_DSA_PUBKEY, BIO *a, a, DSA **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC4(RSA *, PEM_read_bio_RSA_PUBKEY, BIO *a, a, RSA **b, b, pem_password_cb *c, c, void *d, d, return nullptr, return)
DEFINEFUNC2(int, PEM_write_bio_DSA_PUBKEY, BIO *a, a, DSA *b, b, return 0, return)
DEFINEFUNC2(int, PEM_write_bio_RSA_PUBKEY, BIO *a, a, RSA *b, b, return 0, return)
DEFINEFUNC2(int, PEM_write_bio_PUBKEY, BIO *a, a, EVP_PKEY *b, b, return 0, return)
DEFINEFUNC2(void, RAND_seed, const void *a, a, int b, b, return, DUMMYARG)
DEFINEFUNC(int, RAND_status, void, DUMMYARG, return -1, return)
DEFINEFUNC2(int, RAND_bytes, unsigned char *b, b, int n, n, return 0, return)
DEFINEFUNC(RSA *, RSA_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, RSA_free, RSA *a, a, return, DUMMYARG)
DEFINEFUNC(int, SSL_accept, SSL *a, a, return -1, return)
DEFINEFUNC(int, SSL_clear, SSL *a, a, return -1, return)
DEFINEFUNC3(char *, SSL_CIPHER_description, const SSL_CIPHER *a, a, char *b, b, int c, c, return nullptr, return)
DEFINEFUNC2(int, SSL_CIPHER_get_bits, const SSL_CIPHER *a, a, int *b, b, return 0, return)
DEFINEFUNC(BIO *, SSL_get_rbio, const SSL *s, s, return nullptr, return)
DEFINEFUNC(int, SSL_connect, SSL *a, a, return -1, return)
DEFINEFUNC(int, SSL_CTX_check_private_key, const SSL_CTX *a, a, return -1, return)
DEFINEFUNC4(long, SSL_CTX_ctrl, SSL_CTX *a, a, int b, b, long c, c, void *d, d, return -1, return)
DEFINEFUNC(void, SSL_CTX_free, SSL_CTX *a, a, return, DUMMYARG)
DEFINEFUNC(SSL_CTX *, SSL_CTX_new, const SSL_METHOD *a, a, return nullptr, return)
DEFINEFUNC2(int, SSL_CTX_set_cipher_list, SSL_CTX *a, a, const char *b, b, return -1, return)
DEFINEFUNC3(long, SSL_CTX_callback_ctrl, SSL_CTX *ctx, ctx, int dst, dst, GenericCallbackType cb, cb, return 0, return)
DEFINEFUNC(int, SSL_CTX_set_default_verify_paths, SSL_CTX *a, a, return -1, return)
DEFINEFUNC3(void, SSL_CTX_set_verify, SSL_CTX *a, a, int b, b, int (*c)(int, X509_STORE_CTX *), c, return, DUMMYARG)
DEFINEFUNC2(void, SSL_CTX_set_verify_depth, SSL_CTX *a, a, int b, b, return, DUMMYARG)
DEFINEFUNC2(int, SSL_CTX_use_certificate, SSL_CTX *a, a, X509 *b, b, return -1, return)
DEFINEFUNC3(int, SSL_CTX_use_certificate_file, SSL_CTX *a, a, const char *b, b, int c, c, return -1, return)
DEFINEFUNC2(int, SSL_CTX_use_PrivateKey, SSL_CTX *a, a, EVP_PKEY *b, b, return -1, return)
DEFINEFUNC2(int, SSL_CTX_use_RSAPrivateKey, SSL_CTX *a, a, RSA *b, b, return -1, return)
DEFINEFUNC3(int, SSL_CTX_use_PrivateKey_file, SSL_CTX *a, a, const char *b, b, int c, c, return -1, return)
DEFINEFUNC(X509_STORE *, SSL_CTX_get_cert_store, const SSL_CTX *a, a, return nullptr, return)
DEFINEFUNC(SSL_CONF_CTX *, SSL_CONF_CTX_new, DUMMYARG, DUMMYARG, return nullptr, return);
DEFINEFUNC(void, SSL_CONF_CTX_free, SSL_CONF_CTX *a, a, return ,return);
DEFINEFUNC2(void, SSL_CONF_CTX_set_ssl_ctx, SSL_CONF_CTX *a, a, SSL_CTX *b, b, return, return);
DEFINEFUNC2(unsigned int, SSL_CONF_CTX_set_flags, SSL_CONF_CTX *a, a, unsigned int b, b, return 0, return);
DEFINEFUNC(int, SSL_CONF_CTX_finish, SSL_CONF_CTX *a, a, return 0, return);
DEFINEFUNC3(int, SSL_CONF_cmd, SSL_CONF_CTX *a, a, const char *b, b, const char *c, c, return 0, return);
DEFINEFUNC(void, SSL_free, SSL *a, a, return, DUMMYARG)
DEFINEFUNC(STACK_OF(SSL_CIPHER) *, SSL_get_ciphers, const SSL *a, a, return nullptr, return)
DEFINEFUNC(const SSL_CIPHER *, SSL_get_current_cipher, SSL *a, a, return nullptr, return)
DEFINEFUNC(int, SSL_version, const SSL *a, a, return 0, return)
DEFINEFUNC2(int, SSL_get_error, SSL *a, a, int b, b, return -1, return)
DEFINEFUNC(STACK_OF(X509) *, SSL_get_peer_cert_chain, SSL *a, a, return nullptr, return)
DEFINEFUNC(X509 *, SSL_get_peer_certificate, SSL *a, a, return nullptr, return)
DEFINEFUNC(long, SSL_get_verify_result, const SSL *a, a, return -1, return)
DEFINEFUNC(SSL *, SSL_new, SSL_CTX *a, a, return nullptr, return)
DEFINEFUNC(SSL_CTX *, SSL_get_SSL_CTX, SSL *a, a, return nullptr, return)
DEFINEFUNC4(long, SSL_ctrl, SSL *a, a, int cmd, cmd, long larg, larg, void *parg, parg, return -1, return)
DEFINEFUNC3(int, SSL_read, SSL *a, a, void *b, b, int c, c, return -1, return)
DEFINEFUNC3(void, SSL_set_bio, SSL *a, a, BIO *b, b, BIO *c, c, return, DUMMYARG)
DEFINEFUNC(void, SSL_set_accept_state, SSL *a, a, return, DUMMYARG)
DEFINEFUNC(void, SSL_set_connect_state, SSL *a, a, return, DUMMYARG)
DEFINEFUNC(int, SSL_shutdown, SSL *a, a, return -1, return)
DEFINEFUNC(int, SSL_in_init, const SSL *a, a, return 0, return)
DEFINEFUNC(int, SSL_get_shutdown, const SSL *ssl, ssl, return 0, return)
DEFINEFUNC2(int, SSL_set_session, SSL* to, to, SSL_SESSION *session, session, return -1, return)
DEFINEFUNC(void, SSL_SESSION_free, SSL_SESSION *ses, ses, return, DUMMYARG)
DEFINEFUNC(SSL_SESSION*, SSL_get1_session, SSL *ssl, ssl, return nullptr, return)
DEFINEFUNC(SSL_SESSION*, SSL_get_session, const SSL *ssl, ssl, return nullptr, return)
DEFINEFUNC3(int, SSL_set_ex_data, SSL *ssl, ssl, int idx, idx, void *arg, arg, return 0, return)
DEFINEFUNC2(void *, SSL_get_ex_data, const SSL *ssl, ssl, int idx, idx, return nullptr, return)

#ifndef OPENSSL_NO_PSK
DEFINEFUNC2(void, SSL_set_psk_client_callback, SSL* ssl, ssl, q_psk_client_callback_t callback, callback, return, DUMMYARG)
DEFINEFUNC2(void, SSL_set_psk_server_callback, SSL* ssl, ssl, q_psk_server_callback_t callback, callback, return, DUMMYARG)
DEFINEFUNC2(int, SSL_CTX_use_psk_identity_hint, SSL_CTX* ctx, ctx, const char *hint, hint, return 0, return)
#endif // !OPENSSL_NO_PSK

DEFINEFUNC3(int, SSL_write, SSL *a, a, const void *b, b, int c, c, return -1, return)
DEFINEFUNC2(int, X509_cmp, X509 *a, a, X509 *b, b, return -1, return)
DEFINEFUNC4(int, X509_digest, const X509 *x509, x509, const EVP_MD *type, type, unsigned char *md, md, unsigned int *len, len, return -1, return)
DEFINEFUNC(X509 *, X509_dup, X509 *a, a, return nullptr, return)
DEFINEFUNC2(void, X509_print, BIO *a, a, X509 *b, b, return, DUMMYARG);
DEFINEFUNC(ASN1_OBJECT *, X509_EXTENSION_get_object, X509_EXTENSION *a, a, return nullptr, return)
DEFINEFUNC(void, X509_free, X509 *a, a, return, DUMMYARG)
//Q_AUTOTEST_EXPORT ASN1_TIME *q_X509_gmtime_adj(ASN1_TIME *s, long adj);
DEFINEFUNC2(ASN1_TIME *, X509_gmtime_adj, ASN1_TIME *s, s, long adj, adj, return nullptr, return)
DEFINEFUNC(void, ASN1_TIME_free, ASN1_TIME *t, t, return, DUMMYARG)
DEFINEFUNC2(X509_EXTENSION *, X509_get_ext, X509 *a, a, int b, b, return nullptr, return)
DEFINEFUNC(int, X509_get_ext_count, X509 *a, a, return 0, return)
DEFINEFUNC4(void *, X509_get_ext_d2i, X509 *a, a, int b, b, int *c, c, int *d, d, return nullptr, return)
DEFINEFUNC(const X509V3_EXT_METHOD *, X509V3_EXT_get, X509_EXTENSION *a, a, return nullptr, return)
DEFINEFUNC(void *, X509V3_EXT_d2i, X509_EXTENSION *a, a, return nullptr, return)
DEFINEFUNC(int, X509_EXTENSION_get_critical, X509_EXTENSION *a, a, return 0, return)
DEFINEFUNC(ASN1_OCTET_STRING *, X509_EXTENSION_get_data, X509_EXTENSION *a, a, return nullptr, return)
DEFINEFUNC(void, BASIC_CONSTRAINTS_free, BASIC_CONSTRAINTS *a, a, return, DUMMYARG)
DEFINEFUNC(void, AUTHORITY_KEYID_free, AUTHORITY_KEYID *a, a, return, DUMMYARG)
DEFINEFUNC(void, GENERAL_NAME_free, GENERAL_NAME *a, a, return, DUMMYARG)
DEFINEFUNC2(int, ASN1_STRING_print, BIO *a, a, const ASN1_STRING *b, b, return 0, return)
DEFINEFUNC2(int, X509_check_issued, X509 *a, a, X509 *b, b, return -1, return)
DEFINEFUNC(X509_NAME *, X509_get_issuer_name, X509 *a, a, return nullptr, return)
DEFINEFUNC(X509_NAME *, X509_get_subject_name, X509 *a, a, return nullptr, return)
DEFINEFUNC(ASN1_INTEGER *, X509_get_serialNumber, X509 *a, a, return nullptr, return)
DEFINEFUNC(int, X509_verify_cert, X509_STORE_CTX *a, a, return -1, return)
DEFINEFUNC(int, X509_NAME_entry_count, X509_NAME *a, a, return 0, return)
DEFINEFUNC2(X509_NAME_ENTRY *, X509_NAME_get_entry, X509_NAME *a, a, int b, b, return nullptr, return)
DEFINEFUNC(ASN1_STRING *, X509_NAME_ENTRY_get_data, X509_NAME_ENTRY *a, a, return nullptr, return)
DEFINEFUNC(ASN1_OBJECT *, X509_NAME_ENTRY_get_object, X509_NAME_ENTRY *a, a, return nullptr, return)
DEFINEFUNC(EVP_PKEY *, X509_PUBKEY_get, X509_PUBKEY *a, a, return nullptr, return)
DEFINEFUNC(void, X509_STORE_free, X509_STORE *a, a, return, DUMMYARG)
DEFINEFUNC(X509_STORE *, X509_STORE_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC2(int, X509_STORE_add_cert, X509_STORE *a, a, X509 *b, b, return 0, return)
DEFINEFUNC(void, X509_STORE_CTX_free, X509_STORE_CTX *a, a, return, DUMMYARG)
DEFINEFUNC4(int, X509_STORE_CTX_init, X509_STORE_CTX *a, a, X509_STORE *b, b, X509 *c, c, STACK_OF(X509) *d, d, return -1, return)
DEFINEFUNC2(int, X509_STORE_CTX_set_purpose, X509_STORE_CTX *a, a, int b, b, return -1, return)
DEFINEFUNC(int, X509_STORE_CTX_get_error, X509_STORE_CTX *a, a, return -1, return)
DEFINEFUNC(int, X509_STORE_CTX_get_error_depth, X509_STORE_CTX *a, a, return -1, return)
DEFINEFUNC(X509 *, X509_STORE_CTX_get_current_cert, X509_STORE_CTX *a, a, return nullptr, return)
DEFINEFUNC(X509_STORE *, X509_STORE_CTX_get0_store, X509_STORE_CTX *ctx, ctx, return nullptr, return)
DEFINEFUNC(X509_STORE_CTX *, X509_STORE_CTX_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC2(void *, X509_STORE_CTX_get_ex_data, X509_STORE_CTX *ctx, ctx, int idx, idx, return nullptr, return)
DEFINEFUNC(int, SSL_get_ex_data_X509_STORE_CTX_idx, DUMMYARG, DUMMYARG, return -1, return)

#if OPENSSL_VERSION_MAJOR < 3
DEFINEFUNC3(int, SSL_CTX_load_verify_locations, SSL_CTX *ctx, ctx, const char *CAfile, CAfile, const char *CApath, CApath, return 0, return)
#else
DEFINEFUNC2(int, SSL_CTX_load_verify_dir, SSL_CTX *ctx, ctx, const char *CApath, CApath, return 0, return)
#endif // OPENSSL_VERSION_MAJOR

DEFINEFUNC2(int, i2d_SSL_SESSION, SSL_SESSION *in, in, unsigned char **pp, pp, return 0, return)
DEFINEFUNC3(SSL_SESSION *, d2i_SSL_SESSION, SSL_SESSION **a, a, const unsigned char **pp, pp, long length, length, return nullptr, return)

#ifndef OPENSSL_NO_NEXTPROTONEG
DEFINEFUNC6(int, SSL_select_next_proto, unsigned char **out, out, unsigned char *outlen, outlen,
            const unsigned char *in, in, unsigned int inlen, inlen,
            const unsigned char *client, client, unsigned int client_len, client_len,
            return -1, return)
DEFINEFUNC3(void, SSL_CTX_set_next_proto_select_cb, SSL_CTX *s, s,
            int (*cb) (SSL *ssl, unsigned char **out,
                       unsigned char *outlen,
                       const unsigned char *in,
                       unsigned int inlen, void *arg), cb,
            void *arg, arg, return, DUMMYARG)
DEFINEFUNC3(void, SSL_get0_next_proto_negotiated, const SSL *s, s,
            const unsigned char **data, data, unsigned *len, len, return, DUMMYARG)
DEFINEFUNC3(int, SSL_set_alpn_protos, SSL *s, s, const unsigned char *protos, protos,
            unsigned protos_len, protos_len, return -1, return)
DEFINEFUNC3(void, SSL_CTX_set_alpn_select_cb, SSL_CTX *s, s,
            int (*cb) (SSL *ssl, const unsigned char **out,
                       unsigned char *outlen,
                       const unsigned char *in,
                       unsigned int inlen, void *arg), cb,
            void *arg, arg, return, DUMMYARG)
DEFINEFUNC3(void, SSL_get0_alpn_selected, const SSL *s, s, const unsigned char **data, data,
            unsigned *len, len, return, DUMMYARG)
#endif // !OPENSSL_NO_NEXTPROTONEG

// DTLS:
#if QT_CONFIG(dtls)
DEFINEFUNC2(void, SSL_CTX_set_cookie_generate_cb, SSL_CTX *ctx, ctx, CookieGenerateCallback cb, cb, return, DUMMYARG)
DEFINEFUNC2(void, SSL_CTX_set_cookie_verify_cb, SSL_CTX *ctx, ctx, CookieVerifyCallback cb, cb, return, DUMMYARG)
DEFINEFUNC(const SSL_METHOD *, DTLS_server_method, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(const SSL_METHOD *, DTLS_client_method, DUMMYARG, DUMMYARG, return nullptr, return)
#endif // dtls
DEFINEFUNC2(void, BIO_set_flags, BIO *b, b, int flags, flags, return, DUMMYARG)
DEFINEFUNC2(void, BIO_clear_flags, BIO *b, b, int flags, flags, return, DUMMYARG)
DEFINEFUNC2(void *, BIO_get_ex_data, BIO *b, b, int idx, idx, return nullptr, return)
DEFINEFUNC3(int, BIO_set_ex_data, BIO *b, b, int idx, idx, void *data, data, return -1, return)

DEFINEFUNC3(void *, CRYPTO_malloc, size_t num, num, const char *file, file, int line, line, return nullptr, return)
DEFINEFUNC(DH *, DH_new, DUMMYARG, DUMMYARG, return nullptr, return)
DEFINEFUNC(void, DH_free, DH *dh, dh, return, DUMMYARG)
DEFINEFUNC3(DH *, d2i_DHparams, DH**a, a, const unsigned char **pp, pp, long length, length, return nullptr, return)
DEFINEFUNC2(int, i2d_DHparams, DH *a, a, unsigned char **p, p, return -1, return)
#ifndef OPENSSL_NO_DEPRECATED_3_0
DEFINEFUNC2(int, DH_check, DH *dh, dh, int *codes, codes, return 0, return)
#endif // OPENSSL_NO_DEPRECATED_3_0
DEFINEFUNC3(BIGNUM *, BN_bin2bn, const unsigned char *s, s, int len, len, BIGNUM *ret, ret, return nullptr, return)

#ifndef OPENSSL_NO_EC
DEFINEFUNC(EC_KEY *, EC_KEY_dup, const EC_KEY *ec, ec, return nullptr, return)
DEFINEFUNC(EC_KEY *, EC_KEY_new_by_curve_name, int nid, nid, return nullptr, return)
DEFINEFUNC(void, EC_KEY_free, EC_KEY *ecdh, ecdh, return, DUMMYARG)
DEFINEFUNC2(size_t, EC_get_builtin_curves, EC_builtin_curve * r, r, size_t nitems, nitems, return 0, return)
DEFINEFUNC(int, EC_curve_nist2nid, const char *name, name, return 0, return)
#endif // OPENSSL_NO_EC

DEFINEFUNC5(int, PKCS12_parse, PKCS12 *p12, p12, const char *pass, pass, EVP_PKEY **pkey, pkey, \
            X509 **cert, cert, STACK_OF(X509) **ca, ca, return 1, return);
DEFINEFUNC2(PKCS12 *, d2i_PKCS12_bio, BIO *bio, bio, PKCS12 **pkcs12, pkcs12, return nullptr, return);
DEFINEFUNC(void, PKCS12_free, PKCS12 *pkcs12, pkcs12, return, DUMMYARG)

#define RESOLVEFUNC(func) \
    if (!(_q_##func = _q_PTR_##func(libs.ssl->resolve(#func)))     \
        && !(_q_##func = _q_PTR_##func(libs.crypto->resolve(#func)))) \
        qsslSocketCannotResolveSymbolWarning(#func);

#if !defined QT_LINKED_OPENSSL

#if !QT_CONFIG(library)
bool q_resolveOpenSslSymbols()
{
    qCWarning(lcSsl, "QSslSocket: unable to resolve symbols. Qt is configured without the "
                     "'library' feature, which means runtime resolving of libraries won't work.");
    qCWarning(lcSsl, "Either compile Qt statically or with support for runtime resolving "
                     "of libraries.");
    return false;
}
#else

# ifdef Q_OS_UNIX
struct NumericallyLess
{
    typedef bool result_type;
    result_type operator()(const QStringRef &lhs, const QStringRef &rhs) const
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
    typedef bool result_type;
    result_type operator()(const QString &lhs, const QString &rhs) const
    {
        const QVector<QStringRef> lhsparts = lhs.splitRef(QLatin1Char('.'));
        const QVector<QStringRef> rhsparts = rhs.splitRef(QLatin1Char('.'));
        Q_ASSERT(lhsparts.count() > 1 && rhsparts.count() > 1);

        // note: checking rhs < lhs, the same as lhs > rhs
        return std::lexicographical_compare(rhsparts.begin() + 1, rhsparts.end(),
                                            lhsparts.begin() + 1, lhsparts.end(),
                                            NumericallyLess());
    }
};

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
static int dlIterateCallback(struct dl_phdr_info *info, size_t size, void *data)
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
#endif

static QStringList libraryPathList()
{
    QStringList paths;
#  ifdef Q_OS_DARWIN
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
#  else
    paths = QString::fromLatin1(qgetenv("LD_LIBRARY_PATH"))
            .split(QLatin1Char(':'), Qt::SkipEmptyParts);
#  endif
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
#endif

    return paths;
}

Q_NEVER_INLINE
static QStringList findAllLibs(QLatin1String filter)
{
    const QStringList paths = libraryPathList();
    QStringList found;
    const QStringList filters((QString(filter)));

    for (const QString &path : paths) {
        QDir dir(path);
        QStringList entryList = dir.entryList(filters, QDir::Files);

        std::sort(entryList.begin(), entryList.end(), LibGreaterThan());
        for (const QString &entry : qAsConst(entryList))
            found << path + QLatin1Char('/') + entry;
    }

    return found;
}

static QStringList findAllLibSsl()
{
    return findAllLibs(QLatin1String("libssl.*"));
}

static QStringList findAllLibCrypto()
{
    return findAllLibs(QLatin1String("libcrypto.*"));
}
# endif

#ifdef Q_OS_WIN

struct LoadedOpenSsl {
    std::unique_ptr<QSystemLibrary> ssl, crypto;
};

static bool tryToLoadOpenSslWin32Library(QLatin1String ssleay32LibName, QLatin1String libeay32LibName, LoadedOpenSsl &result)
{
    auto ssleay32 = qt_make_unique<QSystemLibrary>(ssleay32LibName);
    if (!ssleay32->load(false)) {
        return FALSE;
    }

    auto libeay32 = qt_make_unique<QSystemLibrary>(libeay32LibName);
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

    // With OpenSSL 1.1 the names have changed to libssl-1_1(-x64) and libcrypto-1_1(-x64), for builds using
    // MSVC and GCC, (-x64 suffix for 64-bit builds).

#ifdef Q_PROCESSOR_X86_64
#define QT_SSL_SUFFIX "-x64"
#else // !Q_PROCESSOFR_X86_64
#define QT_SSL_SUFFIX
#endif // !Q_PROCESSOR_x86_64

    tryToLoadOpenSslWin32Library(QLatin1String("libssl-1_1" QT_SSL_SUFFIX),
                                 QLatin1String("libcrypto-1_1" QT_SSL_SUFFIX), result);

#undef QT_SSL_SUFFIX
    return result;
}
#else

struct LoadedOpenSsl {
    std::unique_ptr<QLibrary> ssl, crypto;
};

static LoadedOpenSsl loadOpenSsl()
{
    LoadedOpenSsl result = {qt_make_unique<QLibrary>(), qt_make_unique<QLibrary>()};

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
#endif

#ifndef Q_OS_DARWIN
    // second attempt: find the development files libssl.so and libcrypto.so
    //
    // disabled on macOS/iOS:
    //  macOS's /usr/lib/libssl.dylib, /usr/lib/libcrypto.dylib will be picked up in the third
    //    attempt, _after_ <bundle>/Contents/Frameworks has been searched.
    //  iOS does not ship a system libssl.dylib, libcrypto.dylib in the first place.
# if defined(Q_OS_ANDROID)
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
# else
    libssl->setFileNameAndVersion(QLatin1String("ssl"), -1);
    libcrypto->setFileNameAndVersion(QLatin1String("crypto"), -1);
# endif
    if (libcrypto->load() && libssl->load()) {
        // libssl.so.0 and libcrypto.so.0 found
        return result;
    } else {
        libssl->unload();
        libcrypto->unload();
    }
#endif

    // third attempt: loop on the most common library paths and find libssl
    const QStringList sslList = findAllLibSsl();
    const QStringList cryptoList = findAllLibCrypto();

    for (const QString &crypto : cryptoList) {
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
# endif
}
#endif

static QBasicMutex symbolResolveMutex;
static QBasicAtomicInt symbolsResolved = Q_BASIC_ATOMIC_INITIALIZER(false);
static bool triedToResolveSymbols = false;

bool q_resolveOpenSslSymbols()
{
    if (symbolsResolved.loadAcquire())
        return true;
    QMutexLocker locker(&symbolResolveMutex);
    if (symbolsResolved.loadRelaxed())
        return true;
    if (triedToResolveSymbols)
        return false;
    triedToResolveSymbols = true;

    LoadedOpenSsl libs = loadOpenSsl();
    if (!libs.ssl || !libs.crypto)
        // failed to load them
        return false;

    RESOLVEFUNC(OPENSSL_init_ssl)
    RESOLVEFUNC(OPENSSL_init_crypto)
    RESOLVEFUNC(ASN1_STRING_get0_data)
    RESOLVEFUNC(EVP_CIPHER_CTX_reset)
    RESOLVEFUNC(EVP_PKEY_up_ref)
    RESOLVEFUNC(EVP_PKEY_CTX_new)
    RESOLVEFUNC(EVP_PKEY_param_check)
    RESOLVEFUNC(EVP_PKEY_CTX_free)
    RESOLVEFUNC(EVP_PKEY_base_id)
    RESOLVEFUNC(RSA_bits)
    RESOLVEFUNC(OPENSSL_sk_new_null)
    RESOLVEFUNC(OPENSSL_sk_push)
    RESOLVEFUNC(OPENSSL_sk_free)
    RESOLVEFUNC(OPENSSL_sk_num)
    RESOLVEFUNC(OPENSSL_sk_pop_free)
    RESOLVEFUNC(OPENSSL_sk_value)
    RESOLVEFUNC(DH_get0_pqg)
    RESOLVEFUNC(SSL_CTX_set_options)
    RESOLVEFUNC(SSL_CTX_get_security_level)
    RESOLVEFUNC(SSL_CTX_set_security_level)
#ifdef TLS1_3_VERSION
    RESOLVEFUNC(SSL_CTX_set_ciphersuites)
    RESOLVEFUNC(SSL_set_psk_use_session_callback)
    RESOLVEFUNC(SSL_CTX_sess_set_new_cb)
    RESOLVEFUNC(SSL_SESSION_is_resumable)
#endif // TLS 1.3 or OpenSSL > 1.1.1

    RESOLVEFUNC(SSL_get_client_random)
    RESOLVEFUNC(SSL_SESSION_get_master_key)
    RESOLVEFUNC(SSL_session_reused)
    RESOLVEFUNC(SSL_get_session)
    RESOLVEFUNC(SSL_set_options)
    RESOLVEFUNC(CRYPTO_get_ex_new_index)
    RESOLVEFUNC(TLS_method)
    RESOLVEFUNC(TLS_client_method)
    RESOLVEFUNC(TLS_server_method)
    RESOLVEFUNC(X509_up_ref)
    RESOLVEFUNC(X509_STORE_CTX_get0_chain)
    RESOLVEFUNC(X509_getm_notBefore)
    RESOLVEFUNC(X509_getm_notAfter)
    RESOLVEFUNC(X509_get_version)
    RESOLVEFUNC(X509_get_pubkey)
    RESOLVEFUNC(X509_STORE_set_verify_cb)
    RESOLVEFUNC(X509_STORE_set_ex_data)
    RESOLVEFUNC(X509_STORE_get_ex_data)
    RESOLVEFUNC(CRYPTO_free)
    RESOLVEFUNC(OpenSSL_version_num)
    RESOLVEFUNC(OpenSSL_version)

    if (!_q_OpenSSL_version) {
        // Apparently, we were built with OpenSSL 1.1 enabled but are now using
        // a wrong library.
        qCWarning(lcSsl, "Incompatible version of OpenSSL");
        return false;
    }

    RESOLVEFUNC(SSL_SESSION_get_ticket_lifetime_hint)
    RESOLVEFUNC(DH_bits)
    RESOLVEFUNC(DSA_bits)

#if QT_CONFIG(dtls)
    RESOLVEFUNC(DTLSv1_listen)
    RESOLVEFUNC(BIO_ADDR_new)
    RESOLVEFUNC(BIO_ADDR_free)
    RESOLVEFUNC(BIO_meth_new)
    RESOLVEFUNC(BIO_meth_free)
    RESOLVEFUNC(BIO_meth_set_write)
    RESOLVEFUNC(BIO_meth_set_read)
    RESOLVEFUNC(BIO_meth_set_puts)
    RESOLVEFUNC(BIO_meth_set_ctrl)
    RESOLVEFUNC(BIO_meth_set_create)
    RESOLVEFUNC(BIO_meth_set_destroy)
#endif // dtls

#if QT_CONFIG(ocsp)
    RESOLVEFUNC(OCSP_SINGLERESP_get0_id)
    RESOLVEFUNC(d2i_OCSP_RESPONSE)
    RESOLVEFUNC(OCSP_RESPONSE_free)
    RESOLVEFUNC(OCSP_response_status)
    RESOLVEFUNC(OCSP_response_get1_basic)
    RESOLVEFUNC(OCSP_BASICRESP_free)
    RESOLVEFUNC(OCSP_basic_verify)
    RESOLVEFUNC(OCSP_resp_count)
    RESOLVEFUNC(OCSP_resp_get0)
    RESOLVEFUNC(OCSP_single_get0_status)
    RESOLVEFUNC(OCSP_check_validity)
    RESOLVEFUNC(OCSP_cert_to_id)
    RESOLVEFUNC(OCSP_id_get0_info)
    RESOLVEFUNC(OCSP_resp_get0_certs)
    RESOLVEFUNC(OCSP_basic_sign)
    RESOLVEFUNC(OCSP_response_create)
    RESOLVEFUNC(i2d_OCSP_RESPONSE)
    RESOLVEFUNC(OCSP_basic_add1_status)
    RESOLVEFUNC(OCSP_BASICRESP_new)
    RESOLVEFUNC(OCSP_CERTID_free)
    RESOLVEFUNC(OCSP_cert_to_id)
    RESOLVEFUNC(OCSP_id_cmp)
#endif // ocsp

    RESOLVEFUNC(BIO_set_data)
    RESOLVEFUNC(BIO_get_data)
    RESOLVEFUNC(BIO_set_init)
    RESOLVEFUNC(BIO_get_shutdown)
    RESOLVEFUNC(BIO_set_shutdown)
    RESOLVEFUNC(ASN1_INTEGER_get)
    RESOLVEFUNC(ASN1_INTEGER_cmp)
    RESOLVEFUNC(ASN1_STRING_length)
    RESOLVEFUNC(ASN1_STRING_to_UTF8)
    RESOLVEFUNC(BIO_ctrl)
    RESOLVEFUNC(BIO_free)
    RESOLVEFUNC(BIO_new)
    RESOLVEFUNC(BIO_new_mem_buf)
    RESOLVEFUNC(BIO_read)
    RESOLVEFUNC(BIO_s_mem)
    RESOLVEFUNC(BIO_write)
    RESOLVEFUNC(BIO_set_flags)
    RESOLVEFUNC(BIO_clear_flags)
    RESOLVEFUNC(BIO_set_ex_data)
    RESOLVEFUNC(BIO_get_ex_data)

#ifndef OPENSSL_NO_EC
    RESOLVEFUNC(EC_KEY_get0_group)
    RESOLVEFUNC(EC_GROUP_get_degree)
#endif
    RESOLVEFUNC(BN_num_bits)
    RESOLVEFUNC(BN_is_word)
    RESOLVEFUNC(BN_mod_word)
    RESOLVEFUNC(DSA_new)
    RESOLVEFUNC(DSA_free)
    RESOLVEFUNC(ERR_error_string)
    RESOLVEFUNC(ERR_error_string_n)
    RESOLVEFUNC(ERR_get_error)
    RESOLVEFUNC(EVP_CIPHER_CTX_new)
    RESOLVEFUNC(EVP_CIPHER_CTX_free)
    RESOLVEFUNC(EVP_CIPHER_CTX_ctrl)
    RESOLVEFUNC(EVP_CIPHER_CTX_set_key_length)
    RESOLVEFUNC(EVP_CipherInit)
    RESOLVEFUNC(EVP_CipherInit_ex)
    RESOLVEFUNC(EVP_CipherUpdate)
    RESOLVEFUNC(EVP_CipherFinal)
    RESOLVEFUNC(EVP_get_digestbyname)
#ifndef OPENSSL_NO_DES
    RESOLVEFUNC(EVP_des_cbc)
    RESOLVEFUNC(EVP_des_ede3_cbc)
#endif
#ifndef OPENSSL_NO_RC2
    RESOLVEFUNC(EVP_rc2_cbc)
#endif
#ifndef OPENSSL_NO_AES
    RESOLVEFUNC(EVP_aes_128_cbc)
    RESOLVEFUNC(EVP_aes_192_cbc)
    RESOLVEFUNC(EVP_aes_256_cbc)
#endif
    RESOLVEFUNC(EVP_sha1)
    RESOLVEFUNC(EVP_PKEY_assign)
    RESOLVEFUNC(EVP_PKEY_set1_RSA)
    RESOLVEFUNC(EVP_PKEY_set1_DSA)
    RESOLVEFUNC(EVP_PKEY_set1_DH)

#ifndef OPENSSL_NO_EC
    RESOLVEFUNC(EVP_PKEY_set1_EC_KEY)
    RESOLVEFUNC(EVP_PKEY_get1_EC_KEY)
    RESOLVEFUNC(PEM_read_bio_ECPrivateKey)
    RESOLVEFUNC(PEM_write_bio_ECPrivateKey)
    RESOLVEFUNC(PEM_read_bio_EC_PUBKEY)
    RESOLVEFUNC(PEM_write_bio_EC_PUBKEY)
#endif // OPENSSL_NO_EC

    RESOLVEFUNC(EVP_PKEY_cmp)
    RESOLVEFUNC(EVP_PKEY_free)
    RESOLVEFUNC(EVP_PKEY_get1_DSA)
    RESOLVEFUNC(EVP_PKEY_get1_RSA)
    RESOLVEFUNC(EVP_PKEY_get1_DH)
    RESOLVEFUNC(EVP_PKEY_new)
    RESOLVEFUNC(EVP_PKEY_type)
    RESOLVEFUNC(OBJ_nid2sn)
    RESOLVEFUNC(OBJ_nid2ln)
    RESOLVEFUNC(OBJ_sn2nid)
    RESOLVEFUNC(OBJ_ln2nid)
    RESOLVEFUNC(i2t_ASN1_OBJECT)
    RESOLVEFUNC(OBJ_obj2txt)
    RESOLVEFUNC(OBJ_obj2nid)
    RESOLVEFUNC(PEM_read_bio_PrivateKey)
    RESOLVEFUNC(PEM_read_bio_DSAPrivateKey)
    RESOLVEFUNC(PEM_read_bio_RSAPrivateKey)
    RESOLVEFUNC(PEM_read_bio_DHparams)
    RESOLVEFUNC(PEM_write_bio_DSAPrivateKey)
    RESOLVEFUNC(PEM_write_bio_RSAPrivateKey)
    RESOLVEFUNC(PEM_write_bio_PrivateKey)
    RESOLVEFUNC(PEM_read_bio_PUBKEY)
    RESOLVEFUNC(PEM_read_bio_DSA_PUBKEY)
    RESOLVEFUNC(PEM_read_bio_RSA_PUBKEY)
    RESOLVEFUNC(PEM_write_bio_DSA_PUBKEY)
    RESOLVEFUNC(PEM_write_bio_RSA_PUBKEY)
    RESOLVEFUNC(PEM_write_bio_PUBKEY)
    RESOLVEFUNC(RAND_seed)
    RESOLVEFUNC(RAND_status)
    RESOLVEFUNC(RAND_bytes)
    RESOLVEFUNC(RSA_new)
    RESOLVEFUNC(RSA_free)
    RESOLVEFUNC(SSL_CIPHER_description)
    RESOLVEFUNC(SSL_CIPHER_get_bits)
    RESOLVEFUNC(SSL_get_rbio)
    RESOLVEFUNC(SSL_CTX_check_private_key)
    RESOLVEFUNC(SSL_CTX_ctrl)
    RESOLVEFUNC(SSL_CTX_free)
    RESOLVEFUNC(SSL_CTX_new)
    RESOLVEFUNC(SSL_CTX_set_cipher_list)
    RESOLVEFUNC(SSL_CTX_callback_ctrl)
    RESOLVEFUNC(SSL_CTX_set_default_verify_paths)
    RESOLVEFUNC(SSL_CTX_set_verify)
    RESOLVEFUNC(SSL_CTX_set_verify_depth)
    RESOLVEFUNC(SSL_CTX_use_certificate)
    RESOLVEFUNC(SSL_CTX_use_certificate_file)
    RESOLVEFUNC(SSL_CTX_use_PrivateKey)
    RESOLVEFUNC(SSL_CTX_use_RSAPrivateKey)
    RESOLVEFUNC(SSL_CTX_use_PrivateKey_file)
    RESOLVEFUNC(SSL_CTX_get_cert_store);
    RESOLVEFUNC(SSL_CONF_CTX_new);
    RESOLVEFUNC(SSL_CONF_CTX_free);
    RESOLVEFUNC(SSL_CONF_CTX_set_ssl_ctx);
    RESOLVEFUNC(SSL_CONF_CTX_set_flags);
    RESOLVEFUNC(SSL_CONF_CTX_finish);
    RESOLVEFUNC(SSL_CONF_cmd);
    RESOLVEFUNC(SSL_accept)
    RESOLVEFUNC(SSL_clear)
    RESOLVEFUNC(SSL_connect)
    RESOLVEFUNC(SSL_free)
    RESOLVEFUNC(SSL_get_ciphers)
    RESOLVEFUNC(SSL_get_current_cipher)
    RESOLVEFUNC(SSL_version)
    RESOLVEFUNC(SSL_get_error)
    RESOLVEFUNC(SSL_get_peer_cert_chain)
    RESOLVEFUNC(SSL_get_peer_certificate)
    RESOLVEFUNC(SSL_get_verify_result)
    RESOLVEFUNC(SSL_new)
    RESOLVEFUNC(SSL_get_SSL_CTX)
    RESOLVEFUNC(SSL_ctrl)
    RESOLVEFUNC(SSL_read)
    RESOLVEFUNC(SSL_set_accept_state)
    RESOLVEFUNC(SSL_set_bio)
    RESOLVEFUNC(SSL_set_connect_state)
    RESOLVEFUNC(SSL_shutdown)
    RESOLVEFUNC(SSL_in_init)
    RESOLVEFUNC(SSL_get_shutdown)
    RESOLVEFUNC(SSL_set_session)
    RESOLVEFUNC(SSL_SESSION_free)
    RESOLVEFUNC(SSL_get1_session)
    RESOLVEFUNC(SSL_get_session)
    RESOLVEFUNC(SSL_set_ex_data)
    RESOLVEFUNC(SSL_get_ex_data)
    RESOLVEFUNC(SSL_get_ex_data_X509_STORE_CTX_idx)

#ifndef OPENSSL_NO_PSK
    RESOLVEFUNC(SSL_set_psk_client_callback)
    RESOLVEFUNC(SSL_set_psk_server_callback)
    RESOLVEFUNC(SSL_CTX_use_psk_identity_hint)
#endif // !OPENSSL_NO_PSK

    RESOLVEFUNC(SSL_write)
    RESOLVEFUNC(X509_NAME_entry_count)
    RESOLVEFUNC(X509_NAME_get_entry)
    RESOLVEFUNC(X509_NAME_ENTRY_get_data)
    RESOLVEFUNC(X509_NAME_ENTRY_get_object)
    RESOLVEFUNC(X509_PUBKEY_get)
    RESOLVEFUNC(X509_STORE_free)
    RESOLVEFUNC(X509_STORE_new)
    RESOLVEFUNC(X509_STORE_add_cert)
    RESOLVEFUNC(X509_STORE_CTX_free)
    RESOLVEFUNC(X509_STORE_CTX_init)
    RESOLVEFUNC(X509_STORE_CTX_new)
    RESOLVEFUNC(X509_STORE_CTX_set_purpose)
    RESOLVEFUNC(X509_STORE_CTX_get_error)
    RESOLVEFUNC(X509_STORE_CTX_get_error_depth)
    RESOLVEFUNC(X509_STORE_CTX_get_current_cert)
    RESOLVEFUNC(X509_STORE_CTX_get0_store)
    RESOLVEFUNC(X509_cmp)
    RESOLVEFUNC(X509_STORE_CTX_get_ex_data)
    RESOLVEFUNC(X509_dup)
    RESOLVEFUNC(X509_print)
    RESOLVEFUNC(X509_digest)
    RESOLVEFUNC(X509_EXTENSION_get_object)
    RESOLVEFUNC(X509_free)
    RESOLVEFUNC(X509_gmtime_adj)
    RESOLVEFUNC(ASN1_TIME_free)
    RESOLVEFUNC(X509_get_ext)
    RESOLVEFUNC(X509_get_ext_count)
    RESOLVEFUNC(X509_get_ext_d2i)
    RESOLVEFUNC(X509V3_EXT_get)
    RESOLVEFUNC(X509V3_EXT_d2i)
    RESOLVEFUNC(X509_EXTENSION_get_critical)
    RESOLVEFUNC(X509_EXTENSION_get_data)
    RESOLVEFUNC(BASIC_CONSTRAINTS_free)
    RESOLVEFUNC(AUTHORITY_KEYID_free)
    RESOLVEFUNC(GENERAL_NAME_free)
    RESOLVEFUNC(ASN1_STRING_print)
    RESOLVEFUNC(X509_check_issued)
    RESOLVEFUNC(X509_get_issuer_name)
    RESOLVEFUNC(X509_get_subject_name)
    RESOLVEFUNC(X509_get_serialNumber)
    RESOLVEFUNC(X509_verify_cert)
    RESOLVEFUNC(d2i_X509)
    RESOLVEFUNC(i2d_X509)
#if OPENSSL_VERSION_MAJOR < 3
    RESOLVEFUNC(SSL_CTX_load_verify_locations)
#else
    RESOLVEFUNC(SSL_CTX_load_verify_dir)
#endif // OPENSSL_VERSION_MAJOR
    RESOLVEFUNC(i2d_SSL_SESSION)
    RESOLVEFUNC(d2i_SSL_SESSION)

#ifndef OPENSSL_NO_NEXTPROTONEG
    RESOLVEFUNC(SSL_select_next_proto)
    RESOLVEFUNC(SSL_CTX_set_next_proto_select_cb)
    RESOLVEFUNC(SSL_get0_next_proto_negotiated)
    RESOLVEFUNC(SSL_set_alpn_protos)
    RESOLVEFUNC(SSL_CTX_set_alpn_select_cb)
    RESOLVEFUNC(SSL_get0_alpn_selected)
#endif // !OPENSSL_NO_NEXTPROTONEG

#if QT_CONFIG(dtls)
    RESOLVEFUNC(SSL_CTX_set_cookie_generate_cb)
    RESOLVEFUNC(SSL_CTX_set_cookie_verify_cb)
    RESOLVEFUNC(DTLS_server_method)
    RESOLVEFUNC(DTLS_client_method)
#endif // dtls

    RESOLVEFUNC(CRYPTO_malloc)
    RESOLVEFUNC(DH_new)
    RESOLVEFUNC(DH_free)
    RESOLVEFUNC(d2i_DHparams)
    RESOLVEFUNC(i2d_DHparams)
#ifndef OPENSSL_NO_DEPRECATED_3_0
    RESOLVEFUNC(DH_check)
#endif // OPENSSL_NO_DEPRECATED_3_0
    RESOLVEFUNC(BN_bin2bn)

#ifndef OPENSSL_NO_EC
    RESOLVEFUNC(EC_KEY_dup)
    RESOLVEFUNC(EC_KEY_new_by_curve_name)
    RESOLVEFUNC(EC_KEY_free)
    RESOLVEFUNC(EC_get_builtin_curves)
#endif // OPENSSL_NO_EC

    RESOLVEFUNC(PKCS12_parse)
    RESOLVEFUNC(d2i_PKCS12_bio)
    RESOLVEFUNC(PKCS12_free)

    symbolsResolved.storeRelease(true);
    return true;
}
#endif // QT_CONFIG(library)

#else // !defined QT_LINKED_OPENSSL

bool q_resolveOpenSslSymbols()
{
#ifdef QT_NO_OPENSSL
    return false;
#endif
    return true;
}
#endif // !defined QT_LINKED_OPENSSL

//==============================================================================
// contributed by Jay Case of Sarvega, Inc.; http://sarvega.com/
// Based on X509_cmp_time() for intitial buffer hacking.
//==============================================================================
QDateTime q_getTimeFromASN1(const ASN1_TIME *aTime)
{
    size_t lTimeLength = aTime->length;
    char *pString = (char *) aTime->data;
    auto isValidPointer = [pString, lTimeLength](const char *const probe){
        return size_t(probe - pString) < lTimeLength;
    };

    if (aTime->type == V_ASN1_UTCTIME) {

        char lBuffer[24];
        char *pBuffer = lBuffer;

        if ((lTimeLength < 11) || (lTimeLength > 17))
            return QDateTime();

        memcpy(pBuffer, pString, 10);
        pBuffer += 10;
        pString += 10;

        if ((*pString == 'Z') || (*pString == '-') || (*pString == '+')) {
            *pBuffer++ = '0';
            *pBuffer++ = '0';
        } else {
            *pBuffer++ = *pString++;
            if (!isValidPointer(pString)) // Nah.
                return {};
            *pBuffer++ = *pString++;
            if (!isValidPointer(pString)) // Nah.
                return {};
            // Skip any fractional seconds...
            if (*pString == '.') {
                pString++;
                if (!isValidPointer(pString)) // Oh no, cannot dereference (see below).
                    return {};
                while ((*pString >= '0') && (*pString <= '9')) {
                    pString++;
                    if (!isValidPointer(pString)) // No and no.
                        return {};
                }
            }
        }

        *pBuffer++ = 'Z';
        *pBuffer++ = '\0';

        time_t lSecondsFromUCT;
        if (*pString == 'Z') {
            lSecondsFromUCT = 0;
        } else {
            if ((*pString != '+') && (*pString != '-'))
                return QDateTime();

            if (!isValidPointer(pString + 4)) {
                // What kind of input parameters we were provided with? To hell with them!
                return {};
            }
            lSecondsFromUCT = ((pString[1] - '0') * 10 + (pString[2] - '0')) * 60;
            lSecondsFromUCT += (pString[3] - '0') * 10 + (pString[4] - '0');
            lSecondsFromUCT *= 60;
            if (*pString == '-')
                lSecondsFromUCT = -lSecondsFromUCT;
        }

        tm lTime;
        lTime.tm_sec = ((lBuffer[10] - '0') * 10) + (lBuffer[11] - '0');
        lTime.tm_min = ((lBuffer[8] - '0') * 10) + (lBuffer[9] - '0');
        lTime.tm_hour = ((lBuffer[6] - '0') * 10) + (lBuffer[7] - '0');
        lTime.tm_mday = ((lBuffer[4] - '0') * 10) + (lBuffer[5] - '0');
        lTime.tm_mon = (((lBuffer[2] - '0') * 10) + (lBuffer[3] - '0')) - 1;
        lTime.tm_year = ((lBuffer[0] - '0') * 10) + (lBuffer[1] - '0');
        if (lTime.tm_year < 50)
            lTime.tm_year += 100; // RFC 2459

        QDate resDate(lTime.tm_year + 1900, lTime.tm_mon + 1, lTime.tm_mday);
        QTime resTime(lTime.tm_hour, lTime.tm_min, lTime.tm_sec);

        QDateTime result(resDate, resTime, Qt::UTC);
        result = result.addSecs(lSecondsFromUCT);
        return result;

    } else if (aTime->type == V_ASN1_GENERALIZEDTIME) {

        if (lTimeLength < 15)
            return QDateTime(); // hopefully never triggered

        // generalized time is always YYYYMMDDHHMMSSZ (RFC 2459, section 4.1.2.5.2)
        tm lTime;
        lTime.tm_sec = ((pString[12] - '0') * 10) + (pString[13] - '0');
        lTime.tm_min = ((pString[10] - '0') * 10) + (pString[11] - '0');
        lTime.tm_hour = ((pString[8] - '0') * 10) + (pString[9] - '0');
        lTime.tm_mday = ((pString[6] - '0') * 10) + (pString[7] - '0');
        lTime.tm_mon = (((pString[4] - '0') * 10) + (pString[5] - '0'));
        lTime.tm_year = ((pString[0] - '0') * 1000) + ((pString[1] - '0') * 100) +
                        ((pString[2] - '0') * 10) + (pString[3] - '0');

        QDate resDate(lTime.tm_year, lTime.tm_mon, lTime.tm_mday);
        QTime resTime(lTime.tm_hour, lTime.tm_min, lTime.tm_sec);

        QDateTime result(resDate, resTime, Qt::UTC);
        return result;

    } else {
        qCWarning(lcSsl, "unsupported date format detected");
        return QDateTime();
    }

}

QT_END_NAMESPACE
