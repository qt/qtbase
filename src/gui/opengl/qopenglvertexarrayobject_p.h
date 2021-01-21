/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sean Harmer <sean.harmer@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOPENGLVERTEXARRAYOBJECT_P_H
#define QOPENGLVERTEXARRAYOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class QOpenGLVertexArrayObjectHelper;
class QOpenGLContext;

void Q_GUI_EXPORT qtInitializeVertexArrayObjectHelper(QOpenGLVertexArrayObjectHelper *helper, QOpenGLContext *context);

class QOpenGLVertexArrayObjectHelper
{
    Q_DISABLE_COPY(QOpenGLVertexArrayObjectHelper)

public:
    explicit inline QOpenGLVertexArrayObjectHelper(QOpenGLContext *context)
        : GenVertexArrays(nullptr)
        , DeleteVertexArrays(nullptr)
        , BindVertexArray(nullptr)
        , IsVertexArray(nullptr)
    {
        qtInitializeVertexArrayObjectHelper(this, context);
    }

    inline bool isValid() const
    {
        return GenVertexArrays && DeleteVertexArrays && BindVertexArray && IsVertexArray;
    }

    inline void glGenVertexArrays(GLsizei n, GLuint *arrays) const
    {
        GenVertexArrays(n, arrays);
    }

    inline void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) const
    {
        DeleteVertexArrays(n, arrays);
    }

    inline void glBindVertexArray(GLuint array) const
    {
        BindVertexArray(array);
    }

    inline GLboolean glIsVertexArray(GLuint array) const
    {
        return IsVertexArray(array);
    }

private:
    friend void Q_GUI_EXPORT qtInitializeVertexArrayObjectHelper(QOpenGLVertexArrayObjectHelper *helper, QOpenGLContext *context);

    // Function signatures are equivalent between desktop core, ARB, APPLE, ES 3 and ES 2 extensions
    typedef void (QOPENGLF_APIENTRYP qt_GenVertexArrays_t)(GLsizei n, GLuint *arrays);
    typedef void (QOPENGLF_APIENTRYP qt_DeleteVertexArrays_t)(GLsizei n, const GLuint *arrays);
    typedef void (QOPENGLF_APIENTRYP qt_BindVertexArray_t)(GLuint array);
    typedef GLboolean (QOPENGLF_APIENTRYP qt_IsVertexArray_t)(GLuint array);

    qt_GenVertexArrays_t GenVertexArrays;
    qt_DeleteVertexArrays_t DeleteVertexArrays;
    qt_BindVertexArray_t BindVertexArray;
    qt_IsVertexArray_t IsVertexArray;
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLVERTEXARRAYOBJECT_P_H
