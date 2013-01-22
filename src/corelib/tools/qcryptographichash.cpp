/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <qcryptographichash.h>

#include "../../3rdparty/md5/md5.h"
#include "../../3rdparty/md5/md5.cpp"
#include "../../3rdparty/md4/md4.h"
#include "../../3rdparty/md4/md4.cpp"
#include "../../3rdparty/sha1/sha1.cpp"

/*
    These #defines replace the typedefs needed by the RFC6234 code. Normally
    the typedefs would come from from stdint.h, but since this header is not
    available on all platforms (MSVC 2008, for example), we #define them to the
    Qt equivalents.
*/
#define uint64_t QT_PREPEND_NAMESPACE(quint64)
#define uint32_t QT_PREPEND_NAMESPACE(quint32)
#define uint8_t QT_PREPEND_NAMESPACE(quint8)
#define int_least16_t QT_PREPEND_NAMESPACE(qint16)

// Header from rfc6234 with 1 modification:
// sha1.h - commented out '#include <stdint.h>' on line 74
#include "../../3rdparty/rfc6234/sha.h"

/*
    These 2 functions replace macros of the same name in sha224-256.c and
    sha384-512.c. Originally, these macros relied on a global static 'addTemp'
    variable. We do not want this for 2 reasons:

    1. since we are including the sources directly, the declaration of the 2 conflict

    2. static variables are not thread-safe, we do not want multiple threads
    computing a hash to corrupt one another
*/
static int SHA224_256AddLength(SHA256Context *context, unsigned int length);
static int SHA384_512AddLength(SHA512Context *context, unsigned int length);

// Sources from rfc6234, with 4 modifications:
// sha224-256.c - commented out 'static uint32_t addTemp;' on line 68
// sha224-256.c - appended 'M' to the SHA224_256AddLength macro on line 70
#include "../../3rdparty/rfc6234/sha224-256.c"
// sha384-512.c - commented out 'static uint64_t addTemp;' on line 302
// sha384-512.c - appended 'M' to the SHA224_256AddLength macro on line 304
#include "../../3rdparty/rfc6234/sha384-512.c"

#undef uint64_t
#undef uint32_t
#undef uint68_t
#undef int_least16_t

#include <qiodevice.h>

static inline int SHA224_256AddLength(SHA256Context *context, unsigned int length)
{
  QT_PREPEND_NAMESPACE(quint32) addTemp;
  return SHA224_256AddLengthM(context, length);
}
static inline int SHA384_512AddLength(SHA512Context *context, unsigned int length)
{
  QT_PREPEND_NAMESPACE(quint64) addTemp;
  return SHA384_512AddLengthM(context, length);
}

QT_BEGIN_NAMESPACE

class QCryptographicHashPrivate
{
public:
    QCryptographicHash::Algorithm method;
    union {
        MD5Context md5Context;
        md4_context md4Context;
        Sha1State sha1Context;
        SHA224Context sha224Context;
        SHA256Context sha256Context;
        SHA384Context sha384Context;
        SHA512Context sha512Context;
    };
    QByteArray result;
};

/*!
  \class QCryptographicHash
  \inmodule QtCore

  \brief The QCryptographicHash class provides a way to generate cryptographic hashes.

  \since 4.3

  \ingroup tools
  \reentrant

  QCryptographicHash can be used to generate cryptographic hashes of binary or text data.

  Currently MD4, MD5, SHA-1, SHA-224, SHA-256, SHA-384, and SHA-512 are supported.
*/

/*!
  \enum QCryptographicHash::Algorithm

  \value Md4 Generate an MD4 hash sum
  \value Md5 Generate an MD5 hash sum
  \value Sha1 Generate an SHA-1 hash sum
  \value Sha224 Generate an SHA-224 hash sum. Introduced in Qt 5.0
  \value Sha256 Generate an SHA-256 hash sum. Introduced in Qt 5.0
  \value Sha384 Generate an SHA-384 hash sum. Introduced in Qt 5.0
  \value Sha512 Generate an SHA-512 hash sum. Introduced in Qt 5.0
*/

/*!
  Constructs an object that can be used to create a cryptographic hash from data using \a method.
*/
QCryptographicHash::QCryptographicHash(Algorithm method)
    : d(new QCryptographicHashPrivate)
{
    d->method = method;
    reset();
}

/*!
  Destroys the object.
*/
QCryptographicHash::~QCryptographicHash()
{
    delete d;
}

/*!
  Resets the object.
*/
void QCryptographicHash::reset()
{
    switch (d->method) {
    case Md4:
        md4_init(&d->md4Context);
        break;
    case Md5:
        MD5Init(&d->md5Context);
        break;
    case Sha1:
        sha1InitState(&d->sha1Context);
        break;
    case Sha224:
        SHA224Reset(&d->sha224Context);
        break;
    case Sha256:
        SHA256Reset(&d->sha256Context);
        break;
    case Sha384:
        SHA384Reset(&d->sha384Context);
        break;
    case Sha512:
        SHA512Reset(&d->sha512Context);
        break;
    }
    d->result.clear();
}

/*!
    Adds the first \a length chars of \a data to the cryptographic
    hash.
*/
void QCryptographicHash::addData(const char *data, int length)
{
    switch (d->method) {
    case Md4:
        md4_update(&d->md4Context, (const unsigned char *)data, length);
        break;
    case Md5:
        MD5Update(&d->md5Context, (const unsigned char *)data, length);
        break;
    case Sha1:
        sha1Update(&d->sha1Context, (const unsigned char *)data, length);
        break;
    case Sha224:
        SHA224Input(&d->sha224Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case Sha256:
        SHA256Input(&d->sha256Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case Sha384:
        SHA384Input(&d->sha384Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case Sha512:
        SHA512Input(&d->sha512Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    }    
    d->result.clear();
}

/*!
  \overload addData()
*/
void QCryptographicHash::addData(const QByteArray &data)
{
    addData(data.constData(), data.length());
}

/*!
  Reads the data from the open QIODevice \a device until it ends
  and hashes it. Returns true if reading was successful.
  \since 5.0
 */
bool QCryptographicHash::addData(QIODevice* device)
{
    if (!device->isReadable())
        return false;

    if (!device->isOpen())
        return false;

    char buffer[1024];
    int length;

    while ((length = device->read(buffer,sizeof(buffer))) > 0)
        addData(buffer,length);

    return device->atEnd();
}


/*!
  Returns the final hash value.

  \sa QByteArray::toHex()
*/
QByteArray QCryptographicHash::result() const
{
    if (!d->result.isEmpty()) 
        return d->result;

    switch (d->method) {
    case Md4: {
        md4_context copy = d->md4Context;
        d->result.resize(MD4_RESULTLEN);
        md4_final(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Md5: {
        MD5Context copy = d->md5Context;
        d->result.resize(16);
        MD5Final(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Sha1: {
        Sha1State copy = d->sha1Context;
        d->result.resize(20);
        sha1FinalizeState(&copy);
        sha1ToHash(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Sha224: {
        SHA224Context copy = d->sha224Context;
        d->result.resize(SHA224HashSize);
        SHA224Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case Sha256:{
        SHA256Context copy = d->sha256Context;
        d->result.resize(SHA256HashSize);
        SHA256Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case Sha384:{
        SHA384Context copy = d->sha384Context;
        d->result.resize(SHA384HashSize);
        SHA384Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case Sha512:{
        SHA512Context copy = d->sha512Context;
        d->result.resize(SHA512HashSize);
        SHA512Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    }
    return d->result;
}

/*!
  Returns the hash of \a data using \a method.
*/
QByteArray QCryptographicHash::hash(const QByteArray &data, Algorithm method)
{
    QCryptographicHash hash(method);
    hash.addData(data);
    return hash.result();
}

QT_END_NAMESPACE
