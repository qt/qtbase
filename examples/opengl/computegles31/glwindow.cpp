/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "glwindow.h"
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLVertexArrayObject>
#include <QtGui/qopengl.h>
#include <QDebug>
#include <QTimer>
#include <math.h>

#ifndef GL_READ_WRITE
#define GL_READ_WRITE 0x88BA
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif

GLWindow::GLWindow()
    : m_texImageInput(0),
      m_texImageTmp(0),
      m_texImageProcessed(0),
      m_shaderDisplay(0),
      m_shaderComputeV(0),
      m_shaderComputeH(0),
      m_blurRadius(0.0f),
      m_animate(true),
      m_vao(0)
{
    const float animationStart = 0.0;
    const float animationEnd = 10.0;
    const float animationLength = 1000;

    m_animationGroup = new QSequentialAnimationGroup(this);
    m_animationGroup->setLoopCount(-1);

    m_animationForward = new QPropertyAnimation(this, QByteArrayLiteral("blurRadius"));
    m_animationForward->setStartValue(animationStart);
    m_animationForward->setEndValue(animationEnd);
    m_animationForward->setDuration(animationLength);
    m_animationGroup->addAnimation(m_animationForward);

    m_animationBackward = new QPropertyAnimation(this, QByteArrayLiteral("blurRadius"));
    m_animationBackward->setStartValue(animationEnd);
    m_animationBackward->setEndValue(animationStart);
    m_animationBackward->setDuration(animationLength);
    m_animationGroup->addAnimation(m_animationBackward);

    m_animationGroup->start();
}

GLWindow::~GLWindow()
{
    makeCurrent();
    delete m_texImageInput;
    delete m_texImageProcessed;
    delete m_texImageTmp;
    delete m_shaderDisplay;
    delete m_shaderComputeH;
    delete m_shaderComputeV;
    delete m_animationGroup;
    delete m_animationForward;
    delete m_animationBackward;
    delete m_vao;
}

void GLWindow::setBlurRadius(float blurRadius)
{
    int radius = int(blurRadius);
    if (radius != m_blurRadius) {
        m_blurRadius = radius;
        update();
    }
}

void GLWindow::setAnimating(bool animate)
{
    m_animate = animate;
    if (animate)
        m_animationGroup->start();
    else
        m_animationGroup->stop();
}

void GLWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Space) { // pause
        setAnimating(!m_animate);
    }
    update();
}




static const char *vsDisplaySource =
    "const vec4 vertices[4] = vec4[4] (\n"
    "        vec4( -1.0,  1.0, 0.0, 1.0),\n"
    "        vec4( -1.0, -1.0, 0.0, 1.0),\n"
    "        vec4(  1.0,  1.0, 0.0, 1.0),\n"
    "        vec4(  1.0, -1.0, 0.0, 1.0)\n"
    ");\n"
    "const vec2 texCoords[4] = vec2[4] (\n"
    "        vec2( 0.0,  1.0),\n"
    "        vec2( 0.0,  0.0),\n"
    "        vec2( 1.0,  1.0),\n"
    "        vec2( 1.0,  0.0)\n"
    ");\n"
    "out vec2 texCoord;\n"
    "uniform mat4 matProjection;\n"
    "uniform vec2 imageRatio;\n"
    "void main() {\n"
    "   gl_Position = matProjection * ( vertices[gl_VertexID] * vec4(imageRatio,0,1)  );\n"
    "   texCoord = texCoords[gl_VertexID];\n"
    "}\n";

static const char *fsDisplaySource =
    "in lowp vec2 texCoord; \n"
    "uniform sampler2D samImage; \n"
    "layout(location = 0) out lowp vec4 color;\n"
    "void main() {\n"
    "   lowp vec4 texColor = texture(samImage,texCoord);\n"
    "   color = vec4(texColor.rgb, 1.0);\n"
    "}\n";

static const char *csComputeSourceV =
        "#define COMPUTEPATCHSIZE 32 \n"
        "#define IMGFMT rgba8 \n"
        "layout (local_size_x = COMPUTEPATCHSIZE, local_size_y = COMPUTEPATCHSIZE) in;\n"
        "layout(binding=0, IMGFMT) uniform readonly highp image2D inputImage; // Use a sampler to improve performance  \n"
        "layout(binding=1, IMGFMT) uniform writeonly highp image2D resultImage;\n"
        "uniform int radius;\n"
        "const float cutoff = 2.2;\n"

        "float expFactor() { // a function, otherwise MESA produces error: initializer of global variable `expFactor' must be a constant expression\n"
        "   float sigma = clamp(float(radius) / cutoff,0.02,100.0);\n"
        "   return 1.0 / (2.0 * sigma * sigma);\n"
        "}\n"

        "float gaussian(float distance, float expfactor) {\n"
        "   return exp( -(distance * distance) * expfactor);\n"
        "}\n"

        "void main() {\n"
        "  ivec2 imgSize = imageSize(resultImage);\n"
        "  int x = int(gl_GlobalInvocationID.x);\n"
        "  int y = int(gl_GlobalInvocationID.y);\n"
        "  if ( (x >= imgSize.x) || (y >= imgSize.y) ) return;\n"
        "  vec4 sumPixels = vec4(0.0);\n"
        "  float sumWeights = 0.0;\n"
        "  int left   = clamp(x - radius, 0, imgSize.x - 1);\n"
        "  int right  = clamp(x + radius, 0, imgSize.x - 1);\n"
        "  int top    = clamp(y - radius, 0, imgSize.y - 1);\n"
        "  int bottom = clamp(y + radius, 0, imgSize.y - 1);\n"
        "  float expfactor = expFactor();\n"
        "  for (int iY = top; iY <= bottom; iY++) {\n"
        "      float dy = float(abs(iY - y));\n"
        "      vec4 imgValue =  imageLoad(inputImage, ivec2(x,iY));\n"
        "      float weight = gaussian(dy, expfactor);\n"
        "      sumWeights += weight;\n"
        "      sumPixels += (imgValue * weight);\n"
        "  }\n"
        "  sumPixels /= sumWeights;\n"
        "  imageStore(resultImage, ivec2(x,y), sumPixels);\n"
        "}\n";

static const char *csComputeSourceH =
        "#define COMPUTEPATCHSIZE 32 \n"
        "#define IMGFMT rgba8 \n"
        "layout (local_size_x = COMPUTEPATCHSIZE, local_size_y = COMPUTEPATCHSIZE) in;\n"
        "layout(binding=0, IMGFMT) uniform readonly highp image2D inputImage; // Use a sampler to improve performance  \n"
        "layout(binding=1, IMGFMT) uniform writeonly highp image2D resultImage;\n"
        "uniform int radius;\n"
        "const float cutoff = 2.2;\n"

        "float expFactor() { // a function, otherwise MESA produces error: initializer of global variable `expFactor' must be a constant expression\n"
        "   float sigma = clamp(float(radius) / cutoff,0.02,100.0);\n"
        "   return 1.0 / (2.0 * sigma * sigma);\n"
        "}\n"

        "float gaussian(float distance, float expfactor) {\n"
        "   return exp( -(distance * distance) * expfactor);\n"
        "}\n"

        "void main() {\n"
        "  ivec2 imgSize = imageSize(resultImage);\n"
        "  int x = int(gl_GlobalInvocationID.x);\n"
        "  int y = int(gl_GlobalInvocationID.y);\n"
        "  if ( (x >= imgSize.x) || (y >= imgSize.y) ) return;\n"
        "  vec4 sumPixels = vec4(0.0);\n"
        "  float sumWeights = 0.0;\n"
        "  int left   = clamp(x - radius, 0, imgSize.x - 1);\n"
        "  int right  = clamp(x + radius, 0, imgSize.x - 1);\n"
        "  int top    = clamp(y - radius, 0, imgSize.y - 1);\n"
        "  int bottom = clamp(y + radius, 0, imgSize.y - 1);\n"
        "  float expfactor = expFactor();\n"
        "  for (int iX = left; iX <= right; iX++) {\n"
        "      float dx = float(abs(iX - x));\n"
        "      vec4 imgValue =  imageLoad(inputImage, ivec2(iX,y));\n"
        "      float weight = gaussian(dx, expfactor);\n"
        "      sumWeights += weight;\n"
        "      sumPixels += (imgValue * weight);\n"
        "  }\n"
        "  sumPixels /= sumWeights;\n"
        "  imageStore(resultImage, ivec2(x,y), sumPixels);\n"
        "}\n";



QByteArray versionedShaderCode(const char *src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
        versionedSrc.append(QByteArrayLiteral("#version 310 es\n"));
    else
        versionedSrc.append(QByteArrayLiteral("#version 430 core\n"));

    versionedSrc.append(src);
    return versionedSrc;
}

void computeProjection(int winWidth, int winHeight, int imgWidth, int imgHeight, QMatrix4x4 &outProjection, QSizeF &outQuadSize)
{
    float ratioImg    = float(imgWidth) / float(imgHeight);
    float ratioCanvas = float(winWidth) / float(winHeight);

    float correction    = ratioImg / ratioCanvas;
    float rescaleFactor = 1.0f;
    float quadWidth     = 1.0f;
    float quadHeight    = 1.0f;

    if (correction < 1.0f)  // canvas larger than image -- height = 1.0, vertical black bands
    {
        quadHeight     = 1.0f;
        quadWidth    = 1.0f * ratioImg;
        rescaleFactor = ratioCanvas;
        correction    = 1.0f / rescaleFactor;
    }
    else                    // image larger than canvas -- width = 1.0, horizontal black bands
    {
        quadWidth  = 1.0f;
        quadHeight = 1.0f / ratioImg;
        correction = 1.0f / ratioCanvas;
    }

    const float frustumWidth  = 1.0f * rescaleFactor;
    const float frustumHeight = 1.0f * rescaleFactor * correction;

    outProjection = QMatrix4x4();
    outProjection.ortho(
        -frustumWidth,
         frustumWidth,
        -frustumHeight,
         frustumHeight,
        -1.0f,
         1.0f);
    outQuadSize = QSizeF(quadWidth,quadHeight);
}

void GLWindow::initializeGL()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    qDebug() << "Got a "
             << ctx->format().majorVersion()
             << "."
             << ctx->format().minorVersion()
             << ((ctx->format().renderableType() == QSurfaceFormat::OpenGLES) ? (" GLES") : (" GL"))
             << " context";

    if (m_texImageInput) {
        delete m_texImageInput;
        m_texImageInput = 0;
    }
    QImage img(":/Qt-logo-medium.png");
    Q_ASSERT(!img.isNull());
    m_texImageInput = new QOpenGLTexture(img.convertToFormat(QImage::Format_RGBA8888).mirrored());

    if (m_texImageTmp) {
        delete m_texImageTmp;
        m_texImageTmp = 0;
    }
    m_texImageTmp = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texImageTmp->setFormat(m_texImageInput->format());
    m_texImageTmp->setSize(m_texImageInput->width(),m_texImageInput->height());
    m_texImageTmp->allocateStorage(QOpenGLTexture::RGBA,QOpenGLTexture::UInt8); // WTF?

    if (m_texImageProcessed) {
        delete m_texImageProcessed;
        m_texImageProcessed = 0;
    }
    m_texImageProcessed = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texImageProcessed->setFormat(m_texImageInput->format());
    m_texImageProcessed->setSize(m_texImageInput->width(),m_texImageInput->height());
    m_texImageProcessed->allocateStorage(QOpenGLTexture::RGBA,QOpenGLTexture::UInt8);

    m_texImageProcessed->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texImageProcessed->setMinificationFilter(QOpenGLTexture::Linear);
    m_texImageProcessed->setWrapMode(QOpenGLTexture::ClampToEdge);

    if (m_shaderDisplay) {
        delete m_shaderDisplay;
        m_shaderDisplay = 0;
    }
    m_shaderDisplay = new QOpenGLShaderProgram;
    // Prepend the correct version directive to the sources. The rest is the
    // same, thanks to the common GLSL syntax.
    m_shaderDisplay->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vsDisplaySource));
    m_shaderDisplay->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fsDisplaySource));
    m_shaderDisplay->link();

    if (m_shaderComputeV) {
        delete m_shaderComputeV;
        m_shaderComputeV = 0;
    }
    m_shaderComputeV = new QOpenGLShaderProgram;
    m_shaderComputeV->addShaderFromSourceCode(QOpenGLShader::Compute, versionedShaderCode(csComputeSourceV));
    m_shaderComputeV->link();

    if (m_shaderComputeH) {
        delete m_shaderComputeH;
        m_shaderComputeH = 0;
    }
    m_shaderComputeH = new QOpenGLShaderProgram;
    m_shaderComputeH->addShaderFromSourceCode(QOpenGLShader::Compute, versionedShaderCode(csComputeSourceH));
    m_shaderComputeH->link();

    // Create a VAO. Not strictly required for ES 3, but it is for plain OpenGL core context.
    m_vao = new QOpenGLVertexArrayObject;
    m_vao->create();
}

void GLWindow::resizeGL(int w, int h)
{
    computeProjection(w,h,m_texImageInput->width(),m_texImageInput->height(),m_proj,m_quadSize);
}

QSize getWorkGroups(int workGroupSize, const QSize &imageSize)
{
    int x = imageSize.width();
    x = (x % workGroupSize) ? (x / workGroupSize) + 1 : (x / workGroupSize);
    int y = imageSize.height();
    y = (y % workGroupSize) ? (y / workGroupSize) + 1 : (y / workGroupSize);
    return QSize(x,y);
}

void GLWindow::paintGL()
{
    // Now use QOpenGLExtraFunctions instead of QOpenGLFunctions as we want to
    // do more than what GL(ES) 2.0 offers.
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();


    // Process input image
    QSize workGroups = getWorkGroups( 32, QSize(m_texImageInput->width(), m_texImageInput->height()));
    // Pass 1
    f->glBindImageTexture(0, m_texImageInput->textureId(), 0, 0, 0,  GL_READ_WRITE, GL_RGBA8);
    f->glBindImageTexture(1, m_texImageTmp->textureId(), 0, 0, 0,  GL_READ_WRITE, GL_RGBA8);
    m_shaderComputeV->bind();
    m_shaderComputeV->setUniformValue("radius",m_blurRadius);
    f->glDispatchCompute(workGroups.width(),workGroups.height(),1);
    f->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_shaderComputeV->release();
    // Pass 2
    f->glBindImageTexture(0, m_texImageTmp->textureId(), 0, 0, 0,  GL_READ_WRITE, GL_RGBA8);
    f->glBindImageTexture(1, m_texImageProcessed->textureId(), 0, 0, 0,  GL_READ_WRITE, GL_RGBA8);
    m_shaderComputeH->bind();
    m_shaderComputeH->setUniformValue("radius",m_blurRadius);
    f->glDispatchCompute(workGroups.width(),workGroups.height(),1);
    f->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_shaderComputeH->release();
    // Compute cleanup
    f->glBindImageTexture(0, 0, 0, 0, 0,  GL_READ_WRITE, GL_RGBA8);
    f->glBindImageTexture(1, 0, 0, 0, 0,  GL_READ_WRITE, GL_RGBA8);

    // Display processed image
    f->glClearColor(0, 0, 0, 1);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_texImageProcessed->bind(0);
    m_shaderDisplay->bind();
    m_shaderDisplay->setUniformValue("matProjection",m_proj);
    m_shaderDisplay->setUniformValue("imageRatio",m_quadSize);
    m_shaderDisplay->setUniformValue("samImage",0);
    m_vao->bind();
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_vao->release();
    m_shaderDisplay->release();
    m_texImageProcessed->release(0);
}

