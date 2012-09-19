/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation
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

#include <private/qsimd_p.h>

#ifdef QT_COMPILER_SUPPORTS_AVX
#define QDRAWHELPER_AVX

#ifndef __AVX__
#error "AVX not enabled in this file, cannot proceed"
#endif

#define qt_blend_argb32_on_argb32_ssse3 qt_blend_argb32_on_argb32_avx
#include "qdrawhelper_ssse3.cpp"

//#define qt_blend_argb32_on_argb32_sse2 qt_blend_argb32_on_argb32_avx
#define qt_blend_rgb32_on_rgb32_sse2 qt_blend_rgb32_on_rgb32_avx
#define comp_func_SourceOver_sse2 comp_func_SourceOver_avx
#define comp_func_Plus_sse2 comp_func_Plus_avx
#define comp_func_Source_sse2 comp_func_Source_avx
#define comp_func_solid_SourceOver_sse2 comp_func_solid_SourceOver_avx
#define qt_memfill32_sse2 qt_memfill32_avx
#define qt_memfill16_sse2 qt_memfill16_avx
#define qt_bitmapblit32_sse2 qt_bitmapblit32_avx
#define qt_bitmapblit16_sse2 qt_bitmapblit16_avx
#define QSimdSse2 QSimdAvx
#define qt_fetch_radial_gradient_sse2 qt_fetch_radial_gradient_avx
#define qt_scale_image_argb32_on_argb32_sse2 qt_scale_image_argb32_on_argb32_avx

#include "qdrawhelper_sse2.cpp"

#endif
