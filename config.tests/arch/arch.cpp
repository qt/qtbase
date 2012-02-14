/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the FOO module of the Qt Toolkit.
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

// NOTE: This file is not meant to be compiled, only preprocessed.
#include "../../src/corelib/global/qprocessordetection.h"
#if defined(Q_PROCESSOR_ALPHA)
alpha
#elif defined(Q_PROCESSOR_ARM)
arm
#elif defined(Q_PROCESSOR_AVR32)
avr32
#elif defined(Q_PROCESSOR_BLACKFIN)
bfin
#elif defined(Q_PROCESSOR_X86_32)
i386
#elif defined(Q_PROCESSOR_X86_64)
x86_64
#elif defined(Q_PROCESSOR_IA64)
ia64
#elif defined(Q_PROCESSOR_MIPS)
mips
#elif defined(Q_PROCESSOR_POWER)
power
#elif defined(Q_PROCESSOR_S390)
s390
#elif defined(Q_PROCESSOR_SH)
sh
#elif defined(Q_PROCESSOR_SPARC)
sparc
#else
unknown
#endif
