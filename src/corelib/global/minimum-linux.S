/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "minimum-linux_p.h"

/* Copied from #include <elf.h>:
 */
#define ELF_NOTE_GNU            "GNU"
#define NT_GNU_ABI_TAG          1
#define ELF_NOTE_OS_LINUX       0

#ifdef __arm__
#  define progbits              %progbits
#  define note                  %note
#else
#  define progbits              @progbits
#  define note                  @note
#endif

/* Add information for the ELF dynamic linker what the minimum Linux version
 * required for Qt is.
 *
 * The .note.ABI-tag note section is defined at
 *  https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/noteabitag.html
 */

    .section    ".note.GNU-stack", "", progbits
    .section    ".note.ABI-tag", "a", note
    .align      4       /* we have 32-bit data */

/*  * For the format of the note section's contents, see Elf32_Nhdr / Elf64_Nhdr */
    .long       .Lnameend-.Lname        /* n_namesz */
    .long       16                      /* n_descsz(16 bytes, normative) */
    .long       NT_GNU_ABI_TAG          /* n_type */

.Lname:
    .asciz      ELF_NOTE_GNU
.Lnameend:

/* Operating systems: */
    .long       ELF_NOTE_OS_LINUX

    .long       MINLINUX_MAJOR
    .long       MINLINUX_MINOR
    .long       MINLINUX_PATCH
