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

#ifndef QGRAPHICSSHADEREFFECT_P_H
#define QGRAPHICSSHADEREFFECT_P_H

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

#include <QtWidgets/qgraphicseffect.h>

#include <QtOpenGL/qtopenglglobal.h>

QT_REQUIRE_CONFIG(graphicseffect);

QT_BEGIN_NAMESPACE

class QGLShaderProgram;
class QGLCustomShaderEffectStage;
class QGraphicsShaderEffectPrivate;

class Q_OPENGL_EXPORT QGraphicsShaderEffect : public QGraphicsEffect
{
    Q_OBJECT
public:
    QGraphicsShaderEffect(QObject *parent = nullptr);
    virtual ~QGraphicsShaderEffect();

    QByteArray pixelShaderFragment() const;
    void setPixelShaderFragment(const QByteArray& code);

protected:
    void draw(QPainter *painter) override;
    void setUniformsDirty();
    virtual void setUniforms(QGLShaderProgram *program);

private:
    Q_DECLARE_PRIVATE(QGraphicsShaderEffect)
    Q_DISABLE_COPY_MOVE(QGraphicsShaderEffect)

    friend class QGLCustomShaderEffectStage;
};

QT_END_NAMESPACE

#endif // QGRAPHICSSHADEREFFECT_P_H
