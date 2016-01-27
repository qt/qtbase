/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtOpenGL/qglfunctions.h>

class tst_QGLFunctions : public QObject
{
    Q_OBJECT
public:
    tst_QGLFunctions() {}
    ~tst_QGLFunctions() {}

private slots:
    void features();
    void multitexture();
    void blendColor();

private:
    static bool hasExtension(const char *name);
};

bool tst_QGLFunctions::hasExtension(const char *name)
{
    QString extensions =
        QString::fromLatin1
            (reinterpret_cast<const char *>(QOpenGLContext::currentContext()->functions()->glGetString(GL_EXTENSIONS)));
    return extensions.split(QLatin1Char(' ')).contains
        (QString::fromLatin1(name));
}

// Check that the reported features are consistent with the platform.
void tst_QGLFunctions::features()
{
    // Before being associated with a context, there should be
    // no features enabled.
    QGLFunctions funcs;
    QVERIFY(!funcs.openGLFeatures());
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::Multitexture));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::Shaders));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::Buffers));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::Framebuffers));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::BlendColor));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::BlendEquation));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::BlendEquationSeparate));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::BlendFuncSeparate));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::BlendSubtract));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::CompressedTextures));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::Multisample));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::StencilSeparate));
    QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::NPOTTextures));

    // Make a context current.
    QGLWidget glw;
    if (!glw.isValid())
        QSKIP("Could not create a GL context");
    glw.makeCurrent();
    funcs.initializeGLFunctions();

    // Validate the features against what we expect for this platform.
    if (QOpenGLContext::currentContext()->isOpenGLES()) {
#if !defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2)
        QGLFunctions::OpenGLFeatures allFeatures =
            (QGLFunctions::Multitexture |
             QGLFunctions::Shaders |
             QGLFunctions::Buffers |
             QGLFunctions::Framebuffers |
             QGLFunctions::BlendColor |
             QGLFunctions::BlendEquation |
             QGLFunctions::BlendEquationSeparate |
             QGLFunctions::BlendFuncSeparate |
             QGLFunctions::BlendSubtract |
             QGLFunctions::CompressedTextures |
             QGLFunctions::Multisample |
             QGLFunctions::StencilSeparate |
             QGLFunctions::NPOTTextures);
        QVERIFY((funcs.openGLFeatures() & allFeatures) == allFeatures);
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Multitexture));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Shaders));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Buffers));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Framebuffers));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::BlendColor));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::BlendEquation));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::BlendEquationSeparate));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::BlendFuncSeparate));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::BlendSubtract));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::CompressedTextures));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Multisample));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::StencilSeparate));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::NPOTTextures));
#elif defined(QT_OPENGL_ES) && !defined(QT_OPENGL_ES_2)
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Multitexture));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Buffers));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::CompressedTextures));
        QVERIFY(funcs.hasOpenGLFeature(QGLFunctions::Multisample));

        QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::Shaders));
        QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::BlendColor));
        QVERIFY(!funcs.hasOpenGLFeature(QGLFunctions::StencilSeparate));

        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::Framebuffers),
                 hasExtension("GL_OES_framebuffer_object"));
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendEquationSeparate),
                 hasExtension("GL_OES_blend_equation_separate"));
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendFuncSeparate),
                 hasExtension("GL_OES_blend_func_separate"));
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendSubtract),
                 hasExtension("GL_OES_blend_subtract"));
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::NPOTTextures),
                 hasExtension("GL_OES_texture_npot"));
#endif
    } else {
        // We check for both the extension name and the minimum OpenGL version
        // for the feature.  This will help us catch situations where a platform
        // doesn't list an extension by name but does have the feature by virtue
        // of its version number.
        QGLFormat::OpenGLVersionFlags versions = QGLFormat::openGLVersionFlags();
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::Multitexture),
                 hasExtension("GL_ARB_multitexture") ||
                 (versions & QGLFormat::OpenGL_Version_1_3) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::Shaders),
                 hasExtension("GL_ARB_shader_objects") ||
                 (versions & QGLFormat::OpenGL_Version_2_0) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::Buffers),
                 (versions & QGLFormat::OpenGL_Version_1_5) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::Framebuffers),
                 hasExtension("GL_EXT_framebuffer_object") ||
                 hasExtension("GL_ARB_framebuffer_object"));
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendColor),
                 hasExtension("GL_EXT_blend_color") ||
                 (versions & QGLFormat::OpenGL_Version_1_2) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendEquation),
                 (versions & QGLFormat::OpenGL_Version_1_2) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendEquationSeparate),
                 hasExtension("GL_EXT_blend_equation_separate") ||
                 (versions & QGLFormat::OpenGL_Version_2_0) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendFuncSeparate),
                 hasExtension("GL_EXT_blend_func_separate") ||
                 (versions & QGLFormat::OpenGL_Version_1_4) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::BlendSubtract),
                 hasExtension("GL_EXT_blend_subtract"));
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::CompressedTextures),
                 hasExtension("GL_ARB_texture_compression") ||
                 (versions & QGLFormat::OpenGL_Version_1_3) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::Multisample),
                 hasExtension("GL_ARB_multisample") ||
                 (versions & QGLFormat::OpenGL_Version_1_3) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::StencilSeparate),
                 (versions & QGLFormat::OpenGL_Version_2_0) != 0);
        QCOMPARE(funcs.hasOpenGLFeature(QGLFunctions::NPOTTextures),
                 hasExtension("GL_ARB_texture_non_power_of_two") ||
                 (versions & QGLFormat::OpenGL_Version_2_0) != 0);
    }
}

// Verify that the multitexture functions appear to resolve and work.
void tst_QGLFunctions::multitexture()
{
    QOpenGLFunctions funcs;
    QGLWidget glw;
    if (!glw.isValid())
        QSKIP("Could not create a GL context");
    glw.makeCurrent();
    funcs.initializeOpenGLFunctions();

    if (!funcs.hasOpenGLFeature(QOpenGLFunctions::Multitexture))
        QSKIP("Multitexture functions are not supported");

    funcs.glActiveTexture(GL_TEXTURE1);

    GLint active = 0;
    funcs.glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    QCOMPARE(active, GL_TEXTURE1);

    funcs.glActiveTexture(GL_TEXTURE0);

    active = 0;
    funcs.glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    QCOMPARE(active, GL_TEXTURE0);
}

// Verify that the glBlendColor() function appears to resolve and work.
void tst_QGLFunctions::blendColor()
{
    QOpenGLFunctions funcs;
    QGLWidget glw;
    if (!glw.isValid())
        QSKIP("Could not create a GL context");
    glw.makeCurrent();
    funcs.initializeOpenGLFunctions();

    if (!funcs.hasOpenGLFeature(QOpenGLFunctions::BlendColor))
        QSKIP("glBlendColor() is not supported");

    funcs.glBlendColor(0.0f, 1.0f, 0.0f, 1.0f);

    GLfloat colors[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    funcs.glGetFloatv(GL_BLEND_COLOR, colors);

    QCOMPARE(colors[0], 0.0f);
    QCOMPARE(colors[1], 1.0f);
    QCOMPARE(colors[2], 0.0f);
    QCOMPARE(colors[3], 1.0f);
}

QTEST_MAIN(tst_QGLFunctions)

#include "tst_qglfunctions.moc"
