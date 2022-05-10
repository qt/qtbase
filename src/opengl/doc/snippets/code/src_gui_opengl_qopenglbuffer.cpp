// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtOpenGL/QOpenGLBuffer>

namespace src_gui_opengl_qopenglbuffer {
void wrapper() {

//! [0]
QOpenGLBuffer buffer1(QOpenGLBuffer::IndexBuffer);
buffer1.create();

QOpenGLBuffer buffer2 = buffer1;
//! [0]


//! [1]
QOpenGLBuffer::release(QOpenGLBuffer::VertexBuffer);
//! [1]

} // wrapper
} // src_gui_opengl_qopenglbuffer
