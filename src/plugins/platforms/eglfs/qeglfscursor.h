/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLFSCURSOR_H
#define QEGLFSCURSOR_H

#include <qpa/qplatformcursor.h>
#include "qeglfsscreen.h"
#include <GLES2/gl2.h>

QT_BEGIN_NAMESPACE

class QOpenGLShaderProgram;

class QEglFSCursor : public QPlatformCursor
{
public:
    QEglFSCursor(QEglFSScreen *screen);
    ~QEglFSCursor();

    void changeCursor(QCursor *cursor, QWindow *widget);
    void pointerEvent(const QMouseEvent &event);

    QPoint pos() const;
    void setPos(const QPoint &pos);

    QRect cursorRect() const;

    void render();

private:
    void createShaderPrograms();
    static void createCursorTexture(uint *texture, const QImage &image);
    void initCursorAtlas();

    QPlatformScreen *m_screen;

    // cursor atlas information
    struct CursorAtlas {
        CursorAtlas() : cursorsPerRow(0), texture(0), cursorWidth(0), cursorHeight(0) { }
        int cursorsPerRow;
        uint texture;
        int width, height; // width and height of the the atlas
        int cursorWidth, cursorHeight; // width and height of cursors inside the atlas
        QList<QPoint> hotSpots;
    } m_cursorAtlas;

    // current cursor information
    struct Cursor {
        Cursor() : texture(0), shape(Qt::BlankCursor) { }
        uint texture; // a texture from 'image' or the atlas
        Qt::CursorShape shape;
        QRectF textureRect; // normalized rect inside texture
        QSize size; // size of the cursor
        QPoint hotSpot;
    } m_cursor;

    QPoint m_pos;

    GLuint m_program;
    int m_vertexCoordEntry;
    int m_textureCoordEntry;
    int m_textureEntry;
};

QT_END_NAMESPACE

#endif // QEGLFSCURSOR_H

