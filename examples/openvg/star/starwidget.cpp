/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "starwidget.h"

StarWidget::StarWidget(QWidget *parent)
    : QWidget(parent)
    , path(VG_INVALID_HANDLE)
    , pen(Qt::red, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , brush(Qt::yellow)
{
    setMinimumSize(220, 250);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

static VGubyte const starSegments[] = {
    VG_MOVE_TO_ABS,
    VG_LINE_TO_REL,
    VG_LINE_TO_REL,
    VG_LINE_TO_REL,
    VG_LINE_TO_REL,
    VG_CLOSE_PATH
};
static VGfloat const starCoords[] = {
    110, 35,
    50, 160,
    -130, -100,
    160, 0,
    -130, 100
};

void StarWidget::paintEvent(QPaintEvent *)
{
    QPainter painter;
    painter.begin(this);

    // Make sure that we are using the OpenVG paint engine.
    if (painter.paintEngine()->type() != QPaintEngine::OpenVG) {
#ifdef Q_WS_QWS
        qWarning("Not using OpenVG: use the '-display' option to specify an OpenVG driver");
#else
        qWarning("Not using OpenVG: specify '-graphicssystem OpenVG'");
#endif
        return;
    }

    // Select a pen and a brush for drawing the star.
    painter.setPen(pen);
    painter.setBrush(brush);

    // We want the star border to be anti-aliased.
    painter.setRenderHints(QPainter::Antialiasing);

    // Flush the state changes to the OpenVG implementation
    // and prepare to perform raw OpenVG calls.
    painter.beginNativePainting();

    // Cache the path if we haven't already or if the path has
    // become invalid because the window's context has changed.
    if (path == VG_INVALID_HANDLE || !vgGetPathCapabilities(path)) {
        path = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1.0f, // scale
                            0.0f, // bias
                            6,    // segmentCapacityHint
                            10,   // coordCapacityHint
                            VG_PATH_CAPABILITY_ALL);
        vgAppendPathData(path, sizeof(starSegments), starSegments, starCoords);
    }

    // Draw the star directly using the OpenVG API.
    vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);

    // Restore normal QPainter operations.
    painter.endNativePainting();

    painter.end();
}
