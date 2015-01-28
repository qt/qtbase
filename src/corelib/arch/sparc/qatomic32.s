!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!
!! Copyright (C) 2015 The Qt Company Ltd.
!! Contact: http://www.qt.io/licensing/
!!
!! This file is part of the QtGui module of the Qt Toolkit.
!!
!! $QT_BEGIN_LICENSE:LGPL21$
!! Commercial License Usage
!! Licensees holding valid commercial Qt licenses may use this file in
!! accordance with the commercial license agreement provided with the
!! Software or, alternatively, in accordance with the terms contained in
!! a written agreement between you and The Qt Company. For licensing terms
!! and conditions see http://www.qt.io/terms-conditions. For further
!! information use the contact form at http://www.qt.io/contact-us.
!!
!! GNU Lesser General Public License Usage
!! Alternatively, this file may be used under the terms of the GNU Lesser
!! General Public License version 2.1 or version 3 as published by the Free
!! Software Foundation and appearing in the file LICENSE.LGPLv21 and
!! LICENSE.LGPLv3 included in the packaging of this file. Please review the
!! following information to ensure the GNU Lesser General Public License
!! requirements will be met: https://www.gnu.org/licenses/lgpl.html and
!! http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
!!
!! As a special exception, The Qt Company gives you certain additional
!! rights. These rights are described in The Qt Company LGPL Exception
!! version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
!!
!! $QT_END_LICENSE$
!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	.section ".text"

	.align 4
	.type q_atomic_trylock_int,#function
	.global q_atomic_trylock_int
q_atomic_trylock_int:
        sethi %hi(-2147483648),%o2
        swap [%o0],%o2
        retl
        mov %o2,%o0
        .size q_atomic_trylock_int,.-q_atomic_trylock_int




        .align 4
        .type q_atomic_trylock_ptr,#function
        .global q_atomic_trylock_ptr
q_atomic_trylock_ptr:
        mov -1, %o2
        swap [%o0], %o2
        retl
        mov %o2, %o0
        .size q_atomic_trylock_ptr,.-q_atomic_trylock_ptr




	.align 4
	.type q_atomic_unlock,#function
	.global q_atomic_unlock
q_atomic_unlock:
	stbar
	retl
	st %o1,[%o0]
	.size q_atomic_unlock,.-q_atomic_unlock




	.align 4
	.type q_atomic_set_int,#function
	.global q_atomic_set_int
q_atomic_set_int:
	swap [%o0],%o1
        stbar
	retl
	mov %o1,%o0
	.size q_atomic_set_int,.-q_atomic_set_int




	.align 4
	.type q_atomic_set_ptr,#function
	.global q_atomic_set_ptr
q_atomic_set_ptr:
	swap [%o0],%o1
        stbar
	retl
	mov %o1,%o0
	.size q_atomic_set_ptr,.-q_atomic_set_ptr

