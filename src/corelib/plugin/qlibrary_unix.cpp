// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"

#include <qcoreapplication.h>
#include <qfile.h>
#include "qlibrary_p.h"
#include <private/qfilesystementry_p.h>
#include <private/qsimd_p.h>

#include <dlfcn.h>

#ifdef Q_OS_DARWIN
#  include <private/qcore_mac_p.h>
#endif

#ifdef Q_OS_ANDROID
#include <private/qjnihelpers_p.h>
#include <QtCore/qjnienvironment.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString qdlerror()
{
    const char *err = dlerror();
    return err ? u'(' + QString::fromLocal8Bit(err) + u')' : QString();
}

QStringList QLibraryPrivate::suffixes_sys(const QString &fullVersion)
{
    QStringList suffixes;
#if defined(Q_OS_HPUX)
    // according to
    // http://docs.hp.com/en/B2355-90968/linkerdifferencesiapa.htm

    // In PA-RISC (PA-32 and PA-64) shared libraries are suffixed
    // with .sl. In IPF (32-bit and 64-bit), the shared libraries
    // are suffixed with .so. For compatibility, the IPF linker
    // also supports the .sl suffix.

    // But since we don't know if we are built on HPUX or HPUXi,
    // we support both .sl (and .<version>) and .so suffixes but
    // .so is preferred.
# if defined(__ia64)
    if (!fullVersion.isEmpty()) {
        suffixes << ".so.%1"_L1.arg(fullVersion);
    } else {
        suffixes << ".so"_L1;
    }
# endif
    if (!fullVersion.isEmpty()) {
        suffixes << ".sl.%1"_L1.arg(fullVersion);
        suffixes << ".%1"_L1.arg(fullVersion);
    } else {
        suffixes << ".sl"_L1;
    }
#elif defined(Q_OS_AIX)
    suffixes << ".a";

#else
    if (!fullVersion.isEmpty()) {
        suffixes << ".so.%1"_L1.arg(fullVersion);
    } else {
        suffixes << ".so"_L1;
# ifdef Q_OS_ANDROID
        suffixes << QStringLiteral(LIBS_SUFFIX);
# endif
    }
#endif
# ifdef Q_OS_DARWIN
    if (!fullVersion.isEmpty()) {
        suffixes << ".%1.bundle"_L1.arg(fullVersion);
        suffixes << ".%1.dylib"_L1.arg(fullVersion);
    } else {
        suffixes << ".bundle"_L1 << ".dylib"_L1;
    }
#endif
    return suffixes;
}

QStringList QLibraryPrivate::prefixes_sys()
{
    return QStringList() << "lib"_L1;
}

bool QLibraryPrivate::load_sys()
{
#if defined(Q_OS_WASM) && defined(QT_STATIC)
    // emscripten does not support dlopen when using static linking
    return false;
#endif

    QMutexLocker locker(&mutex);
    QString attempt;
    QFileSystemEntry fsEntry(fileName);

    QString path = fsEntry.path();
    QString name = fsEntry.fileName();
    if (path == "."_L1 && !fileName.startsWith(path))
        path.clear();
    else
        path += u'/';

    QStringList suffixes;
    QStringList prefixes;
    if (pluginState != IsAPlugin) {
        prefixes = prefixes_sys();
        suffixes = suffixes_sys(fullVersion);
    }
    int dlFlags = 0;
    auto loadHints = this->loadHints();
    if (loadHints & QLibrary::ResolveAllSymbolsHint) {
        dlFlags |= RTLD_NOW;
    } else {
        dlFlags |= RTLD_LAZY;
    }
    if (loadHints & QLibrary::ExportExternalSymbolsHint) {
        dlFlags |= RTLD_GLOBAL;
    }
#if !defined(Q_OS_CYGWIN)
    else {
        dlFlags |= RTLD_LOCAL;
    }
#endif
#if defined(RTLD_DEEPBIND)
    if (loadHints & QLibrary::DeepBindHint)
        dlFlags |= RTLD_DEEPBIND;
#endif

    // Provide access to RTLD_NODELETE flag on Unix
    // From GNU documentation on RTLD_NODELETE:
    // Do not unload the library during dlclose(). Consequently, the
    // library's specific static variables are not reinitialized if the
    // library is reloaded with dlopen() at a later time.
#if defined(RTLD_NODELETE)
    if (loadHints & QLibrary::PreventUnloadHint) {
#   ifdef Q_OS_ANDROID // RTLD_NODELETE flag is supported by Android 23+
        if (QtAndroidPrivate::androidSdkVersion() > 22)
#   endif
            dlFlags |= RTLD_NODELETE;
    }
#endif

#if defined(Q_OS_AIX)   // Not sure if any other platform actually support this thing.
    if (loadHints & QLibrary::LoadArchiveMemberHint) {
        dlFlags |= RTLD_MEMBER;
    }
#endif

    // If the filename is an absolute path then we want to try that first as it is most likely
    // what the callee wants. If we have been given a non-absolute path then lets try the
    // native library name first to avoid unnecessary calls to dlopen().
    if (fsEntry.isAbsolute()) {
        suffixes.prepend(QString());
        prefixes.prepend(QString());
    } else {
        suffixes.append(QString());
        prefixes.append(QString());
    }

#if defined(Q_PROCESSOR_X86) && !defined(Q_OS_DARWIN)
    if (qCpuHasFeature(ArchHaswell)) {
        auto transform = [](QStringList &list, void (*f)(QString *)) {
            QStringList tmp;
            qSwap(tmp, list);
            list.reserve(tmp.size() * 2);
            for (const QString &s : std::as_const(tmp)) {
                QString modifiedPath = s;
                f(&modifiedPath);
                list.append(modifiedPath);
                list.append(s);
            }
        };
        if (pluginState == IsAPlugin) {
            // add ".avx2" to each suffix in the list
            transform(suffixes, [](QString *s) { s->append(".avx2"_L1); });
        } else {
            // prepend "haswell/" to each prefix in the list
            transform(prefixes, [](QString *s) { s->prepend("haswell/"_L1); });
        }
    }
#endif

    locker.unlock();
    bool retry = true;
    Handle hnd = nullptr;
    for (int prefix = 0; retry && !hnd && prefix < prefixes.size(); prefix++) {
        for (int suffix = 0; retry && !hnd && suffix < suffixes.size(); suffix++) {
            if (!prefixes.at(prefix).isEmpty() && name.startsWith(prefixes.at(prefix)))
                continue;
            if (path.isEmpty() && prefixes.at(prefix).contains(u'/'))
                continue;
            if (!suffixes.at(suffix).isEmpty() && name.endsWith(suffixes.at(suffix)))
                continue;
            if (loadHints & QLibrary::LoadArchiveMemberHint) {
                attempt = name;
                qsizetype lparen = attempt.indexOf(u'(');
                if (lparen == -1)
                    lparen = attempt.size();
                attempt = path + prefixes.at(prefix) + attempt.insert(lparen, suffixes.at(suffix));
            } else {
                attempt = path + prefixes.at(prefix) + name + suffixes.at(suffix);
            }

            hnd = dlopen(QFile::encodeName(attempt), dlFlags);
#ifdef Q_OS_ANDROID
            if (!hnd) {
                auto attemptFromBundle = attempt;
                hnd = dlopen(QFile::encodeName(attemptFromBundle.replace(u'/', u'_')), dlFlags);
            }
            if (hnd) {
                using JniOnLoadPtr = jint (*)(JavaVM *vm, void *reserved);
                JniOnLoadPtr jniOnLoad = reinterpret_cast<JniOnLoadPtr>(dlsym(hnd, "JNI_OnLoad"));
                if (jniOnLoad && jniOnLoad(QJniEnvironment::javaVM(), nullptr) == JNI_ERR) {
                    dlclose(hnd);
                    hnd = nullptr;
                }
            }
#endif

            if (!hnd && fileName.startsWith(u'/') && QFile::exists(attempt)) {
                // We only want to continue if dlopen failed due to that the shared library did not exist.
                // However, we are only able to apply this check for absolute filenames (since they are
                // not influenced by the content of LD_LIBRARY_PATH, /etc/ld.so.cache, DT_RPATH etc...)
                // This is all because dlerror is flawed and cannot tell us the reason why it failed.
                retry = false;
            }
        }
    }

#ifdef Q_OS_DARWIN
    if (!hnd) {
        QByteArray utf8Bundle = fileName.toUtf8();
        QCFType<CFURLRef> bundleUrl = CFURLCreateFromFileSystemRepresentation(NULL, reinterpret_cast<const UInt8*>(utf8Bundle.data()), utf8Bundle.length(), true);
        QCFType<CFBundleRef> bundle = CFBundleCreate(NULL, bundleUrl);
        if (bundle) {
            QCFType<CFURLRef> url = CFBundleCopyExecutableURL(bundle);
            char executableFile[FILENAME_MAX];
            CFURLGetFileSystemRepresentation(url, true, reinterpret_cast<UInt8*>(executableFile), FILENAME_MAX);
            attempt = QString::fromUtf8(executableFile);
            hnd = dlopen(QFile::encodeName(attempt), dlFlags);
        }
    }
#endif

    locker.relock();
    if (!hnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(fileName, qdlerror());
    }
    if (hnd) {
        qualifiedFileName = attempt;
        errorString.clear();
    }
    pHnd.storeRelaxed(hnd);
    return (hnd != nullptr);
}

bool QLibraryPrivate::unload_sys()
{
    if (dlclose(pHnd.loadAcquire())) {
#if defined (Q_OS_QNX)                // Workaround until fixed in QNX; fixes crash in
        char *error = dlerror();      // QtDeclarative auto test "qqmlenginecleanup" for instance
        if (!qstrcmp(error, "Shared objects still referenced")) // On QNX that's only "informative"
            return true;
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName,
                                                                       QLatin1StringView(error));
#else
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName, qdlerror());
#endif
        return false;
    }
    errorString.clear();
    return true;
}

#if defined(Q_OS_LINUX)
Q_CORE_EXPORT QFunctionPointer qt_linux_find_symbol_sys(const char *symbol)
{
    return QFunctionPointer(dlsym(RTLD_DEFAULT, symbol));
}
#endif

#ifdef Q_OS_DARWIN
Q_CORE_EXPORT QFunctionPointer qt_mac_resolve_sys(void *handle, const char *symbol)
{
    return QFunctionPointer(dlsym(handle, symbol));
}
#endif

QFunctionPointer QLibraryPrivate::resolve_sys(const char *symbol)
{
    QFunctionPointer address = QFunctionPointer(dlsym(pHnd.loadAcquire(), symbol));
    return address;
}

QT_END_NAMESPACE
