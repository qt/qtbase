/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QGLSHADERPROGRAM_H
#define QGLSHADERPROGRAM_H

#include <QtOpenGL/qgl.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>
#include <QtGui/qmatrix4x4.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(OpenGL)

#if !defined(QT_OPENGL_ES_1)

class QGLShaderProgram;
class QGLShaderPrivate;

class Q_OPENGL_EXPORT QGLShader : public QObject
{
    Q_OBJECT
public:
    enum ShaderTypeBit
    {
        Vertex          = 0x0001,
        Fragment        = 0x0002,
        Geometry        = 0x0004
    };
    Q_DECLARE_FLAGS(ShaderType, ShaderTypeBit)

    explicit QGLShader(QGLShader::ShaderType type, QObject *parent = 0);
    QGLShader(QGLShader::ShaderType type, const QGLContext *context, QObject *parent = 0);
    virtual ~QGLShader();

    QGLShader::ShaderType shaderType() const;

    bool compileSourceCode(const char *source);
    bool compileSourceCode(const QByteArray& source);
    bool compileSourceCode(const QString& source);
    bool compileSourceFile(const QString& fileName);

    QByteArray sourceCode() const;

    bool isCompiled() const;
    QString log() const;

    GLuint shaderId() const;

    static bool hasOpenGLShaders(ShaderType type, const QGLContext *context = 0);

private:
    friend class QGLShaderProgram;

    Q_DISABLE_COPY(QGLShader)
    Q_DECLARE_PRIVATE(QGLShader)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGLShader::ShaderType)


class QGLShaderProgramPrivate;

#ifndef GL_EXT_geometry_shader4
#  define GL_LINES_ADJACENCY_EXT 0xA
#  define GL_LINE_STRIP_ADJACENCY_EXT 0xB
#  define GL_TRIANGLES_ADJACENCY_EXT 0xC
#  define GL_TRIANGLE_STRIP_ADJACENCY_EXT 0xD
#endif


class Q_OPENGL_EXPORT QGLShaderProgram : public QObject
{
    Q_OBJECT
public:
    explicit QGLShaderProgram(QObject *parent = 0);
    explicit QGLShaderProgram(const QGLContext *context, QObject *parent = 0);
    virtual ~QGLShaderProgram();

    bool addShader(QGLShader *shader);
    void removeShader(QGLShader *shader);
    QList<QGLShader *> shaders() const;

    bool addShaderFromSourceCode(QGLShader::ShaderType type, const char *source);
    bool addShaderFromSourceCode(QGLShader::ShaderType type, const QByteArray& source);
    bool addShaderFromSourceCode(QGLShader::ShaderType type, const QString& source);
    bool addShaderFromSourceFile(QGLShader::ShaderType type, const QString& fileName);

    void removeAllShaders();

    virtual bool link();
    bool isLinked() const;
    QString log() const;

    bool bind();
    void release();

    GLuint programId() const;

    int maxGeometryOutputVertices() const;

    void setGeometryOutputVertexCount(int count);
    int geometryOutputVertexCount() const;

    void setGeometryInputType(GLenum inputType);
    GLenum geometryInputType() const;

    void setGeometryOutputType(GLenum outputType);
    GLenum geometryOutputType() const;

    void bindAttributeLocation(const char *name, int location);
    void bindAttributeLocation(const QByteArray& name, int location);
    void bindAttributeLocation(const QString& name, int location);

    int attributeLocation(const char *name) const;
    int attributeLocation(const QByteArray& name) const;
    int attributeLocation(const QString& name) const;

    void setAttributeValue(int location, GLfloat value);
    void setAttributeValue(int location, GLfloat x, GLfloat y);
    void setAttributeValue(int location, GLfloat x, GLfloat y, GLfloat z);
    void setAttributeValue(int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void setAttributeValue(int location, const QVector2D& value);
    void setAttributeValue(int location, const QVector3D& value);
    void setAttributeValue(int location, const QVector4D& value);
    void setAttributeValue(int location, const QColor& value);
    void setAttributeValue(int location, const GLfloat *values, int columns, int rows);

    void setAttributeValue(const char *name, GLfloat value);
    void setAttributeValue(const char *name, GLfloat x, GLfloat y);
    void setAttributeValue(const char *name, GLfloat x, GLfloat y, GLfloat z);
    void setAttributeValue(const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void setAttributeValue(const char *name, const QVector2D& value);
    void setAttributeValue(const char *name, const QVector3D& value);
    void setAttributeValue(const char *name, const QVector4D& value);
    void setAttributeValue(const char *name, const QColor& value);
    void setAttributeValue(const char *name, const GLfloat *values, int columns, int rows);

    void setAttributeArray
        (int location, const GLfloat *values, int tupleSize, int stride = 0);
    void setAttributeArray
        (int location, const QVector2D *values, int stride = 0);
    void setAttributeArray
        (int location, const QVector3D *values, int stride = 0);
    void setAttributeArray
        (int location, const QVector4D *values, int stride = 0);
    void setAttributeArray
        (int location, GLenum type, const void *values, int tupleSize, int stride = 0);
    void setAttributeArray
        (const char *name, const GLfloat *values, int tupleSize, int stride = 0);
    void setAttributeArray
        (const char *name, const QVector2D *values, int stride = 0);
    void setAttributeArray
        (const char *name, const QVector3D *values, int stride = 0);
    void setAttributeArray
        (const char *name, const QVector4D *values, int stride = 0);
    void setAttributeArray
        (const char *name, GLenum type, const void *values, int tupleSize, int stride = 0);

    void setAttributeBuffer
        (int location, GLenum type, int offset, int tupleSize, int stride = 0);
    void setAttributeBuffer
        (const char *name, GLenum type, int offset, int tupleSize, int stride = 0);

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
    void setAttributeArray
        (int location, QMacCompatGLenum type, const void *values, int tupleSize, int stride = 0);
    void setAttributeArray
        (const char *name, QMacCompatGLenum type, const void *values, int tupleSize, int stride = 0);
    void setAttributeBuffer
        (int location, QMacCompatGLenum type, int offset, int tupleSize, int stride = 0);
    void setAttributeBuffer
        (const char *name, QMacCompatGLenum type, int offset, int tupleSize, int stride = 0);
#endif

    void enableAttributeArray(int location);
    void enableAttributeArray(const char *name);
    void disableAttributeArray(int location);
    void disableAttributeArray(const char *name);

    int uniformLocation(const char *name) const;
    int uniformLocation(const QByteArray& name) const;
    int uniformLocation(const QString& name) const;

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
    void setUniformValue(int location, QMacCompatGLint value);
    void setUniformValue(int location, QMacCompatGLuint value);
    void setUniformValue(const char *name, QMacCompatGLint value);
    void setUniformValue(const char *name, QMacCompatGLuint value);
    void setUniformValueArray(int location, const QMacCompatGLint *values, int count);
    void setUniformValueArray(int location, const QMacCompatGLuint *values, int count);
    void setUniformValueArray(const char *name, const QMacCompatGLint *values, int count);
    void setUniformValueArray(const char *name, const QMacCompatGLuint *values, int count);
#endif

    void setUniformValue(int location, GLfloat value);
    void setUniformValue(int location, GLint value);
    void setUniformValue(int location, GLuint value);
    void setUniformValue(int location, GLfloat x, GLfloat y);
    void setUniformValue(int location, GLfloat x, GLfloat y, GLfloat z);
    void setUniformValue(int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void setUniformValue(int location, const QVector2D& value);
    void setUniformValue(int location, const QVector3D& value);
    void setUniformValue(int location, const QVector4D& value);
    void setUniformValue(int location, const QColor& color);
    void setUniformValue(int location, const QPoint& point);
    void setUniformValue(int location, const QPointF& point);
    void setUniformValue(int location, const QSize& size);
    void setUniformValue(int location, const QSizeF& size);
    void setUniformValue(int location, const QMatrix2x2& value);
    void setUniformValue(int location, const QMatrix2x3& value);
    void setUniformValue(int location, const QMatrix2x4& value);
    void setUniformValue(int location, const QMatrix3x2& value);
    void setUniformValue(int location, const QMatrix3x3& value);
    void setUniformValue(int location, const QMatrix3x4& value);
    void setUniformValue(int location, const QMatrix4x2& value);
    void setUniformValue(int location, const QMatrix4x3& value);
    void setUniformValue(int location, const QMatrix4x4& value);
    void setUniformValue(int location, const GLfloat value[2][2]);
    void setUniformValue(int location, const GLfloat value[3][3]);
    void setUniformValue(int location, const GLfloat value[4][4]);
    void setUniformValue(int location, const QTransform& value);

    void setUniformValue(const char *name, GLfloat value);
    void setUniformValue(const char *name, GLint value);
    void setUniformValue(const char *name, GLuint value);
    void setUniformValue(const char *name, GLfloat x, GLfloat y);
    void setUniformValue(const char *name, GLfloat x, GLfloat y, GLfloat z);
    void setUniformValue(const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void setUniformValue(const char *name, const QVector2D& value);
    void setUniformValue(const char *name, const QVector3D& value);
    void setUniformValue(const char *name, const QVector4D& value);
    void setUniformValue(const char *name, const QColor& color);
    void setUniformValue(const char *name, const QPoint& point);
    void setUniformValue(const char *name, const QPointF& point);
    void setUniformValue(const char *name, const QSize& size);
    void setUniformValue(const char *name, const QSizeF& size);
    void setUniformValue(const char *name, const QMatrix2x2& value);
    void setUniformValue(const char *name, const QMatrix2x3& value);
    void setUniformValue(const char *name, const QMatrix2x4& value);
    void setUniformValue(const char *name, const QMatrix3x2& value);
    void setUniformValue(const char *name, const QMatrix3x3& value);
    void setUniformValue(const char *name, const QMatrix3x4& value);
    void setUniformValue(const char *name, const QMatrix4x2& value);
    void setUniformValue(const char *name, const QMatrix4x3& value);
    void setUniformValue(const char *name, const QMatrix4x4& value);
    void setUniformValue(const char *name, const GLfloat value[2][2]);
    void setUniformValue(const char *name, const GLfloat value[3][3]);
    void setUniformValue(const char *name, const GLfloat value[4][4]);
    void setUniformValue(const char *name, const QTransform& value);

    void setUniformValueArray(int location, const GLfloat *values, int count, int tupleSize);
    void setUniformValueArray(int location, const GLint *values, int count);
    void setUniformValueArray(int location, const GLuint *values, int count);
    void setUniformValueArray(int location, const QVector2D *values, int count);
    void setUniformValueArray(int location, const QVector3D *values, int count);
    void setUniformValueArray(int location, const QVector4D *values, int count);
    void setUniformValueArray(int location, const QMatrix2x2 *values, int count);
    void setUniformValueArray(int location, const QMatrix2x3 *values, int count);
    void setUniformValueArray(int location, const QMatrix2x4 *values, int count);
    void setUniformValueArray(int location, const QMatrix3x2 *values, int count);
    void setUniformValueArray(int location, const QMatrix3x3 *values, int count);
    void setUniformValueArray(int location, const QMatrix3x4 *values, int count);
    void setUniformValueArray(int location, const QMatrix4x2 *values, int count);
    void setUniformValueArray(int location, const QMatrix4x3 *values, int count);
    void setUniformValueArray(int location, const QMatrix4x4 *values, int count);

    void setUniformValueArray(const char *name, const GLfloat *values, int count, int tupleSize);
    void setUniformValueArray(const char *name, const GLint *values, int count);
    void setUniformValueArray(const char *name, const GLuint *values, int count);
    void setUniformValueArray(const char *name, const QVector2D *values, int count);
    void setUniformValueArray(const char *name, const QVector3D *values, int count);
    void setUniformValueArray(const char *name, const QVector4D *values, int count);
    void setUniformValueArray(const char *name, const QMatrix2x2 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix2x3 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix2x4 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix3x2 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix3x3 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix3x4 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix4x2 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix4x3 *values, int count);
    void setUniformValueArray(const char *name, const QMatrix4x4 *values, int count);

    static bool hasOpenGLShaderPrograms(const QGLContext *context = 0);

private Q_SLOTS:
    void shaderDestroyed();

private:
    Q_DISABLE_COPY(QGLShaderProgram)
    Q_DECLARE_PRIVATE(QGLShaderProgram)

    bool init();
};

#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif
