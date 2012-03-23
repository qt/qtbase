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

#ifndef QT_MIPS_DSP_H__
#define QT_MIPS_DSP_H__

#define zero $0
#define AT   $1
#define v0   $2
#define v1   $3
#define a0   $4
#define a1   $5
#define a2   $6
#define a3   $7
#define t0   $8
#define t1   $9
#define t2   $10
#define t3   $11
#define t4   $12
#define t5   $13
#define t6   $14
#define t7   $15
#define s0   $16
#define s1   $17
#define s2   $18
#define s3   $19
#define s4   $20
#define s5   $21
#define s6   $22
#define s7   $23
#define t8   $24
#define t9   $25
#define k0   $26
#define k1   $27
#define gp   $28
#define sp   $29
#define fp   $30
#define s8   $30
#define ra   $31

/*
 * LEAF_MIPS32R2 - declare leaf_mips32r2 routine
 */
#define LEAF_MIPS32R2(symbol)                           \
                .globl  symbol;                         \
                .align  2;                              \
                .type   symbol,@function;               \
                .ent    symbol,0;                       \
symbol:         .frame  sp, 0, ra;                      \
                .set    arch=mips32r2;                  \
                .set    noreorder;

/*
 * LEAF_MIPS_DSP - declare leaf_mips_dsp routine
 */
#define LEAF_MIPS_DSP(symbol)                           \
LEAF_MIPS32R2(symbol)                                   \
                .set    dsp;

/*
 * LEAF_MIPS_DSPR2 - declare leaf_mips_dspr2 routine
 */
#define LEAF_MIPS_DSPR2(symbol)                         \
LEAF_MIPS32R2(symbol)                                   \
                .set   dspr2;

/*
 * END - mark end of function
 */
#define END(function)                                   \
                .set    reorder;                        \
                .end    function;                       \
                .size   function,.-function

#endif //QT_MIPS_DSP_H__
