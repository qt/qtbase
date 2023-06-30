// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qlibrary.h"
#include "qlibrary_p.h"

#include <q20algorithm.h>
#include <qbytearraymatcher.h>
#include <qdebug.h>
#include <qendian.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qjsondocument.h>
#include <qmap.h>
#include <qmutex.h>
#include <qoperatingsystemversion.h>
#include <qstringlist.h>

#ifdef Q_OS_DARWIN
#  include <private/qcore_mac_p.h>
#endif
#include <private/qcoreapplication_p.h>
#include <private/qloggingregistry_p.h>
#include <private/qsystemerror_p.h>

#include "qcoffpeparser_p.h"
#include "qelfparser_p.h"
#include "qfactoryloader_p.h"
#include "qmachparser_p.h"

#include <qtcore_tracepoints_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_TRACE_POINT(qtcore, QLibraryPrivate_load_entry, const QString &fileName);
Q_TRACE_POINT(qtcore, QLibraryPrivate_load_exit, bool success);

// On Unix systema and on Windows with MinGW, we can mix and match debug and
// release plugins without problems. (unless compiled in debug-and-release mode
// - why?)
static constexpr bool PluginMustMatchQtDebug =
        QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows
#if defined(Q_CC_MINGW)
        && QT_CONFIG(debug_and_release)
#endif
        ;

#ifdef QT_NO_DEBUG
static constexpr bool QtBuildIsDebug = false;
#else
static constexpr bool QtBuildIsDebug = true;
#endif

Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(qt_lcDebugPlugins, "QT_DEBUG_PLUGINS", "qt.core.plugin.loader")
static Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(lcDebugLibrary, "QT_DEBUG_PLUGINS", "qt.core.library")

/*!
    \class QLibrary
    \inmodule QtCore
    \reentrant
    \brief The QLibrary class loads shared libraries at runtime.


    \ingroup plugins

    An instance of a QLibrary object operates on a single shared
    object file (which we call a "library", but is also known as a
    "DLL"). A QLibrary provides access to the functionality in the
    library in a platform independent way. You can either pass a file
    name in the constructor, or set it explicitly with setFileName().
    When loading the library, QLibrary searches in all the
    system-specific library locations (e.g. \c LD_LIBRARY_PATH on
    Unix), unless the file name has an absolute path.

    If the file name is an absolute path then an attempt is made to
    load this path first. If the file cannot be found, QLibrary tries
    the name with different platform-specific file prefixes, like
    "lib" on Unix and Mac, and suffixes, like ".so" on Unix, ".dylib"
    on the Mac, or ".dll" on Windows.

    If the file path is not absolute then QLibrary modifies the search
    order to try the system-specific prefixes and suffixes first,
    followed by the file path specified.

    This makes it possible to specify shared libraries that are only
    identified by their basename (i.e. without their suffix), so the
    same code will work on different operating systems yet still
    minimise the number of attempts to find the library.

    The most important functions are load() to dynamically load the
    library file, isLoaded() to check whether loading was successful,
    and resolve() to resolve a symbol in the library. The resolve()
    function implicitly tries to load the library if it has not been
    loaded yet. Multiple instances of QLibrary can be used to access
    the same physical library. Once loaded, libraries remain in memory
    until the application terminates. You can attempt to unload a
    library using unload(), but if other instances of QLibrary are
    using the same library, the call will fail, and unloading will
    only happen when every instance has called unload().

    A typical use of QLibrary is to resolve an exported symbol in a
    library, and to call the C function that this symbol represents.
    This is called "explicit linking" in contrast to "implicit
    linking", which is done by the link step in the build process when
    linking an executable against a library.

    The following code snippet loads a library, resolves the symbol
    "mysymbol", and calls the function if everything succeeded. If
    something goes wrong, e.g. the library file does not exist or the
    symbol is not defined, the function pointer will be \nullptr and
    won't be called.

    \snippet code/src_corelib_plugin_qlibrary.cpp 0

    The symbol must be exported as a C function from the library for
    resolve() to work. This means that the function must be wrapped in
    an \c{extern "C"} block if the library is compiled with a C++
    compiler. On Windows, this also requires the use of a \c dllexport
    macro; see resolve() for the details of how this is done. For
    convenience, there is a static resolve() function which you can
    use if you just want to call a function in a library without
    explicitly loading the library first:

    \snippet code/src_corelib_plugin_qlibrary.cpp 1

    \sa QPluginLoader
*/

/*!
    \enum QLibrary::LoadHint

    This enum describes the possible hints that can be used to change the way
    libraries are handled when they are loaded. These values indicate how
    symbols are resolved when libraries are loaded, and are specified using
    the setLoadHints() function.

    \value ResolveAllSymbolsHint
    Causes all symbols in a library to be resolved when it is loaded, not
    simply when resolve() is called.
    \value ExportExternalSymbolsHint
    Exports unresolved and external symbols in the library so that they can be
    resolved in other dynamically-loaded libraries loaded later.
    \value LoadArchiveMemberHint
    Allows the file name of the library to specify a particular object file
    within an archive file.
    If this hint is given, the filename of the library consists of
    a path, which is a reference to an archive file, followed by
    a reference to the archive member.
    \value PreventUnloadHint
    Prevents the library from being unloaded from the address space if close()
    is called. The library's static variables are not reinitialized if open()
    is called at a later time.
    \value DeepBindHint
    Instructs the linker to prefer definitions in the loaded library
    over exported definitions in the loading application when resolving
    external symbols in the loaded library. This option is only supported
    on Linux.

    \sa loadHints
*/

static QLibraryScanResult qt_find_pattern(const char *s, qsizetype s_len, QString *errMsg)
{
    /*
      We used to search from the end of the file so we'd skip the code and find
      the read-only data that usually follows. Unfortunately, in debug builds,
      the debug sections come after and are usually much bigger than everything
      else, making this process slower than necessary with debug plugins.

      More importantly, the pattern string may exist in the debug information due
      to it being used in the plugin in the first place.
    */
#if defined(Q_OF_ELF)
    return QElfParser::parse({s, s_len}, errMsg);
#elif defined(Q_OF_MACH_O)
    return QMachOParser::parse(s, s_len, errMsg);
#elif defined(Q_OS_WIN)
    return QCoffPeParser::parse({s, s_len}, errMsg);
#else
#   warning "Qt does not know how to efficiently parse your platform's binary format; using slow fall-back."
#endif
    static constexpr auto matcher = [] {
        // QPluginMetaData::MagicString is not NUL-terminated, but
        // qMakeStaticByteArrayMatcher requires its argument to be, so
        // duplicate here, but statically check we didn't mess up:
        constexpr auto &pattern = "QTMETADATA !";
        constexpr auto magic = std::string_view(QPluginMetaData::MagicString,
                                                sizeof(QPluginMetaData::MagicString));
        static_assert(pattern == magic);
        return qMakeStaticByteArrayMatcher(pattern);
    }();
    qsizetype i = matcher.indexIn({s, s_len});
    if (i < 0) {
        *errMsg = QLibrary::tr("'%1' is not a Qt plugin").arg(*errMsg);
        return QLibraryScanResult{};
    }
    i += sizeof(QPluginMetaData::MagicString);
    return { i, s_len - i };
}

/*
  This opens the specified library, mmaps it into memory, and searches
  for the QT_PLUGIN_VERIFICATION_DATA.  The advantage of this approach is that
  we can get the verification data without have to actually load the library.
  This lets us detect mismatches more safely.

  Returns \c false if version information is not present, or if the
                information could not be read.
  Returns  true if version information is present and successfully read.
*/
static QLibraryScanResult findPatternUnloaded(const QString &library, QLibraryPrivate *lib)
{
    QFile file(library);
    if (!file.open(QIODevice::ReadOnly)) {
        if (lib)
            lib->errorString = file.errorString();
        qCWarning(qt_lcDebugPlugins, "%ls: cannot open: %ls", qUtf16Printable(library),
                  qUtf16Printable(file.errorString()));
        return {};
    }

    // Files can be bigger than the virtual memory size on 32-bit systems, so
    // we limit to 512 MB there. For 64-bit, we allow up to 2^40 bytes.
    constexpr qint64 MaxMemoryMapSize =
            Q_INT64_C(1) << (sizeof(qsizetype) > 4 ? 40 : 29);

    qsizetype fdlen = qMin(file.size(), MaxMemoryMapSize);
    const char *filedata = reinterpret_cast<char *>(file.map(0, fdlen));

#ifdef Q_OS_UNIX
    if (filedata == nullptr) {
        // If we can't mmap(), then the dynamic loader won't be able to either.
        // This can't be used as a plugin.
        qCWarning(qt_lcDebugPlugins, "%ls: failed to map to memory: %ls",
                  qUtf16Printable(library), qUtf16Printable(file.errorString()));
        return {};
    }
#else
    QByteArray data;
    if (filedata == nullptr) {
        // It's unknown at this point whether Windows supports LoadLibrary() on
        // files that fail to CreateFileMapping / MapViewOfFile, so we err on
        // the side of doing a regular read into memory (up to 64 MB).
        data = file.read(64 * 1024 * 1024);
        filedata = data.constData();
        fdlen = data.size();
    }
#endif

    QString errMsg = library;
    QLibraryScanResult r = qt_find_pattern(filedata, fdlen, &errMsg);
    if (r.length) {
#if defined(Q_OF_MACH_O)
        if (r.isEncrypted)
            return r;
#endif
        if (!lib->metaData.parse(QByteArrayView(filedata + r.pos, r.length))) {
            errMsg = lib->metaData.errorString();
            qCDebug(qt_lcDebugPlugins, "Found invalid metadata in lib %ls: %ls",
                      qUtf16Printable(library), qUtf16Printable(errMsg));
        } else {
            qCDebug(qt_lcDebugPlugins, "Found metadata in lib %ls, metadata=\n%s\n",
                    qUtf16Printable(library),
                    QJsonDocument(lib->metaData.toJson()).toJson().constData());
            return r;
        }
    } else {
        qCDebug(qt_lcDebugPlugins, "Failed to find metadata in lib %ls: %ls",
                qUtf16Printable(library), qUtf16Printable(errMsg));
    }

    lib->errorString = QLibrary::tr("Failed to extract plugin meta data from '%1': %2")
            .arg(library, errMsg);
    return {};
}

static void installCoverageTool(QLibraryPrivate *libPrivate)
{
#ifdef __COVERAGESCANNER__
    /*
      __COVERAGESCANNER__ is defined when Qt has been instrumented for code
      coverage by TestCocoon. CoverageScanner is the name of the tool that
      generates the code instrumentation.
      This code is required here when code coverage analysis with TestCocoon
      is enabled in order to allow the loading application to register the plugin
      and then store its execution report. The execution report gathers information
      about each part of the plugin's code that has been used when
      the plugin was loaded by the launching application.
      The execution report for the plugin will go to the same execution report
      as the one defined for the application loading it.
    */

    int ret = __coveragescanner_register_library(libPrivate->fileName.toLocal8Bit());

    if (ret >= 0) {
            qDebug("coverage data for %ls registered",
                   qUtf16Printable(libPrivate->fileName));
        } else {
            qWarning("could not register %ls: error %d; coverage data may be incomplete",
                     qUtf16Printable(libPrivate->fileName),
                     ret);
        }
    }
#else
    Q_UNUSED(libPrivate);
#endif
}

class QLibraryStore
{
public:
    inline ~QLibraryStore();
    static inline QLibraryPrivate *findOrCreate(const QString &fileName, const QString &version, QLibrary::LoadHints loadHints);
    static inline void releaseLibrary(QLibraryPrivate *lib);

    static inline void cleanup();

private:
    static inline QLibraryStore *instance();

    // all members and instance() are protected by qt_library_mutex
    typedef QMap<QString, QLibraryPrivate *> LibraryMap;
    LibraryMap libraryMap;
};

Q_CONSTINIT static QBasicMutex qt_library_mutex;
Q_CONSTINIT static QLibraryStore *qt_library_data = nullptr;
Q_CONSTINIT static bool qt_library_data_once;

QLibraryStore::~QLibraryStore()
{
    qt_library_data = nullptr;
}

inline void QLibraryStore::cleanup()
{
    QLibraryStore *data = qt_library_data;
    if (!data)
        return;

    // find any libraries that are still loaded but have a no one attached to them
    LibraryMap::Iterator it = data->libraryMap.begin();
    for (; it != data->libraryMap.end(); ++it) {
        QLibraryPrivate *lib = it.value();
        if (lib->libraryRefCount.loadRelaxed() == 1) {
            if (lib->libraryUnloadCount.loadRelaxed() > 0) {
                Q_ASSERT(lib->pHnd.loadRelaxed());
                lib->libraryUnloadCount.storeRelaxed(1);
#if defined(Q_OS_DARWIN)
                // We cannot fully unload libraries, as we don't know if there are
                // lingering references (in system threads e.g.) to Objective-C classes
                // defined in the library.
                lib->unload(QLibraryPrivate::NoUnloadSys);
#else
                lib->unload();
#endif
            }
            delete lib;
            it.value() = nullptr;
        }
    }

    // dump all objects that remain
    if (lcDebugLibrary().isDebugEnabled()) {
        for (QLibraryPrivate *lib : std::as_const(data->libraryMap)) {
            if (lib)
                qDebug(lcDebugLibrary)
                        << "On QtCore unload," << lib->fileName << "was leaked, with"
                        << lib->libraryRefCount.loadRelaxed() << "users";
        }
    }

    delete data;
}

static void qlibraryCleanup()
{
    QLibraryStore::cleanup();
}
Q_DESTRUCTOR_FUNCTION(qlibraryCleanup)

// must be called with a locked mutex
QLibraryStore *QLibraryStore::instance()
{
    if (Q_UNLIKELY(!qt_library_data_once && !qt_library_data)) {
        // only create once per process lifetime
        qt_library_data = new QLibraryStore;
        qt_library_data_once = true;
    }
    return qt_library_data;
}

inline QLibraryPrivate *QLibraryStore::findOrCreate(const QString &fileName, const QString &version,
                                                    QLibrary::LoadHints loadHints)
{
    QMutexLocker locker(&qt_library_mutex);
    QLibraryStore *data = instance();

    QString mapName = version.isEmpty() ? fileName : fileName + u'\0' + version;

    // check if this library is already loaded
    QLibraryPrivate *lib = nullptr;
    if (Q_LIKELY(data)) {
        lib = data->libraryMap.value(mapName);
        if (lib)
            lib->mergeLoadHints(loadHints);
    }
    if (!lib)
        lib = new QLibraryPrivate(fileName, version, loadHints);

    // track this library
    if (Q_LIKELY(data) && !fileName.isEmpty())
        data->libraryMap.insert(mapName, lib);

    lib->libraryRefCount.ref();
    return lib;
}

inline void QLibraryStore::releaseLibrary(QLibraryPrivate *lib)
{
    QMutexLocker locker(&qt_library_mutex);
    QLibraryStore *data = instance();

    if (lib->libraryRefCount.deref()) {
        // still in use
        return;
    }

    // no one else is using
    Q_ASSERT(lib->libraryUnloadCount.loadRelaxed() == 0);

    if (Q_LIKELY(data) && !lib->fileName.isEmpty()) {
        qsizetype n = erase_if(data->libraryMap, [lib](LibraryMap::iterator it) {
            return it.value() == lib;
        });
        Q_ASSERT_X(n, "~QLibrary", "Did not find this library in the library map");
        Q_UNUSED(n);
    }
    delete lib;
}

QLibraryPrivate::QLibraryPrivate(const QString &canonicalFileName, const QString &version, QLibrary::LoadHints loadHints)
    : fileName(canonicalFileName), fullVersion(version), pluginState(MightBeAPlugin)
{
    loadHintsInt.storeRelaxed(loadHints.toInt());
    if (canonicalFileName.isEmpty())
        errorString = QLibrary::tr("The shared library was not found.");
}

QLibraryPrivate *QLibraryPrivate::findOrCreate(const QString &fileName, const QString &version,
                                               QLibrary::LoadHints loadHints)
{
    return QLibraryStore::findOrCreate(fileName, version, loadHints);
}

QLibraryPrivate::~QLibraryPrivate()
{
}

void QLibraryPrivate::mergeLoadHints(QLibrary::LoadHints lh)
{
    // if the library is already loaded, we can't change the load hints
    if (pHnd.loadRelaxed())
        return;

    loadHintsInt.storeRelaxed(lh.toInt());
}

QFunctionPointer QLibraryPrivate::resolve(const char *symbol)
{
    if (!pHnd.loadRelaxed())
        return nullptr;
    return resolve_sys(symbol);
}

void QLibraryPrivate::setLoadHints(QLibrary::LoadHints lh)
{
    // this locks a global mutex
    QMutexLocker lock(&qt_library_mutex);
    mergeLoadHints(lh);
}

QObject *QLibraryPrivate::pluginInstance()
{
    // first, check if the instance is cached and hasn't been deleted
    QObject *obj = [&](){ QMutexLocker locker(&mutex); return inst.data(); }();
    if (obj)
        return obj;

    // We need to call the plugin's factory function. Is that cached?
    // skip increasing the reference count (why? -Thiago)
    QtPluginInstanceFunction factory = instanceFactory.loadAcquire();
    if (!factory)
        factory = loadPlugin();

    if (!factory)
        return nullptr;

    obj = factory();

    // cache again
    QMutexLocker locker(&mutex);
    if (inst)
        obj = inst;
    else
        inst = obj;
    return obj;
}

bool QLibraryPrivate::load()
{
    if (pHnd.loadRelaxed()) {
        libraryUnloadCount.ref();
        return true;
    }
    if (fileName.isEmpty())
        return false;

    Q_TRACE(QLibraryPrivate_load_entry, fileName);

    bool ret = load_sys();
    qCDebug(lcDebugLibrary)
            << fileName
            << (ret ? "loaded library" : qUtf8Printable(u"cannot load: " + errorString));
    if (ret) {
        //when loading a library we add a reference to it so that the QLibraryPrivate won't get deleted
        //this allows to unload the library at a later time
        libraryUnloadCount.ref();
        libraryRefCount.ref();
        installCoverageTool(this);
    }

    Q_TRACE(QLibraryPrivate_load_exit, ret);

    return ret;
}

bool QLibraryPrivate::unload(UnloadFlag flag)
{
    if (!pHnd.loadRelaxed())
        return false;
    if (libraryUnloadCount.loadRelaxed() > 0 && !libraryUnloadCount.deref()) { // only unload if ALL QLibrary instance wanted to
        QMutexLocker locker(&mutex);
        delete inst.data();
        if (flag == NoUnloadSys || unload_sys()) {
            qCDebug(lcDebugLibrary) << fileName << "unloaded library"
                                    << (flag == NoUnloadSys ? "(faked)" : "");
            // when the library is unloaded, we release the reference on it so that 'this'
            // can get deleted
            libraryRefCount.deref();
            pHnd.storeRelaxed(nullptr);
            instanceFactory.storeRelaxed(nullptr);
            return true;
        }
    }

    return false;
}

void QLibraryPrivate::release()
{
    QLibraryStore::releaseLibrary(this);
}

QtPluginInstanceFunction QLibraryPrivate::loadPlugin()
{
    if (auto ptr = instanceFactory.loadAcquire()) {
        libraryUnloadCount.ref();
        return ptr;
    }
    if (pluginState == IsNotAPlugin)
        return nullptr;
    if (load()) {
        auto ptr = reinterpret_cast<QtPluginInstanceFunction>(resolve("qt_plugin_instance"));
        instanceFactory.storeRelease(ptr); // two threads may store the same value
        return ptr;
    }
    qCDebug(qt_lcDebugPlugins) << "QLibraryPrivate::loadPlugin failed on" << fileName << ":" << errorString;
    pluginState = IsNotAPlugin;
    return nullptr;
}

/*!
    Returns \c true if \a fileName has a valid suffix for a loadable
    library; otherwise returns \c false.

    \table
    \header \li Platform \li Valid suffixes
    \row \li Windows     \li \c .dll, \c .DLL
    \row \li Unix/Linux  \li \c .so
    \row \li AIX  \li \c .a
    \row \li HP-UX       \li \c .sl, \c .so (HP-UXi)
    \row \li \macos and iOS   \li \c .dylib, \c .bundle, \c .so
    \endtable

    Trailing versioning numbers on Unix are ignored.
 */
bool QLibrary::isLibrary(const QString &fileName)
{
#if defined(Q_OS_WIN)
    return fileName.endsWith(".dll"_L1, Qt::CaseInsensitive);
#else // Generic Unix
# if defined(Q_OS_DARWIN)
    // On Apple platforms, dylib look like libmylib.1.0.0.dylib
    if (fileName.endsWith(".dylib"_L1))
        return true;
# endif
    QString completeSuffix = QFileInfo(fileName).completeSuffix();
    if (completeSuffix.isEmpty())
        return false;

    // if this throws an empty-array error, you need to fix the #ifdef's:
    const QLatin1StringView candidates[] = {
# if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
*/
        "sl"_L1,
#  if defined __ia64
        "so"_L1,
#  endif
# elif defined(Q_OS_AIX)
        "a"_L1,
        "so"_L1,
# elif defined(Q_OS_DARWIN)
        "so"_L1,
        "bundle"_L1,
# elif defined(Q_OS_UNIX)
        "so"_L1,
# endif
    }; // candidates

    auto isValidSuffix = [&candidates](QStringView s) {
        return std::find(std::begin(candidates), std::end(candidates), s) != std::end(candidates);
    };

    // Examples of valid library names:
    //  libfoo.so
    //  libfoo.so.0
    //  libfoo.so.0.3
    //  libfoo-0.3.so
    //  libfoo-0.3.so.0.3.0

    auto suffixes = qTokenize(completeSuffix, u'.');
    auto it = suffixes.begin();
    const auto end = suffixes.end();

    auto isNumeric = [](QStringView s) { bool ok; (void)s.toInt(&ok); return ok; };

    while (it != end) {
        if (isValidSuffix(*it++))
            return q20::ranges::all_of(it, end, isNumeric);
    }
    return false; // no valid suffix found
#endif
}

static bool qt_get_metadata(QLibraryPrivate *priv, QString *errMsg)
{
    auto error = [=](QString &&explanation) {
        *errMsg = QLibrary::tr("'%1' is not a Qt plugin (%2)").arg(priv->fileName, std::move(explanation));
        return false;
    };

    QPluginMetaData metaData;
    QFunctionPointer pfn = priv->resolve("qt_plugin_query_metadata_v2");
    if (pfn) {
        metaData = reinterpret_cast<QPluginMetaData (*)()>(pfn)();
#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0)
    } else if ((pfn = priv->resolve("qt_plugin_query_metadata"))) {
        metaData = reinterpret_cast<QPluginMetaData (*)()>(pfn)();
        if (metaData.size < sizeof(QPluginMetaData::MagicHeader))
            return error(QLibrary::tr("metadata too small"));

        // adjust the meta data to point to the header
        auto data = reinterpret_cast<const char *>(metaData.data);
        data += sizeof(QPluginMetaData::MagicString);
        metaData.data = data;
        metaData.size -= sizeof(QPluginMetaData::MagicString);
#endif
    } else {
        return error(QLibrary::tr("entrypoint to query the plugin meta data not found"));
    }

    if (metaData.size < sizeof(QPluginMetaData::Header))
        return error(QLibrary::tr("metadata too small"));

    if (priv->metaData.parse(metaData))
        return true;
    *errMsg = priv->metaData.errorString();
    return false;
}

bool QLibraryPrivate::isPlugin()
{
    if (pluginState == MightBeAPlugin)
        updatePluginState();

    return pluginState == IsAPlugin;
}

void QLibraryPrivate::updatePluginState()
{
    QMutexLocker locker(&mutex);
    errorString.clear();
    if (pluginState != MightBeAPlugin)
        return;

    bool success = false;

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    if (fileName.endsWith(".debug"_L1)) {
        // refuse to load a file that ends in .debug
        // these are the debug symbols from the libraries
        // the problem is that they are valid shared library files
        // and dlopen is known to crash while opening them

        // pretend we didn't see the file
        errorString = QLibrary::tr("The shared library was not found.");
        pluginState = IsNotAPlugin;
        return;
    }
#endif

    if (!pHnd.loadRelaxed()) {
        // scan for the plugin metadata without loading
        QLibraryScanResult result = findPatternUnloaded(fileName, this);
#if defined(Q_OF_MACH_O)
        if (result.length && result.isEncrypted) {
            // We found the .qtmetadata section, but since the library is encrypted
            // we need to dlopen() it before we can parse the metadata for further
            // validation.
            qCDebug(qt_lcDebugPlugins, "Library is encrypted. Doing prospective load before parsing metadata");
            locker.unlock();
            load();
            locker.relock();
            success = qt_get_metadata(this, &errorString);
        } else
#endif
        {
            success = result.length != 0;
        }
    } else {
        // library is already loaded (probably via QLibrary)
        // simply get the target function and call it.
        success = qt_get_metadata(this, &errorString);
    }

    if (!success) {
        if (errorString.isEmpty()) {
            if (fileName.isEmpty())
                errorString = QLibrary::tr("The shared library was not found.");
            else
                errorString = QLibrary::tr("The file '%1' is not a valid Qt plugin.").arg(fileName);
        }
        pluginState = IsNotAPlugin;
        return;
    }

    pluginState = IsNotAPlugin; // be pessimistic

    uint qt_version = uint(metaData.value(QtPluginMetaDataKeys::QtVersion).toInteger());
    bool debug = metaData.value(QtPluginMetaDataKeys::IsDebug).toBool();
    if ((qt_version & 0x00ff00) > (QT_VERSION & 0x00ff00) || (qt_version & 0xff0000) != (QT_VERSION & 0xff0000)) {
        qCDebug(qt_lcDebugPlugins, "In %s:\n"
                 "  Plugin uses incompatible Qt library (%d.%d.%d) [%s]",
                 QFile::encodeName(fileName).constData(),
                 (qt_version&0xff0000) >> 16, (qt_version&0xff00) >> 8, qt_version&0xff,
                 debug ? "debug" : "release");
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library. (%2.%3.%4) [%5]")
            .arg(fileName,
                 QString::number((qt_version & 0xff0000) >> 16),
                 QString::number((qt_version & 0xff00) >> 8),
                 QString::number(qt_version & 0xff),
                 debug ? "debug"_L1 : "release"_L1);
    } else if (PluginMustMatchQtDebug && debug != QtBuildIsDebug) {
        //don't issue a qWarning since we will hopefully find a non-debug? --Sam
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library."
                 " (Cannot mix debug and release libraries.)").arg(fileName);
    } else {
        pluginState = IsAPlugin;
    }
}

/*!
    Loads the library and returns \c true if the library was loaded
    successfully; otherwise returns \c false. Since resolve() always
    calls this function before resolving any symbols it is not
    necessary to call it explicitly. In some situations you might want
    the library loaded in advance, in which case you would use this
    function.

    \sa unload()
*/
bool QLibrary::load()
{
    if (!d)
        return false;
    if (d.tag() == Loaded)
        return d->pHnd.loadRelaxed();
    if (d->load()) {
        d.setTag(Loaded);
        return true;
    }
    return false;
}

/*!
    Unloads the library and returns \c true if the library could be
    unloaded; otherwise returns \c false.

    This happens automatically on application termination, so you
    shouldn't normally need to call this function.

    If other instances of QLibrary are using the same library, the
    call will fail, and unloading will only happen when every instance
    has called unload().

    Note that on \macos, dynamic libraries cannot be unloaded.
    QLibrary::unload() will return \c true, but the library will remain
    loaded into the process.

    \sa resolve(), load()
*/
bool QLibrary::unload()
{
    if (d.tag() == Loaded) {
        d.setTag(NotLoaded);
        return d->unload();
    }
    return false;
}

/*!
    Returns \c true if load() succeeded; otherwise returns \c false.

    \note Prior to Qt 6.6, this function would return \c true even without a
    call to load() if another QLibrary object on the same library had caused it
    to be loaded.

    \sa load()
 */
bool QLibrary::isLoaded() const
{
    return d.tag() == Loaded;
}


/*!
    Constructs a library with the given \a parent.
 */
QLibrary::QLibrary(QObject *parent) : QObject(parent)
{
}


/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on \macos and iOS, and ".dll" on Windows. (See \l{fileName}.)
 */
QLibrary::QLibrary(const QString &fileName, QObject *parent) : QObject(parent)
{
    setFileName(fileName);
}

/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName and major version number \a verNum.
    Currently, the version number is ignored on Windows.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on \macos and iOS, and ".dll" on Windows. (See \l{fileName}.)
*/
QLibrary::QLibrary(const QString &fileName, int verNum, QObject *parent) : QObject(parent)
{
    setFileNameAndVersion(fileName, verNum);
}

/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName and full version number \a version.
    Currently, the version number is ignored on Windows.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on \macos and iOS, and ".dll" on Windows. (See \l{fileName}.)
 */
QLibrary::QLibrary(const QString &fileName, const QString &version, QObject *parent)
    : QObject(parent)
{
    setFileNameAndVersion(fileName, version);
}

/*!
    Destroys the QLibrary object.

    Unless unload() was called explicitly, the library stays in memory
    until the application terminates.

    \sa isLoaded(), unload()
*/
QLibrary::~QLibrary()
{
    if (d)
        d->release();
}

/*!
    \property QLibrary::fileName
    \brief the file name of the library

    We recommend omitting the file's suffix in the file name, since
    QLibrary will automatically look for the file with the appropriate
    suffix (see isLibrary()).

    When loading the library, QLibrary searches in all system-specific
    library locations (for example, \c LD_LIBRARY_PATH on Unix), unless the
    file name has an absolute path. After loading the library
    successfully, fileName() returns the fully-qualified file name of
    the library, including the full path to the library if one was given
    in the constructor or passed to setFileName().

    For example, after successfully loading the "GL" library on Unix
    platforms, fileName() will return "libGL.so". If the file name was
    originally passed as "/usr/lib/libGL", fileName() will return
    "/usr/lib/libGL.so".
*/

void QLibrary::setFileName(const QString &fileName)
{
    setFileNameAndVersion(fileName, QString());
}

QString QLibrary::fileName() const
{
    if (d) {
        QMutexLocker locker(&d->mutex);
        return d->qualifiedFileName.isEmpty() ? d->fileName : d->qualifiedFileName;
    }
    return QString();
}

/*!
    \fn void QLibrary::setFileNameAndVersion(const QString &fileName, int versionNumber)

    Sets the fileName property and major version number to \a fileName
    and \a versionNumber respectively.
    The \a versionNumber is ignored on Windows.

    \sa setFileName()
*/
void QLibrary::setFileNameAndVersion(const QString &fileName, int verNum)
{
    setFileNameAndVersion(fileName, verNum >= 0 ? QString::number(verNum) : QString());
}

/*!
    \since 4.4

    Sets the fileName property and full version number to \a fileName
    and \a version respectively.
    The \a version parameter is ignored on Windows.

    \sa setFileName()
*/
void QLibrary::setFileNameAndVersion(const QString &fileName, const QString &version)
{
    QLibrary::LoadHints lh;
    if (d) {
        lh = d->loadHints();
        d->release();
    }
    QLibraryPrivate *dd = QLibraryPrivate::findOrCreate(fileName, version, lh);
    d = QTaggedPointer(dd, NotLoaded);      // we haven't load()ed
}

/*!
    Returns the address of the exported symbol \a symbol. The library is
    loaded if necessary. The function returns \nullptr if the symbol could
    not be resolved or if the library could not be loaded.

    Example:
    \snippet code/src_corelib_plugin_qlibrary.cpp 2

    The symbol must be exported as a C function from the library. This
    means that the function must be wrapped in an \c{extern "C"} if
    the library is compiled with a C++ compiler. On Windows you must
    also explicitly export the function from the DLL using the
    \c{__declspec(dllexport)} compiler directive, for example:

    \snippet code/src_corelib_plugin_qlibrary.cpp 3

    with \c MY_EXPORT defined as

    \snippet code/src_corelib_plugin_qlibrary.cpp 4
*/
QFunctionPointer QLibrary::resolve(const char *symbol)
{
    if (!isLoaded() && !load())
        return nullptr;
    return d->resolve(symbol);
}

/*!
    \overload

    Loads the library \a fileName and returns the address of the
    exported symbol \a symbol. Note that \a fileName should not
    include the platform-specific file suffix; (see \l{fileName}). The
    library remains loaded until the application exits.

    The function returns \nullptr if the symbol could not be resolved or if
    the library could not be loaded.

    \sa resolve()
*/
QFunctionPointer QLibrary::resolve(const QString &fileName, const char *symbol)
{
    QLibrary library(fileName);
    return library.resolve(symbol);
}

/*!
    \overload

    Loads the library \a fileName with major version number \a verNum and
    returns the address of the exported symbol \a symbol.
    Note that \a fileName should not include the platform-specific file suffix;
    (see \l{fileName}). The library remains loaded until the application exits.
    \a verNum is ignored on Windows.

    The function returns \nullptr if the symbol could not be resolved or if
    the library could not be loaded.

    \sa resolve()
*/
QFunctionPointer QLibrary::resolve(const QString &fileName, int verNum, const char *symbol)
{
    QLibrary library(fileName, verNum);
    return library.resolve(symbol);
}

/*!
    \overload
    \since 4.4

    Loads the library \a fileName with full version number \a version and
    returns the address of the exported symbol \a symbol.
    Note that \a fileName should not include the platform-specific file suffix;
    (see \l{fileName}). The library remains loaded until the application exits.
    \a version is ignored on Windows.

    The function returns \nullptr if the symbol could not be resolved or if
    the library could not be loaded.

    \sa resolve()
*/
QFunctionPointer QLibrary::resolve(const QString &fileName, const QString &version, const char *symbol)
{
    QLibrary library(fileName, version);
    return library.resolve(symbol);
}

/*!
    \since 4.2

    Returns a text string with the description of the last error that occurred.
    Currently, errorString will only be set if load(), unload() or resolve() for some reason fails.
*/
QString QLibrary::errorString() const
{
    QString str;
    if (d) {
        QMutexLocker locker(&d->mutex);
        str = d->errorString;
    }
    return str.isEmpty() ? tr("Unknown error") : str;
}

/*!
    \property QLibrary::loadHints
    \brief Give the load() function some hints on how it should behave.

    You can give some hints on how the symbols are resolved. Usually,
    the symbols are not resolved at load time, but resolved lazily,
    (that is, when resolve() is called). If you set the loadHints to
    ResolveAllSymbolsHint, then all symbols will be resolved at load time
    if the platform supports it.

    Setting ExportExternalSymbolsHint will make the external symbols in the
    library available for resolution in subsequent loaded libraries.

    If LoadArchiveMemberHint is set, the file name
    is composed of two components: A path which is a reference to an
    archive file followed by the second component which is the reference to
    the archive member. For instance, the fileName \c libGL.a(shr_64.o) will refer
    to the library \c shr_64.o in the archive file named \c libGL.a. This
    is only supported on the AIX platform.

    The interpretation of the load hints is platform dependent, and if
    you use it you are probably making some assumptions on which platform
    you are compiling for, so use them only if you understand the consequences
    of them.

    By default, none of these flags are set, so libraries will be loaded with
    lazy symbol resolution, and will not export external symbols for resolution
    in other dynamically-loaded libraries.

    \note Setting this property after the library has been loaded has no effect
    and loadHints() will not reflect those changes.

    \note This property is shared among all QLibrary instances that refer to
    the same library.
*/
void QLibrary::setLoadHints(LoadHints hints)
{
    if (!d) {
        d = QLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        d->errorString.clear();
    }
    d->setLoadHints(hints);
}

QLibrary::LoadHints QLibrary::loadHints() const
{
    return d ? d->loadHints() : QLibrary::LoadHints();
}

/* Internal, for debugging */
bool qt_debug_component()
{
    static int debug_env = QT_PREPEND_NAMESPACE(qEnvironmentVariableIntValue)("QT_DEBUG_PLUGINS");
    return debug_env != 0;
}

QT_END_NAMESPACE

#include "moc_qlibrary.cpp"
