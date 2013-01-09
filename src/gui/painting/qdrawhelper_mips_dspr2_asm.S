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

LEAF_MIPS_DSPR2(qConvertRgb16To32_asm_mips_dspr2)
/*
 * a0 - dst (a8r8g8b8)
 * a1 - src (r5g6b5)
 * a2 - w
 */

    beqz              a2, 3f
     nop
    addiu             t1, a2, -1
    beqz              t1, 2f
     nop
    li                t4, 0x07e007e0
    li                t5, 0x001F001F
/* Convert two pixels at time (2 x rgb565 -> 2 x rgb8888) */
1:
    lhu               t0, 0(a1)
    lhu               t1, 2(a1)
    addiu             a1, a1, 4
    addiu             a2, a2, -2

    sll               t6, t0, 16
    or                t6, t6, t1          /* t6 = R1 G1 B1 | R2 G2 B2 */
    lui               t3, 0xff00
    ori               t3, t3, 0xff00      /* t3 = FF 00 | FF 00 (in place) */
    shrl.ph           t7, t6, 11          /* t7 = 0 R1 | 0 R2 */
    and               t8, t6, t4          /* t8 = 0 G1 0 | 0 G2 0 */
    shra.ph           t9, t7, 2           /* t9 = 0 R1 | 0 R2   (lower) */
    shll.ph           t7, t7, 3           /* t7 = 0 R1 | 0 R2   (higher) */
    shll.ph           t8, t8, 5           /* t8 = G1 0 | G2 0   (higher) */
    or                t7, t7, t9          /* t7 = 0 R1 | 0 R2   (in place) */
    shrl.qb           t9, t8, 6           /* t9 = G1 0 | G2 0   (lower) */
    or                t3, t3, t7          /* t3 = FF R1 | FF R2 (in place) */
    or                t8, t8, t9          /* t8 = G1 0 | G2 0   (in place) */
    and               t6, t6, t5          /* t6 = 0 B1 | 0 B2 */
    shll.ph           t7, t6, 3           /* t7 = 0 B1 | 0 B2   (higher) */
    shra.ph           t9, t6, 2           /* t9 = 0 B1 | 0 B2   (lower) */
    or                t7, t7, t9          /* t7 = 0 B1 | 0 B2   (in place) */
    or                t8, t7, t8          /* t8 = G1 B1 | G2 B2 (in place) */
    precrq.ph.w       t2, t3, t8          /* t2 = FF R1 G1 B1   (in place) */
    precr_sra.ph.w    t3, t8, 0           /* t3 = FF R2 G2 B2   (in place) */

    sw                t2, 0(a0)
    sw                t3, 4(a0)

    addiu             t2, a2, -1
    bgtz              t2, 1b
     addiu            a0, a0, 8
2:
    beqz              a2, 3f
     nop
    lhu               t0, 0(a1)

/* Remaining pixel conversion (rgb565 -> rgb8888) */
    lui               t1, 0xff00
    sll               t2, t0, 0x3
    andi              t3, t2, 0xff
    ext               t2, t0, 0x2, 0x3
    or                t2, t3, t2
    or                t1, t1, t2

    sll               t2, t0, 0x5
    andi              t2, t2, 0xfc00
    srl               t3, t0, 0x1
    andi              t3, t3, 0x300
    or                t3, t2, t3
    or                t1, t1, t3

    andi              t2, t0, 0xf800
    srl               t3, t2, 0x5
    andi              t3, t3, 0xff00
    or                t2, t2, t3
    sll               t2, t2, 0x8
    or                t1, t1, t2

    sw                t1, 0(a0)
3:
    j                 ra
     nop

END(qConvertRgb16To32_asm_mips_dspr2)
