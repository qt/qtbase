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

LEAF_MIPS_DSP(INTERPOLATE_PIXEL_255_asm_mips_dsp)
/*
 * a0 - uint x (First value to multiply)
 * a1 - uint a (Multiplicator byte for first value)
 * a2 - uint y (Second value to multiply)
 * a3 - uint b (Multiplicator byte for second value)
 */

    .set reorder
    li                t4, 8388736
    preceu.ph.qbra    t0, a0        /* (x & 0xff00ff) */
    mul               t0, t0, a1    /* (x & 0xff00ff) * a */
    preceu.ph.qbra    t1, a2        /* (y & 0xff00ff) */
    mul               t1, t1, a3    /* (y & 0xff00ff) * b */
    addu              t0, t0, t1    /* (x & 0xff00ff) * a +
                                     * (y & 0xff00ff) * b
                                     */
    preceu.ph.qbla    t1, t0        /* (t >> 8) & 0xff00ff */
    addu              t0, t0, t1    /* t + ((t >> 8) & 0xff00ff */
    addu              t0, t0, t4    /* t + ((t >> 8) & 0xff00ff) + 0x800080 */
    preceu.ph.qbla    t0, t0        /* t >> 8 and t&=0xff00ff */
    preceu.ph.qbla    t2, a0        /* (x>>8) & 0xff00ff */
    mul               t2, t2, a1    /* ((x>>8) & 0xff00ff) * a */
    preceu.ph.qbla    t3, a2        /* ((y>>8) & 0xff00ff) */
    mul               t3, t3, a3    /* ((y>>8) & 0xff00ff) * b */
    addu              t2, t2, t3    /* ((x>>8) & 0xff00ff) * a +
                                     * ((y >> 8) & 0xff00ff) * b
                                     */
    preceu.ph.qbla    t3, t2        /* (x>>8) & 0xff00ff */
    addu              t2, t2, t3    /* (x>>8) & 0xff00ff) + 0x800080 */
    addu              t2, t2, t4    /* x + ((x>>8) & 0xff00ff) + 0x800080 */
    and               t2, t2, 0xff00ff00
    or                t1, t0, t2
    move              v0, t1
    j                 ra

END(INTERPOLATE_PIXEL_255_asm_mips_dsp)

LEAF_MIPS_DSP(BYTE_MUL_asm_mips_dsp)
/*
 * a0 - uint x (Value to multiply)
 * a1 - uint a (Multiplicator byte)
 */

    .set reorder
    replv.ph          a1, a1         /* a1 = 0x00a00a */
    li                t4, 8388736    /* t4 = 0x800080 */
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

END(BYTE_MUL_asm_mips_dsp)

LEAF_MIPS_DSP(destfetchARGB32_asm_mips_dsp)
/*
 * a0 - buffer address (dst)
 * a1 - data address (src)
 * a2 - length
 */

    beqz              a2, 2f
     move             v0, a0         /* just return the address of buffer
                                      * for storing returning values */
    move              v0, a0
    andi              t1, a2, 0x1
    li                t7, 8388736    /* t7 = 0x800080 */
    beqz              t1, 1f
     nop
    lw                t8, 0(a1)
    addiu             a2, a2, -1
    srl               t6, t8, 24     /* t6 = alpha */

    preceu.ph.qbra    t0, t8
    mul               t1, t0, t6
    preceu.ph.qbla    t4, t8
    mul               t5, t4, t6

    preceu.ph.qbla    t2, t1
    addq.ph           t3, t1, t2
    addq.ph           t3, t3, t7
    preceu.ph.qbla    t1, t3         /* t1 holds R & B blended with alpha
                                      * | 0 | dRab | 0 | dBab | */
    preceu.ph.qbla    t2, t5
    addq.ph           t3, t2, t5
    addq.ph           t4, t3, t7
    preceu.ph.qbla    t2, t4         /* t2 holds A & G blended with alpha
                                      * | 0 | dAab | 0 | dGab | */
    andi              t2, t2, 255    /* t2 = 0xff */

    sll               t0, t6, 24
    sll               t3, t2, 8
    or                t4, t0, t3
    or                t0, t1, t4
    sw                t0, 0(a0)
    addiu             a0, a0, 4
    addiu             a1, a1, 4
    beqz              a2, 2f         /* there was only one member */
     nop
1:
    lw                t0, 0(a1)      /* t0 = src1 */
    lw                t1, 4(a1)      /* t1 = src2 */
    precrq.qb.ph      t4, t0, t1     /* t4 = a1 G1 a2 G2 */
    preceu.ph.qbra    t3, t4         /* t3 = 0 G1 0 G2 */
    preceu.ph.qbla    t2, t4         /* t2 = | 0 | a1 | 0 | a2 | */
    srl               t5, t2, 8
    or                t8, t2, t5     /* t8 = 0 a1 a1 a2 */
    muleu_s.ph.qbr    t5, t8, t3

    addiu             a2, a2, -2
    addiu             a1, a1, 8
    precrq.ph.w       t9, t0, t1
    preceu.ph.qbra    t9, t9

    preceu.ph.qbla    t6, t5
    addq.ph           t5, t5, t6
    addq.ph           t2, t5, t7
    muleu_s.ph.qbr    t6, t8, t9
    sll               t3, t1, 16
    packrl.ph         t3, t0, t3
    preceu.ph.qbra    t3, t3
    muleu_s.ph.qbr    t8, t8, t3
    preceu.ph.qbla    t3, t6
    addq.ph           t3, t6, t3
    addq.ph           t3, t3, t7
    preceu.ph.qbla    t5, t8
    addq.ph           t5, t8, t5
    addq.ph           t5, t5, t7

    precrq.ph.w       t0, t4, t3     /* t0 = | 0 |  a1 | 0 | dR1 | */
    precrq.ph.w       t1, t2, t5     /* t1 = | 0 | dG1 | 0 | dB1 | */
    precrq.qb.ph      t6, t0, t1     /* t6 = | a1 | dR1 | dG1 | dB1 | */
    sll               t3, t3, 16
    sll               t5, t5, 16
    packrl.ph         t0, t4, t3
    packrl.ph         t1, t2, t5
    precrq.qb.ph      t8, t0, t1     /* t8 = | a2 | dR2 | dG2 | dB2 | */
    sw                t6, 0(a0)
    sw                t8, 4(a0)
    bnez              a2, 1b
     addiu            a0, a0, 8
2:
    j                 ra
     nop

END(destfetchARGB32_asm_mips_dsp)

LEAF_MIPS_DSP(qt_memfill32_asm_mips_dsp)
/*
 * a0 - destination address (dst)
 * a1 - value
 * a2 - count
 */

    beqz      a2, 5f
     nop
    li        t8, 8
    andi      t0, a2, 0x7    /* t0 holds how many counts exceeds 8 */
    beqzl     t0, 2f         /* count is multiple of 8 (8, 16, 24, ....) */
     addiu    a2, a2, -8
    subu      a2, a2, t0
1:
    sw        a1, 0(a0)
    addiu     t0, t0, -1
    bnez      t0, 1b
     addiu    a0, a0, 4
    bgeu      a2, t8, 2f
     addiu    a2, a2, -8
    b         5f
     nop
2:
    beqz      a2, 4f
     nop
3:
    pref      30, 32(a0)
    addiu     a2, a2, -8
    sw        a1, 0( a0)
    sw        a1, 4(a0)
    sw        a1, 8(a0)
    sw        a1, 12(a0)
    addiu     a0, a0, 32
    sw        a1, -16(a0)
    sw        a1, -12(a0)
    sw        a1, -8(a0)
    bnez      a2, 3b
     sw       a1, -4(a0)
4:
    sw        a1, 0(a0)
    sw        a1, 4(a0)
    sw        a1, 8(a0)
    sw        a1, 12(a0)
    addiu     a0, a0, 32
    sw        a1, -16(a0)
    sw        a1, -12(a0)
    sw        a1, -8(a0)
    sw        a1, -4(a0)
5:
    jr        ra
     nop

END(qt_memfill32_asm_mips_dsp)

LEAF_MIPS_DSP(comp_func_SourceOver_asm_mips_dsp)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    beqz              a2, 5f
     nop
    li                t8, 0xff
    li                t7, 8388736    /* t7 = 0x800080 */
    bne               a3, t8, 4f
     nop

/* part where const_alpha = 255 */
    b                 2f
     nop
1:
    addiu             a0, a0, 4
    addiu             a2, a2, -1
    beqz              a2, 5f
     nop
2:
    lw                t0, 0(a1)      /* t0 = s = src[i] */
    addiu             a1, a1, 4
    nor               t1, t0, zero
    srl               t1, t1, 24     /* t1 = ~qAlpha(s) */
    bnez              t1, 3f
     nop
    sw                t0, 0(a0)      /* dst[i] = src[i] */
    addiu             a2, a2, -1
    bnez              a2, 2b
     addiu            a0, a0, 4
    b 5f
     nop
3:
    beqz              t0, 1b
     nop

    lw                t4, 0(a0)
    replv.ph          t6, t1
    muleu_s.ph.qbl    t2, t4, t6
    muleu_s.ph.qbr    t3, t4, t6
    addiu             a2, a2, -1
    preceu.ph.qbla    t4, t2
    addq.ph           t4, t2, t4
    addq.ph           t4, t4, t7
    preceu.ph.qbla    t5, t3
    addq.ph           t5, t5, t3
    addq.ph           t5, t5, t7
    precrq.qb.ph      t8, t4, t5    /* t8 = | dsA | dsR | dsG | dsB | */
    addu              t8, t0, t8    /* dst[i] =
                                     * s + BYTE_MUL(dst[i],~qAlpha(s)) */
    sw                t8, 0(a0)
    bnez              a2, 2b
     addiu            a0, a0, 4
    b                 5f
     nop
4:
    lw                t0, 0(a0)     /* t0 - dst[i] "1" */
    lw                t1, 0(a1)     /* t1 - src[i] "2" */
    addiu             a1, a1, 4
    addiu             a2, a2, -1
    replv.ph          t6, a3        /* a1 = 0x00a00a */
    muleu_s.ph.qbl    t2, t1, t6
    muleu_s.ph.qbr    t3, t1, t6
    preceu.ph.qbla    t4, t2
    addq.ph           t4, t2, t4
    addq.ph           t4, t4, t7
    preceu.ph.qbla    t5, t3
    addq.ph           t5, t5, t3
    addq.ph           t5, t5, t7
    precrq.qb.ph      t8, t4, t5    /* t8 = | dsA | dsR | dsG | dsB | */

    nor               t6, t8, zero
    srl               t6, t6, 24
    replv.ph          t6, t6

    muleu_s.ph.qbl    t2, t0, t6
    muleu_s.ph.qbr    t3, t0, t6
    preceu.ph.qbla    t4, t2
    addq.ph           t4, t2, t4
    addq.ph           t4, t4, t7
    preceu.ph.qbla    t5, t3
    addq.ph           t5, t5, t3
    addq.ph           t5, t5, t7
    precrq.qb.ph      t6, t4, t5    /* t6 = | ddA | ddR | ddG | ddB | */

    addu              t0, t8, t6
    sw                t0, 0(a0)
    bnez              a2, 4b
     addiu            a0, a0, 4
5:
    jr                ra
     nop

END(comp_func_SourceOver_asm_mips_dsp)

LEAF_MIPS_DSP(qt_destStoreARGB32_asm_mips_dsp)
/*
 * a0 - uint * data
 * a1 - const uint *buffer
 * a2 - int length
 */

    blez      a2, 6f
    move      v1, zero
    li        t0, 255
    lui       a3, 0xff
    j         2f
     lui      t2, 0xff00
1:
    addiu     v1, v1, 1
    sw        zero, 0(a0)
    addiu     a1, a1, 4
    beq       v1, a2, 6f
    addiu     a0, a0, 4
2:
    lw        v0, 0(a1)
    srl       t3, v0, 0x18
    beql      t3, t0, 5f
    addiu     v1, v1, 1
    beqz      t3, 1b

    srl       t1, v0, 0x8
    andi      t1, t1, 0xff

    teq       t3, zero, 0x7
    div       zero, a3, t3
    move      t8, t3
    andi      t6, v0, 0xff

    srl       t3,v0,0x10
    andi      t3,t3,0xff

    and       t5, v0, t2
    mflo      t4

    mult      $ac0, t4, t6
    mult      $ac1, t1, t4
    mul       t4, t3, t4

    sltiu     t8, t8, 2
    beqz      t8, 3f
     nop
    mflo      t6, $ac0
    mflo      t1, $ac1
    sra       t6, t6, 0x10
    sra       t1, t1, 0x8
    b         4f
     nop
3:
    extr.w    t6, $ac0, 0x10
    extr.w    t1, $ac1, 0x8
4:
    and       v0, t4, a3
    or        v0, v0, t6
    or        v0, v0, t5
    andi      t1, t1, 0xff00
    or        v0, v0, t1
    addiu     v1, v1, 1
5:
    sw        v0, 0(a0)
    addiu     a1, a1, 4
    bne       v1, a2, 2b
    addiu     a0, a0, 4
6:
    jr        ra
     nop

END(qt_destStoreARGB32_asm_mips_dsp)
