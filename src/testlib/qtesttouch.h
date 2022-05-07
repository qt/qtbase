/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QTESTTOUCH_H
#define QTESTTOUCH_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qttestglobal.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtCore/qmap.h>
#include <QtGui/qevent.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/qwindow.h>
#include <QtGui/qpointingdevice.h>
#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qtestsupport_widgets.h>
#else
#include <QtGui/qtestsupport_gui.h>
#endif

QT_BEGIN_NAMESPACE

namespace QTest
{
#if defined(QT_WIDGETS_LIB) || defined(Q_CLANG_QDOC)
    inline
    QTouchEventWidgetSequence touchEvent(QWidget *widget,
                                   QPointingDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventWidgetSequence(widget, device, autoCommit);
    }
#endif
    inline
    QTouchEventSequence touchEvent(QWindow *window,
                                   QPointingDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(window, device, autoCommit);
    }

}

QT_END_NAMESPACE

#endif // QTESTTOUCH_H
