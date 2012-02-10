/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPROCESSORDETECTION_H
#define QPROCESSORDETECTION_H

/*
    This file uses preprocessor #defines to set various Q_PROCESSOR_* #defines
    based on the following patterns:

    Q_PROCESSOR_{FAMILY}
    Q_PROCESSOR_{FAMILY}_{VARIANT}
    Q_PROCESSOR_{FAMILY}_{REVISION}

    The first is always defined. Defines for the various revisions/variants are
    optional and usually dependent on how the compiler was invoked. Variants
    that are a superset of another should have a define for the superset.
*/

/*
    Alpha family, no revisions or variants
*/
// #elif defined(__alpha__) || defined(_M_ALPHA)
// #  define Q_PROCESSOR_ALPHA

/*
  ARM family, known revisions: V5, V6, and V7
*/
#if defined(__arm__) || defined(__TARGET_ARCH_ARM)
#  define Q_PROCESSOR_ARM
#  if defined(__ARM_ARCH_7__) \
      || defined(__ARM_ARCH_7A__) \
      || defined(__ARM_ARCH_7R__) \
      || defined(__ARM_ARCH_7M__) \
      || (__TARGET_ARCH_ARM-0 >= 7)
#    define Q_PROCESSOR_ARM_V7
#    define Q_PROCESSOR_ARM_V6
#    define Q_PROCESSOR_ARM_V5
#  elif defined(__ARM_ARCH_6__) \
      || defined(__ARM_ARCH_6J__) \
      || defined(__ARM_ARCH_6T2__) \
      || defined(__ARM_ARCH_6Z__) \
      || defined(__ARM_ARCH_6K__) \
      || defined(__ARM_ARCH_6ZK__) \
      || defined(__ARM_ARCH_6M__) \
      || (__TARGET_ARCH_ARM-0 >= 6)
#    define Q_PROCESSOR_ARM_V6
#    define Q_PROCESSOR_ARM_V5
#  elif defined(__ARM_ARCH_5TEJ__) \
        || (__TARGET_ARCH_ARM-0 >= 5)
#    define Q_PROCESSOR_ARM_V5
#  endif

/*
    AVR32 family, no revisions or variants
*/
// #elif defined(__avr32__)
// #  define Q_PROCESSOR_AVR32

/*
    Blackfin family, no revisions or variants
*/
// #elif defined(__bfin__)
// #  define Q_PROCESSOR_BLACKFIN

/*
    X86 family, known variants: 32- and 64-bit
*/
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#  define Q_PROCESSOR_X86
#  define Q_PROCESSOR_X86_32
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#  define Q_PROCESSOR_X86
#  define Q_PROCESSOR_X86_64

/*
    Itanium (IA-64) family, no revisions or variants
*/
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#  define Q_PROCESSOR_IA64

/*
    MIPS family, known revisions: I, II, III, IV, 32, 64
*/
#elif defined(__mips) || defined(__mips__) || defined(_M_MRX000)
#  define Q_PROCESSOR_MIPS
#  if defined(_MIPS_ARCH_MIPS1) || (defined(__mips) && __mips - 0 >= 1)
#    define Q_PROCESSOR_MIPS_I
#  endif
#  if defined(_MIPS_ARCH_MIPS2) || (defined(__mips) && __mips - 0 >= 2)
#    define Q_PROCESSOR_MIPS_II
#  endif
#  if defined(_MIPS_ARCH_MIPS32) || defined(__mips32)
#    define Q_PROCESSOR_MIPS_32
#  endif
#  if defined(_MIPS_ARCH_MIPS3) || (defined(__mips) && __mips - 0 >= 3)
#    define Q_PROCESSOR_MIPS_III
#  endif
#  if defined(_MIPS_ARCH_MIPS4) || (defined(__mips) && __mips - 0 >= 4)
#    define Q_PROCESSOR_MIPS_IV
#  endif
#  if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
#    define Q_PROCESSOR_MIPS_64
#  endif

/*
    PA-RISC family, no revisions or variants
*/
// #elif defined(__parisc__)
// #  define Q_PROCESSOR_PARISC

/*
    POWER family, optional variant: 64-bit

    There are many more known variants/revisions that we do not handle/detect.
    See http://en.wikipedia.org/wiki/Power_Architecture
    and http://en.wikipedia.org/wiki/File:PowerISA-evolution.svg
*/
// #elif defined(__powerpc__) || defined(__ppc__) || defined(_M_MPPC) || defined(_M_PPC)
// #  define Q_PROCESSOR_POWERPC
// #  if defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
// #    define Q_PROCESSOR_POWERPC_64
// #  endif

/*
    S390 family, known variant: S390X (64-bit)
*/
// #elif defined(__s390__)
// #  define Q_PROCESSOR_S390
// #  if defined(__s390x__)
// #    define Q_PROCESSOR_S390_X
// #  endif

/*
    SuperH family, optional revision: SH-4A
*/
// #elif defined(__sh__)
// #  define Q_PROCESSOR_SH
// #  if defined(__sh4a__)
// #    define Q_PROCESSOR_SH_4A
// #  endif

/*
    SPARC family, optional revision: V9
*/
// #elif defined(__sparc__)
// #  define Q_PROCESSOR_SPARC
// #  if defined(__sparc_v9__)
// #    define Q_PROCESSOR_SPARC_V9
// #  endif

#endif

#endif // QPROCESSORDETECTION_H
