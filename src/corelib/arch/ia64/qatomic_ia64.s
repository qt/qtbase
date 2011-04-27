;/****************************************************************************
;**
;** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
;** All rights reserved.
;** Contact: Nokia Corporation (qt-info@nokia.com)
;**
;** This file is part of the QtGui module of the Qt Toolkit.
;**
;** $QT_BEGIN_LICENSE:LGPL$
;** No Commercial Usage
;** This file contains pre-release code and may not be distributed.
;** You may use this file in accordance with the terms and conditions
;** contained in the Technology Preview License Agreement accompanying
;** this package.
;**
;** GNU Lesser General Public License Usage
;** Alternatively, this file may be used under the terms of the GNU Lesser
;** General Public License version 2.1 as published by the Free Software
;** Foundation and appearing in the file LICENSE.LGPL included in the
;** packaging of this file.  Please review the following information to
;** ensure the GNU Lesser General Public License version 2.1 requirements
;** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
;**
;** In addition, as a special exception, Nokia gives you certain additional
;** rights.  These rights are described in the Nokia Qt LGPL Exception
;** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
;**
;** If you have questions regarding the use of this file, please contact
;** Nokia at qt-info@nokia.com.
;**
;**
;**
;**
;**
;**
;**
;**
;** $QT_END_LICENSE$
;**
;****************************************************************************/
	.pred.safe_across_calls p1-p5,p16-p63
.text
	.align 16
	.global q_atomic_test_and_set_int#
	.proc q_atomic_test_and_set_int#
q_atomic_test_and_set_int:
	.prologue
	.body
	mov ar.ccv=r33
	;;
	cmpxchg4.acq r34=[r32],r34,ar.ccv
	;;
	cmp4.eq p6, p7 = r33, r34
	;;
	(p6) addl r8 = 1, r0
	(p7) mov r8 = r0
	br.ret.sptk.many b0
	.endp q_atomic_test_and_set_int#
	.align 16
	.global q_atomic_test_and_set_ptr#
	.proc q_atomic_test_and_set_ptr#
q_atomic_test_and_set_ptr:
	.prologue
	.body
	mov ar.ccv=r33
	;;
	cmpxchg8.acq r34=[r32],r34,ar.ccv
	;;
	cmp.eq p6, p7 = r33, r34
	;;
	(p6) addl r8 = 1, r0
	(p7) mov r8 = r0
	br.ret.sptk.many b0
	.endp q_atomic_test_and_set_ptr#
