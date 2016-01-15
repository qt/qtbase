/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QOPENGLTEXTUREBLITWINDOW_H
#define QOPENGLTEXTUREBLITWINDOW_H

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/private/qopengltextureblitter_p.h>

class QOpenGLTextureBlitWindow : public QWindow
{
    Q_OBJECT
public:
    QOpenGLTextureBlitWindow();

    void render();
protected:
    void exposeEvent(QExposeEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    qreal dWidth() const { return width() * devicePixelRatio(); }
    qreal dHeight() const { return height() * devicePixelRatio(); }

    QScopedPointer<QOpenGLContext> m_context;
    QOpenGLTextureBlitter m_blitter;
    QImage m_image;
    QImage m_image_mirrord;
};

#endif
