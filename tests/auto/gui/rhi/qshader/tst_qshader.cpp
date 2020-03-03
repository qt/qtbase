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
    void mslResourceMapping();
    void loadV3();
    void serializeShaderDesc();
    void comparison();
    void loadV4();
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
    QShader s = getShader(QLatin1String(":/data/color_spirv_v1.vert.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 1);
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
    QShader s = getShader(QLatin1String(":/data/color_all_v1.vert.qsb"));
    // spirv, glsl 100, glsl 330, glsl 120, hlsl 50, msl 12
    // + batchable variants
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 1);
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
    QShader s = getShader(QLatin1String(":/data/color_spirv_v1.vert.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 1);
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
    QCOMPARE(d0, d1);

    d1.detach();
    QVERIFY(QShaderDescriptionPrivate::get(&d0) != QShaderDescriptionPrivate::get(&d1));
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);
    QCOMPARE(d1.inputVariables().count(), 2);
    QCOMPARE(d1.outputVariables().count(), 1);
    QCOMPARE(d1.uniformBlocks().count(), 1);
    QCOMPARE(d0, d1);

    d1 = QShaderDescription();
    QVERIFY(d0 != d1);
}

void tst_QShader::bakedShaderImplicitSharing()
{
    QShader s0 = getShader(QLatin1String(":/data/color_spirv_v1.vert.qsb"));
    QVERIFY(s0.isValid());
    QCOMPARE(QShaderPrivate::get(&s0)->qsbVersion, 1);
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

void tst_QShader::mslResourceMapping()
{
    QShader s = getShader(QLatin1String(":/data/texture_all_v2.frag.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 2);

    const QVector<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 7);
    QVERIFY(availableShaders.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::MslShader, QShaderVersion(12))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::HlslShader, QShaderVersion(50))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(120))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(150))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));

    const QShader::NativeResourceBindingMap *resMap =
            s.nativeResourceBindingMap(QShaderKey(QShader::GlslShader, QShaderVersion(330)));
    QVERIFY(!resMap);

    // The Metal shader must come with a mapping table for binding points 0
    // (uniform buffer) and 1 (combined image sampler mapped to a texture and
    // sampler in the shader).
    resMap = s.nativeResourceBindingMap(QShaderKey(QShader::MslShader, QShaderVersion(12)));
    QVERIFY(resMap);

    QCOMPARE(resMap->count(), 2);
    QCOMPARE(resMap->value(0).first, 0); // mapped to native buffer index 0
    QCOMPARE(resMap->value(1), qMakePair(0, 0)); // mapped to native texture index 0 and sampler index 0
}

void tst_QShader::loadV3()
{
    // qsb version 3: QShaderDescription is serialized as CBOR. Ensure the deserialized data is as expected.
    QShader s = getShader(QLatin1String(":/data/texture_all_v3.frag.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 3);

    const QVector<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 7);
    QVERIFY(availableShaders.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::MslShader, QShaderVersion(12))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::HlslShader, QShaderVersion(50))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(120))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(150))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));

    const QShaderDescription desc = s.description();
    QVERIFY(desc.isValid());
    QCOMPARE(desc.inputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.inputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QLatin1String("qt_TexCoord"));
            QCOMPARE(v.type, QShaderDescription::Vec2);
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
            QCOMPARE(v.name, QLatin1String("fragColor"));
            QCOMPARE(v.type, QShaderDescription::Vec4);
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
            QCOMPARE(v.name, QLatin1String("qt_Matrix"));
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

void tst_QShader::serializeShaderDesc()
{
    // default constructed QShaderDescription
    {
        QShaderDescription desc;
        QVERIFY(!desc.isValid());

        QByteArray data;
        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::WriteOnly));
            desc.serialize(&ds);
        }
        QVERIFY(!data.isEmpty());

        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::ReadOnly));
            QShaderDescription desc2 = QShaderDescription::deserialize(&ds, QShaderPrivate::QSB_VERSION);
            QVERIFY(!desc2.isValid());
        }
    }

    // a QShaderDescription with inputs, outputs, uniform block and combined image sampler
    {
        QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s.isValid());
        const QShaderDescription desc = s.description();
        QVERIFY(desc.isValid());

        QByteArray data;
        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::WriteOnly));
            desc.serialize(&ds);
        }
        QVERIFY(!data.isEmpty());

        {
            QShaderDescription desc2;
            QVERIFY(!desc2.isValid());
            QVERIFY(!(desc == desc2));
            QVERIFY(desc != desc2);
        }

        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::ReadOnly));
            QShaderDescription desc2 = QShaderDescription::deserialize(&ds, QShaderPrivate::QSB_VERSION);
            QVERIFY(desc2.isValid());
            QCOMPARE(desc, desc2);
        }
    }
}

void tst_QShader::comparison()
{
    // exercise QShader and QShaderDescription comparisons
    {
        QShader s1 = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s1.isValid());
        QShader s2 = getShader(QLatin1String(":/data/color_all_v1.vert.qsb"));
        QVERIFY(s2.isValid());

        QVERIFY(s1.description().isValid());
        QVERIFY(s2.description().isValid());

        QVERIFY(s1 != s2);
        QVERIFY(s1.description() != s2.description());
    }

    {
        QShader s1 = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s1.isValid());
        QShader s2 = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s2.isValid());

        QVERIFY(s1.description().isValid());
        QVERIFY(s2.description().isValid());

        QVERIFY(s1 == s2);
        QVERIFY(s1.description() == s2.description());
    }
}

void tst_QShader::loadV4()
{
    // qsb version 4: QShaderDescription is serialized via QDataStream. Ensure the deserialized data is as expected.
    QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 4);

    const QVector<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 7);
    QVERIFY(availableShaders.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::MslShader, QShaderVersion(12))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::HlslShader, QShaderVersion(50))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(120))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(150))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));

    const QShaderDescription desc = s.description();
    QVERIFY(desc.isValid());
    QCOMPARE(desc.inputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.inputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QLatin1String("qt_TexCoord"));
            QCOMPARE(v.type, QShaderDescription::Vec2);
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
            QCOMPARE(v.name, QLatin1String("fragColor"));
            QCOMPARE(v.type, QShaderDescription::Vec4);
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
            QCOMPARE(v.name, QLatin1String("qt_Matrix"));
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

#include <tst_qshader.moc>
QTEST_MAIN(tst_QShader)
