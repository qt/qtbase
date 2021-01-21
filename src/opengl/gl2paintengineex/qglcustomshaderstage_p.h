/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QGL_CUSTOM_SHADER_STAGE_H
#define QGL_CUSTOM_SHADER_STAGE_H

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

#include <QGLShaderProgram>

QT_BEGIN_NAMESPACE


class QGLCustomShaderStagePrivate;
class Q_OPENGL_EXPORT QGLCustomShaderStage
{
    Q_DECLARE_PRIVATE(QGLCustomShaderStage)
public:
    QGLCustomShaderStage();
    virtual ~QGLCustomShaderStage();
    virtual void setUniforms(QGLShaderProgram*) {}

    void setUniformsDirty();

    bool setOnPainter(QPainter*);
    void removeFromPainter(QPainter*);
    QByteArray source() const;

    void setInactive();
protected:
    void setSource(const QByteArray&);

private:
    QGLCustomShaderStagePrivate* d_ptr;
};


QT_END_NAMESPACE


#endif
