/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperhelpers.h"

pp::Rect toPPRect(const QRect rect)
{
    return pp::Rect(rect.x(), rect.y(), rect.size().width(), rect.size().height());
}

QRect toQRect(pp::Rect rect) { return QRect(rect.x(), rect.y(), rect.width(), rect.height()); }

QSize toQSize(pp::Size size) { return QSize(size.width(), size.height()); }

pp::Size toPPSize(const QSize &size) { return pp::Size(size.width(), size.height()); }

QPointF toQPointF(pp::FloatPoint point) { return QPointF(point.x(), point.y()); }

pp::FloatPoint toPPFloatPoint(QPointF point) { return pp::FloatPoint(point.x(), point.y()); }

QPoint toQPointF(pp::Point point) { return QPoint(point.x(), point.y()); }

pp::Point toPPPoint(QPoint point) { return pp::Point(point.x(), point.y()); }

QByteArray toQByteArray(const pp::Var &var) { return QByteArray(var.AsString().data()); }

pp::Var toPPVar(const QByteArray &data) { return pp::Var(data.constData()); }

ThreadSafeRefCount::ThreadSafeRefCount()
    : ref(0)
{
}

int32_t ThreadSafeRefCount::AddRef() { return ref.fetchAndAddOrdered(1); }

int32_t ThreadSafeRefCount::Release() { return ref.fetchAndAddOrdered(-1); }
