/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfscursor.h"
#include "qeglfsintegration.h"
#include "qeglfsscreen.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qopenglvertexarrayobject_p.h>

#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif

QT_BEGIN_NAMESPACE

QEglFSCursor::QEglFSCursor(QPlatformScreen *screen)
    : m_visible(true),
      m_screen(static_cast<QEglFSScreen *>(screen)),
      m_program(0),
      m_textureEntry(0),
      m_deviceListener(0),
      m_updateRequested(false)
{
    QByteArray hideCursorVal = qgetenv("QT_QPA_EGLFS_HIDECURSOR");
    if (!hideCursorVal.isEmpty())
        m_visible = hideCursorVal.toInt() == 0;
    if (!m_visible)
        return;

    // Try to load the cursor atlas. If this fails, m_visible is set to false and
    // paintOnScreen() and setCurrentCursor() become no-ops.
    initCursorAtlas();

    // initialize the cursor
#ifndef QT_NO_CURSOR
    QCursor cursor(Qt::ArrowCursor);
    setCurrentCursor(&cursor);
#endif

    m_deviceListener = new QEglFSCursorDeviceListener(this);
    connect(QGuiApplicationPrivate::inputDeviceManager(), &QInputDeviceManager::deviceListChanged,
            m_deviceListener, &QEglFSCursorDeviceListener::onDeviceListChanged);
    updateMouseStatus();
}

QEglFSCursor::~QEglFSCursor()
{
    resetResources();
    delete m_deviceListener;
}

void QEglFSCursor::updateMouseStatus()
{
    m_visible = m_deviceListener->hasMouse();
}

bool QEglFSCursorDeviceListener::hasMouse() const
{
    return QGuiApplicationPrivate::inputDeviceManager()->deviceCount(QInputDeviceManager::DeviceTypePointer) > 0;
}

void QEglFSCursorDeviceListener::onDeviceListChanged(QInputDeviceManager::DeviceType type)
{
    if (type == QInputDeviceManager::DeviceTypePointer)
        m_cursor->updateMouseStatus();
}

void QEglFSCursor::resetResources()
{
    if (QOpenGLContext::currentContext()) {
        delete m_program;
        glDeleteTextures(1, &m_cursor.customCursorTexture);
        glDeleteTextures(1, &m_cursorAtlas.texture);
    }
    m_program = 0;
    m_cursor.customCursorTexture = 0;
    m_cursor.customCursorPending = !m_cursor.customCursorImage.isNull();
    m_cursorAtlas.texture = 0;
}

void QEglFSCursor::createShaderPrograms()
{
    static const char *textureVertexProgram =
        "attribute highp vec2 vertexCoordEntry;\n"
        "attribute highp vec2 textureCoordEntry;\n"
        "varying highp vec2 textureCoord;\n"
        "void main() {\n"
        "   textureCoord = textureCoordEntry;\n"
        "   gl_Position = vec4(vertexCoordEntry, 1.0, 1.0);\n"
        "}\n";

    static const char *textureFragmentProgram =
        "uniform sampler2D texture;\n"
        "varying highp vec2 textureCoord;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(texture, textureCoord).bgra;\n"
        "}\n";

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
    m_program->bindAttributeLocation("vertexCoordEntry", 0);
    m_program->bindAttributeLocation("textureCoordEntry", 1);
    m_program->link();

    m_textureEntry = m_program->uniformLocation("texture");
}

void QEglFSCursor::createCursorTexture(uint *texture, const QImage &image)
{
    if (!*texture)
        glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA, image.width(), image.height(), 0 /* border */,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
}

void QEglFSCursor::initCursorAtlas()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_CURSOR");
    if (json.isEmpty())
        json = ":/cursor.json";

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        m_visible = false;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject object = doc.object();

    QString atlas = object.value(QLatin1String("image")).toString();
    Q_ASSERT(!atlas.isEmpty());

    const int cursorsPerRow = object.value(QLatin1String("cursorsPerRow")).toDouble();
    Q_ASSERT(cursorsPerRow);
    m_cursorAtlas.cursorsPerRow = cursorsPerRow;

    const QJsonArray hotSpots = object.value(QLatin1String("hotSpots")).toArray();
    Q_ASSERT(hotSpots.count() == Qt::LastCursor + 1);
    for (int i = 0; i < hotSpots.count(); i++) {
        QPoint hotSpot(hotSpots[i].toArray()[0].toDouble(), hotSpots[i].toArray()[1].toDouble());
        m_cursorAtlas.hotSpots << hotSpot;
    }

    QImage image = QImage(atlas).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_cursorAtlas.cursorWidth = image.width() / m_cursorAtlas.cursorsPerRow;
    m_cursorAtlas.cursorHeight = image.height() / ((Qt::LastCursor + cursorsPerRow) / cursorsPerRow);
    m_cursorAtlas.width = image.width();
    m_cursorAtlas.height = image.height();
    m_cursorAtlas.image = image;
}

#ifndef QT_NO_CURSOR
void QEglFSCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    Q_UNUSED(window);
    const QRect oldCursorRect = cursorRect();
    if (setCurrentCursor(cursor))
        update(oldCursorRect | cursorRect());
}

bool QEglFSCursor::setCurrentCursor(QCursor *cursor)
{
    if (!m_visible)
        return false;

    const Qt::CursorShape newShape = cursor ? cursor->shape() : Qt::ArrowCursor;
    if (m_cursor.shape == newShape && newShape != Qt::BitmapCursor)
        return false;

    if (m_cursor.shape == Qt::BitmapCursor) {
        m_cursor.customCursorImage = QImage();
        m_cursor.customCursorPending = false;
    }
    m_cursor.shape = newShape;
    if (newShape != Qt::BitmapCursor) { // standard cursor
        const float ws = (float)m_cursorAtlas.cursorWidth / m_cursorAtlas.width,
                    hs = (float)m_cursorAtlas.cursorHeight / m_cursorAtlas.height;
        m_cursor.textureRect = QRectF(ws * (m_cursor.shape % m_cursorAtlas.cursorsPerRow),
                                      hs * (m_cursor.shape / m_cursorAtlas.cursorsPerRow),
                                      ws, hs);
        m_cursor.hotSpot = m_cursorAtlas.hotSpots[m_cursor.shape];
        m_cursor.texture = m_cursorAtlas.texture;
        m_cursor.size = QSize(m_cursorAtlas.cursorWidth, m_cursorAtlas.cursorHeight);
    } else {
        QImage image = cursor->pixmap().toImage();
        m_cursor.textureRect = QRectF(0, 0, 1, 1);
        m_cursor.hotSpot = cursor->hotSpot();
        m_cursor.texture = 0; // will get updated in the next render()
        m_cursor.size = image.size();
        m_cursor.customCursorImage = image;
        m_cursor.customCursorPending = true;
    }

    return true;
}
#endif

class CursorUpdateEvent : public QEvent
{
public:
    CursorUpdateEvent(const QPoint &pos, const QRegion &rgn)
        : QEvent(QEvent::Type(QEvent::User + 1)),
          m_pos(pos),
          m_region(rgn)
        { }
    QPoint pos() const { return m_pos; }
    QRegion region() const { return m_region; }

private:
    QPoint m_pos;
    QRegion m_region;
};

bool QEglFSCursor::event(QEvent *e)
{
    if (e->type() == QEvent::User + 1) {
        CursorUpdateEvent *ev = static_cast<CursorUpdateEvent *>(e);
        m_updateRequested = false;
        QWindowSystemInterface::handleExposeEvent(m_screen->topLevelAt(ev->pos()), ev->region());
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
        return true;
    }
    return QPlatformCursor::event(e);
}

void QEglFSCursor::update(const QRegion &rgn)
{
    if (!m_updateRequested) {
        // Must not flush the window system events directly from here since we are likely to
        // be a called directly from QGuiApplication's processMouseEvents. Flushing events
        // could cause reentering by dispatching more queued mouse events.
        m_updateRequested = true;
        QCoreApplication::postEvent(this, new CursorUpdateEvent(m_cursor.pos, rgn));
    }
}

QRect QEglFSCursor::cursorRect() const
{
    return QRect(m_cursor.pos - m_cursor.hotSpot, m_cursor.size);
}

QPoint QEglFSCursor::pos() const
{
    return m_cursor.pos;
}

void QEglFSCursor::setPos(const QPoint &pos)
{
    QGuiApplicationPrivate::inputDeviceManager()->setCursorPos(pos);
    const QRect oldCursorRect = cursorRect();
    m_cursor.pos = pos;
    update(oldCursorRect | cursorRect());
    m_screen->handleCursorMove(m_cursor.pos);
}

void QEglFSCursor::pointerEvent(const QMouseEvent &event)
{
    if (event.type() != QEvent::MouseMove)
        return;
    const QRect oldCursorRect = cursorRect();
    m_cursor.pos = event.screenPos().toPoint();
    update(oldCursorRect | cursorRect());
    m_screen->handleCursorMove(m_cursor.pos);
}

void QEglFSCursor::paintOnScreen()
{
    if (!m_visible)
        return;

    const QRectF cr = cursorRect();
    const QRect screenRect(m_screen->geometry());
    const GLfloat x1 = 2 * (cr.left() / screenRect.width()) - 1;
    const GLfloat x2 = 2 * (cr.right() / screenRect.width()) - 1;
    const GLfloat y1 = 1 - (cr.top() / screenRect.height()) * 2;
    const GLfloat y2 = 1 - (cr.bottom() / screenRect.height()) * 2;
    QRectF r(QPointF(x1, y1), QPointF(x2, y2));

    draw(r);
}

// In order to prevent breaking code doing custom OpenGL rendering while
// expecting the state in the context unchanged, save and restore all the state
// we touch. The exception is Qt Quick where the scenegraph is known to be able
// to deal with the changes we make.
struct StateSaver
{
    StateSaver() {
        f = QOpenGLContext::currentContext()->functions();
        vaoHelper = new QOpenGLVertexArrayObjectHelper(QOpenGLContext::currentContext());

        static bool windowsChecked = false;
        static bool shouldSave = true;
        if (!windowsChecked) {
            windowsChecked = true;
            QWindowList windows = QGuiApplication::allWindows();
            if (!windows.isEmpty() && windows[0]->inherits("QQuickWindow"))
                shouldSave = false;
        }
        saved = shouldSave;
        if (!shouldSave)
            return;

        f->glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        f->glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        f->glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        f->glGetIntegerv(GL_FRONT_FACE, &frontFace);
        cull = f->glIsEnabled(GL_CULL_FACE);
        depthTest = f->glIsEnabled(GL_DEPTH_TEST);
        blend = f->glIsEnabled(GL_BLEND);
        f->glGetIntegerv(GL_BLEND_SRC_RGB, blendFunc);
        f->glGetIntegerv(GL_BLEND_SRC_ALPHA, blendFunc + 1);
        f->glGetIntegerv(GL_BLEND_DST_RGB, blendFunc + 2);
        f->glGetIntegerv(GL_BLEND_DST_ALPHA, blendFunc + 3);
        f->glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuf);
        if (vaoHelper->isValid())
            f->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        for (int i = 0; i < 2; ++i) {
            f->glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &va[i].enabled);
            f->glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &va[i].size);
            f->glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &va[i].type);
            f->glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &va[i].normalized);
            f->glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &va[i].stride);
            f->glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &va[i].buffer);
            f->glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &va[i].pointer);
        }
    }
    ~StateSaver() {
        if (saved) {
            f->glUseProgram(program);
            f->glBindTexture(GL_TEXTURE_2D, texture);
            f->glActiveTexture(activeTexture);
            f->glFrontFace(frontFace);
            if (cull)
                f->glEnable(GL_CULL_FACE);
            else
                f->glDisable(GL_CULL_FACE);
            if (depthTest)
                f->glEnable(GL_DEPTH_TEST);
            else
                f->glDisable(GL_DEPTH_TEST);
            if (blend)
                f->glEnable(GL_BLEND);
            else
                f->glDisable(GL_BLEND);
            f->glBlendFuncSeparate(blendFunc[0], blendFunc[1], blendFunc[2], blendFunc[3]);
            f->glBindBuffer(GL_ARRAY_BUFFER, arrayBuf);
            if (vaoHelper->isValid())
                vaoHelper->glBindVertexArray(vao);
            for (int i = 0; i < 2; ++i) {
                if (va[i].enabled)
                    f->glEnableVertexAttribArray(i);
                else
                    f->glDisableVertexAttribArray(i);
                f->glBindBuffer(GL_ARRAY_BUFFER, va[i].buffer);
                f->glVertexAttribPointer(i, va[i].size, va[i].type, va[i].normalized, va[i].stride, va[i].pointer);
            }
        }
        delete vaoHelper;
    }
    QOpenGLFunctions *f;
    QOpenGLVertexArrayObjectHelper *vaoHelper;
    bool saved;
    GLint program;
    GLint texture;
    GLint activeTexture;
    GLint frontFace;
    bool cull;
    bool depthTest;
    bool blend;
    GLint blendFunc[4];
    GLint vao;
    GLint arrayBuf;
    struct { GLint enabled, type, size, normalized, stride, buffer; GLvoid *pointer; } va[2];
};

void QEglFSCursor::draw(const QRectF &r)
{
    StateSaver stateSaver;

    if (!m_program) {
        // one time initialization
        initializeOpenGLFunctions();

        createShaderPrograms();

        if (!m_cursorAtlas.texture) {
            createCursorTexture(&m_cursorAtlas.texture, m_cursorAtlas.image);

            if (m_cursor.shape != Qt::BitmapCursor)
                m_cursor.texture = m_cursorAtlas.texture;
        }
    }

    if (m_cursor.shape == Qt::BitmapCursor && m_cursor.customCursorPending) {
        // upload the custom cursor
        createCursorTexture(&m_cursor.customCursorTexture, m_cursor.customCursorImage);
        m_cursor.texture = m_cursor.customCursorTexture;
        m_cursor.customCursorPending = false;
    }

    Q_ASSERT(m_cursor.texture);

    m_program->bind();

    const GLfloat x1 = r.left();
    const GLfloat x2 = r.right();
    const GLfloat y1 = r.top();
    const GLfloat y2 = r.bottom();
    const GLfloat cursorCoordinates[] = {
        x1, y2,
        x2, y2,
        x1, y1,
        x2, y1
    };

    const GLfloat s1 = m_cursor.textureRect.left();
    const GLfloat s2 = m_cursor.textureRect.right();
    const GLfloat t1 = m_cursor.textureRect.top();
    const GLfloat t2 = m_cursor.textureRect.bottom();
    const GLfloat textureCoordinates[] = {
        s1, t2,
        s2, t2,
        s1, t1,
        s2, t1
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_cursor.texture);

    if (stateSaver.vaoHelper->isValid())
        stateSaver.vaoHelper->glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    m_program->setAttributeArray(0, cursorCoordinates, 2);
    m_program->setAttributeArray(1, textureCoordinates, 2);

    m_program->setUniformValue(m_textureEntry, 0);

    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // disable depth testing to make sure cursor is always on top

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->disableAttributeArray(0);
    m_program->disableAttributeArray(1);
    m_program->release();
}

QT_END_NAMESPACE
