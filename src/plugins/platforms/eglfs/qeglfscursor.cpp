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

#include "qeglfscursor.h"
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QOpenGLContext>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtDebug>

QT_BEGIN_NAMESPACE

QEglFSCursor::QEglFSCursor(QEglFSScreen *screen)
    : m_screen(screen), m_program(0), m_vertexCoordEntry(0), m_textureCoordEntry(0), m_textureEntry(0)
{
    initCursorAtlas();

    // initialize the cursor
#ifndef QT_NO_CURSOR
    QCursor cursor(Qt::ArrowCursor);
    setCurrentCursor(&cursor);
#endif
}

QEglFSCursor::~QEglFSCursor()
{
    if (QOpenGLContext::currentContext()) {
        glDeleteProgram(m_program);
        glDeleteTextures(1, &m_cursor.customCursorTexture);
        glDeleteTextures(1, &m_cursorAtlas.texture);
    }
}

static GLuint createShader(GLenum shaderType, const char *program)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1 /* count */, &program, NULL /* lengths */);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE)
        return shader;

    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char *infoLog = new char[length];
    glGetShaderInfoLog(shader, length, NULL, infoLog);
    qDebug("%s shader compilation error: %s", shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment", infoLog);
    delete [] infoLog;
    return 0;
}

static GLuint createProgram(GLuint vshader, GLuint fshader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE)
        return program;

    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    char *infoLog = new char[length];
    glGetProgramInfoLog(program, length, NULL, infoLog);
    qDebug("program link error: %s", infoLog);
    delete [] infoLog;
    return 0;
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

    GLuint vertexShader = createShader(GL_VERTEX_SHADER, textureVertexProgram);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, textureFragmentProgram);
    m_program = createProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    m_vertexCoordEntry = glGetAttribLocation(m_program, "vertexCoordEntry");
    m_textureCoordEntry = glGetAttribLocation(m_program, "textureCoordEntry");
    m_textureEntry = glGetUniformLocation(m_program, "texture");
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

    QFile file(json);
    file.open(QFile::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject object = doc.object();

    QString atlas = object.value("image").toString();
    Q_ASSERT(!atlas.isEmpty());

    const int cursorsPerRow = object.value("cursorsPerRow").toDouble();
    Q_ASSERT(cursorsPerRow);
    m_cursorAtlas.cursorsPerRow = cursorsPerRow;

    const QJsonArray hotSpots = object.value("hotSpots").toArray();
    Q_ASSERT(hotSpots.count() == Qt::LastCursor);
    for (int i = 0; i < hotSpots.count(); i++) {
        QPoint hotSpot(hotSpots[i].toArray()[0].toDouble(), hotSpots[i].toArray()[1].toDouble());
        m_cursorAtlas.hotSpots << hotSpot;
    }

    QImage image = QImage(atlas).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_cursorAtlas.cursorWidth = image.width() / m_cursorAtlas.cursorsPerRow;
    m_cursorAtlas.cursorHeight = image.height() / ((Qt::LastCursor + cursorsPerRow - 1) / cursorsPerRow);
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
    const Qt::CursorShape newShape = cursor ? cursor->shape() : Qt::ArrowCursor;
    if (m_cursor.shape == newShape && newShape != Qt::BitmapCursor)
        return false;

    if (m_cursor.shape == Qt::BitmapCursor) {
        m_cursor.customCursorImage = QImage(); // in case render() never uploaded it
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
    }

    return true;
}
#endif

void QEglFSCursor::update(const QRegion &rgn)
{
    QWindowSystemInterface::handleExposeEvent(m_screen->topLevelAt(m_cursor.pos), rgn);
    QWindowSystemInterface::flushWindowSystemEvents();
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
    const QRect oldCursorRect = cursorRect();
    m_cursor.pos = pos;
    update(oldCursorRect | cursorRect());
}

void QEglFSCursor::pointerEvent(const QMouseEvent &event)
{
    if (event.type() != QEvent::MouseMove)
        return;
    const QRect oldCursorRect = cursorRect();
    m_cursor.pos = event.pos();
    update(oldCursorRect | cursorRect());
}

void QEglFSCursor::paintOnScreen()
{
    const QRectF cr = cursorRect();
    const QRect screenRect(m_screen->geometry());
    const GLfloat x1 = 2 * (cr.left() / screenRect.width()) - 1;
    const GLfloat x2 = 2 * (cr.right() / screenRect.width()) - 1;
    const GLfloat y1 = 1 - (cr.top() / screenRect.height()) * 2;
    const GLfloat y2 = 1 - (cr.bottom() / screenRect.height()) * 2;
    QRectF r(QPointF(x1, y1), QPointF(x2, y2));

    draw(r);
}

void QEglFSCursor::draw(const QRectF &r)
{
    if (!m_program) {
        // one time initialization
        createShaderPrograms();

        if (!m_cursorAtlas.texture) {
            createCursorTexture(&m_cursorAtlas.texture, m_cursorAtlas.image);
            m_cursorAtlas.image = QImage();

            if (m_cursor.shape != Qt::BitmapCursor)
                m_cursor.texture = m_cursorAtlas.texture;
        }
    }

    if (m_cursor.shape == Qt::BitmapCursor && !m_cursor.customCursorImage.isNull()) {
        // upload the custom cursor
        createCursorTexture(&m_cursor.customCursorTexture, m_cursor.customCursorImage);
        m_cursor.texture = m_cursor.customCursorTexture;
        m_cursor.customCursorImage = QImage();
    }

    Q_ASSERT(m_cursor.texture);

    glUseProgram(m_program);

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

    glBindTexture(GL_TEXTURE_2D, m_cursor.texture);

    glEnableVertexAttribArray(m_vertexCoordEntry);
    glEnableVertexAttribArray(m_textureCoordEntry);

    glVertexAttribPointer(m_vertexCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, cursorCoordinates);
    glVertexAttribPointer(m_textureCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates);

    glUniform1f(m_textureEntry, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // disable depth testing to make sure cursor is always on top
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(m_vertexCoordEntry);
    glDisableVertexAttribArray(m_textureCoordEntry);

    glUseProgram(0);
}

QT_END_NAMESPACE
