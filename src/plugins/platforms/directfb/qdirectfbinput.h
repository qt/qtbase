/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
