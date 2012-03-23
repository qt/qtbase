/****************************************************************************
**
** Copyright (C) 2012 MIPS Technologies, www.mips.com, author Damir Tatalovic <dtatalovic@mips.com>
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt_mips_asm_dsp.h"

LEAF_MIPS_DSPR2(INTERPOLATE_PIXEL_255_asm_mips_dspr2)
/*
 * a0 - uint x (First value to multiply)
 * a1 - uint a (Multiplicator byte for first value)
 * a2 - uint y (Second value to multiply)
 * a3 - uint b (Multiplicator byte for second value)
 */

    .set reorder
    replv.ph          a1, a1
    replv.ph          a3, a3
    li                t8, 8388736
    muleu_s.ph.qbl    t0, a0, a1
    muleu_s.ph.qbl    t1, a2, a3
    muleu_s.ph.qbr    t2, a0, a1
    muleu_s.ph.qbr    t3, a2, a3
    addu.ph           t4, t0, t1
    addu.ph           t5, t2, t3
    preceu.ph.qbla    t0, t4
    addu              t1, t0, t8
    addu              t1, t4, t1
    preceu.ph.qbla    t6, t5
    addu              t7, t6, t8
    addu              t7, t5, t7
    precrq.qb.ph      t2, t1, t7
    move              v0, t2
    j                 ra

END(INTERPOLATE_PIXEL_255_asm_mips_dspr2)

LEAF_MIPS_DSPR2(BYTE_MUL_asm_mips_dspr2)
/*
 * a0 - uint x (Value to multiply)
 * a1 - uint a (Multiplicator byte)
 */

    .set reorder
    replv.ph          a1, a1              /* a1 = 0x00a00a */
    li                t4, 8388736         /* t4 = 0x800080 */
    muleu_s.ph.qbl    t0, a0, a1
    muleu_s.ph.qbr    t2, a0, a1
    preceu.ph.qbla    t1, t0
    addu              t0, t0, t1
    addu              t0, t0, t4
    preceu.ph.qbla    t3, t2
    addu              t2, t2, t3
    addu              t2, t2, t4
    precrq.qb.ph      t4, t0, t2
    move              v0, t4
    j                 ra

END(BYTE_MUL_asm_mips_dspr2)
