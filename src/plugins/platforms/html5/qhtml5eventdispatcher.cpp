/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhtml5eventdispatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

QHtml5EventDispatcher::QHtml5EventDispatcher(QObject *parent)
    : QUnixEventDispatcherQPA(parent)
    , m_hasPendingProcessEvents(false)
{
}

QHtml5EventDispatcher::~QHtml5EventDispatcher() {}

bool QHtml5EventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    bool processed = false;

    // We need to give the control back to the browser due to lack of PTHREADS
    // Limit the number of events that may be processed at the time
    int maxProcessedEvents = 10;
    int processedCount = 0;
    do {
        processed = QUnixEventDispatcherQPA::processEvents(flags);
        processedCount += 1;
    } while (processed && hasPendingEvents() && processedCount < maxProcessedEvents);
    return true;
}

bool QHtml5EventDispatcher::hasPendingEvents()
{
    return QUnixEventDispatcherQPA::hasPendingEvents();
}
