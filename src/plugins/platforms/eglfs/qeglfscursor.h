/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
class QEglFSScreen;

class QEglFSCursor : public QPlatformCursor
{
public:
    QEglFSCursor(QEglFSScreen *screen);
    ~QEglFSCursor();

#ifndef QT_NO_CURSOR
    void changeCursor(QCursor *cursor, QWindow *widget) Q_DECL_OVERRIDE;
#endif
    void pointerEvent(const QMouseEvent &event) Q_DECL_OVERRIDE;

    QPoint pos() const Q_DECL_OVERRIDE;
    void setPos(const QPoint &pos) Q_DECL_OVERRIDE;

    QRect cursorRect() const;

    virtual void paintOnScreen();

protected:
#ifndef QT_NO_CURSOR
    bool setCurrentCursor(QCursor *cursor);
#endif
    void draw(const QRectF &rect);
    void update(const QRegion &region);

    QEglFSScreen *m_screen;

    // current cursor information
    struct Cursor {
        Cursor() : texture(0), shape(Qt::BlankCursor), customCursorTexture(0) { }
        uint texture; // a texture from 'image' or the atlas
        Qt::CursorShape shape;
        QRectF textureRect; // normalized rect inside texture
        QSize size; // size of the cursor
        QPoint hotSpot;
        QImage customCursorImage;
        QPoint pos; // current cursor position
        uint customCursorTexture;
    } m_cursor;

private:
    void createShaderPrograms();
    static void createCursorTexture(uint *texture, const QImage &image);
    void initCursorAtlas();

    // cursor atlas information
    struct CursorAtlas {
        CursorAtlas() : cursorsPerRow(0), texture(0), cursorWidth(0), cursorHeight(0) { }
        int cursorsPerRow;
        uint texture;
        int width, height; // width and height of the the atlas
        int cursorWidth, cursorHeight; // width and height of cursors inside the atlas
        QList<QPoint> hotSpots;
        QImage image; // valid until it's uploaded
    } m_cursorAtlas;

    GLuint m_program;
    int m_vertexCoordEntry;
    int m_textureCoordEntry;
    int m_textureEntry;
};

QT_END_NAMESPACE

#endif // QEGLFSCURSOR_H

