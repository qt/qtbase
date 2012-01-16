/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef SVGALIBSCREEN_H
#define SVGALIBSCREEN_H

#include <QScreen>

#include <vga.h>
#include <vgagl.h>

//! [0]
class SvgalibScreen : public QScreen
{
public:
    SvgalibScreen(int displayId) : QScreen(displayId) {}
    ~SvgalibScreen() {}

    bool connect(const QString &displaySpec);
    bool initDevice();
    void shutdownDevice();
    void disconnect();

    void setMode(int, int, int) {}
    void blank(bool) {}

    void blit(const QImage &img, const QPoint &topLeft, const QRegion &region);
    void solidFill(const QColor &color, const QRegion &region);
//! [0]

    QWSWindowSurface* createSurface(QWidget *widget) const;
    QWSWindowSurface* createSurface(const QString &key) const;

//! [1]
private:
    void initColorMap();
    void blit16To8(const QImage &image,
                   const QPoint &topLeft, const QRegion &region);
    void blit32To8(const QImage &image,
                   const QPoint &topLeft, const QRegion &region);

    GraphicsContext *context;
};
//! [1]

#endif // SVGALIBSCREEN_H
