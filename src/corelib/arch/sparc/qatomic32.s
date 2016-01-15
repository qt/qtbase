!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!
!! Copyright (C) 2016 The Qt Company Ltd.
!! Contact: https://www.qt.io/licensing/
!!
!! This file is part of the QtGui module of the Qt Toolkit.
!!
!! $QT_BEGIN_LICENSE:LGPL$
!! Commercial License Usage
!! Licensees holding valid commercial Qt licenses may use this file in
!! accordance with the commercial license agreement provided with the
!! Software or, alternatively, in accordance with the terms contained in
!! a written agreement between you and The Qt Company. For licensing terms
!! and conditions see https://www.qt.io/terms-conditions. For further
!! information use the contact form at https://www.qt.io/contact-us.
!!
!! GNU Lesser General Public License Usage
!! Alternatively, this file may be used under the terms of the GNU Lesser
!! General Public License version 3 as published by the Free Software
!! Foundation and appearing in the file LICENSE.LGPL3 included in the
!! packaging of this file. Please review the following information to
!! ensure the GNU Lesser General Public License version 3 requirements
!! will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
!!
!! GNU General Public License Usage
!! Alternatively, this file may be used under the terms of the GNU
!! General Public License version 2.0 or (at your option) the GNU General
!! Public license version 3 or any later version approved by the KDE Free
!! Qt Foundation. The licenses are as published by the Free Software
!! Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
!! included in the packaging of this file. Please review the following
!! information to ensure the GNU General Public License requirements will
!! be met: https://www.gnu.org/licenses/gpl-2.0.html and
!! https://www.gnu.org/licenses/gpl-3.0.html.
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

