/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectfbwindow.h"
#include "qdirectfbinput.h"
#include "qdirectfbglcontext.h"

#include <QWidget>

#include "qdirectfbwindowsurface.h"

#include <directfb.h>

QDirectFbWindow::QDirectFbWindow(QWidget *tlw, QDirectFbInput *inputhandler)
    : QPlatformWindow(tlw), m_inputHandler(inputhandler), m_context(0)
{
    IDirectFBDisplayLayer *layer = QDirectFbConvenience::dfbDisplayLayer();
    DFBDisplayLayerConfig layerConfig;
    layer->GetConfiguration(layer,&layerConfig);

    DFBWindowDescription description;
    memset(&description,0,sizeof(DFBWindowDescription));
    description.flags = DFBWindowDescriptionFlags(DWDESC_WIDTH|DWDESC_HEIGHT|DWDESC_POSX|DWDESC_POSY|DWDESC_SURFACE_CAPS
#if DIRECTFB_MINOR_VERSION >= 1
                                                  |DWDESC_OPTIONS
#endif
                                                  |DWDESC_CAPS);
    description.width = tlw->rect().width();
    description.height = tlw->rect().height();
    description.posx = tlw->rect().x();
    description.posy = tlw->rect().y();

    if (layerConfig.surface_caps & DSCAPS_PREMULTIPLIED)
        description.surface_caps = DSCAPS_PREMULTIPLIED;
    description.pixelformat = layerConfig.pixelformat;

#if DIRECTFB_MINOR_VERSION >= 1
    description.options = DFBWindowOptions(DWOP_ALPHACHANNEL);
#endif
    description.caps = DFBWindowCapabilities(DWCAPS_DOUBLEBUFFER|DWCAPS_ALPHACHANNEL);
    description.surface_caps = DSCAPS_PREMULTIPLIED;

    DFBResult result = layer->CreateWindow(layer,&description,&m_dfbWindow);
    if (result != DFB_OK) {
        DirectFBError("QDirectFbGraphicsSystemScreen: failed to create window",result);
    }

    m_dfbWindow->SetOpacity(m_dfbWindow,0xff);

    setVisible(widget()->isVisible());

    DFBWindowID id;
    m_dfbWindow->GetID(m_dfbWindow, &id);
    m_inputHandler->addWindow(id,tlw);
}

QDirectFbWindow::~QDirectFbWindow()
{
    m_inputHandler->removeWindow(winId());
    m_dfbWindow->Destroy(m_dfbWindow);
}

void QDirectFbWindow::setGeometry(const QRect &rect)
{
    bool isMoveOnly = (rect.topLeft() != geometry().topLeft()) && (rect.size() == geometry().size());
    QPlatformWindow::setGeometry(rect);
    if (widget()->isVisible() && !(widget()->testAttribute(Qt::WA_DontShowOnScreen))) {
        m_dfbWindow->SetBounds(m_dfbWindow, rect.x(),rect.y(),
                               rect.width(), rect.height());

        //Hack. When moving since the WindowSurface of a window becomes invalid when moved
        if (isMoveOnly) { //if resize then windowsurface is updated.
            widget()->windowSurface()->resize(rect.size());
            widget()->update();
        }
    }
}

void QDirectFbWindow::setOpacity(qreal level)
{
    const quint8 windowOpacity = quint8(level * 0xff);
    m_dfbWindow->SetOpacity(m_dfbWindow,windowOpacity);
}

void QDirectFbWindow::setVisible(bool visible)
{
    if (visible) {
        int x = geometry().x();
        int y = geometry().y();
        m_dfbWindow->MoveTo(m_dfbWindow,x,y);
    } else {
        IDirectFBDisplayLayer *displayLayer;
        QDirectFbConvenience::dfbInterface()->GetDisplayLayer(QDirectFbConvenience::dfbInterface(),DLID_PRIMARY,&displayLayer);

        DFBDisplayLayerConfig config;
        displayLayer->GetConfiguration(displayLayer,&config);
        m_dfbWindow->MoveTo(m_dfbWindow,config.width+1,config.height + 1);
    }
}

Qt::WindowFlags QDirectFbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    switch (flags & Qt::WindowType_Mask) {
    case Qt::ToolTip: {
        DFBWindowOptions options;
        m_dfbWindow->GetOptions(m_dfbWindow,&options);
        options = DFBWindowOptions(options | DWOP_GHOST);
        m_dfbWindow->SetOptions(m_dfbWindow,options);
        break; }
    default:
        break;
    }

    m_dfbWindow->SetStackingClass(m_dfbWindow, flags & Qt::WindowStaysOnTopHint ? DWSC_UPPER : DWSC_MIDDLE);
    return flags;
}

void QDirectFbWindow::raise()
{
    m_dfbWindow->RaiseToTop(m_dfbWindow);
}

void QDirectFbWindow::lower()
{
    m_dfbWindow->LowerToBottom(m_dfbWindow);
}

WId QDirectFbWindow::winId() const
{
    DFBWindowID id;
    m_dfbWindow->GetID(m_dfbWindow, &id);
    return WId(id);
}

QPlatformGLContext *QDirectFbWindow::glContext() const
{
    if (!m_context) {
        IDirectFBSurface *surface;
        DFBResult result = m_dfbWindow->GetSurface(m_dfbWindow,&surface);
        if (result != DFB_OK) {
            qWarning("could not retrieve surface in QDirectFbWindow::glContext()");
            return 0;
        }
        IDirectFBGL *gl;
        result = surface->GetGL(surface,&gl);
        if (result != DFB_OK) {
            qWarning("could not retrieve IDirectFBGL in QDirectFbWindow::glContext()");
            return 0;
        }
        const_cast<QDirectFbWindow *>(this)->m_context = new QDirectFbGLContext(gl);
    }
    return m_context;
}
