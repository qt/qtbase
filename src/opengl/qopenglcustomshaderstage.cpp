// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenglcustomshaderstage_p.h"
#include "qopenglengineshadermanager_p.h"
#include "qopenglpaintengine_p.h"
#include <private/qpainter_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLCustomShaderStagePrivate
{
public:
    QOpenGLCustomShaderStagePrivate() :
        m_manager(nullptr) {}

    QPointer<QOpenGLEngineShaderManager> m_manager;
    QByteArray              m_source;
};




QOpenGLCustomShaderStage::QOpenGLCustomShaderStage()
    : d_ptr(new QOpenGLCustomShaderStagePrivate)
{
}

QOpenGLCustomShaderStage::~QOpenGLCustomShaderStage()
{
    Q_D(QOpenGLCustomShaderStage);
    if (d->m_manager) {
        d->m_manager->removeCustomStage();
        d->m_manager->sharedShaders->cleanupCustomStage(this);
    }
    delete d_ptr;
}

void QOpenGLCustomShaderStage::setUniformsDirty()
{
    Q_D(QOpenGLCustomShaderStage);
    if (d->m_manager)
        d->m_manager->setDirty(); // ### Probably a bit overkill!
}

bool QOpenGLCustomShaderStage::setOnPainter(QPainter* p)
{
    Q_D(QOpenGLCustomShaderStage);
    if (p->paintEngine()->type() != QPaintEngine::OpenGL2) {
        qWarning("QOpenGLCustomShaderStage::setOnPainter() - paint engine not OpenGL2");
        return false;
    }
    if (d->m_manager)
        qWarning("Custom shader is already set on a painter");

    QOpenGL2PaintEngineEx *engine = static_cast<QOpenGL2PaintEngineEx*>(p->paintEngine());
    d->m_manager = QOpenGL2PaintEngineExPrivate::shaderManagerForEngine(engine);
    Q_ASSERT(d->m_manager);

    d->m_manager->setCustomStage(this);
    return true;
}

void QOpenGLCustomShaderStage::removeFromPainter(QPainter* p)
{
    Q_D(QOpenGLCustomShaderStage);
    if (p->paintEngine()->type() != QPaintEngine::OpenGL2)
        return;

    QOpenGL2PaintEngineEx *engine = static_cast<QOpenGL2PaintEngineEx*>(p->paintEngine());
    d->m_manager = QOpenGL2PaintEngineExPrivate::shaderManagerForEngine(engine);
    Q_ASSERT(d->m_manager);

    // Just set the stage to null, don't call removeCustomStage().
    // This should leave the program in a compiled/linked state
    // if the next custom shader stage is this one again.
    d->m_manager->setCustomStage(nullptr);
    d->m_manager = nullptr;
}

QByteArray QOpenGLCustomShaderStage::source() const
{
    Q_D(const QOpenGLCustomShaderStage);
    return d->m_source;
}

// Called by the shader manager if another custom shader is attached or
// the manager is deleted
void QOpenGLCustomShaderStage::setInactive()
{
    Q_D(QOpenGLCustomShaderStage);
    d->m_manager = nullptr;
}

void QOpenGLCustomShaderStage::setSource(const QByteArray& s)
{
    Q_D(QOpenGLCustomShaderStage);
    d->m_source = s;
}

QT_END_NAMESPACE
