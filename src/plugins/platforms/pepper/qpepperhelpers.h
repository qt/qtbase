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

#ifndef QPEPPERHELPERS_H
#define QPEPPERHELPERS_H

#include <QRect>
#include <QByteArray>
#include <QAtomicInt>

#include <ppapi/cpp/point.h>
#include <ppapi/cpp/rect.h>
#include <ppapi/cpp/var.h>

pp::Rect toPPRect(const QRect rect);
QRect toQRect(pp::Rect rect);
QSize toQSize(pp::Size size);
pp::Size toPPSize(const QSize &size);
QPointF toQPointF(pp::FloatPoint point);
pp::FloatPoint toPPFloatPoint(QPointF point);
QPoint toQPointF(pp::Point point);
pp::Point toPPFloatPoint(QPoint point);
QByteArray toQByteArray(const pp::Var &var);
pp::Var toPPVar(const QByteArray &data);

// QAtomic-based ref count for the callback factory.
class ThreadSafeRefCount
{
public:
    ThreadSafeRefCount();
    int32_t AddRef();
    int32_t Release();
    QAtomicInt ref;
};

#endif
