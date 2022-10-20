/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDIRECTFBINPUT_H
#define QDIRECTFBINPUT_H

#include <QThread>
#include <QHash>
#include <QPoint>
#include <QEvent>

#include <QtGui/qwindowdefs.h>

#include "qdirectfbconvenience.h"

QT_BEGIN_NAMESPACE

class QDirectFbInput : public QThread
{
    Q_OBJECT
public:
    QDirectFbInput(IDirectFB *dfb, IDirectFBDisplayLayer *dfbLayer);
    void addWindow(IDirectFBWindow *window, QWindow *platformWindow);
    void removeWindow(IDirectFBWindow *window);

    void stopInputEventLoop();

protected:
    void run();

private:
    void handleEvents();
    void handleMouseEvents(const DFBEvent &event);
    void handleWheelEvent(const DFBEvent &event);
    void handleKeyEvents(const DFBEvent &event);
    void handleEnterLeaveEvents(const DFBEvent &event);
    void handleGotFocusEvent(const DFBEvent &event);
    void handleCloseEvent(const DFBEvent& event);
    void handleGeometryEvent(const DFBEvent& event);


    IDirectFB *m_dfbInterface;
    IDirectFBDisplayLayer *m_dfbDisplayLayer;
    QDirectFBPointer<IDirectFBEventBuffer> m_eventBuffer;

    bool m_shouldStop;
    QHash<DFBWindowID,QWindow *>m_tlwMap;
};

QT_END_NAMESPACE

#endif // QDIRECTFBINPUT_H
