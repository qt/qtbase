/****************************************************************************
**
** Copyright (C) 2014 Intel Corporation
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FORKFD_H
#define FORKFD_H

#include <fcntl.h>
#include <unistd.h> // to get the POSIX flags

#ifdef _POSIX_SPAWN
#  include <spawn.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FFD_CLOEXEC  1
#define FFD_NONBLOCK 2

#define FFD_CHILD_PROCESS (-2)

int forkfd(int flags, pid_t *ppid);

#ifdef _POSIX_SPAWN
/* only for spawnfd: */
#  define FFD_SPAWN_SEARCH_PATH   O_RDWR

int spawnfd(int flags, pid_t *ppid, const char *path, const posix_spawn_file_actions_t *file_actions,
            posix_spawnattr_t *attrp, char *const argv[], char *const envp[]);
#endif

#ifdef __cplusplus
}
#endif

#endif // FORKFD_H
