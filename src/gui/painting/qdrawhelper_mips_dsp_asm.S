/****************************************************************************
**
** Copyright (C) 2012 MIPS Technologies, www.mips.com, author Damir Tatalovic <dtatalovic@mips.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qt_mips_asm_dsp_p.h"

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

LEAF_MIPS_DSPR2(qt_destStoreARGB32_asm_mips_dsp)
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

LEAF_MIPS_DSP(comp_func_solid_Source_dsp_asm_x2)
/*
 * a0 - const uint *dest
 * a1 - int length
 * a2 - uint color
 * a3 - uint ialpha
 */

    beqz              a1, 2f
     nop
    replv.ph          a3, a3
    li                t9, 8388736    /* t9 = 0x800080 */
1:
    lw                t0, 0(a0)
    lw                t1, 4(a0)
    or                t2, t0, t1    /* if both dest are zero, no computation needed */
    beqz              t2, 12f
     addiu             a1, -2

    BYTE_MUL_x2 t0, t1, t6, t7, a3, a3, t9, t2, t3, t4, t5, 0
11:
    addu              t2, a2, t6
    addu              t3, a2, t7
    sw                t2, 0(a0)
    sw                t3, 4(a0)
    bnez              a1, 1b
     addiu             a0, 8
    b                 2f
12:
    addu              t2, a2, t0
    addu              t3, a2, t1
    sw                t2, 0(a0)
    sw                t3, 4(a0)
    bnez              a1, 1b
     addiu             a0, 8
2:
    jr                ra
     nop

END(comp_func_solid_Source_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_DestinationOver_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint color
 */

    addiu             sp, sp, -8
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    beqz              a1, 2f
     nop
    beqz              a2, 2f
     nop
    li                t9, 8388736    /* t4 = 0x800080 */

1:
    lw                t0, 0(a0)
    lw                t1, 4(a0)
    not               t2, t0
    not               t3, t1
    srl               t4, t2, 24
    srl               t5, t3, 24
    or                t2, t4, t5    /* if both dest are zero, no computation needed */
    beqz              t2, 11f
     addiu             a1, -2
    replv.ph          t2, t4
    replv.ph          t3, t5

    BYTE_MUL_x2 a2, a2, t8, a3, t2, t3, t9, t4, t5, t6, t7

    addu              t0, t0, t8
    addu              t1, t1, a3
11:
    sw                t0, 0(a0)
    sw                t1, 4(a0)
    bnez              a1, 1b
     addiu             a0, 8

2:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    addiu             sp, sp, 8
    jr                ra
     nop

END(comp_func_solid_DestinationOver_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_DestinationOver_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, sp, -8
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736    /* t4 = 0x800080 */
    li                t0, 0xff
    beq               a3, t0, 2f
     nop

/* part where const_alpha != 255 */
1:
    replv.ph          a3, a3
11:
    lw                t0, 0(a1)     # src_1
    lw                t1, 4(a1)     # src_2
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, t8, AT, a3, a3, t9, t4, t5, t6, t7, 0
                                    # t8 = s1
                                    # AT = s2
    lw                t0, 0(a0)     # dest_1
    lw                t1, 4(a0)     # dest_2
    addiu             a1, 8
    not               t2, t0
    not               t3, t1
    srl               t4, t2, 24
    srl               t5, t3, 24
    replv.ph          t2, t4        # qAlpha(~d) 1
    replv.ph          t3, t5        # qAlpha(~d) 2

    BYTE_MUL_x2 t8, AT, s0, s1, t2, t3, t9, t4, t5, t6, t7

    addu              t0, t0, s0
    addu              t1, t1, s1
    sw                t0, 0(a0)
    sw                t1, 4(a0)
    bnez              a2, 11b
     addiu             a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t0, 0(a0)     # dest 1
    lw                t1, 4(a0)     # dest 2
    lw                s0, 0(a1)     # src 1
    lw                s1, 4(a1)     # src 2
    not               t2, t0
    not               t3, t1
    srl               t4, t2, 24
    srl               t5, t3, 24
    replv.ph          t2, t4
    replv.ph          t3, t5
    addiu             a1, 8
    addiu             a2, -2

    BYTE_MUL_x2 s0, s1, t8, AT, t2, t3, t9, t4, t5, t6, t7

    addu              t0, t0, t8
    addu              t1, t1, AT
    sw                t0, 0(a0)
    sw                t1, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    addiu             sp, sp, 8
    jr                ra
     nop
    .set              at

END(comp_func_DestinationOver_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_SourceIn_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint color
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -12
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    beqz              a1, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    replv.ph          t0, a3
    li                t5, 0xff
    BYTE_MUL a2, a2, t0, t9, t1, t2, t3, t4    /* a2 = color ( = BYTE_MUL(color, const_alpha)); */
    subu              t1, t5, a3               /* t1 = cia = 255 - const_alpha */
11:
    lw                t2, 0(a0)                /* t2 = d */
    lw                s0, 4(a0)
    addiu             a1, -2
    srl               t3, t2, 24               /* t3 = qAlpha(d) */
    srl               s2, s0, 24

    INTERPOLATE_PIXEL_255 a2, t3, t2, t1, AT, t9, t8, t4, t5, t6, t7
    INTERPOLATE_PIXEL_255 a2, s2, s0, t1, s1, t9, t8, t4, t5, t6, t7

    sw                AT, 0(a0)
    sw                s1, 4(a0)
    bnez              a1, 11b
     addiu            a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t0, 0(a0)                /* dest 1 */
    lw                t1, 4(a0)                /* dest 2 */
    srl               t4, t0, 24
    srl               t5, t1, 24
    replv.ph          t2, t4
    replv.ph          t3, t5
    addiu             a1, -2

    BYTE_MUL_x2 a2, a2, t8, AT, t2, t3, t9, t4, t5, t6, t7

    sw                t8, 0(a0)
    sw                AT, 4(a0)
    bnez              a1, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    addiu             sp, 12
    jr                ra
     nop
    .set              at

END(comp_func_solid_SourceIn_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_SourceIn_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -16
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    sw                s3, 12(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    li                t5, 0xff
    subu              t7, t5, a3               /* t7 = cia = 255 - const_alpha */
    replv.ph          a3, a3
11:
    lw                t0, 0(a1)                /* t0 = src 1 */
    lw                t1, 4(a1)                /* t1 = src 2 */
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, AT, s0, a3, a3, t9, t3, t4, t5, t6, 0

    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addiu             a1, 8

    srl               t2, t0, 24               /* t2 = qAlpha(d) 1 */
    srl               t3, t1, 24               /* t3 = qAlpha(d) 2 */

    INTERPOLATE_PIXEL_255 AT, t2, t0, t7, s1, t9, t8, t4, t5, t6, s3
    INTERPOLATE_PIXEL_255 s0, t3, t1, t7, s2, t9, t8, t4, t5, t6, s3

    sw                s1, 0(a0)
    sw                s2, 4(a0)
    bnez              a2, 11b
     addiu            a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a0)                /* dest 1 */
    lw                t3, 4(a0)                /* dest 2 */
    lw                t0, 0(a1)                /* src 1 */
    lw                t1, 4(a1)                /* src 2 */
    srl               t4, t2, 24
    srl               t5, t3, 24
    replv.ph          t2, t4
    replv.ph          t3, t5
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, t8, AT, t2, t3, t9, t4, t5, t6, t7

    addiu             a1, 8
    sw                t8, 0(a0)
    sw                AT, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    lw                s3, 12(sp)
    addiu             sp, 16
    jr                ra
     nop
    .set              at

END(comp_func_SourceIn_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_DestinationIn_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint a
 */

    .set              noat
    beqz              a1, 2f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    replv.ph          a2, a2
1:
    lw                t0, 0(a0)
    lw                t1, 4(a0)
    addiu             a1, -2

    BYTE_MUL_x2 t0, t1, t8, AT, a2, a2, t9, t4, t5, t6, t7, 0

    sw                t8, 0(a0)
    sw                AT, 4(a0)
    bnez              a1, 1b
     addiu            a0, 8
2:
    jr                ra
     nop
    .set              at

END(comp_func_solid_DestinationIn_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_DestinationIn_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    addiu             sp, -8
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    li                t0, 0xff
    beq               a3, t0, 2f
     nop

/* part where const_alpha != 255 */
1:
    li                t5, 0xff
    subu              t8, t5, a3               /* t8 = cia = 255 - const_alpha */
    replv.ph          a3, a3
11:
    lw                t0, 0(a1)                /* t0 = src 1 */
    lw                t1, 4(a1)                /* t1 = src 2 */
    addiu             a2, -2
    srl               t0, t0, 24
    srl               t1, t1, 24

    BYTE_MUL_x2 t0, t1, s1, t7, a3, a3, t9, t3, t4, t5, t6, 0

    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addu              s1, s1, t8               /* a 1 */
    addu              t7, t7, t8               /* a 2 */
    replv.ph          t2, s1
    replv.ph          t3, t7

    BYTE_MUL_x2 t0, t1, s1, t7, t2, t3, t9, t4, t5, t6, s0

    addiu             a1, 8
    sw                s1, 0(a0)
    sw                t7, 4(a0)
    bnez              a2, 11b
     addiu            a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a1)                /* src 1 */
    lw                t3, 4(a1)                /* src 2 */
    lw                t0, 0(a0)                /* dest 1 */
    lw                t1, 4(a0)                /* dest 2 */
    srl               t4, t2, 24
    srl               t5, t3, 24
    replv.ph          t2, t4                   /* t2 = qAlpha(src 1) */
    replv.ph          t3, t5                   /* t3 = qAlpha(src 2) */
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, t8, s1, t2, t3, t9, t4, t5, t6, t7

    addiu             a1, 8
    sw                t8, 0(a0)
    sw                s1, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    addiu             sp, 8
    jr                ra
     nop

END(comp_func_DestinationIn_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_DestinationOut_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -4
    sw                s0, 0(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    li                t0, 0xff
    beq               a3, t0, 2f
     nop

/* part where const_alpha != 255 */
1:
    li                t5, 0xff
    subu              t8, t5, a3               /* t8 = cia = 255 - const_alpha */
    replv.ph          a3, a3
11:
    lw                t0, 0(a1)                /* t0 = src 1 */
    lw                t1, 4(a1)                /* t1 = src 2 */
    not               t0, t0
    not               t1, t1
    addiu             a2, -2
    srl               t0, t0, 24
    srl               t1, t1, 24

    BYTE_MUL_x2       t0, t1, AT, t7, a3, a3, t9, t3, t4, t5, t6, 0

    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addu              AT, AT, t8               /* a 1 */
    addu              t7, t7, t8               /* a 2 */
    replv.ph          t2, AT
    replv.ph          t3, t7

    BYTE_MUL_x2 t0, t1, AT, t7, t2, t3, t9, t4, t5, t6, s0

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                t7, 4(a0)
    bnez              a2, 11b
     addiu            a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a1)                /* src 1 */
    lw                t3, 4(a1)                /* src 2 */
    not               t2, t2
    not               t3, t3
    lw                t0, 0(a0)                /* dest 1 */
    lw                t1, 4(a0)                /* dest 2 */
    srl               t4, t2, 24
    srl               t5, t3, 24
    replv.ph          t2, t4                   /* t2 = qAlpha(src 1) */
    replv.ph          t3, t5                   /* t3 = qAlpha(src 2) */
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, t8, AT, t2, t3, t9, t4, t5, t6, t7

    addiu             a1, 8
    sw                t8, 0(a0)
    sw                AT, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    addiu             sp, 4
    jr                ra
     nop
    .set              at

END(comp_func_DestinationOut_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_SourceAtop_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint color
 * a3 - uint sia
 */

    .set              noat
    addu              sp, -4
    sw                s0, 0(sp)
    beqz              a1, 2f
     nop
    li                t9, 8388736              /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    ori               t8, t8, 0xff00           /* t8 = 0xff00ff00 (andi_factor) */
1:
    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addiu             a1, -2
    srl               t2, t0, 24               /* t2 = qAlpha(dest 1) */
    srl               t3, t1, 24               /* t3 = qAlpha(dest 2) */

    INTERPOLATE_PIXEL_255 a2, t2, t0, a3, AT, t9, t8, t4, t5, t6, t7
    INTERPOLATE_PIXEL_255 a2, t3, t1, a3, s0, t9, t8, t4, t5, t6, t7

    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a1, 1b
     addiu            a0, 8
2:
    lw                s0, 0(sp)
    addiu             sp, 4
    jr                ra
     nop
    .set              at

END(comp_func_solid_SourceAtop_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_SourceAtop_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -20
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    sw                s3, 12(sp)
    sw                s4, 16(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    replv.ph          a3, a3
11:
    lw                AT, 0(a1)                /* src 1 */
    lw                s0, 4(a1)                /* src 2 */

    BYTE_MUL_x2 AT, s0, t0, t1, a3, a3, t9, t3, t4, t5, t6, 0
                                               /* t0 = s */

    lw                t2, 0(a0)                /* t2 = dest 1 */
    lw                t3, 4(a0)                /* t3 = dest 2 */

    srl               t4, t2, 24               /* t4 = qAplpha(dest 1) */
    srl               t5, t3, 24
    not               t6, t0
    not               t7, t1
    srl               t6, t6, 24               /* t6 = qAlpha(~s) */
    srl               t7, t7, 24
    addiu             a2, -2

    INTERPOLATE_PIXEL_255 t0, t4, t2, t6, AT, t9, t8, s1, s2, s3, s4
    INTERPOLATE_PIXEL_255 t1, t5, t3, t7, s0, t9, t8, s1, s2, s3, s4

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 11b
     addiu             a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a0)                /* dest 1 */
    lw                t3, 4(a0)                /* dest 2 */
    lw                t0, 0(a1)                /* src 1 */
    lw                t1, 4(a1)                /* src 2 */
    srl               t4, t2, 24
    srl               t5, t3, 24
    not               t6, t0
    not               t7, t1
    srl               t6, t6, 24
    srl               t7, t7, 24
    addiu             a2, -2

    INTERPOLATE_PIXEL_255 t0, t4, t2, t6, AT, t9, t8, s1, s2, s3, s4
    INTERPOLATE_PIXEL_255 t1, t5, t3, t7, s0, t9, t8, s1, s2, s3, s4

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    lw                s3, 12(sp)
    lw                s4, 16(sp)
    addiu             sp, 20
    jr                 ra
     nop
    .set              at

END(comp_func_SourceAtop_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_DestinationAtop_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint color
 * a3 - uint a
 */

    .set              noat
    addiu             sp, -4
    sw                s0, 0(sp)
    beqz              a1, 2f
     nop
    li                t9, 8388736              /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    ori               t8, t8, 0xff00           /* t8 = 0xff00ff00 (andi_factor) */
1:
    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addiu             a1, -2
    not               t2, t0
    not               t3, t1
    srl               t2, t2, 24               /* t2 = qAlpha(~(dest 1)) */
    srl               t3, t3, 24               /* t3 = qAlpha(~(dest 2)) */

    INTERPOLATE_PIXEL_255 t0, a3, a2, t2, AT, t9, t8, t4, t5, t6, t7
    INTERPOLATE_PIXEL_255 t1, a3, a2, t3, s0, t9, t8, t4, t5, t6, t7

    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a1, 1b
     addiu            a0, 8
2:
    lw                s0, 0(sp)
    addiu              sp, 4
    jr                ra
     nop
    .set              at

END(comp_func_solid_DestinationAtop_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_DestinationAtop_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -24
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    sw                s3, 12(sp)
    sw                s4, 16(sp)
    sw                s5, 20(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    li                s5, 0xff
    subu              s5, s5, a3               /* s5 = cia = 255 - const_alpha */
    replv.ph          a3, a3
11:
    lw                AT, 0(a1)                /* src 1 */
    lw                s0, 4(a1)                /* src 2 */

    BYTE_MUL_x2 AT, s0, t0, t1, a3, a3, t9, t3, t4, t5, t6, 0
                                               /* t0 = s */

    lw                t2, 0(a0)                /* t2 = dest 1 */
    lw                t3, 4(a0)                /* t3 = dest 2 */

    not               t4, t2
    not               t5, t3
    srl               t4, t4, 24               /* t4 = qAplpha(~(dest 1)) */
    srl               t5, t5, 24
    srl               t6, t0, 24
    srl               t7, t1, 24
    addu              t6, t6, s5               /* t6 = a = qAlpha(s1) + cia */
    addu              t7, t7, s5
    addiu             a2, -2

    INTERPOLATE_PIXEL_255 t2, t6, t0, t4, AT, t9, t8, s1, s2, s3, s4
    INTERPOLATE_PIXEL_255 t3, t7, t1, t5, s0, t9, t8, s1, s2, s3, s4

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 11b
     addiu             a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a0)                /* d1 */
    lw                t3, 4(a0)                /* d2 */
    lw                t0, 0(a1)                /* s1 */
    lw                t1, 4(a1)                /* s2 */
    srl               t4, t0, 24               /* t4 = qAlpha(s1) */
    srl               t5, t1, 24
    not               t6, t2
    not               t7, t3
    srl               t6, t6, 24               /* qAlpha(~d1) */
    srl               t7, t7, 24
    addiu             a2, -2

    INTERPOLATE_PIXEL_255 t2, t4, t0, t6, AT, t9, t8, s1, s2, s3, s4
    INTERPOLATE_PIXEL_255 t3, t5, t1, t7, s0, t9, t8, s1, s2, s3, s4

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    lw                s3, 12(sp)
    lw                s4, 16(sp)
    lw                s5, 20(sp)
    addiu             sp, 24
    jr                ra
     nop
    .set              at

END(comp_func_DestinationAtop_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_XOR_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint color
 * a3 - uint sia
 */

    .set              noat
    addu              sp, -4
    sw                s0, 0(sp)
    beqz              a1, 2f
     nop
    li                t9, 8388736              /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    ori               t8, t8, 0xff00           /* t8 = 0xff00ff00 (andi_factor) */
1:
    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addiu             a1, -2
    not               t2, t0
    not               t3, t1
    srl               t2, t2, 24               /* t2 = qAlpha(~(dest 1)) */
    srl               t3, t3, 24               /* t3 = qAlpha(~(dest 2)) */

    INTERPOLATE_PIXEL_255 a2, t2, t0, a3, AT, t9, t8, t4, t5, t6, t7
    INTERPOLATE_PIXEL_255 a2, t3, t1, a3, s0, t9, t8, t4, t5, t6, t7

    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a1, 1b
     addiu            a0, 8
2:
    lw                s0, 0(sp)
    addu              sp, 4
    jr                ra
     nop
    .set              at

END(comp_func_solid_XOR_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_XOR_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -20
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    sw                s3, 12(sp)
    sw                s4, 16(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    replv.ph          a3, a3
11:
    lw                AT, 0(a1)                /* src 1 */
    lw                s0, 4(a1)                /* src 2 */

    BYTE_MUL_x2 AT, s0, t0, t1, a3, a3, t9, t3, t4, t5, t6, 0
                                               /* t0 = s1 */
                                               /* t1 = s2 */

    lw                t2, 0(a0)                /* t2 = dest 1 */
    lw                t3, 4(a0)                /* t3 = dest 2 */

    not               t4, t2
    not               t5, t3
    srl               t4, t4, 24               /* t4 = qAplpha(~(dest 1)) */
    srl               t5, t5, 24
    not               t6, t0
    not               t7, t1
    srl               t6, t6, 24               /* t6 = qAlpha(~s) */
    srl               t7, t7, 24
    addiu             a2, -2

    INTERPOLATE_PIXEL_255 t0, t4, t2, t6, AT, t9, t8, s1, s2, s3, s4
    INTERPOLATE_PIXEL_255 t1, t5, t3, t7, s0, t9, t8, s1, s2, s3, s4

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 11b
     addiu             a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a0)                /* d1 */
    lw                t3, 4(a0)                /* d2 */
    lw                t0, 0(a1)                /* s1 */
    lw                t1, 4(a1)                /* s2 */
    not               t4, t0
    not               t5, t1
    srl               t4, t4, 24               /* t4 = qAlpha(~s1) */
    srl               t5, t5, 24
    not               t6, t2
    not               t7, t3
    srl               t6, t6, 24               /* qAlpha(~d1) */
    srl               t7, t7, 24
    addiu             a2, -2

    INTERPOLATE_PIXEL_255 t0, t6, t2, t4, AT, t9, t8, s1, s2, s3, s4
    INTERPOLATE_PIXEL_255 t1, t7, t3, t5, s0, t9, t8, s1, s2, s3, s4

    addiu             a1, 8
    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    lw                s3, 12(sp)
    lw                s4, 16(sp)
    addiu             sp, 20
    jr                ra
     nop
    .set              at

END(comp_func_XOR_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_solid_SourceOut_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - int length
 * a2 - uint color
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -12
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    beqz              a1, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    replv.ph          t0, a3
    li                t5, 0xff
    BYTE_MUL a2, a2, t0, t9, t1, t2, t3, t4    /* a2 = color ( = BYTE_MUL(color, const_alpha)); */
    subu              t1, t5, a3               /* t1 = cia = 255 - const_alpha */
11:
    lw                t2, 0(a0)                /* t2 = d1 */
    lw                s0, 4(a0)                /* s0 = d2 */
    addiu             a1, -2
    not               t3, t2
    not               s2, s0
    srl               t3, t3, 24               /* t3 = qAlpha(~d1) */
    srl               s2, s2, 24               /* s2 = qAlpha(~d2) */

    INTERPOLATE_PIXEL_255 a2, t3, t2, t1, AT, t9, t8, t4, t5, t6, t7
    INTERPOLATE_PIXEL_255 a2, s2, s0, t1, s1, t9, t8, t4, t5, t6, t7

    sw                AT, 0(a0)
    sw                s1, 4(a0)
    bnez              a1, 11b
     addiu            a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t0, 0(a0)                /* dest 1 */
    lw                t1, 4(a0)                /* dest 2 */
    not               t4, t0
    not               t5, t1
    srl               t4, t4, 24
    srl               t5, t5, 24
    replv.ph          t2, t4
    replv.ph          t3, t5
    addiu             a1, -2

    BYTE_MUL_x2 a2, a2, t8, AT, t2, t3, t9, t4, t5, t6, t7

    sw                t8, 0(a0)
    sw                AT, 4(a0)
    bnez              a1, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    addiu             sp, 12
    jr                ra
     nop
    .set              at

END(comp_func_solid_SourceOut_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_SourceOut_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -16
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    sw                s3, 12(sp)
    beqz              a2, 3f
     nop
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    li                t0, 0xff
    beq               a3, t0, 2f
     ori               t8, t8, 0xff00         /* t8 = 0xff00ff00 (andi_factor) */

/* part where const_alpha != 255 */
1:
    li                t5, 0xff
    subu              t7, t5, a3               /* t7 = cia = 255 - const_alpha */
    replv.ph          a3, a3
11:
    lw                t0, 0(a1)                /* t0 = src 1 */
    lw                t1, 4(a1)                /* t1 = src 2 */
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, AT, s0, a3, a3, t9, t3, t4, t5, t6, 0

    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    addiu             a1, 8

    not               t2, t0
    not               t3, t1
    srl               t2, t2, 24               /* t2 = qAlpha(~d1) */
    srl               t3, t3, 24               /* t3 = qAlpha(~d2) */

    INTERPOLATE_PIXEL_255 AT, t2, t0, t7, s1, t9, t8, t4, t5, t6, s3
    INTERPOLATE_PIXEL_255 s0, t3, t1, t7, s2, t9, t8, t4, t5, t6, s3

    sw                s1, 0(a0)
    sw                s2, 4(a0)
    bnez              a2, 11b
     addiu            a0, 8
    b                 3f
     nop

/* part where const_alpha = 255 */
2:
    lw                t2, 0(a0)                /* dest 1 */
    lw                t3, 4(a0)                /* dest 2 */
    lw                t0, 0(a1)                /* src 1 */
    lw                t1, 4(a1)                /* src 2 */
    not               t4, t2
    not               t5, t3
    srl               t4, t4, 24               /* qAlpha(~d1) */
    srl               t5, t5, 24               /* qAlpha(~d2) */
    replv.ph          t2, t4
    replv.ph          t3, t5
    addiu             a2, -2

    BYTE_MUL_x2 t0, t1, t8, AT, t2, t3, t9, t4, t5, t6, t7

    addiu             a1, 8
    sw                t8, 0(a0)
    sw                AT, 4(a0)
    bnez              a2, 2b
     addiu             a0, 8

3:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    lw                s3, 12(sp)
    addiu             sp, 16
    jr                 ra
     nop
    .set              at

END(comp_func_SourceOut_dsp_asm_x2)

LEAF_MIPS_DSP(comp_func_Source_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -8
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    beqz              a2, 2f
     nop
    li                t9, 8388736              /* t9 = 0x800080 (rounding_factor) */
    lui               t8, 0xff00
    ori               t8, t8, 0xff00           /* t8 = 0xff00ff00 (andi_factor) */
    li                t7, 0xff
    subu              t7, t7, a3               /* t7 = ialpha */
1:
    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    lw                t2, 0(a1)                /* t2 = src 1 */
    lw                t3, 4(a1)                /* t3 = src 2 */
    addiu             a2, -2
    addiu             a1, 8

    INTERPOLATE_PIXEL_255 t2, a3, t0, t7, AT, t9, t8, t4, t5, t6, s1
    INTERPOLATE_PIXEL_255 t3, a3, t1, t7, s0, t9, t8, t4, t5, t6, s1

    sw                AT, 0(a0)
    sw                s0, 4(a0)
    bnez              a2, 1b
     addiu            a0, 8
2:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    addiu             sp, 8
    jr                ra
     nop
    .set              at

END(comp_func_Source_dsp_asm_x2)

LEAF_MIPS_DSP(qt_blend_argb32_on_argb32_mips_dsp_asm_x2)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 * a3 - uint const_alpha
 */

    .set              noat
    addiu             sp, -12
    sw                s0, 0(sp)
    sw                s1, 4(sp)
    sw                s2, 8(sp)
    beqz              a2, 2f
     nop
    replv.ph          a3, a3
    li                t9, 8388736             /* t9 = 0x800080 (rounding_factor) */

1:
    lw                t0, 0(a1)                /* t0 = src 1 */
    lw                t1, 4(a1)                /* t1 = src 2 */
    addiu             a2, -2

    BYTE_MUL_x2       t0, t1, AT, t7, a3, a3, t9, t3, t4, t5, t6, 0

    lw                t0, 0(a0)                /* t0 = dest 1 */
    lw                t1, 4(a0)                /* t1 = dest 2 */
    not               s1, AT
    not               s2, t7
    srl               s1, s1, 24               /* s1 = qAlpha(~s1) */
    srl               s2, s2, 24               /* s2 = qAlpha(~s2) */
    replv.ph          s1, s1
    replv.ph          s2, s2

    BYTE_MUL_x2 t0, t1, t2, t3, s1, s2, t9, t4, t5, t6, s0

    addiu             a1, 8
    addu              AT, AT, t2
    addu              t7, t7, t3
    sw                AT, 0(a0)
    sw                t7, 4(a0)
    bnez              a2, 1b
     addiu            a0, 8

2:
    lw                s0, 0(sp)
    lw                s1, 4(sp)
    lw                s2, 8(sp)
    addiu             sp, 12
    jr                ra
     nop
    .set              at

END(qt_blend_argb32_on_argb32_mips_dsp_asm_x2)

LEAF_MIPS_DSP(qt_blend_argb32_on_argb32_const_alpha_256_mips_dsp_asm)
/*
 * a0 - uint *dest
 * a1 - const uint *src
 * a2 - int length
 */

    beqz              a2, 5f
     nop
    li                t7, 8388736    /* t7 = 0x800080 */
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
     replv.ph          t6, t1        /* | 0 | qAlpha(~s) | 0 | qAlpha(~s) | */

    lw                t4, 0(a0)
    addiu             a2, a2, -1
    beqz              t4, 31f
     move             t8, zero

    BYTE_MUL t4, t8, t6, t7, t1, t2, t3, t4
31:
    addu              t8, t0, t8    /* dst[i] =
                                     * s + BYTE_MUL(dst[i],~qAlpha(s)) */
    sw                t8, 0(a0)
    bnez              a2, 2b
     addiu            a0, a0, 4
    b                 5f
     nop
5:
    jr                ra
     nop

END(qt_blend_argb32_on_argb32_const_alpha_256_mips_dsp_asm)
