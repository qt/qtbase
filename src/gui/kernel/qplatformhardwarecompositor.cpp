/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
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

#include "qplatformhardwarecompositor.h"

QT_BEGIN_NAMESPACE

/*
    Layer:

    'transform' is a generic transform which can position the layer at
    any given position with optional scale and rotation. It is quite likely that
    the hardware compositor does not support arbitrary rotations and projective
    transforms, but this is an easy way of expressing a number of different
    scale/rotations in a generic way so deal with it.

    'opacity' is the opacity of the layer

    'subRect' is the region of the layer to actually draw. By default
    this is (0,0)->(1,1). The values are relative to size and can
    thus support subpixels.

    'color' If color is a valid color and handle is 0, render the layer as
    a solid color layer.

    'buffer' The buffer to compose. Please note that the compositor might
    lock buffers for read access and retain that lock after the composition
    step has finished.

   // Screen geometry (in pixels)
   +-----------------------------------------
   |\
   | \ transform
   |  \  (0, 0) is top left in "layer coordinates"
   |   +-----------------------------------------+ size.width (in pixels)
   |   |                                         |
   |   |                                         |
   |   |                                         |
   |   |           subRect                       |
   |   |      +---------------------------+      |
   |   |      | float coords in the range |      |
   |   |      | 0-1. 0 is top/left. 1 is  |      |
   |   |      | bottom/right. Relative to |      |
   |   |      | size.                     |      |
   |   |      +---------------------------+      |
   |   |                                         |
   |   +-----------------------------------------+
   |   size.height (in pixels)
   |

*/

QPlatformHardwareCompositor::Layer::Layer()
    : opacity(1)
    , subRect(0, 0, 1, 1)
    , buffer(0)
{
}

QT_END_NAMESPACE
