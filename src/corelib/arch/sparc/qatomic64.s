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
	.type q_atomic_test_and_set_int,#function
	.global q_atomic_test_and_set_int
q_atomic_test_and_set_int:
	cas [%o0],%o1,%o2
	cmp %o1,%o2
	clr %o0
	retl
	move %icc,1,%o0
	.size q_atomic_test_and_set_int,.-q_atomic_test_and_set_int

	.align 4
	.type q_atomic_test_and_set_acquire_int,#function
	.global q_atomic_test_and_set_acquire_int
q_atomic_test_and_set_acquire_int:
	cas [%o0],%o1,%o2
	cmp %o1,%o2
	clr %o0
	membar #LoadLoad | #LoadStore
	retl
	move %icc,1,%o0
	.size q_atomic_test_and_set_acquire_int,.-q_atomic_test_and_set_acquire_int

	.align 4
	.type q_atomic_test_and_set_release_int,#function
	.global q_atomic_test_and_set_release_int
q_atomic_test_and_set_release_int:
        membar #LoadStore | #StoreStore
	cas [%o0],%o1,%o2
	cmp %o1,%o2
	clr %o0
	retl
	move %icc,1,%o0
	.size q_atomic_test_and_set_release_int,.-q_atomic_test_and_set_release_int

	.align 4
	.type q_atomic_test_and_set_ptr,#function
	.global q_atomic_test_and_set_ptr
q_atomic_test_and_set_ptr:
	casx [%o0],%o1,%o2
	cmp %o1,%o2
	clr %o0
	retl
	move %icc,1,%o0
	.size q_atomic_test_and_set_ptr,.-q_atomic_test_and_set_ptr

	.align 4
	.type q_atomic_increment,#function
	.global q_atomic_increment
q_atomic_increment:
q_atomic_increment_retry:
	ld [%o0],%o3
	add %o3,1,%o4
	cas [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_increment_retry
	nop
	cmp %o4,-1
	clr %o0
	retl
	movne %icc,1,%o0
	.size q_atomic_increment,.-q_atomic_increment

	.align 4
	.type q_atomic_decrement,#function
	.global q_atomic_decrement
q_atomic_decrement:
q_atomic_decrement_retry:
	ld [%o0],%o3
	add %o3,-1,%o4
	cas [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_decrement_retry
	nop
	cmp %o4,1
	clr %o0
	retl
	movne %icc,1,%o0
	.size q_atomic_decrement,.-q_atomic_decrement

	.align 4
	.type q_atomic_set_int,#function
	.global q_atomic_set_int
q_atomic_set_int:
q_atomic_set_int_retry:
	ld [%o0],%o2
	cas [%o0],%o2,%o1
	cmp %o2,%o1
	bne q_atomic_set_int_retry
	nop
	retl
	mov %o1,%o0
	.size q_atomic_set_int,.-q_atomic_set_int

	.align 4
	.type q_atomic_set_ptr,#function
	.global q_atomic_set_ptr
q_atomic_set_ptr:
q_atomic_set_ptr_retry:
	ldx [%o0],%o2
	casx [%o0],%o2,%o1
	cmp %o2,%o1
	bne q_atomic_set_ptr_retry
	nop
	retl
	mov %o1,%o0
	.size q_atomic_set_ptr,.-q_atomic_set_ptr

        .align 4
	.type q_atomic_fetch_and_add_int,#function
	.global q_atomic_fetch_and_add_int
q_atomic_fetch_and_add_int:
q_atomic_fetch_and_add_int_retry:
	ld [%o0],%o3
	add %o3,%o1,%o4
	cas [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_fetch_and_add_int_retry
	nop
	retl
	mov %o3,%o0
	.size q_atomic_fetch_and_add_int,.-q_atomic_fetch_and_add_int

        .align 4
	.type q_atomic_fetch_and_add_acquire_int,#function
	.global q_atomic_fetch_and_add_acquire_int
q_atomic_fetch_and_add_acquire_int:
q_atomic_fetch_and_add_acquire_int_retry:
	ld [%o0],%o3
	add %o3,%o1,%o4
	cas [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_fetch_and_add_acquire_int_retry
	nop
	membar #LoadLoad | #LoadStore
	retl
	mov %o3,%o0
	.size q_atomic_fetch_and_add_acquire_int,.-q_atomic_fetch_and_add_acquire_int

        .align 4
	.type q_atomic_fetch_and_add_release_int,#function
	.global q_atomic_fetch_and_add_release_int
q_atomic_fetch_and_add_release_int:
q_atomic_fetch_and_add_release_int_retry:
	membar #LoadStore | #StoreStore
	ld [%o0],%o3
	add %o3,%o1,%o4
	cas [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_fetch_and_add_release_int_retry
	nop
	retl
	mov %o3,%o0
	.size q_atomic_fetch_and_add_release_int,.-q_atomic_fetch_and_add_release_int

	.align 4
	.type q_atomic_fetch_and_store_acquire_int,#function
	.global q_atomic_fetch_and_store_acquire_int
q_atomic_fetch_and_store_acquire_int:
q_atomic_fetch_and_store_acquire_int_retry:
	ld [%o0],%o2
	cas [%o0],%o2,%o1
	cmp %o2,%o1
	bne q_atomic_fetch_and_store_acquire_int_retry
	nop
	membar #LoadLoad | #LoadStore
	retl
	mov %o1,%o0
	.size q_atomic_fetch_and_store_acquire_int,.-q_atomic_fetch_and_store_acquire_int

	.align 4
	.type q_atomic_fetch_and_store_release_int,#function
	.global q_atomic_fetch_and_store_release_int
q_atomic_fetch_and_store_release_int:
q_atomic_fetch_and_store_release_int_retry:
	membar #LoadStore | #StoreStore
	ld [%o0],%o2
	cas [%o0],%o2,%o1
	cmp %o2,%o1
	bne q_atomic_fetch_and_store_release_int_retry
	nop
	retl
	mov %o1,%o0
	.size q_atomic_fetch_and_store_release_int,.-q_atomic_fetch_and_store_release_int

	.align 4
	.type q_atomic_test_and_set_acquire_ptr,#function
	.global q_atomic_test_and_set_acquire_ptr
q_atomic_test_and_set_acquire_ptr:
	casx [%o0],%o1,%o2
	cmp %o1,%o2
	clr %o0
	membar #LoadLoad | #LoadStore
	retl
	move %icc,1,%o0
	.size q_atomic_test_and_set_acquire_ptr,.-q_atomic_test_and_set_acquire_ptr

	.align 4
	.type q_atomic_test_and_set_release_ptr,#function
	.global q_atomic_test_and_set_release_ptr
q_atomic_test_and_set_release_ptr:
        membar #LoadStore | #StoreStore
	casx [%o0],%o1,%o2
	cmp %o1,%o2
	clr %o0
	retl
	move %icc,1,%o0
	.size q_atomic_test_and_set_release_ptr,.-q_atomic_test_and_set_release_ptr

	.align 4
	.type q_atomic_fetch_and_store_acquire_ptr,#function
	.global q_atomic_fetch_and_store_acquire_ptr
q_atomic_fetch_and_store_acquire_ptr:
q_atomic_fetch_and_store_acquire_ptr_retry:
	ldx [%o0],%o2
	casx [%o0],%o2,%o1
	cmp %o2,%o1
	bne q_atomic_fetch_and_store_acquire_ptr_retry
	nop
	membar #LoadLoad | #LoadStore
	retl
	mov %o1,%o0
	.size q_atomic_fetch_and_store_acquire_ptr,.-q_atomic_fetch_and_store_acquire_ptr

	.align 4
	.type q_atomic_fetch_and_store_release_ptr,#function
	.global q_atomic_fetch_and_store_release_ptr
q_atomic_fetch_and_store_release_ptr:
q_atomic_fetch_and_store_release_ptr_retry:
	membar #LoadStore | #StoreStore
	ldx [%o0],%o2
	casx [%o0],%o2,%o1
	cmp %o2,%o1
	bne q_atomic_fetch_and_store_release_ptr_retry
	nop
	retl
	mov %o1,%o0
	.size q_atomic_fetch_and_store_release_ptr,.-q_atomic_fetch_and_store_release_ptr

        .align 4
	.type q_atomic_fetch_and_add_ptr,#function
	.global q_atomic_fetch_and_add_ptr
q_atomic_fetch_and_add_ptr:
q_atomic_fetch_and_add_ptr_retry:
	ldx [%o0],%o3
	add %o3,%o1,%o4
	casx [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_fetch_and_add_ptr_retry
	nop
	retl
	mov %o3,%o0
	.size q_atomic_fetch_and_add_ptr,.-q_atomic_fetch_and_add_ptr

        .align 4
	.type q_atomic_fetch_and_add_acquire_ptr,#function
	.global q_atomic_fetch_and_add_acquire_ptr
q_atomic_fetch_and_add_acquire_ptr:
q_atomic_fetch_and_add_acquire_ptr_retry:
	ldx [%o0],%o3
	add %o3,%o1,%o4
	casx [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_fetch_and_add_acquire_ptr_retry
	nop
	membar #LoadLoad | #LoadStore
	retl
	mov %o3,%o0
	.size q_atomic_fetch_and_add_acquire_ptr,.-q_atomic_fetch_and_add_acquire_ptr

        .align 4
	.type q_atomic_fetch_and_add_release_ptr,#function
	.global q_atomic_fetch_and_add_release_ptr
q_atomic_fetch_and_add_release_ptr:
q_atomic_fetch_and_add_release_ptr_retry:
	membar #LoadStore | #StoreStore
	ldx [%o0],%o3
	add %o3,%o1,%o4
	casx [%o0],%o3,%o4
	cmp %o3,%o4
	bne q_atomic_fetch_and_add_release_ptr_retry
	nop
	retl
	mov %o3,%o0
	.size q_atomic_fetch_and_add_release_ptr,.-q_atomic_fetch_and_add_release_ptr
