/****************************************************************************
**
** Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
** Contact: http://www.qt.io/licensing/
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

#include <QtTest/QtTest>
#include <QSslDiffieHellmanParameters>
#include <QSslSocket>
#include <QByteArray>

class tst_QSslDiffieHellmanParameters : public QObject
{
    Q_OBJECT

#ifndef QT_NO_SSL
private Q_SLOTS:
    void constructionEmpty();
    void constructionDefault();
    void constructionDER();
    void constructionPEM();
    void unsafe512Bits();
    void unsafeNonPrime();
#endif
};

#ifndef QT_NO_SSL

void tst_QSslDiffieHellmanParameters::constructionEmpty()
{
    QSslDiffieHellmanParameters dh;

    QCOMPARE(dh.isEmpty(), true);
    QCOMPARE(dh.isValid(), true);
    QCOMPARE(dh.error(), QSslDiffieHellmanParameters::NoError);
}

void tst_QSslDiffieHellmanParameters::constructionDefault()
{
    QSslDiffieHellmanParameters dh = QSslDiffieHellmanParameters::defaultParameters();

#ifndef QT_NO_OPENSSL
    QCOMPARE(dh.isValid(), true);
    QCOMPARE(dh.error(), QSslDiffieHellmanParameters::NoError);
#endif
}

void tst_QSslDiffieHellmanParameters::constructionDER()
{
    // Uniquely generated with 'openssl dhparam -outform DER -out out.der -check -2 4096'
    const auto dh = QSslDiffieHellmanParameters::fromEncoded(QByteArray::fromBase64(QByteArrayLiteral(
        "MIICCAKCAgEAsbQYx57ZlyEyWF8jD5WYEswGR2aTVFsHqP3026SdyTwcjY+YlMOae0EagK"
        "jDA0UlPcih1kguQOvOVgyc5gI3YbBb4pCNEdy048xITlsdqG7qC3+2VvFR3vfixEbQQll9"
        "2cGIIneD/36p7KJcDnBNUwwWj/VJKhTwelTfKTj2T39si9xGMkqZiQuCaXRk6vSKZ4ZDPk"
        "jiq5Ti1kHVFbL9SMWRa8zplPtDMrVfhSyw10njgD4qKd1UoUPdmhEPhRZlHaZ/cAHNSHMj"
        "uhDakeMpN+XP2/sl5IpPZ3/vVOk9PhBDFO1NYzKx/b7RQgZCUmXoglKYpfBiz8OheoI0hK"
        "V0fU/OCtHjRrP4hE9vIHA2aE+gaQZiYCciGcR9BjHQ7Y8K9qHyTX8UIz2G4ZKzQZK9G+pA"
        "K0xD+1H3qZ/MaUhzNDQOwwihnTjjXzTjfIGqYDdbouAhw+tX51CsGonI0cL3s3QMa3CwGH"
        "mw+AH2b/Z68dTSy0sC3CYn9cNbrctqyeHwQrsx9FfpOz+Z6sk2WsPgqgSp/pDVVgm5oSfO"
        "2mN7WAWgUlf9TQuj1HIRCTI+PbBq2vYvn+YResMRo+8ng1QptKAAgQoVVGNRYxZ9iAZlvO"
        "52DcHKlsqDuafQ1XVGmzVIrKtBi2gfLtPqY4v6g6v26l8gbzK67PpWstllHiPb4VMCAQI="
    )), QSsl::Der);

#ifndef QT_NO_OPENSSL
    QCOMPARE(dh.isValid(), true);
    QCOMPARE(dh.error(), QSslDiffieHellmanParameters::NoError);
#endif
}

void tst_QSslDiffieHellmanParameters::constructionPEM()
{
    // Uniquely generated with 'openssl dhparam -outform PEM -out out.pem -check -2 4096'
    const auto dh = QSslDiffieHellmanParameters::fromEncoded(QByteArrayLiteral(
        "-----BEGIN DH PARAMETERS-----\n"
        "MIICCAKCAgEA9QTdqhQkbGuhWzBsW5X475AjjrITpg1BHX5+mp1sstUd84Lshq1T\n"
        "+S2QQQtdl25EPoUblpyyLAf8krFSH4YwR7jjLWklA8paDOwRYod0zLmVZ1Wx6og3\n"
        "PRc8P+SCs+6gKTXfv//bJJhiJXnM73lDFsGHbSqN+msf20ei/zy5Rwey2t8dPjLC\n"
        "Q+qkb/avlovi2t2rsUWcxMT1875TQ4HuApayqw3R3lTQe9u05b9rTrinmT7AE4mm\n"
        "xGqO9FZJdXYE2sOKwwJkpM48KFyV90uJANmqJnQrkgdukaGTHwxZxgAyO6ur/RWC\n"
        "kzf9STFT6IY4Qy05q+oZVJfh8xPHszKmmC8nWaLfiHMYBnL5fv+1kh/aU11Kz9TG\n"
        "iDXwQ+tzhKAutQPUwe3IGQUYQMZPwZI4vegdU88/7YPXuWt7b/0Il5+2ma5FbtG2\n"
        "u02PMi+J3JZsYi/tEUv1tJBVHGH0kDpgcyOm8rvkCtNbNkETzfwUPoEgA0oPMhVt\n"
        "sFGub1av+jLRyFNGNBJcqXAO+Tq2zXG00DxbGY+aooJ50qU/Lh5gfnCEMDXlMM9P\n"
        "T8JVpWaaNLCC+0Z5txsfYp+FO8mOttIPIF6F8FtmTnm/jhNntvqKvsU+NHylIYzr\n"
        "o42EpiWwS7ktPPUS2GtG+IUdy8rvdO1xJ5kNxs7ZlygY4W1htOhbUusCAQI=\n"
        "-----END DH PARAMETERS-----\n"
    ), QSsl::Pem);

#ifndef QT_NO_OPENSSL
    QCOMPARE(dh.isValid(), true);
    QCOMPARE(dh.error(), QSslDiffieHellmanParameters::NoError);
#endif
}

void tst_QSslDiffieHellmanParameters::unsafe512Bits()
{
    // Uniquely generated with 'openssl dhparam -outform PEM -out out.pem -check -2 512'
    const auto dh = QSslDiffieHellmanParameters::fromEncoded(QByteArrayLiteral(
        "-----BEGIN DH PARAMETERS-----\n"
        "MEYCQQCf8goDn56akiliAtEL1ZG7VH+9wfLxsv8/B1emTUG+rMKB1yaVAU7HaAiM\n"
        "Gtmo2bAWUqBczUTOTzqmWTm28P6bAgEC\n"
        "-----END DH PARAMETERS-----\n"
    ), QSsl::Pem);

#ifndef QT_NO_OPENSSL
    QCOMPARE(dh.isValid(), false);
    QCOMPARE(dh.error(), QSslDiffieHellmanParameters::UnsafeParametersError);
#endif
}

void tst_QSslDiffieHellmanParameters::unsafeNonPrime()
{
    // Uniquely generated with 'openssl dhparam -outform DER -out out.der -check -2 1024'
    // and then modified by hand to make P not be a prime number.
    const auto dh = QSslDiffieHellmanParameters::fromEncoded(QByteArray::fromBase64(QByteArrayLiteral(
        "MIGHAoGBALLcOLg+ow8TMnbCUeNjwys6wUTIH9mn4ZSeIbD6qvCsJgg4cUxXwJQmPY"
        "Xl15AsKXgkXWh0n+/N6tjH0sSRJnzDvN2H3KxFLKkvxmBYrDOJMdCuMgZD50aOsVyd"
        "vholAW9zilkoYkB6sqwxY1Z2dbpTWajCsUAWZQ0AIP4Y5nesAgEC"
    )), QSsl::Der);

#ifndef QT_NO_OPENSSL
    QCOMPARE(dh.isValid(), false);
    QCOMPARE(dh.error(), QSslDiffieHellmanParameters::UnsafeParametersError);
#endif
}

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslDiffieHellmanParameters)
#include "tst_qssldiffiehellmanparameters.moc"
