/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QFile>
#include <QtGui/private/qshaderdescription_p_p.h>
#include <QtGui/private/qshader_p_p.h>

class tst_QShader : public QObject
{
    Q_OBJECT

private slots:
    void simpleCompileCheckResults();
    void genVariants();
    void shaderDescImplicitSharing();
    void bakedShaderImplicitSharing();
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void tst_QShader::simpleCompileCheckResults()
{
    QShader s = getShader(QLatin1String(":/data/color_simple.vert.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(s.availableShaders().count(), 1);

    const QShaderCode shader = s.shader(QShaderKey(QShader::SpirvShader,
                                                   QShaderVersion(100)));
    QVERIFY(!shader.shader().isEmpty());
    QCOMPARE(shader.entryPoint(), QByteArrayLiteral("main"));

    const QShaderDescription desc = s.description();
    QVERIFY(desc.isValid());
    QCOMPARE(desc.inputVariables().count(), 2);
    for (const QShaderDescription::InOutVariable &v : desc.inputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QLatin1String("position"));
            QCOMPARE(v.type, QShaderDescription::Vec4);
            break;
        case 1:
            QCOMPARE(v.name, QLatin1String("color"));
            QCOMPARE(v.type, QShaderDescription::Vec3);
            break;
        default:
            QVERIFY(false);
            break;
        }
    }
    QCOMPARE(desc.outputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.outputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QLatin1String("v_color"));
            QCOMPARE(v.type, QShaderDescription::Vec3);
            break;
        default:
            QVERIFY(false);
            break;
        }
    }
    QCOMPARE(desc.uniformBlocks().count(), 1);
    const QShaderDescription::UniformBlock blk = desc.uniformBlocks().first();
    QCOMPARE(blk.blockName, QLatin1String("buf"));
    QCOMPARE(blk.structName, QLatin1String("ubuf"));
    QCOMPARE(blk.size, 68);
    QCOMPARE(blk.binding, 0);
    QCOMPARE(blk.descriptorSet, 0);
    QCOMPARE(blk.members.count(), 2);
    for (int i = 0; i < blk.members.count(); ++i) {
        const QShaderDescription::BlockVariable v = blk.members[i];
        switch (i) {
        case 0:
            QCOMPARE(v.offset, 0);
            QCOMPARE(v.size, 64);
            QCOMPARE(v.name, QLatin1String("mvp"));
            QCOMPARE(v.type, QShaderDescription::Mat4);
            QCOMPARE(v.matrixStride, 16);
            break;
        case 1:
            QCOMPARE(v.offset, 64);
            QCOMPARE(v.size, 4);
            QCOMPARE(v.name, QLatin1String("opacity"));
            QCOMPARE(v.type, QShaderDescription::Float);
            break;
        default:
            QVERIFY(false);
            break;
        }
    }
}

void tst_QShader::genVariants()
{
    QShader s = getShader(QLatin1String(":/data/color.vert.qsb"));
    // spirv, glsl 100, glsl 330, glsl 120, hlsl 50, msl 12
    // + batchable variants
    QVERIFY(s.isValid());
    QCOMPARE(s.availableShaders().count(), 2 * 6);

    int batchableVariantCount = 0;
    int batchableGlslVariantCount = 0;
    for (const QShaderKey &key : s.availableShaders()) {
        if (key.sourceVariant() == QShader::BatchableVertexShader) {
            ++batchableVariantCount;
            if (key.source() == QShader::GlslShader) {
                ++batchableGlslVariantCount;
                const QByteArray src = s.shader(key).shader();
                QVERIFY(src.contains(QByteArrayLiteral("_qt_order * ")));
            }
        }
    }
    QCOMPARE(batchableVariantCount, 6);
    QCOMPARE(batchableGlslVariantCount, 3);
}

void tst_QShader::shaderDescImplicitSharing()
{
    QShader s = getShader(QLatin1String(":/data/color_simple.vert.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(s.availableShaders().count(), 1);
    QVERIFY(s.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));

    QShaderDescription d0 = s.description();
    QVERIFY(d0.isValid());
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);

    QShaderDescription d1 = d0;
    QVERIFY(QShaderDescriptionPrivate::get(&d0) == QShaderDescriptionPrivate::get(&d1));
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);
    QCOMPARE(d1.inputVariables().count(), 2);
    QCOMPARE(d1.outputVariables().count(), 1);
    QCOMPARE(d1.uniformBlocks().count(), 1);

    d1.detach();
    QVERIFY(QShaderDescriptionPrivate::get(&d0) != QShaderDescriptionPrivate::get(&d1));
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);
    QCOMPARE(d1.inputVariables().count(), 2);
    QCOMPARE(d1.outputVariables().count(), 1);
    QCOMPARE(d1.uniformBlocks().count(), 1);
}

void tst_QShader::bakedShaderImplicitSharing()
{
    QShader s0 = getShader(QLatin1String(":/data/color_simple.vert.qsb"));
    QVERIFY(s0.isValid());
    QCOMPARE(s0.availableShaders().count(), 1);
    QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));

    {
        QShader s1 = s0;
        QVERIFY(QShaderPrivate::get(&s0) == QShaderPrivate::get(&s1));
        QCOMPARE(s0.availableShaders().count(), 1);
        QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s1.availableShaders().count(), 1);
        QVERIFY(s1.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s0.stage(), s1.stage());
        QCOMPARE(s0, s1);

        s1.detach();
        QVERIFY(QShaderPrivate::get(&s0) != QShaderPrivate::get(&s1));
        QCOMPARE(s0.availableShaders().count(), 1);
        QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s1.availableShaders().count(), 1);
        QVERIFY(s1.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s0.stage(), s1.stage());
        QCOMPARE(s0, s1);
    }

    {
        QShader s1 = s0;
        QVERIFY(QShaderPrivate::get(&s0) == QShaderPrivate::get(&s1));
        QCOMPARE(s0.stage(), s1.stage());

        s1.setStage(QShader::FragmentStage); // call a setter to trigger a detach
        QVERIFY(QShaderPrivate::get(&s0) != QShaderPrivate::get(&s1));
        QCOMPARE(s0.availableShaders().count(), 1);
        QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s1.availableShaders().count(), 1);
        QVERIFY(s1.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QShaderDescription d0 = s0.description();
        QCOMPARE(d0.inputVariables().count(), 2);
        QCOMPARE(d0.outputVariables().count(), 1);
        QCOMPARE(d0.uniformBlocks().count(), 1);
        QShaderDescription d1 = s1.description();
        QCOMPARE(d1.inputVariables().count(), 2);
        QCOMPARE(d1.outputVariables().count(), 1);
        QCOMPARE(d1.uniformBlocks().count(), 1);
        QVERIFY(s0 != s1);
    }
}

#include <tst_qshader.moc>
QTEST_MAIN(tst_QShader)
