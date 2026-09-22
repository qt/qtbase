// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpluginloader.h"

#include "qcoreapplication.h"
#include "qdebug.h"
#include "qdir.h"
#include "qfactoryloader_p.h"
#include "qfileinfo.h"
#include "qjsondocument.h"

#if QT_CONFIG(library)
#  include "qlibrary_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(library)

/*!
    \class QPluginLoader
    \inmodule QtCore
    \reentrant
    \brief The QPluginLoader class loads a plugin at run-time.


    \ingroup plugins

    QPluginLoader provides access to a \l{How to Create Qt
    Plugins}{Qt plugin}. A Qt plugin is stored in a shared library (a
    DLL) and offers these benefits over shared libraries accessed
    using QLibrary:

    \list
    \li QPluginLoader checks that a plugin is linked against the same
       version of Qt as the application.
    \li QPluginLoader provides direct access to a root component object
       (instance()), instead of forcing you to resolve a C function manually.
    \endlist

    An instance of a QPluginLoader object operates on a single shared
    library file, which we call a plugin. It provides access to the
    functionality in the plugin in a platform-independent way. To
    specify which plugin to load, either pass a file name in
    the constructor or set it with setFileName().

    The most important functions are load() to dynamically load the
    plugin file, isLoaded() to check whether loading was successful,
    and instance() to access the root component in the plugin. The
    instance() function implicitly tries to load the plugin if it has
    not been loaded yet. Multiple instances of QPluginLoader can be
    used to access the same physical plugin.

    Once loaded, plugins remain in memory until all instances of
    QPluginLoader has been unloaded, or until the application
    terminates. You can attempt to unload a plugin using unload(),
    but if other instances of QPluginLoader are using the same
    library, the call will fail, and unloading will only happen when
    every instance has called unload(). Right before the unloading
    happens, the root component will also be deleted.

    See \l{How to Create Qt Plugins} for more information about
    how to make your application extensible through plugins.

    Note that the QPluginLoader cannot be used if your application is
    statically linked against Qt. In this case, you will also have to
    link to plugins statically. You can use QLibrary if you need to
    load dynamic libraries in a statically linked application.

    \sa QLibrary, {Echo Plugin Example}
*/

static constexpr QLibrary::LoadHints defaultLoadHints = QLibrary::PreventUnloadHint;

/*!
    Constructs a plugin loader with the given \a parent.
*/
QPluginLoader::QPluginLoader(QObject *parent)
    : QObject(parent), d(nullptr), did_load(false)
{
}

/*!
    Constructs a plugin loader with the given \a parent that will
    load the plugin specified by \a fileName.

    To be loadable, the file's suffix must be a valid suffix for a
    loadable library in accordance with the platform, e.g. \c .so on
    Unix, - \c .dylib on \macos and iOS, and \c .dll on Windows. The suffix
    can be verified with QLibrary::isLibrary().

    \sa setFileName()
*/
QPluginLoader::QPluginLoader(const QString &fileName, QObject *parent)
    : QObject(parent), d(nullptr), did_load(false)
{
    setFileName(fileName);
    setLoadHints(defaultLoadHints);
}

/*!
    Destroys the QPluginLoader object.

    Unless unload() was called explicitly, the plugin stays in memory
    until the application terminates.

    \sa isLoaded(), unload()
*/
QPluginLoader::~QPluginLoader()
{
    if (d)
        d->release();
}

/*!
    Returns the root component object of the plugin. The plugin is
    loaded if necessary. The function returns \nullptr if the plugin could
    not be loaded or if the root component object could not be
    instantiated.

    If the root component object was destroyed, calling this function
    creates a new instance.

    The root component, returned by this function, is not deleted when
    the QPluginLoader is destroyed. If you want to ensure that the root
    component is deleted, you should call unload() as soon you don't
    need to access the core component anymore.  When the library is
    finally unloaded, the root component will automatically be deleted.

    The component object is a QObject. Use qobject_cast() to access
    interfaces you are interested in.

    \sa load()
*/
QObject *QPluginLoader::instance()
{
    if (!isLoaded() && !load())
        return nullptr;
    return d->pluginInstance();
}

/*!
    Returns the meta data for this plugin. The meta data is data specified
    in a json format using the Q_PLUGIN_METADATA() macro when compiling
    the plugin.

    The meta data can be queried in a fast and inexpensive way without
    actually loading the plugin. This makes it possible to e.g. store
    capabilities of the plugin in there, and make the decision whether to
    load the plugin dependent on this meta data.
 */
QJsonObject QPluginLoader::metaData() const
{
    if (!d)
        return QJsonObject();
    return d->metaData.toJson();
}

/*!
    Loads the plugin and returns \c true if the plugin was loaded
    successfully; otherwise returns \c false. Since instance() always
    calls this function before resolving any symbols it is not
    necessary to call it explicitly. In some situations you might want
    the plugin loaded in advance, in which case you would use this
    function.

    \sa unload()
*/
bool QPluginLoader::load()
{
    if (!d || d->fileName.isEmpty())
        return false;
    if (did_load)
        return d->pHnd && d->instanceFactory.loadAcquire();
    if (!d->isPlugin())
        return false;
    did_load = true;
    return d->loadPlugin();
}

/*!
    Unloads the plugin and returns \c true if the plugin could be
    unloaded; otherwise returns \c false.

    This happens automatically on application termination, so you
    shouldn't normally need to call this function.

    If other instances of QPluginLoader are using the same plugin, the
    call will fail, and unloading will only happen when every instance
    has called unload().

    Don't try to delete the root component. Instead rely on
    that unload() will automatically delete it when needed.

    \sa instance(), load()
*/
bool QPluginLoader::unload()
{
    if (did_load) {
        did_load = false;
        return d->unload();
    }
    if (d) // Ouch
        d->errorString = tr("The plugin was not loaded.");
    return false;
}

/*!
    Returns \c true if the plugin is loaded; otherwise returns \c false.

    \sa load()
 */
bool QPluginLoader::isLoaded() const
{
    return d && d->pHnd && d->instanceFactory.loadRelaxed();
}

#if defined(QT_SHARED)
static QString locatePlugin(const QString& fileName)
{
    const bool isAbsolute = QDir::isAbsolutePath(fileName);
    if (isAbsolute) {
        QFileInfo fi(fileName);
        if (fi.isFile()) {
            return fi.canonicalFilePath();
        }
    }
    QStringList prefixes = QLibraryPrivate::prefixes_sys();
    prefixes.prepend(QString());
    QStringList suffixes = QLibraryPrivate::suffixes_sys(QString());
    suffixes.prepend(QString());

    // Split up "subdir/filename"
    const qsizetype slash = fileName.lastIndexOf(u'/');
    const auto baseName = QStringView{fileName}.mid(slash + 1);
    const auto basePath = isAbsolute ? QStringView() : QStringView{fileName}.left(slash + 1); // keep the '/'

    QStringList paths;
    if (isAbsolute) {
        paths.append(fileName.left(slash)); // don't include the '/'
    } else {
        paths = QCoreApplication::libraryPaths();
    }

    for (const QString &path : std::as_const(paths)) {
        for (const QString &prefix : std::as_const(prefixes)) {
            for (const QString &suffix : std::as_const(suffixes)) {
#ifdef Q_OS_ANDROID
                {
                    QString pluginPath = basePath + prefix + baseName + suffix;
                    const QString fn = path + "/lib"_L1 + pluginPath.replace(u'/', u'_');
                    qCDebug(qt_lcDebugPlugins) << "Trying..." << fn;
                    if (QFileInfo(fn).isFile())
                        return fn;
                }
#endif
                const QString fn = path + u'/' + basePath + prefix + baseName + suffix;
                qCDebug(qt_lcDebugPlugins) << "Trying..." << fn;
                if (QFileInfo(fn).isFile())
                    return fn;
            }
        }
    }
    qCDebug(qt_lcDebugPlugins) << fileName << "not found";
    return QString();
}
#endif

/*!
    \property QPluginLoader::fileName
    \brief the file name of the plugin

    We recommend omitting the file's suffix in the file name, since
    QPluginLoader will automatically look for the file with the appropriate
    suffix (see QLibrary::isLibrary()).

    When loading the plugin, QPluginLoader searches
    in all plugin locations specified by QCoreApplication::libraryPaths(),
    unless the file name has an absolute path. After loading the plugin
    successfully, fileName() returns the fully-qualified file name of
    the plugin, including the full path to the plugin if one was given
    in the constructor or passed to setFileName().

    If the file name does not exist, it will not be set. This property
    will then contain an empty string.

    By default, this property contains an empty string.

    \sa load()
*/
void QPluginLoader::setFileName(const QString &fileName)
{
#if defined(QT_SHARED)
    QLibrary::LoadHints lh = defaultLoadHints;
    if (d) {
        lh = d->loadHints();
        d->release();
        d = nullptr;
        did_load = false;
    }

    const QString fn = locatePlugin(fileName);

    d = QLibraryPrivate::findOrCreate(fn, QString(), lh);
    if (!fn.isEmpty())
        d->updatePluginState();

#else
    qCWarning(qt_lcDebugPlugins, "Cannot load '%ls' into a statically linked Qt library.",
              qUtf16Printable(fileName));
#endif
}

QString QPluginLoader::fileName() const
{
    if (d)
        return d->fileName;
    return QString();
}

/*!
    \since 4.2

    Returns a text string with the description of the last error that occurred.
*/
QString QPluginLoader::errorString() const
{
    return (!d || d->errorString.isEmpty()) ? tr("Unknown error") : d->errorString;
}

/*! \since 4.4

    \property QPluginLoader::loadHints
    \brief Give the load() function some hints on how it should behave.

    You can give hints on how the symbols in the plugin are
    resolved. By default since Qt 5.7, QLibrary::PreventUnloadHint is set.

    See the documentation of QLibrary::loadHints for a complete
    description of how this property works.

    \sa QLibrary::loadHints
*/

void QPluginLoader::setLoadHints(QLibrary::LoadHints loadHints)
{
    if (!d) {
        d = QLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        d->errorString.clear();
    }
    d->setLoadHints(loadHints);
}

QLibrary::LoadHints QPluginLoader::loadHints() const
{
    // Not having a d-pointer means that the user hasn't called
    // setLoadHints() / setFileName() yet. In setFileName() we will
    // then force defaultLoadHints on loading, so we must return them
    // from here as well.

    return d ? d->loadHints() : defaultLoadHints;
}

#endif // QT_CONFIG(library)

typedef QList<QStaticPlugin> StaticPluginList;
Q_GLOBAL_STATIC(StaticPluginList, staticPluginList)

/*!
    \relates QPluginLoader
    \since 5.0

    Registers the \a plugin specified with the plugin loader, and is used
    by Q_IMPORT_PLUGIN().
*/
void Q_CORE_EXPORT qRegisterStaticPluginFunction(QStaticPlugin plugin)
{
    // using operator* because we shouldn't be registering plugins while
    // unloading the application!
    StaticPluginList &plugins = *staticPluginList;

    // insert the plugin in the list, sorted by address, so we can detect
    // duplicate registrations
    auto comparator = [=](const QStaticPlugin &p1, const QStaticPlugin &p2) {
        using Less = std::less<decltype(plugin.instance)>;
        return Less{}(p1.instance, p2.instance);
    };
    auto pos = std::lower_bound(plugins.constBegin(), plugins.constEnd(), plugin, comparator);
    if (pos == plugins.constEnd() || pos->instance != plugin.instance)
        plugins.insert(pos, plugin);
}

/*!
    Returns a list of static plugin instances (root components) held
    by the plugin loader.
    \sa staticPlugins()
*/
QObjectList QPluginLoader::staticInstances()
{
    QObjectList instances;
    if (staticPluginList.exists()) {
        const StaticPluginList &plugins = *staticPluginList;
        instances.reserve(plugins.size());
        for (QStaticPlugin plugin : plugins)
            instances += plugin.instance();
    }
    return instances;
}

/*!
    Returns a list of QStaticPlugins held by the plugin
    loader. The function is similar to \l staticInstances()
    with the addition that a QStaticPlugin also contains
    meta data information.
    \sa staticInstances()
*/
QList<QStaticPlugin> QPluginLoader::staticPlugins()
{
    StaticPluginList *plugins = staticPluginList();
    if (plugins)
        return *plugins;
    return QList<QStaticPlugin>();
}

/*!
    \class QStaticPlugin
    \inmodule QtCore
    \since 5.2

    \brief QStaticPlugin is a struct containing a reference to a
    static plugin instance together with its meta data.

    \sa QPluginLoader, {How to Create Qt Plugins}
*/

/*!
    \fn QStaticPlugin::QStaticPlugin(QtPluginInstanceFunction i, QtPluginMetaDataFunction m)
    \internal
*/

/*!
    \variable QStaticPlugin::instance

    Holds the plugin instance.

    \sa QPluginLoader::staticInstances()
*/

/*!
    Returns a the meta data for the plugin as a QJsonObject.

    \sa Q_PLUGIN_METADATA()
*/
QJsonObject QStaticPlugin::metaData() const
{
    QByteArrayView data(static_cast<const char *>(rawMetaData), rawMetaDataSize);
    QPluginParsedMetaData parsed(data);
    Q_ASSERT(!parsed.isError());
    return parsed.toJson();
}

QT_END_NAMESPACE

#include "moc_qpluginloader.cpp"
