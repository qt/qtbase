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

#include "qglcustomshaderstage_p.h"
#include "qglengineshadermanager_p.h"
#include "qpaintengineex_opengl2_p.h"
#include <private/qpainter_p.h>

QT_BEGIN_NAMESPACE

class QGLCustomShaderStagePrivate
{
public:
    QGLCustomShaderStagePrivate() :
        m_manager(0) {}

    QPointer<QGLEngineShaderManager> m_manager;
    QByteArray              m_source;
};




QGLCustomShaderStage::QGLCustomShaderStage()
    : d_ptr(new QGLCustomShaderStagePrivate)
{
}

QGLCustomShaderStage::~QGLCustomShaderStage()
{
    Q_D(QGLCustomShaderStage);
    if (d->m_manager) {
        d->m_manager->removeCustomStage();
        d->m_manager->sharedShaders->cleanupCustomStage(this);
    }
    delete d_ptr;
}

void QGLCustomShaderStage::setUniformsDirty()
{
    Q_D(QGLCustomShaderStage);
    if (d->m_manager)
        d->m_manager->setDirty(); // ### Probably a bit overkill!
}

bool QGLCustomShaderStage::setOnPainter(QPainter* p)
{
    Q_D(QGLCustomShaderStage);
    if (p->paintEngine()->type() != QPaintEngine::OpenGL2) {
        qWarning("QGLCustomShaderStage::setOnPainter() - paint engine not OpenGL2");
        return false;
    }
    if (d->m_manager)
        qWarning("Custom shader is already set on a painter");

    QGL2PaintEngineEx *engine = static_cast<QGL2PaintEngineEx*>(p->paintEngine());
    d->m_manager = QGL2PaintEngineExPrivate::shaderManagerForEngine(engine);
    Q_ASSERT(d->m_manager);

    d->m_manager->setCustomStage(this);
    return true;
}

void QGLCustomShaderStage::removeFromPainter(QPainter* p)
{
    Q_D(QGLCustomShaderStage);
    if (p->paintEngine()->type() != QPaintEngine::OpenGL2)
        return;

    QGL2PaintEngineEx *engine = static_cast<QGL2PaintEngineEx*>(p->paintEngine());
    d->m_manager = QGL2PaintEngineExPrivate::shaderManagerForEngine(engine);
    Q_ASSERT(d->m_manager);

    // Just set the stage to null, don't call removeCustomStage().
    // This should leave the program in a compiled/linked state
    // if the next custom shader stage is this one again.
    d->m_manager->setCustomStage(0);
    d->m_manager = 0;
}

QByteArray QGLCustomShaderStage::source() const
{
    Q_D(const QGLCustomShaderStage);
    return d->m_source;
}

// Called by the shader manager if another custom shader is attached or
// the manager is deleted
void QGLCustomShaderStage::setInactive()
{
    Q_D(QGLCustomShaderStage);
    d->m_manager = 0;
}

void QGLCustomShaderStage::setSource(const QByteArray& s)
{
    Q_D(QGLCustomShaderStage);
    d->m_source = s;
}

QT_END_NAMESPACE
