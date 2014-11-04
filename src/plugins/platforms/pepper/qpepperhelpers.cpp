/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperhelpers.h"

pp::Rect toPPRect(const QRect rect)
{
    return pp::Rect(rect.x(), rect.y(), rect.size().width(), rect.size().height());
}

QRect toQRect(pp::Rect rect)
{
    return QRect(rect.x(), rect.y(), rect.width(), rect.height());
}

QSize toQSize(pp::Size size)
{
    return QSize(size.width(), size.height());
}

pp::Size toPPSize(const QSize &size)
{
    return pp::Size(size.width(), size.height());
}

QPointF toQPointF(pp::FloatPoint point)
{
    return QPointF(point.x(), point.y());
}

pp::FloatPoint toPPFloatPoint(QPointF point)
{
    return pp::FloatPoint(point.x(), point.y());
}

ThreadSafeRefCount::ThreadSafeRefCount()
   : ref(0) { }

int32_t ThreadSafeRefCount::AddRef()
{
    return ref.fetchAndAddOrdered(1);
}

int32_t ThreadSafeRefCount::Release()
{
    return ref.fetchAndAddOrdered(-1);
}




