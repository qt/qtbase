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

#ifndef QOPENGLVERTEXARRAYOBJECT_H
#define QOPENGLVERTEXARRAYOBJECT_H

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_OPENGL

#include <QtCore/QObject>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class QOpenGLVertexArrayObjectPrivate;

class Q_GUI_EXPORT QOpenGLVertexArrayObject : public QObject
{
    Q_OBJECT

public:
    explicit QOpenGLVertexArrayObject(QObject* parent = nullptr);
    ~QOpenGLVertexArrayObject();

    bool create();
    void destroy();
    bool isCreated() const;
    GLuint objectId() const;
    void bind();
    void release();

    class Q_GUI_EXPORT Binder
    {
    public:
        inline Binder(QOpenGLVertexArrayObject *v)
            : vao(v)
        {
            Q_ASSERT(v);
            if (vao->isCreated() || vao->create())
                vao->bind();
        }

        inline ~Binder()
        {
            release();
        }

        inline void release()
        {
            vao->release();
        }

        inline void rebind()
        {
            vao->bind();
        }

    private:
        Q_DISABLE_COPY(Binder)
        QOpenGLVertexArrayObject *vao;
    };

private:
    Q_DISABLE_COPY(QOpenGLVertexArrayObject)
    Q_DECLARE_PRIVATE(QOpenGLVertexArrayObject)
    Q_PRIVATE_SLOT(d_func(), void _q_contextAboutToBeDestroyed())
    QOpenGLVertexArrayObject(QOpenGLVertexArrayObjectPrivate &dd);
};

QT_END_NAMESPACE

#endif

#endif // QOPENGLVERTEXARRAYOBJECT_H
