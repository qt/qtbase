/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwssharedmemory_p.h"

#if !defined(QT_NO_QWS_MULTIPROCESS)

#include <sys/shm.h>

QT_BEGIN_NAMESPACE

QWSSharedMemory::QWSSharedMemory()
    : shmBase(0), shmSize(0), character(0),  shmId(-1), key(-1)
{
}


QWSSharedMemory::~QWSSharedMemory()
{
    detach();
}

/*
  man page says:
    On  Linux,  it is possible to attach a shared memory segment even if it
    is already marked to be deleted.  However, POSIX.1-2001 does not  spec-
    ify this behaviour and many other implementations do not support it.
*/

bool QWSSharedMemory::create(int size)
{
    if (shmId != -1)
        detach();
    shmId = shmget(IPC_PRIVATE, size, IPC_CREAT|0600);

    if (shmId == -1) {
#ifdef QT_SHM_DEBUG
        perror("QWSSharedMemory::create allocating shared memory");
        qWarning("Error allocating shared memory of size %d", size);
#endif
        return false;
    }
    shmBase = shmat(shmId,0,0);
    shmctl(shmId, IPC_RMID, 0);
    if (shmBase == (void*)-1) {
#ifdef QT_SHM_DEBUG
        perror("QWSSharedMemory::create attaching to shared memory");
        qWarning("Error attaching to shared memory id %d", shmId);
#endif
        shmBase = 0;
        return false;
    }
    return true;
}

bool QWSSharedMemory::attach(int id)
{
    if (shmId == id)
        return id != -1;
    if (shmId != -1)
        detach();

    shmBase = shmat(id,0,0);
    if (shmBase == (void*)-1) {
#ifdef QT_SHM_DEBUG
        perror("QWSSharedMemory::attach attaching to shared memory");
        qWarning("Error attaching to shared memory 0x%x of size %d",
                 id, size());
#endif
        shmBase = 0;
        return false;
    }
    shmId = id;
    return true;
}


void QWSSharedMemory::detach ()
{
    if (!shmBase)
        return;
    shmdt (shmBase);
    shmBase = 0;
    shmSize = 0;
    shmId = -1;
}

void QWSSharedMemory::setPermissions (mode_t mode)
{
  struct shmid_ds shm;
  shmctl (shmId, IPC_STAT, &shm);
  shm.shm_perm.mode = mode;
  shmctl (shmId, IPC_SET, &shm);
}

int QWSSharedMemory::size () const
{
    struct shmid_ds shm;
    shmctl (shmId, IPC_STAT, &shm);
    return shm.shm_segsz;
}


// old API



QWSSharedMemory::QWSSharedMemory (int size, const QString &filename, char c)
{
  shmSize = size;
  shmFile = filename;
  shmBase = 0;
  shmId = -1;
  character = c;
  key = ftok (shmFile.toLatin1().constData(), c);
}



bool QWSSharedMemory::create ()
{
  shmId = shmget (key, shmSize, IPC_CREAT | 0666);
  return (shmId != -1);
}

void QWSSharedMemory::destroy ()
{
    if (shmId != -1)
        shmctl(shmId, IPC_RMID, 0);
}

bool QWSSharedMemory::attach ()
{
  if (shmId == -1)
    shmId = shmget (key, shmSize, 0);

  shmBase = shmat (shmId, 0, 0);
  if ((long)shmBase == -1)
      shmBase = 0;

  return (long)shmBase != 0;
}


QT_END_NAMESPACE

#endif // QT_NO_QWS_MULTIPROCESS
