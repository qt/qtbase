// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qshader_p.h"
#include <QDataStream>
#include <QBuffer>

#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QShader
    \ingroup painting-3D
    \inmodule QtGui
    \since 6.6

    \brief Contains multiple versions of a shader translated to multiple shading languages,
    together with reflection metadata.

    QShader is the entry point to shader code in the graphics API agnostic
    Qt world. Instead of using GLSL shader sources, as was the custom with Qt
    5.x, new graphics systems with backends for multiple graphics APIs, such
    as, Vulkan, Metal, Direct3D, and OpenGL, take QShader as their input
    whenever a shader needs to be specified.

    \warning The QRhi family of classes in the Qt Gui module, including QShader
    and QShaderDescription, offer limited compatibility guarantees. There are
    no source or binary compatibility guarantees for these classes, meaning the
    API is only guaranteed to work with the Qt version the application was
    developed against. Source incompatible changes are however aimed to be kept
    at a minimum and will only be made in minor releases (6.7, 6.8, and so on).
    To use these classes in an application, link to
    \c{Qt::GuiPrivate} (if using CMake), and include the headers with the \c
    rhi prefix, for example \c{#include <rhi/qshader.h>}.

    A QShader instance is empty and thus invalid by default. To get a useful
    instance, the two typical methods are:

    \list

    \li Generate the contents offline, during build time or earlier, using the
    \c qsb command line tool. The result is a binary file that is shipped with
    the application, read via QIODevice::readAll(), and then deserialized via
    fromSerialized(). For more information, see QShaderBaker.

    \li Generate at run time via QShaderBaker. This is an expensive operation,
    but allows applications to use user-provided or dynamically generated
    shader source strings.

    \endlist

    When used together with the Qt Rendering Hardware Interface and its
    classes, like QRhiGraphicsPipeline, no further action is needed from the
    application's side as these classes are prepared to consume a QShader
    whenever a shader needs to be specified for a given stage of the graphics
    pipeline.

    Alternatively, applications can access

    \list

    \li the source or byte code for any of the shading language versions that
    are included in the QShader,

    \li the name of the entry point for the shader,

    \li the reflection metadata containing a description of the shader's
    inputs, outputs and resources like uniform blocks. This is essential when
    an application or framework needs to discover the inputs of a shader at
    runtime due to not having advance knowledge of the vertex attributes or the
    layout of the uniform buffers used by the shader.

    \endlist

    QShader makes no assumption about the shading language that was used
    as the source for generating the various versions and variants that are
    included in it.

    QShader uses implicit sharing similarly to many core Qt types, and so
    can be returned or passed by value. Detach happens implicitly when calling
    a setter.

    For reference, a typical, portable QRhi expects that a QShader suitable for
    all its backends contains at least the following. (this excludes support
    for core profile OpenGL contexts, add GLSL 150 or newer for that)

    \list

    \li SPIR-V 1.0 bytecode suitable for Vulkan 1.0 or newer

    \li GLSL/ES 100 source code suitable for OpenGL ES 2.0 or newer

    \li GLSL 120 source code suitable for OpenGL 2.1 or newer

    \li HLSL Shader Model 5.0 source code or the corresponding DXBC bytecode suitable for Direct3D 11/12

    \li Metal Shading Language 1.2 source code or the corresponding bytecode suitable for Metal 1.2 or newer

    \endlist

    \sa QShaderBaker
 */

/*!
    \enum QShader::Stage
    Describes the stage of the graphics pipeline the shader is suitable for.

    \value VertexStage Vertex shader
    \value TessellationControlStage Tessellation control (hull) shader
    \value TessellationEvaluationStage Tessellation evaluation (domain) shader
    \value GeometryStage Geometry shader
    \value FragmentStage Fragment (pixel) shader
    \value ComputeStage Compute shader
 */

/*!
    \class QShaderVersion
    \inmodule QtGui
    \since 6.6

    \brief Specifies the shading language version.

    While languages like SPIR-V or the Metal Shading Language use traditional
    version numbers, shaders for other APIs can use slightly different
    versioning schemes. All those are mapped to a single version number in
    here, however. For HLSL, the version refers to the Shader Model version,
    like 5.0, 5.1, or 6.0. For GLSL an additional flag is needed to choose
    between GLSL and GLSL/ES.

    Below is a list with the most common examples of shader versions for
    different graphics APIs:

    \list

    \li Vulkan (SPIR-V): 100
    \li OpenGL: 120, 330, 440, etc.
    \li OpenGL ES: 100 with GlslEs, 300 with GlslEs, etc.
    \li Direct3D: 50, 51, 60
    \li Metal: 12, 20
    \endlist

    A default constructed QShaderVersion contains a version of 100 and no
    flags set.

    \note This is a RHI API with limited compatibility guarantees, see \l QShader
    for details.
 */

/*!
    \enum QShaderVersion::Flag

    Describes the flags that can be set.

    \value GlslEs Indicates that GLSL/ES is meant in combination with GlslShader
 */

/*!
    \class QShaderKey
    \inmodule QtGui
    \since 6.6

    \brief Specifies the shading language, the version with flags, and the variant.

    A default constructed QShaderKey has source set to SpirvShader and
    sourceVersion set to 100. sourceVariant defaults to StandardShader.

    \note This is a RHI API with limited compatibility guarantees, see \l QShader
    for details.
 */

/*!
    \enum QShader::Source
    Describes what kind of shader code an entry contains.

    \value SpirvShader SPIR-V
    \value GlslShader GLSL
    \value HlslShader HLSL
    \value DxbcShader Direct3D bytecode (HLSL compiled by \c fxc)
    \value MslShader Metal Shading Language
    \value DxilShader Direct3D bytecode (HLSL compiled by \c dxc)
    \value MetalLibShader Pre-compiled Metal bytecode
    \value WgslShader WGSL
 */

/*!
    \enum QShader::Variant
    Describes what kind of shader code an entry contains.

    \value StandardShader The normal, unmodified version of the shader code.

    \value BatchableVertexShader Vertex shader rewritten to be suitable for Qt Quick scenegraph batching.

    \value UInt16IndexedVertexAsComputeShader A vertex shader meant to be used
    in a Metal pipeline with tessellation in combination with indexed draw
    calls sourcing index data from a uint16 index buffer. To support the Metal
    tessellation pipeline, the vertex shader is translated to a compute shader
    that may be dependent on the index buffer usage in the draw calls (e.g. if
    the shader is using gl_VertexIndex), hence the need for three dedicated
    variants.

    \value UInt32IndexedVertexAsComputeShader A vertex shader meant to be used
    in a Metal pipeline with tessellation in combination with indexed draw
    calls sourcing index data from a uint32 index buffer. To support the Metal
    tessellation pipeline, the vertex shader is translated to a compute shader
    that may be dependent on the index buffer usage in the draw calls (e.g. if
    the shader is using gl_VertexIndex), hence the need for three dedicated
    variants.

    \value NonIndexedVertexAsComputeShader A vertex shader meant to be used in
    a Metal pipeline with tessellation in combination with non-indexed draw
    calls. To support the Metal tessellation pipeline, the vertex shader is
    translated to a compute shader that may be dependent on the index buffer
    usage in the draw calls (e.g. if the shader is using gl_VertexIndex), hence
    the need for three dedicated variants.

    \value [since 6.10] HdrCapableFragmentShader A fragment shader rewritten to support high
    dynamic range rendering in a Qt Quick scenegraph.
 */

/*!
    \enum QShader::SerializedFormatVersion
    Describes the desired output format when serializing the QShader.

    The default value for the \c version argument of serialized() is \c Latest.
    This is sufficient in the vast majority of cases. Specifying another value
    is needed only when the intention is to generate serialized data that can
    be loaded by earlier Qt versions. For example, the \c qsb tool uses these
    enum values when the \c{--qsbversion} command-line argument is given.

    \note Targeting earlier versions will make certain features disfunctional
    with the generated asset. This is not an issue when using the asset with
    the specified, older Qt version, given that that Qt version does not have
    the newer features in newer Qt versions that rely on additional data
    generated in the QShader and the serialized data stream, but may become a
    problem if the generated asset is then used with a newer Qt version.

    \value Latest The current Qt version
    \value Qt_6_5 Qt 6.5
    \value Qt_6_4 Qt 6.4
 */

/*!
    \class QShaderCode
    \inmodule QtGui
    \since 6.6

    \brief Contains source or binary code for a shader and additional metadata.

    When shader() is empty after retrieving a QShaderCode instance from
    QShader, it indicates no shader code was found for the requested key.

    \note This is a RHI API with limited compatibility guarantees, see \l QShader
    for details.
 */

/*!
    Constructs a new, empty (and thus invalid) QShader instance.
 */
QShader::QShader()
    : d(nullptr)
{
}

/*!
    \internal
 */
void QShader::detach()
{
    if (d)
        qAtomicDetach(d);
    else
        d = new QShaderPrivate;
}

/*!
    Constructs a copy of \a other.
 */
QShader::QShader(const QShader &other)
    : d(other.d)
{
    if (d)
        d->ref.ref();
}

/*!
    Assigns \a other to this object.
 */
QShader &QShader::operator=(const QShader &other)
{
    if (d) {
        if (other.d) {
            qAtomicAssign(d, other.d);
        } else {
            if (!d->ref.deref())
                delete d;
            d = nullptr;
        }
    } else if (other.d) {
        other.d->ref.ref();
        d = other.d;
    }
    return *this;
}

/*!
    \fn QShader::QShader(QShader &&other) noexcept
    \since 6.7

    Move-constructs a new QShader from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    \fn QShader &QShader::operator=(QShader &&other)
    \since 6.7

    Move-assigns \a other to this QShader instance.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    Destructor.
 */
QShader::~QShader()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    \fn void QShader::swap(QShader &other)
    \since 6.7
    \memberswap{shader}
*/

/*!
    \return true if the QShader contains at least one shader version.
 */
bool QShader::isValid() const
{
    return d ? !d->shaders.isEmpty() : false;
}

/*!
    \return the pipeline stage the shader is meant for.
 */
QShader::Stage QShader::stage() const
{
    return d ? d->stage : QShader::VertexStage;
}

/*!
    Sets the pipeline \a stage.
 */
void QShader::setStage(Stage stage)
{
    if (!d || stage != d->stage) {
        detach();
        d->stage = stage;
    }
}

/*!
    \return the reflection metadata for the shader.
 */
QShaderDescription QShader::description() const
{
    return d ? d->desc : QShaderDescription();
}

/*!
    Sets the reflection metadata to \a desc.
 */
void QShader::setDescription(const QShaderDescription &desc)
{
    detach();
    d->desc = desc;
}

/*!
    \return the list of available shader versions
 */
QList<QShaderKey> QShader::availableShaders() const
{
    return d ? d->shaders.keys().toVector() : QList<QShaderKey>();
}

/*!
    \return the source or binary code for a given shader version specified by \a key.
 */
QShaderCode QShader::shader(const QShaderKey &key) const
{
    return d ? d->shaders.value(key) : QShaderCode();
}

/*!
    Stores the source or binary \a shader code for a given shader version specified by \a key.
 */
void QShader::setShader(const QShaderKey &key, const QShaderCode &shader)
{
    if (d && d->shaders.value(key) == shader)
        return;

    detach();
    d->shaders[key] = shader;
}

/*!
    Removes the source or binary shader code for a given \a key.
    Does nothing when not found.
 */
void QShader::removeShader(const QShaderKey &key)
{
    if (!d)
        return;

    auto it = d->shaders.find(key);
    if (it == d->shaders.end())
        return;

    detach();
    d->shaders.erase(it);
}

static void writeShaderKey(QDataStream *ds, const QShaderKey &k)
{
    *ds << int(k.source());
    *ds << k.sourceVersion().version();
    *ds << k.sourceVersion().flags();
    *ds << int(k.sourceVariant());
}

/*!
    \return a serialized binary version of all the data held by the
    QShader, suitable for writing to files or other I/O devices.

    By default the latest serialization format is used. Use \a version
    parameter to serialize for a compatibility Qt version. Only when it is
    known that the generated data stream must be made compatible with an older
    Qt version at the expense of making it incompatible with features
    introduced since that Qt version, should another value (for example,
    \l{SerializedFormatVersion}{Qt_6_5} for Qt 6.5) be used.

    \sa fromSerialized()
 */
QByteArray QShader::serialized(SerializedFormatVersion version) const
{
    static QShaderPrivate sd;
    QShaderPrivate *dd = d ? d : &sd;

    QBuffer buf;
    QDataStream ds(&buf);
    ds.setVersion(QDataStream::Qt_5_10);
    if (!buf.open(QIODevice::WriteOnly))
        return QByteArray();

    const int qsbVersion = QShaderPrivate::qtQsbVersion(version);
    ds << qsbVersion;

    ds << int(dd->stage);
    dd->desc.serialize(&ds, qsbVersion);
    ds << int(dd->shaders.size());
    for (auto it = dd->shaders.cbegin(), itEnd = dd->shaders.cend(); it != itEnd; ++it) {
        const QShaderKey &k(it.key());
        writeShaderKey(&ds, k);
        const QShaderCode &shader(dd->shaders.value(k));
        ds << shader.shader();
        ds << shader.entryPoint();
    }
    ds << int(dd->bindings.size());
    for (auto it = dd->bindings.cbegin(), itEnd = dd->bindings.cend(); it != itEnd; ++it) {
        const QShaderKey &k(it.key());
        writeShaderKey(&ds, k);
        const NativeResourceBindingMap &map(it.value());
        ds << int(map.size());
        for (auto mapIt = map.cbegin(), mapItEnd = map.cend(); mapIt != mapItEnd; ++mapIt) {
            ds << mapIt.key();
            ds << mapIt.value().first;
            ds << mapIt.value().second;
        }
    }
    ds << int(dd->combinedImageMap.size());
    for (auto it = dd->combinedImageMap.cbegin(), itEnd = dd->combinedImageMap.cend(); it != itEnd; ++it) {
        const QShaderKey &k(it.key());
        writeShaderKey(&ds, k);
        const SeparateToCombinedImageSamplerMappingList &list(it.value());
        ds << int(list.size());
        for (auto listIt = list.cbegin(), listItEnd = list.cend(); listIt != listItEnd; ++listIt) {
            ds << listIt->combinedSamplerName;
            ds << listIt->textureBinding;
            ds << listIt->samplerBinding;
        }
    }
    if (qsbVersion > QShaderPrivate::QSB_VERSION_WITHOUT_NATIVE_SHADER_INFO) {
        ds << int(dd->nativeShaderInfoMap.size());
        for (auto it = dd->nativeShaderInfoMap.cbegin(), itEnd = dd->nativeShaderInfoMap.cend(); it != itEnd; ++it) {
            const QShaderKey &k(it.key());
            writeShaderKey(&ds, k);
            ds << it->flags;
            ds << int(it->extraBufferBindings.size());
            for (auto mapIt = it->extraBufferBindings.cbegin(), mapItEnd = it->extraBufferBindings.cend();
                 mapIt != mapItEnd; ++mapIt)
            {
                ds << mapIt.key();
                ds << mapIt.value();
            }
        }
    }

    return qCompress(buf.buffer());
}

static void readShaderKey(QDataStream *ds, QShaderKey *k)
{
    int intVal;
    *ds >> intVal;
    k->setSource(QShader::Source(intVal));
    QShaderVersion ver;
    *ds >> intVal;
    ver.setVersion(intVal);
    *ds >> intVal;
    ver.setFlags(QShaderVersion::Flags(intVal));
    k->setSourceVersion(ver);
    *ds >> intVal;
    k->setSourceVariant(QShader::Variant(intVal));
}

/*!
    Creates a new QShader instance from the given \a data.

    If \a data cannot be deserialized successfully, the result is a default
    constructed QShader for which isValid() returns \c false.

    \warning Shader packages, including \c{.qsb} files in the filesystem, are
    assumed to be trusted content. Application developers are advised to
    carefully consider the potential implications before allowing the loading of
    user-provided content that is not part of the application.

    \sa serialized()
  */
QShader QShader::fromSerialized(const QByteArray &data)
{
    QByteArray udata = qUncompress(data);
    QBuffer buf(&udata);
    QDataStream ds(&buf);
    ds.setVersion(QDataStream::Qt_5_10);
    if (!buf.open(QIODevice::ReadOnly))
        return QShader();

    QShader bs;
    bs.detach(); // to get d created
    QShaderPrivate *d = QShaderPrivate::get(&bs);
    Q_ASSERT(d->ref.loadRelaxed() == 1); // must be detached
    int intVal;
    ds >> intVal;
    d->qsbVersion = intVal;
    if (d->qsbVersion != QShaderPrivate::QSB_VERSION
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_INPUT_OUTPUT_INTERFACE_BLOCKS
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_EXTENDED_STORAGE_BUFFER_INFO
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_NATIVE_SHADER_INFO
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_VAR_ARRAYDIMS
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITH_CBOR
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITH_BINARY_JSON
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_BINDINGS)
    {
        qWarning("Attempted to deserialize QShader with unknown version %d.", d->qsbVersion);
        return QShader();
    }

    ds >> intVal;
    d->stage = Stage(intVal);
    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITH_CBOR) {
        d->desc = QShaderDescription::deserialize(&ds, d->qsbVersion);
    } else if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITH_BINARY_JSON) {
        qWarning("Can no longer load QShaderDescription from CBOR.");
        d->desc = QShaderDescription();
    } else {
        qWarning("Can no longer load QShaderDescription from binary JSON.");
        d->desc = QShaderDescription();
    }
    int count;
    ds >> count;
    for (int i = 0; i < count; ++i) {
        QShaderKey k;
        readShaderKey(&ds, &k);
        QShaderCode shader;
        QByteArray s;
        ds >> s;
        shader.setShader(s);
        ds >> s;
        shader.setEntryPoint(s);
        d->shaders[k] = shader;
    }

    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITHOUT_BINDINGS) {
        ds >> count;
        for (int i = 0; i < count; ++i) {
            QShaderKey k;
            readShaderKey(&ds, &k);
            NativeResourceBindingMap map;
            int mapSize;
            ds >> mapSize;
            for (int b = 0; b < mapSize; ++b) {
                int binding;
                ds >> binding;
                int firstNativeBinding;
                ds >> firstNativeBinding;
                int secondNativeBinding;
                ds >> secondNativeBinding;
                map.insert(binding, { firstNativeBinding, secondNativeBinding });
            }
            d->bindings.insert(k, map);
        }
    }

    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS) {
        ds >> count;
        for (int i = 0; i < count; ++i) {
            QShaderKey k;
            readShaderKey(&ds, &k);
            SeparateToCombinedImageSamplerMappingList list;
            int listSize;
            ds >> listSize;
            for (int b = 0; b < listSize; ++b) {
                QByteArray combinedSamplerName;
                ds >> combinedSamplerName;
                int textureBinding;
                ds >> textureBinding;
                int samplerBinding;
                ds >> samplerBinding;
                list.append({ combinedSamplerName, textureBinding, samplerBinding });
            }
            d->combinedImageMap.insert(k, list);
        }
    }

    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITHOUT_NATIVE_SHADER_INFO) {
        ds >> count;
        for (int i = 0; i < count; ++i) {
            QShaderKey k;
            readShaderKey(&ds, &k);
            int flags;
            ds >> flags;
            QMap<int, int> extraBufferBindings;
            int mapSize;
            ds >> mapSize;
            for (int b = 0; b < mapSize; ++b) {
                int k, v;
                ds >> k;
                ds >> v;
                extraBufferBindings.insert(k, v);
            }
            d->nativeShaderInfoMap.insert(k, { flags, extraBufferBindings });
        }
    }

    return bs;
}

/*!
    \fn QShaderVersion::QShaderVersion() = default
 */

/*!
    Constructs a new QShaderVersion with version \a v and flags \a f.
 */
QShaderVersion::QShaderVersion(int v, Flags f)
    : m_version(v), m_flags(f)
{
}

/*!
    \fn int QShaderVersion::version() const
    \return the version.
 */

/*!
    \fn void QShaderVersion::setVersion(int v)
    Sets the shading language version to \a v.
 */

/*!
    \fn QShaderVersion::Flags QShaderVersion::flags() const
    \return the flags.
 */

/*!
    \fn void QShaderVersion::setFlags(Flags f)
    Sets the flags \a f.
 */

/*!
    \fn QShaderCode::QShaderCode() = default
 */

/*!
    Constructs a new QShaderCode with the specified shader source \a code and
    \a entry point name.
 */
QShaderCode::QShaderCode(const QByteArray &code, const QByteArray &entry)
    : m_shader(code), m_entryPoint(entry)
{
}

/*!
    \fn QByteArray QShaderCode::shader() const
    \return the shader source or bytecode.
 */

/*!
    \fn void QShaderCode::setShader(const QByteArray &code)
    Sets the shader source or byte \a code.
 */

/*!
    \fn QByteArray QShaderCode::entryPoint() const
    \return the entry point name.
 */

/*!
    \fn void QShaderCode::setEntryPoint(const QByteArray &entry)
    Sets the \a entry point name.
 */

/*!
    \fn QShaderKey::QShaderKey() = default
 */

/*!
    Constructs a new QShaderKey with shader type \a s, version \a sver, and
    variant \a svar.
 */
QShaderKey::QShaderKey(QShader::Source s,
                       const QShaderVersion &sver,
                       QShader::Variant svar)
    : m_source(s),
      m_sourceVersion(sver),
      m_sourceVariant(svar)
{
}

/*!
    \fn QShader::Source QShaderKey::source() const
    \return the shader type.
 */

/*!
    \fn void QShaderKey::setSource(QShader::Source s)
    Sets the shader type \a s.
 */

/*!
    \fn QShaderVersion QShaderKey::sourceVersion() const
    \return the shading language version.
 */

/*!
    \fn void QShaderKey::setSourceVersion(const QShaderVersion &sver)
    Sets the shading language version \a sver.
 */

/*!
    \fn QShader::Variant QShaderKey::sourceVariant() const
    \return the type of the variant to use.
 */

/*!
    \fn void QShaderKey::setSourceVariant(QShader::Variant svar)
    Sets the type of variant to use to \a svar.
 */

/*!
    Returns \c true if the two QShader objects \a lhs and \a rhs are equal,
    meaning they are for the same stage with matching sets of shader source or
    binary code.

    \relates QShader
 */
bool operator==(const QShader &lhs, const QShader &rhs) noexcept
{
    if (!lhs.d || !rhs.d)
        return lhs.d == rhs.d;

    return lhs.d->stage == rhs.d->stage
            && lhs.d->shaders == rhs.d->shaders
            && lhs.d->bindings == rhs.d->bindings;
}

/*!
    \fn bool operator!=(const QShader &lhs, const QShader &rhs)

    Returns \c false if the values in the two QShader objects \a lhs and \a rhs
    are equal; otherwise returns \c true.

    \relates QShader
 */

/*!
    \fn size_t qHash(const QShader &key, size_t seed)
    \qhashold{QShader}
 */
size_t qHash(const QShader &s, size_t seed) noexcept
{
    if (s.d) {
        QtPrivate::QHashCombineWithSeed hash(seed);
        seed = hash(seed, s.stage());
        if (!s.d->shaders.isEmpty()) {
            seed = hash(seed, s.d->shaders.firstKey());
            seed = hash(seed, std::as_const(s.d->shaders).first());
        }
    }
    return seed;
}

/*!
    Returns \c true if the two QShaderVersion objects \a lhs and \a rhs are
    equal.

    \relates QShaderVersion
 */
bool operator==(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept
{
    return lhs.version() == rhs.version() && lhs.flags() == rhs.flags();
}

#ifdef Q_OS_INTEGRITY
size_t qHash(const QShaderVersion &s, size_t seed) noexcept
{
    return qHashMulti(seed, s.version(), s.flags());
}
#endif

/*!
    \return true if \a lhs is smaller than \a rhs.

    Establishes a sorting order between the two QShaderVersion \a lhs and \a rhs.

    \relates QShaderVersion
 */
bool operator<(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept
{
    if (lhs.version() < rhs.version())
        return true;

    if (lhs.version() == rhs.version())
        return int(lhs.flags()) < int(rhs.flags());

    return false;
}

/*!
    \fn bool operator!=(const QShaderVersion &lhs, const QShaderVersion &rhs)

    Returns \c false if the values in the two QShaderVersion objects \a lhs
    and \a rhs are equal; otherwise returns \c true.

    \relates QShaderVersion
 */

/*!
    Returns \c true if the two QShaderKey objects \a lhs and \a rhs are equal.

    \relates QShaderKey
 */
bool operator==(const QShaderKey &lhs, const QShaderKey &rhs) noexcept
{
    return lhs.source() == rhs.source() && lhs.sourceVersion() == rhs.sourceVersion()
            && lhs.sourceVariant() == rhs.sourceVariant();
}

/*!
    \return true if \a lhs is smaller than \a rhs.

    Establishes a sorting order between the two keys \a lhs and \a rhs.

    \relates QShaderKey
 */
bool operator<(const QShaderKey &lhs, const QShaderKey &rhs) noexcept
{
    if (int(lhs.source()) < int(rhs.source()))
        return true;

    if (int(lhs.source()) == int(rhs.source())) {
        if (lhs.sourceVersion() < rhs.sourceVersion())
            return true;
        if (lhs.sourceVersion() == rhs.sourceVersion()) {
            if (int(lhs.sourceVariant()) < int(rhs.sourceVariant()))
                return true;
        }
    }

    return false;
}

/*!
    \fn bool operator!=(const QShaderKey &lhs, const QShaderKey &rhs)

    Returns \c false if the values in the two QShaderKey objects \a lhs
    and \a rhs are equal; otherwise returns \c true.

    \relates QShaderKey
 */

/*!
    \fn size_t qHash(const QShaderKey &key, size_t seed)
    \qhashold{QShaderKey}
 */
size_t qHash(const QShaderKey &k, size_t seed) noexcept
{
    return qHashMulti(seed,
                      k.source(),
                      k.sourceVersion().version(),
                      k.sourceVersion().flags(),
                      k.sourceVariant());
}

/*!
    Returns \c true if the two QShaderCode objects \a lhs and \a rhs are equal.

    \relates QShaderCode
 */
bool operator==(const QShaderCode &lhs, const QShaderCode &rhs) noexcept
{
    return lhs.shader() == rhs.shader() && lhs.entryPoint() == rhs.entryPoint();
}

/*!
    \fn bool operator!=(const QShaderCode &lhs, const QShaderCode &rhs)

    Returns \c false if the values in the two QShaderCode objects \a lhs
    and \a rhs are equal; otherwise returns \c true.

    \relates QShaderCode
 */

/*!
    \fn size_t qHash(const QShaderCode &key, size_t seed)
    \qhashold{QShaderCode}
 */
size_t qHash(const QShaderCode &k, size_t seed) noexcept
{
    return qHash(k.shader(), seed);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QShader &bs)
{
    const QShaderPrivate *d = bs.d;
    QDebugStateSaver saver(dbg);

    if (d) {
        dbg.nospace() << "QShader("
                      << "stage=" << d->stage
                      << " shaders=" << d->shaders.keys()
                      << " desc.isValid=" << d->desc.isValid()
                      << ')';
    } else {
        dbg.nospace() << "QShader()";
    }

    return dbg;
}

QDebug operator<<(QDebug dbg, const QShaderKey &k)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "ShaderKey(" << k.source()
                  << " " << k.sourceVersion()
                  << " " << k.sourceVariant() << ")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const QShaderVersion &v)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "Version(" << v.version() << " " << v.flags() << ")";
    return dbg;
}
#endif // QT_NO_DEBUG_STREAM

/*!
    \typedef QShader::NativeResourceBindingMap

    Synonym for QMap<int, std::pair<int, int>>.

    The resource binding model QRhi assumes is based on SPIR-V. This means that
    uniform buffers, storage buffers, combined image samplers, and storage
    images share a common binding point space. The binding numbers in
    QShaderDescription and QRhiShaderResourceBinding are expected to match the
    \c binding layout qualifier in the Vulkan-compatible GLSL shader.

    Graphics APIs other than Vulkan may use a resource binding model that is
    not fully compatible with this. The generator of the shader code translated
    from SPIR-V may choose not to take the SPIR-V binding qualifiers into
    account, for various reasons. This is the case with the Metal backend of
    SPIRV-Cross, for example. In addition, even when an automatic, implicit
    translation is mostly possible (e.g. by using SPIR-V binding points as HLSL
    resource register indices), assigning resource bindings without being
    constrained by the SPIR-V binding points can lead to better results.

    Therefore, a QShader may expose an additional map that describes what the
    native binding point for a given SPIR-V binding is. The QRhi backends, for
    which this is relevant, are expected to use this map automatically, as
    appropriate. The value is a pair, because combined image samplers may map
    to two native resources (a texture and a sampler) in some shading
    languages. In that case the second value refers to the sampler.

    \note The native binding may be -1, in case there is no active binding for
    the resource in the shader. (for example, there is a uniform block
    declared, but it is not used in the shader code) The map is always
    complete, meaning there is an entry for all declared uniform blocks,
    storage blocks, image objects, and combined samplers, but the value will be
    -1 for those that are not actually referenced in the shader functions.
*/

/*!
    \return the native binding map for \a key. The map is empty if no mapping
    is available for \a key (for example, because the map is not applicable for
    the API and shading language described by \a key).
 */
QShader::NativeResourceBindingMap QShader::nativeResourceBindingMap(const QShaderKey &key) const
{
    if (!d)
        return {};

    auto it = d->bindings.constFind(key);
    if (it == d->bindings.cend())
        return {};

    return it.value();
}

/*!
    Stores the given native resource binding \a map associated with \a key.

    \sa nativeResourceBindingMap()
 */
void QShader::setResourceBindingMap(const QShaderKey &key, const NativeResourceBindingMap &map)
{
    detach();
    d->bindings[key] = map;
}

/*!
    Removes the native resource binding map for \a key.
 */
void QShader::removeResourceBindingMap(const QShaderKey &key)
{
    if (!d)
        return;

    auto it = d->bindings.find(key);
    if (it == d->bindings.end())
        return;

    detach();
    d->bindings.erase(it);
}

/*!
    \typedef QShader::SeparateToCombinedImageSamplerMappingList

    Synonym for QList<QShader::SeparateToCombinedImageSamplerMapping>.
 */

/*!
    \struct QShader::SeparateToCombinedImageSamplerMapping
    \inmodule QtGui
    \brief Mapping metadata for sampler uniforms.

    Describes a mapping from a traditional combined image sampler uniform to
    binding points for a separate texture and sampler.

    For example, if \c combinedImageSampler is \c{"_54"}, \c textureBinding is
    \c 1, and \c samplerBinding is \c 2, this means that the GLSL shader code
    contains a \c sampler2D (or sampler3D, etc.) uniform with the name of
    \c{_54} which corresponds to two separate resource bindings (\c 1 and \c 2)
    in the original shader.

    \note This is a RHI API with limited compatibility guarantees, see \l QShader
    for details.
 */

/*!
    \variable QShader::SeparateToCombinedImageSamplerMapping::combinedSamplerName
*/

/*!
    \variable QShader::SeparateToCombinedImageSamplerMapping::textureBinding
*/

/*!
    \variable QShader::SeparateToCombinedImageSamplerMapping::samplerBinding
*/

/*!
    \return the combined image sampler mapping list for \a key, or an empty
    list if there is no data available for \a key, for example because such a
    mapping is not applicable for the shading language.
 */
QShader::SeparateToCombinedImageSamplerMappingList QShader::separateToCombinedImageSamplerMappingList(const QShaderKey &key) const
{
    if (!d)
        return {};

    auto it = d->combinedImageMap.constFind(key);
    if (it == d->combinedImageMap.cend())
        return {};

    return it.value();
}

/*!
    Stores the given combined image sampler mapping \a list associated with \a key.

    \sa separateToCombinedImageSamplerMappingList()
 */
void QShader::setSeparateToCombinedImageSamplerMappingList(const QShaderKey &key,
                                                           const SeparateToCombinedImageSamplerMappingList &list)
{
    detach();
    d->combinedImageMap[key] = list;
}

/*!
    Removes the combined image sampler mapping list for \a key.
 */
void QShader::removeSeparateToCombinedImageSamplerMappingList(const QShaderKey &key)
{
    if (!d)
        return;

    auto it = d->combinedImageMap.find(key);
    if (it == d->combinedImageMap.end())
        return;

    detach();
    d->combinedImageMap.erase(it);
}

/*!
    \struct QShader::NativeShaderInfo
    \inmodule QtGui
    \brief Additional metadata about the native shader code.

    Describes information about the native shader code, if applicable. This
    becomes relevant with certain shader languages for certain shader stages,
    in case the translation from SPIR-V involves the introduction of
    additional, "magic" inputs, outputs, or resources in the generated shader.
    Such additions may be dependent on the original source code (i.e. the usage
    of various GLSL language constructs or built-ins), and therefore it needs
    to be indicated in a dynamic manner if certain features got added to the
    generated shader code.

    As an example, consider a tessellation control shader with a per-patch (not
    per-vertex) output variable. This is translated to a Metal compute shader
    outputting (among others) into an spvPatchOut buffer. But this buffer would
    not be present at all if per-patch output variables were not used. The fact
    that the shader code relies on such a buffer present can be indicated by
    the data in this struct.

    \note This is a RHI API with limited compatibility guarantees, see \l QShader
    for details.
 */

/*!
    \variable QShader::NativeShaderInfo::flags
*/

/*!
    \variable QShader::NativeShaderInfo::extraBufferBindings
*/

/*!
    \return the native shader info struct for \a key, or an empty object if
    there is no data available for \a key, for example because such a mapping
    is not applicable for the shading language or the shader stage.
 */
QShader::NativeShaderInfo QShader::nativeShaderInfo(const QShaderKey &key) const
{
    if (!d)
        return {};

    auto it = d->nativeShaderInfoMap.constFind(key);
    if (it == d->nativeShaderInfoMap.cend())
        return {};

    return it.value();
}

/*!
    Stores the given native shader \a info associated with \a key.

    \sa nativeShaderInfo()
 */
void QShader::setNativeShaderInfo(const QShaderKey &key, const NativeShaderInfo &info)
{
    detach();
    d->nativeShaderInfoMap[key] = info;
}

/*!
    Removes the native shader information for \a key.
 */
void QShader::removeNativeShaderInfo(const QShaderKey &key)
{
    if (!d)
        return;

    auto it = d->nativeShaderInfoMap.find(key);
    if (it == d->nativeShaderInfoMap.end())
        return;

    detach();
    d->nativeShaderInfoMap.erase(it);
}

QT_END_NAMESPACE
