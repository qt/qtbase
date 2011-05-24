/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTRANSPORTAUTHDEFS_QWS_H
#define QTRANSPORTAUTHDEFS_QWS_H

#include <sys/types.h>
#include <string.h>

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#define QSXE_KEY_LEN 16
#define QSXE_MAGIC_BYTES 4

// Number of bytes of each message to authenticate.  Just need to ensure
// that the command at the beginning hasn't been tampered with.  This value
// does not matter for trusted transports.
#define AMOUNT_TO_AUTHENTICATE 200

#define AUTH_ID(k) ((unsigned char)(k[QSXE_KEY_LEN]))
#define AUTH_KEY(k) ((unsigned char *)(k))

// must be a largish -ve number under any endianess when cast as an int
const unsigned char magic[QSXE_MAGIC_BYTES] = { 0xBA, 0xD4, 0xD4, 0xBA };
const int magicInt = 0xBAD4D4BA;

#define QSXE_KEYFILE "keyfile"

/*
  Header in above format, less the magic bytes.
  Useful for reading off the socket
*/
struct AuthHeader
{
    unsigned char len;
    unsigned char pad;
    unsigned char digest[QSXE_KEY_LEN];
    unsigned char id;
    unsigned char seq;
};

/*
  Header in a form suitable for authentication routines
*/
struct AuthMessage
{
    AuthMessage()
    {
        ::memset( authData, 0, sizeof(authData) );
        ::memcpy( pad_magic, magic, QSXE_MAGIC_BYTES );
    }
    unsigned char pad_magic[QSXE_MAGIC_BYTES];
    union {
        AuthHeader hdr;
        char authData[sizeof(AuthHeader)];
    };
    char payLoad[AMOUNT_TO_AUTHENTICATE];
};

/**
  Auth data as stored in _key
*/
struct AuthCookie
{
    unsigned char key[QSXE_KEY_LEN];
    unsigned char pad;
    unsigned char progId;
};

/*
  Auth data as written to the key file - SUPERSEDED by usr_key_entry

  This is still used internally for some functions, ie the socket
  related calls.
*/
struct AuthRecord
{
    union {
        AuthCookie auth;
        char data[sizeof(struct AuthCookie)];
    };
    time_t change_time;
};

/*!
  \class usr_key_entry
  This comes from the SXE kernel patch file include/linux/lidsif.h

  This is the (new) data record for the key file (version 2).

  The key file is (now) either /proc/lids/keys (and the per-process
  keys in /proc/<pid>/lids_key) OR for desktop/development ONLY (not
  for production) it is $QPEDIR/etc/keyfile

  The key file maps keys to files.

  File are identified by inode and device numbers, not paths.

  (See the "installs" file for path to inode/device mapping)
*/
struct usr_key_entry
{
    char key[QSXE_KEY_LEN];
    ino_t ino;
    dev_t dev;
};


/*!
  \class IdBlock
  \brief Data record for the manifest file.
  The manifest file maps program id's to files
*/
struct IdBlock
{
    quint64 inode;
    quint64 device;
    unsigned char pad;
    unsigned char progId;
    unsigned short installId;
    unsigned int keyOffset;
    qint64 install_time;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTRANSPORTAUTHDEFS_QWS_H

