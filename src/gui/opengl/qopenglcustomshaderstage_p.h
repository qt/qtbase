/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QOPENGL_CUSTOM_SHADER_STAGE_H
#define QOPENGL_CUSTOM_SHADER_STAGE_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QOpenGLShaderProgram>

QT_BEGIN_NAMESPACE


class QPainter;
class QOpenGLCustomShaderStagePrivate;
class Q_GUI_EXPORT QOpenGLCustomShaderStage
{
    Q_DECLARE_PRIVATE(QOpenGLCustomShaderStage)
public:
    QOpenGLCustomShaderStage();
    virtual ~QOpenGLCustomShaderStage();
    virtual void setUniforms(QOpenGLShaderProgram*) {}

    void setUniformsDirty();

    bool setOnPainter(QPainter*);
    void removeFromPainter(QPainter*);
    QByteArray source() const;

    void setInactive();
protected:
    void setSource(const QByteArray&);

private:
    QOpenGLCustomShaderStagePrivate* d_ptr;

    Q_DISABLE_COPY_MOVE(QOpenGLCustomShaderStage)
};


QT_END_NAMESPACE


#endif
