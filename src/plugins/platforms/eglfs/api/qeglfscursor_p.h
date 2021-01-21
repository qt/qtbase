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

#ifndef QEGLFSCURSOR_H
#define QEGLFSCURSOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qeglfsglobal_p.h"
#include <qpa/qplatformcursor.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/private/qinputdevicemanager_p.h>

#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QOpenGLShaderProgram;
class QEglFSCursor;
class QEglFSScreen;

class QEglFSCursorDeviceListener : public QObject
{
    Q_OBJECT

public:
    QEglFSCursorDeviceListener(QEglFSCursor *cursor) : m_cursor(cursor) { }
    bool hasMouse() const;

public slots:
    void onDeviceListChanged(QInputDeviceManager::DeviceType type);

private:
    QEglFSCursor *m_cursor;
};

#if QT_CONFIG(opengl)

struct QEglFSCursorData {
    QScopedPointer<QOpenGLShaderProgram> program;
    int textureEntry = 0;
    int matEntry = 0;
    uint customCursorTexture = 0;
    uint atlasTexture = 0;
    qint64 customCursorKey = 0;
};

class Q_EGLFS_EXPORT QEglFSCursor : public QPlatformCursor
                                  , protected QOpenGLFunctions
{
    Q_OBJECT
public:
    QEglFSCursor(QPlatformScreen *screen);
    ~QEglFSCursor();

#ifndef QT_NO_CURSOR
    void changeCursor(QCursor *cursor, QWindow *widget) override;
#endif
    void pointerEvent(const QMouseEvent &event) override;
    QPoint pos() const override;
    void setPos(const QPoint &pos) override;

    QRect cursorRect() const;
    void paintOnScreen();
    void resetResources();

    void updateMouseStatus();

private:
    bool event(QEvent *e) override;
#ifndef QT_NO_CURSOR
    bool setCurrentCursor(QCursor *cursor);
#endif
    void draw(const QRectF &rect);
    void update(const QRect &rect, bool allScreens);
    void createShaderPrograms();
    void createCursorTexture(uint *texture, const QImage &image);
    void initCursorAtlas();

    // current cursor information
    struct Cursor {
        Cursor() : shape(Qt::BlankCursor), customCursorPending(false), customCursorKey(0), useCustomCursor(false) { }
        Qt::CursorShape shape;
        QRectF textureRect; // normalized rect inside texture
        QSize size; // size of the cursor
        QPoint hotSpot;
        QImage customCursorImage;
        QPoint pos; // current cursor position
        bool customCursorPending;
        qint64 customCursorKey;
        bool useCustomCursor;
    } m_cursor;

    // cursor atlas information
    struct CursorAtlas {
        CursorAtlas() : cursorsPerRow(0), cursorWidth(0), cursorHeight(0) { }
        int cursorsPerRow;
        int width, height; // width and height of the atlas
        int cursorWidth, cursorHeight; // width and height of cursors inside the atlas
        QVector<QPoint> hotSpots;
        QImage image; // valid until it's uploaded
    } m_cursorAtlas;

    bool m_visible;
    QEglFSScreen *m_screen;
    QPlatformScreen *m_activeScreen;
    QEglFSCursorDeviceListener *m_deviceListener;
    bool m_updateRequested;
    QMatrix4x4 m_rotationMatrix;
};
#endif // QT_CONFIG(opengl)

QT_END_NAMESPACE

#endif // QEGLFSCURSOR_H
